#include "timers.h"

#include "purc-errors.h"
#include "private/errors.h"

#include <string.h>
#include <time.h>

static inline void
timer_release(struct pcvariant_timer *timer)
{
    free(timer->id);
    timer->id = NULL;

    timer->activated = 0;
}

void
pcvariant_timers_init(struct pcvariant_timers *timers)
{
    timers->root = RB_ROOT;
}

void
pcvariant_timers_release(struct pcvariant_timers *timers)
{
    struct rb_root *root = &timers->root;

    if (RB_EMPTY_ROOT(root))
        return;

    struct rb_node *p, *n;
    for (p = pcutils_rbtree_first(root);
         ({n = p ? pcutils_rbtree_next(p) : NULL; p;});
         p = n)
    {
        struct pcvariant_timer *timer;
        timer = container_of(p, struct pcvariant_timer, node);
        pcutils_rbtree_erase(p, root);
        timer_release(timer);
        free(timer);
    }
}

static inline void
timer_adjust(struct pcvariant_timer *timer)
{
    struct timespec now;
    clockid_t clk = CLOCK_REALTIME;
    int r = clock_gettime(clk, &now);
    PC_ASSERT(r == 0);

    // FIXME: now or timer->expire?
    time_t sec  = now.tv_sec  + timer->milli_secs / 1000;
    long   nsec = now.tv_nsec + timer->milli_secs % 1000 * 1000000;
    sec  += nsec / 1000000000;
    nsec %= 1000000000;

    timer->expire.tv_sec       = sec;
    timer->expire.tv_nsec      = nsec;
}

static inline void
timer_check_expired(struct pcvariant_timers *timers,
        struct pcvariant_timer *timer,
        void *ud, timer_expired_handler handler)
{
    PC_ASSERT(!timer->zombie);
    PC_ASSERT(!timer->expired);

    if (!timer->activated)
        return;

    struct timespec now;
    clockid_t clk = CLOCK_REALTIME;
    int r = clock_gettime(clk, &now);
    PC_ASSERT(r == 0);

    if (now.tv_sec < timer->expire.tv_sec)
        return;
    if (now.tv_sec == timer->expire.tv_sec) {
        if (now.tv_nsec < timer->expire.tv_nsec)
            return;
    }

    timer->expired = 1;
    handler(timers, timer->id, ud);
    timer->expired = 0;

    if (timer->zombie) {
        pcutils_rbtree_erase(&timer->node, &timers->root);
        timer_release(timer);
        free(timer);
        return;
    }

    if (!timer->activated)
        return;

    timer_adjust(timer);
}

void
pcvariant_timers_expired(struct pcvariant_timers *timers,
        void *ud, timer_expired_handler handler)
{
    PC_ASSERT(handler);

    struct rb_root *root = &timers->root;

    if (RB_EMPTY_ROOT(root))
        return;

    struct rb_node *p, *n;
    for (p = pcutils_rbtree_first(root);
         ({n = p ? pcutils_rbtree_next(p) : NULL; p;});
         p = n)
    {
        struct pcvariant_timer *timer;
        timer = container_of(p, struct pcvariant_timer, node);
        timer_check_expired(timers, timer, ud, handler);
    }
}

int
pcvariant_timers_activate_timer(struct pcvariant_timers *timers,
        const char *id, int activate)
{
    struct rb_root *root = &timers->root;

    if (RB_EMPTY_ROOT(root)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_EXISTS);
        return -1;
    }

    struct rb_node *p, *n;
    for (p = pcutils_rbtree_first(root);
         ({n = p ? pcutils_rbtree_next(p) : NULL; p;});
         p = n)
    {
        struct pcvariant_timer *timer;
        timer = container_of(p, struct pcvariant_timer, node);
        if (strcmp(timer->id, id))
            continue;

        if (timer->activated == activate)
            return 0;

        if (activate) {
            timer_adjust(timer);
            timer->activated = 1;
            return 0;
        }
    }

    pcinst_set_error(PCVARIANT_ERROR_NOT_EXISTS);
    return -1;
}

int
pcvariant_timers_add_timer(struct pcvariant_timers *timers,
        const char *id, int milli_secs, int activate)
{
    struct rb_root *root = &timers->root;

    if (!RB_EMPTY_ROOT(root)) {
        struct rb_node *p, *n;
        for (p = pcutils_rbtree_first(root);
             ({n = p ? pcutils_rbtree_next(p) : NULL; p;});
             p = n)
        {
            struct pcvariant_timer *timer;
            timer = container_of(p, struct pcvariant_timer, node);
            if (strcmp(timer->id, id))
                continue;

            pcinst_set_error(PCVARIANT_ERROR_DUPLICATED);
            return -1;
        }
    }

    struct pcvariant_timer *timer;
    timer = (struct pcvariant_timer*)calloc(1, sizeof(*timer));
    if (!timer) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    timer->id = strdup(id);
    if (!timer->id) {
        free(timer);
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    timer->milli_secs    = milli_secs;
    timer->activated     = activate;
    if (activate) {
        timer_adjust(timer);
    }

    // FIXME:
    struct rb_node *parent = NULL;
    struct rb_node **pnode = NULL;

    // FIXME:
    pcutils_rbtree_link_node(&timer->node, parent, pnode);
    pcutils_rbtree_insert_color(&timer->node, root);

    return 0;
}

int
pcvariant_timers_del_timer(struct pcvariant_timers *timers,
        const char *id)
{
    struct rb_root *root = &timers->root;

    if (RB_EMPTY_ROOT(root)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_EXISTS);
        return -1;
    }

    struct rb_node *p, *n;
    for (p = pcutils_rbtree_first(root);
         ({n = p ? pcutils_rbtree_next(p) : NULL; p;});
         p = n)
    {
        struct pcvariant_timer *timer;
        timer = container_of(p, struct pcvariant_timer, node);
        if (strcmp(timer->id, id))
            continue;

        timer->activated = 0;

        if (timer->expired || timer->zombie) {
            timer->zombie = 1;
            return 0;
        }

        pcutils_rbtree_erase(&timer->node, root);
        timer_release(timer);
        free(timer);

        return 0;
    }

    pcinst_set_error(PCVARIANT_ERROR_NOT_EXISTS);
    return -1;
}


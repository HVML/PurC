#pragma once

#include <vector>

#include "purc.h"

#if OS(LINUX) || OS(UNIX)

// get path from env or __FILE__/../<rel> otherwise
#define test_getpath_from_env_or_rel(_path, _len, _env, _rel)           \
do {                                                                    \
    const char *p = getenv(_env);                                       \
    if (p) {                                                            \
        snprintf(_path, _len, "%s", p);                                 \
    } else {                                                            \
        char tmp[PATH_MAX+1];                                           \
        snprintf(tmp, sizeof(tmp), __FILE__);                           \
        const char *folder = dirname(tmp);                              \
        snprintf(_path, _len, "%s/%s", folder, _rel);                   \
    }                                                                   \
} while (0)

#define test_getbool_from_env_or_default(_env, _def)                    \
({                                                                      \
    bool _v = _def;                                                     \
    const char *p = getenv(_env);                                       \
    if (p && (strcmp(p, "1")==0                                         \
          || strcasecmp(p, "TRUE")==0                                   \
          || strcasecmp(p, "ON")==0))                                   \
    {                                                                   \
        _v = true;                                                      \
    }                                                                   \
    _v;                                                                 \
})

#else

#error "Please define test_getpath_from_env_or_rel for this operating system"

#endif // OS(LINUX) || OS(UNIX)

// Workaround: gtest, INSTANTIATE_TEST_SUITE_P, valgrind
class MemCollector
{
public:
    ~MemCollector(void) {
        cleanup();
    }
public:
    static char* strdup(const char *s) {
        char *p = ::strdup(s);
        get_singleton()->allocates.push_back(p);
        return p;
    }
private:
    static MemCollector* get_singleton(void) {
        static MemCollector         single;
        return &single;
    }
    void cleanup() {
        for (char *v : allocates) {
            free(v);
        }
    }
private:
    std::vector<char *>         allocates;
};

class PurCInstance
{
public:
    PurCInstance(void) {
        init_ok = -1;
        info = {};
        if (purc_init ("cn.fmsoft.hybridos.test", "test_init", &info))
            return;
        init_ok = 0;
    }

    PurCInstance(bool enable_remote_fetcher) {
        init_ok = -1;
        info.enable_remote_fetcher = enable_remote_fetcher;
        if (purc_init ("cn.fmsoft.hybridos.test", "test_init", &info))
            return;
        init_ok = 0;
    }

    ~PurCInstance(void) {
        if (init_ok == 0) {
            purc_cleanup();
        }
    }

public:
    operator bool(void) const {
        return init_ok == 0;
    }
    struct purc_instance_extra_info* get_info(void) {
        if (init_ok)
            return &info;
        return NULL;
    }

private:
    int init_ok;
    struct purc_instance_extra_info    info;
};


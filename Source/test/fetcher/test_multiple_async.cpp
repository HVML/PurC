#include "purc.h"

#include "private/fetcher.h"
#include "private/debug.h"
#include "purc-runloop.h"

#include "../helpers.h"

#include <gtest/gtest.h>
#include <wtf/RunLoop.h>
#include <wtf/FileSystem.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UniStdExtras.h>
#include <wtf/SystemTracing.h>
#include <wtf/WorkQueue.h>
#include <wtf/glib/GLibUtilities.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/URL.h>
#include <wtf/Threading.h>

#include <gio/gio.h>


class ThreadFetcher
{
public:
    ThreadFetcher(const char *name, const char *url)
    {
        if (url) {
            this->url = strdup(url);
        }
        this->name = strdup(name);
    }
    ~ThreadFetcher()
    {
        if (isRun) {
            waitRunLoopExit.wait();
        }
        if (name) {
            free(name);
        }
        if (url) {
            free(url);
        }
    }

    static void async_handler(purc_variant_t request_id, void *ctxt,
            const struct pcfetcher_resp_header *resp_header,
            purc_rwstream_t resp)
    {
        ThreadFetcher *tf = (ThreadFetcher *)ctxt;
        fprintf(stderr, "async_handler|begin......\n");
        fprintf(stderr, "async_handler|name=%s\n", tf->name);
        fprintf(stderr, "async_handler|url=%s\n", tf->url);
        fprintf(stderr, "async_handler|ret_code=%d\n", resp_header->ret_code);
        fprintf(stderr, "async_handler|mime_type=%s\n", resp_header->mime_type);
        fprintf(stderr, "async_handler|sz_resp=%ld\n", resp_header->sz_resp);
        fprintf(stderr, "async_handler|end......\n");

        if (resp) {
            purc_rwstream_destroy(resp);
        }

        if (request_id != PURC_VARIANT_INVALID) {
            purc_variant_unref(request_id);
        }
        tf->runLoop->stop();
    }

    void run() {
        isRun = true;
//        BinarySemaphore semaphore;
        Thread::create(name, [&] {
                fprintf(stderr, "name=%s|url=%s\n", name, url);
                runLoop = &RunLoop::current();
//                semaphore.signal();
                initPurc();

                pcfetcher_request_async(url,
                        PCFETCHER_REQUEST_METHOD_GET,
                        NULL,
                        10,
                        async_handler,
                        this);
                runLoop->run();
                cleanupPurc();
                waitRunLoopExit.signal();
                })->detach();
 //       semaphore.wait();
    }

private:
    void initPurc()
    {
        purc_instance_extra_info info = {};
        purc_init_ex(PURC_MODULE_HVML | PURC_HAVE_FETCHER_R,
                "cn.fmsoft.hybridos.mutiple", name, &info);
    }

    void cleanupPurc()
    {
        purc_cleanup();
    }

private:
    bool isRun;
    char *name;
    char *url;
    RunLoop* runLoop;
    BinarySemaphore waitRunLoopExit;
};

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    ThreadFetcher tf_0("fmsoft", "http://www.fmsoft.cn");
    ThreadFetcher tf_1("baidu", "http://www.baidu.com");
    ThreadFetcher tf_2("163", "http://www.163.com");
    tf_0.run();
    tf_1.run();
    tf_2.run();
    return 0;
}

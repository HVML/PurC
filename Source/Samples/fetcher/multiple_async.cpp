#include "purc.h"

#include "private/fetcher.h"
#include "private/debug.h"
#include "purc-runloop.h"

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
    ThreadFetcher(int idx, const char *name, const char *url) : idx(idx)
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
        fprintf(stderr,
                "res|idx=%d|name=%s|url=%s|ret_code=%d|mime=%s|sz_resp=%ld\n",
                tf->idx, tf->name, tf->url,
                resp_header->ret_code,
                resp_header->mime_type,
                resp_header->sz_resp);

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
        Thread::create(name, [&] {
                runLoop = &RunLoop::current();
                initPurc();

                fprintf(stderr, "req|idx=%d|name=%s|url=%s\n", idx, name,
                        url);
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
    int idx;
    bool isRun;
    char *name;
    char *url;
    RunLoop* runLoop;
    BinarySemaphore waitRunLoopExit;
};

struct testcase {
    const char *name;
    const char *url;
};

struct testcase cases[] {
    {
        "fmsoft",
        "https://www.fmsoft.cn"
    },
    {
        "baidu",
        "https://www.baidu.com"
    },
    {
        "163",
        "https://www.163.com"
    },
    {
        "qq",
        "https://www.qq.com"
    },
    {
        "weibo",
        "https://www.weibo.com"
    },
    {
        "jd",
        "https://www.jd.com"
    },
    {
        "csdn",
        "https://www.csdn.net"
    },
    {
        "sina",
        "https://www.sina.com"
    },
    {
        "sohu",
        "https://www.sohu.com"
    },
    {
        "taobao",
        "https://www.taobao.com"
    },
};

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    int sz = PCA_TABLESIZE(cases);
    ThreadFetcher** tfs = new ThreadFetcher*[sz];

    for (int i = 0; i < sz; i++) {
        tfs[i] = new ThreadFetcher(i, cases[i].name, cases[i].url);
        tfs[i]->run();
    }

    for (int i = 0; i < sz; i++) {
        delete tfs[i];
    }
    delete[] tfs;

    return 0;
}

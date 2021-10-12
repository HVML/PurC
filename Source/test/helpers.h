#pragma once

#if OS(LINUX) || OS(UNIX)
// get path from env or __FILE__/../<rel> otherwise
#define test_getpath_from_env_or_rel(_path, _len, _env, _rel)        \
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

#else

#error "Please define test_getpath_from_env_or_rel for this operating system"

#endif // OS(LINUX) || OS(UNIX)


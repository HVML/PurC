#pragma once

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


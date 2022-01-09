#include "purc.h"

#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "hvml/hvml-token.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <gtest/gtest.h>

using namespace std;

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stderr, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stderr, __VA_ARGS__);                                     \
    } while(false)

#if OS(LINUX) || OS(UNIX)
// get path from env or __FILE__/../<rel> otherwise
#define getpath_from_env_or_rel(_path, _len, _env, _rel) do {  \
    const char *p = getenv(_env);                                      \
    if (p) {                                                           \
        snprintf(_path, _len, "%s", p);                                \
    } else {                                                           \
        char tmp[PATH_MAX+1];                                          \
        snprintf(tmp, sizeof(tmp), __FILE__);                          \
        const char *folder = dirname(tmp);                             \
        snprintf(_path, _len, "%s/%s", folder, _rel);                  \
    }                                                                  \
} while (0)

#endif // OS(LINUX) || OS(UNIX)

#define PRINT_VARIANT(v)                                                    \
    do {                                                                    \
        purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUFFER,          \
                MAX_BUFFER);                                                \
        size_t len_expected = 0;                                            \
        purc_variant_serialize(v, rws,                                      \
                0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);           \
        char* buf = (char*)purc_rwstream_get_mem_buffer_ex(rws, NULL, NULL, \
                true);                                                      \
        PRINTF(stderr, "variant=%s\n", buf);                               \
        free(buf);                                                          \
        purc_rwstream_destroy(rws);                                         \
    } while (0)

enum container_ops_type {
    CONTAINER_OPS_TYPE_DISPLACE,
    CONTAINER_OPS_TYPE_APPEND,
    CONTAINER_OPS_TYPE_PREPEND,
    CONTAINER_OPS_TYPE_MERGE,
    CONTAINER_OPS_TYPE_REMOVE,
    CONTAINER_OPS_TYPE_INSERT_BEFORE,
    CONTAINER_OPS_TYPE_INSERT_AFTER,
    CONTAINER_OPS_TYPE_UNITE,
    CONTAINER_OPS_TYPE_INTERSECT,
    CONTAINER_OPS_TYPE_SUBTRACT,
    CONTAINER_OPS_TYPE_XOR,
    CONTAINER_OPS_TYPE_OVERWRITE
};

struct container_ops_test_data {
    char* name;
    enum container_ops_type ops_type;

    char* dst;
    purc_variant_type dst_type;
    char* dst_unique_key;  // only unique set 

    char* src;
    purc_variant_type src_type;
    char* src_unique_key;  // only unique set 

    char* comp;
    int error;
};

#define TO_TYPE(type_name, type_enum)                       \
    if (strcmp (type, #type_name) == 0) {                   \
        return type_enum;                                   \
    }

enum container_ops_type to_ops_type(const char* type)
{
    TO_TYPE("displace", CONTAINER_OPS_TYPE_DISPLACE);
    TO_TYPE("append", CONTAINER_OPS_TYPE_APPEND);
    TO_TYPE("prepend", CONTAINER_OPS_TYPE_PREPEND);
    TO_TYPE("merge", CONTAINER_OPS_TYPE_MERGE);
    TO_TYPE("remove", CONTAINER_OPS_TYPE_REMOVE);
    TO_TYPE("insertBefore", CONTAINER_OPS_TYPE_INSERT_BEFORE);
    TO_TYPE("insertAfter", CONTAINER_OPS_TYPE_INSERT_AFTER);
    TO_TYPE("unite", CONTAINER_OPS_TYPE_UNITE);
    TO_TYPE("intersect", CONTAINER_OPS_TYPE_INTERSECT);
    TO_TYPE("subtract", CONTAINER_OPS_TYPE_SUBTRACT);
    TO_TYPE("xor", CONTAINER_OPS_TYPE_XOR);
    TO_TYPE("overwrite", CONTAINER_OPS_TYPE_OVERWRITE);

    return CONTAINER_OPS_TYPE_DISPLACE;
}

purc_variant_type to_variant_type(const char* type)
{
    TO_TYPE("object", PURC_VARIANT_TYPE_OBJECT);
    TO_TYPE("array", PURC_VARIANT_TYPE_ARRAY);
    TO_TYPE("set", PURC_VARIANT_TYPE_SET);
    return PURC_VARIANT_TYPE_OBJECT;
}

static inline void
push_back(std::vector<container_ops_test_data> &vec,
        const char* name,
        enum container_ops_type ops_type,
        const char* dst,
        purc_variant_type dst_type,
        const char* dst_unique_key,
        const char* src,
        purc_variant_type src_type,
        const char* src_unique_key,
        const char* comp,
        int error)
{
    container_ops_test_data data;
    memset(&data, 0, sizeof(data));

    data.name = MemCollector::strdup(name);
    data.ops_type = ops_type;

    data.dst = MemCollector::strdup(dst);
    data.dst_type = dst_type;
    data.dst_unique_key = dst_unique_key ? MemCollector::strdup(dst_unique_key)
        : NULL;

    data.src = MemCollector::strdup(src);
    data.src_type = src_type;
    data.src_unique_key = src_unique_key ? MemCollector::strdup(src_unique_key)
        : NULL;

    data.comp = MemCollector::strdup(comp);
    data.error = error;

    vec.push_back(data);
}

char* trim(char *str)
{
    if (!str)
    {
        return NULL;
    }
    char *end;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if(*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
}


class Variant_container_data : public testing::TestWithParam<container_ops_test_data>
{
protected:
    void SetUp() {
        purc_init ("cn.fmsoft.hybridos.test", "purc_variant", NULL);
    }
    void TearDown() {
        purc_cleanup ();
    }

    const container_ops_test_data get_data() {
        return GetParam();
    }
};

TEST_P(Variant_container_data, container_ops)
{
}

char* read_file (const char* file)
{
    FILE* fp = fopen (file, "r");
    if (fp == NULL) {
        return NULL;
    }
    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);
    char* buf = (char*) malloc(sz + 1);
    fseek (fp, 0, SEEK_SET);
    sz = fread (buf, 1, sz, fp);
    fclose (fp);
    buf[sz] = 0;
    return buf;
}

std::vector<container_ops_test_data> read_container_ops_test_data()
{
    size_t sz = 0;
    char* input_buf = NULL;
    char file[PATH_MAX + 16] = {0};
    char input_data_file_path[1024] = {0};
    purc_variant_t input_variant = PURC_VARIANT_INVALID;

    std::vector<container_ops_test_data> vec;

    const char* env = "VARIANT_TEST_CONTAINER_OPS_PATH";
    char data_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(data_path, sizeof(data_path), env, "data");

    if (strlen(data_path)) {
        goto end;
    }

    strcpy (input_data_file_path, data_path);
    strcat (input_data_file_path, "/container_ops.json");

    input_buf = read_file (input_data_file_path);
    if (input_buf == NULL) {
        goto end;
    }

    input_variant = purc_variant_make_from_json_string(input_buf,
            strlen(input_buf));
    if (input_variant == PURC_VARIANT_INVALID) {
        goto end;
    }

    if (!purc_variant_is_array(input_variant)) {
        goto end;
    }

    sz = purc_variant_array_get_size(input_variant);
    for (size_t i = 0; i < sz; i++) {
        purc_variant_t test_data_var = purc_variant_array_get(input_variant, i);
        if (!purc_variant_is_object(test_data_var)) {
            continue;
        }

        purc_variant_t name_var = purc_variant_object_get_by_ckey(test_data_var,
                "name", false);
        if (name_var == PURC_VARIANT_INVALID) {
            continue;
        }
        const char* name = purc_variant_get_string_const(name_var);

        purc_variant_t ops_var = purc_variant_object_get_by_ckey(test_data_var,
                "ops", false);
        if (ops_var == PURC_VARIANT_INVALID) {
            continue;
        }
        const char* ops = purc_variant_get_string_const(ops_var);

        const char* dst_unique_key = NULL;
        purc_variant_t dst_unique_key_var = purc_variant_object_get_by_ckey(test_data_var,
                "dst_unique_key", false);
        if (dst_unique_key_var != PURC_VARIANT_INVALID) {
            dst_unique_key = purc_variant_get_string_const(dst_unique_key_var);
        }

        const char* dst_type = NULL;
        purc_variant_t dst_type_var = purc_variant_object_get_by_ckey(test_data_var,
                "dst_type", false);
        if (dst_type_var != PURC_VARIANT_INVALID) {
            dst_type = purc_variant_get_string_const(dst_type_var);
        }

        const char* src_unique_key = NULL;
        purc_variant_t src_unique_key_var = purc_variant_object_get_by_ckey(test_data_var,
                "src_unique_key", false);
        if (src_unique_key_var != PURC_VARIANT_INVALID) {
            src_unique_key = purc_variant_get_string_const(src_unique_key_var);
        }

        const char* src_type = NULL;
        purc_variant_t src_type_var = purc_variant_object_get_by_ckey(test_data_var,
                "src_type", false);
        if (src_type_var != PURC_VARIANT_INVALID) {
            src_type = purc_variant_get_string_const(src_type_var);
        }

        int64_t error = 0;
        purc_variant_t error_var = purc_variant_object_get_by_ckey(test_data_var,
                "error", false);
        if (error_var != PURC_VARIANT_INVALID) {
            purc_variant_cast_to_longint(error_var, &error, false);
        }

        int n;
        n = snprintf(file, sizeof(file), "%s/%s.dst", data_path, name);
        if (n>=0 && (size_t) n>=sizeof(file)) {
            // to circumvent format-truncation warning
            ;
        }
        char* dst = read_file (file);
        if (!dst) {
            continue;
        }

        n = snprintf(file, sizeof(file), "%s/%s.src", data_path, name);
        if (n>=0 && (size_t) n>=sizeof(file)) {
            // to circumvent format-truncation warning
            ;
        }
        char* src = read_file (file);
        if (!src) {
            free(dst);
            continue;
        }

        n = snprintf(file, sizeof(file), "%s/%s.cmp", data_path, name);
        if (n>=0 && (size_t) n>=sizeof(file)) {
            // to circumvent format-truncation warning
            ;
        }
        char* cmp = read_file (file);
        if (!cmp) {
            free(src);
            free(dst);
            continue;
        }

        push_back(vec,
                name,
                to_ops_type(ops),
                dst,
                to_variant_type(dst_type),
                dst_unique_key,
                src,
                to_variant_type(src_type),
                src_unique_key,
                trim(cmp),
                error
                );
    }

end:
    if (input_buf) {
        free(input_buf);
    }

    if (vec.empty()) {
        push_back(vec,
                "000_inner_test",
                CONTAINER_OPS_TYPE_DISPLACE,
                "{\"key\",100}",
                PURC_VARIANT_TYPE_OBJECT,
                NULL,
                "{\"key\",999}",
                PURC_VARIANT_TYPE_OBJECT,
                NULL,
                "{\"key\",999}",
                 0
                 );
    }
    if (input_variant != PURC_VARIANT_INVALID) {
        purc_variant_unref(input_variant);
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(purc_variant, Variant_container_data,
        testing::ValuesIn(read_container_ops_test_data()));


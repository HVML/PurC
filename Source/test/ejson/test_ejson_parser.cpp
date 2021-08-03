#include "purc.h"

#include "private/ejson.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <gtest/gtest.h>

#if 1
using namespace std;

typedef std::pair<std::string, std::string> ejson_test_data;

class ejson_parser_vcm_eval : public testing::TestWithParam<ejson_test_data>
{
protected:
    void SetUp() {
        purc_init ("cn.fmsoft.hybridos.test", "ejson", NULL);
        json = GetParam().first;
        comp = GetParam().second;
    }
    void TearDown() {
        purc_cleanup ();
    }
    const char* get_json() {
        return json.c_str();
    }
    const char* get_comp() {
        return comp.c_str();
    }

private:
    string json;
    string comp;
};

TEST_P(ejson_parser_vcm_eval, parse_and_serialize)
{
    const char* json = get_json();
    const char* comp = get_comp();
    fprintf(stderr, "json=%s|len=%ld\n", json, strlen(json));
    fprintf(stderr, "comp=%s\n", comp);
    // read end of string as eof
    size_t sz = strlen (json) + 1;
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)json, sz);

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024] = {0};
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, comp);
    fprintf(stderr, "buf=%s\n", buf);
    fprintf(stderr, "com=%s\n", comp);

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pctree_node_destroy (pcvcm_node_to_pctree_node(root),
            pcvcm_node_pctree_node_destory_callback);

    pcejson_destroy(parser);
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

std::vector<ejson_test_data> read_ejson_test_data()
{
    std::vector<ejson_test_data> vec;

#if 1
    char* data_path = getenv("EJSON_DATA_PATH");

    if (data_path) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_list");

        FILE* fp = fopen(file_path, "r");
        if (fp) {
            char file[1024] = {0};

            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;
            while ((read = getline(&line, &sz, fp)) != -1) {
                if (line && line[0] != '#') {
                    char* name = trim (line);
                    sprintf(file, "%s/%s.json", data_path, name);
                    char* json_buf = read_file (file);
                    if (!json_buf) {
                        continue;
                    }

                    sprintf(file, "%s/%s.serial", data_path, name);
                    char* comp_buf = read_file (file);
                    if (!comp_buf) {
                        free (json_buf);
                        continue;
                    }

                    vec.push_back(make_pair(json_buf, trim(comp_buf)));

                    free (json_buf);
                    free (comp_buf);
                }
            }
            free (line);
            fclose(fp);
        }
    }
#endif

    if (vec.empty()) {
        vec.push_back(make_pair("[123]", "[123]"));
        vec.push_back(make_pair("{key:1}", "{\"key\":1}"));
        vec.push_back(make_pair("{'key':'2'}", "{\"key\":\"2\"}"));
    }
    return vec;
}

INSTANTIATE_TEST_CASE_P(ejson, ejson_parser_vcm_eval,
        testing::ValuesIn(read_ejson_test_data()));

#endif

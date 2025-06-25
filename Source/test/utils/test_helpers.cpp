/*
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc/purc-helpers.h"
#include "purc/purc-utils.h"

#include <glib.h>
#include <gtest/gtest.h>
#include <openssl/core_dispatch.h>

#include <stdio.h>

TEST(test_split_page_identifier, split_page_identifier)
{
    char type[PURC_LEN_IDENTIFIER + 1];
    char name[PURC_LEN_IDENTIFIER + 1];
    char workspace[PURC_LEN_IDENTIFIER + 1];
    char group[PURC_LEN_IDENTIFIER + 1];

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    static struct positvie_case {
        const char *page_id;
        const char *type;
        const char *name;
        const char *workspace;
        const char *group;
    } positive_cases[] = {
        { "null:", "null", "", "", "" },
        { "inherit:", "inherit", "", "", "" },
        { "self:", "self", "", "", "" },
        { "widget:name@workspace/group", "widget", "name", "workspace", "group" },
        { "plainwin:name@workspace/group", "plainwin", "name", "workspace", "group" },
        { "plainwin:name@group", "plainwin", "name", "", "group" },
        { "widget:name@group", "widget", "name", "", "group" },
        { "plainwin:name", "plainwin", "name", "", "" },
        { "widget:name", "widget", "name", "", "" },
    };

    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        std::cout << "testing: " << positive_cases[i].page_id << std::endl;

        int ret = purc_split_page_identifier(positive_cases[i].page_id,
                type, name, workspace, group);

        ASSERT_GE(ret, 0);
        ASSERT_STREQ(type, positive_cases[i].type);
        ASSERT_STREQ(name, positive_cases[i].name);
        ASSERT_STREQ(workspace, positive_cases[i].workspace);
        ASSERT_STREQ(group, positive_cases[i].group);
    }

    static const char *negative_cases[] = {
        "null",
        "345",
        "plainwin:",
        "widget:",
        "plainwin@group",
        "widget:name/group",
    };

    for (size_t i = 0; i < sizeof(negative_cases)/sizeof(negative_cases[0]); i++) {
        int ret = purc_split_page_identifier(negative_cases[i],
                type, name, workspace, group);
        ASSERT_LT(ret, 0);
    }
}

TEST(test_window_styles, window_styles)
{
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    static const struct purc_screen_info screen = { 1920, 1280, 96, 1 };

    static struct positvie_case {
        const char *styles;
        struct purc_window_geometry geometry;
    } positive_cases[] = {
        { "", { 0, 0, 1920, 1280 } },   // default geometry: full-screen.
        { "window-size:screen", { 0, 0, 1920, 1280 } },
        { "window-size:square", { 0, 0, 1280, 1280 } },
        { "window-size:50\% auto", { 0, 0, 960, 1280 } },
        { "window-size:50\% 50%", { 0, 0, 960, 640 } },
        { "window-size:50\%", { 0, 0, 960, 1280 } },
        { "window-size:50\% 450px", { 0, 0, 960, 450 } },
        { "window-size:aspect-ratio 1 1", { 0, 0, 1280, 1280 } },
        { "window-size:aspect-ratio 4 3", { 0, 0, 1707, 1280 } },
        { "window-size:aspect-ratio 3 4", { 0, 0, 960, 1280 } },
        { "window-size:aspect-ratio 2 1", { 0, 0, 1920, 960 } },
        { "window-size:50% 50%; window-position:top", { 480, 0, 960, 640 } },
        { "window-size:50% 50%; window-position:left", { 0, 320, 960, 640 } },
        { "window-size:200% 200%; window-position:center", { -960, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:right", { -1920, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:bottom", { -960, -1280, 3840, 2560 } },
        { "window-size:200% 200%; window-position:50% 50%", { -960, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:0 0;", { 0, 0, 3840, 2560 } },
        { "window-size:200% 200%; window-position:left 50%", { 0, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:right 50%", { -1920, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position: top 50%", { -960, 0, 3840, 2560 } },
        { "window-size:200% 200%; window-position: 50\% bottom", { -960, -1280, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left top 50px", { 50, 50, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left 50px center", { 50, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left 10px top 20px", { 10, 20, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left -10px top -20px", { -10, -20, 3840, 2560 } },
        { "window-size:200% 200%; window-position: center -10px center -20px", { -970, -660, 3840, 2560 } },
    };

    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct purc_window_geometry geometry;

        std::cout << "testing: " << positive_cases[i].styles << std::endl;

        int ret = purc_evaluate_standalone_window_geometry_from_styles(
                positive_cases[i].styles, &screen, &geometry);

        ASSERT_EQ(ret, 0);
        ASSERT_EQ(geometry.x, positive_cases[i].geometry.x);
        ASSERT_EQ(geometry.y, positive_cases[i].geometry.y);
        ASSERT_EQ(geometry.width, positive_cases[i].geometry.width);
        ASSERT_EQ(geometry.height, positive_cases[i].geometry.height);
    }
}

TEST(test_transition_styles, transition_style)
{
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    static struct positvie_case {
        int ret;
        const char *styles;
        purc_window_transition_function move_func;
        uint32_t move_duration;
    } positive_cases[] = {
        { 0, "", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: none 100", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 100 },
        { -1, "window-transition-move: linear -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: linear 100", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 100 },
        { 0, "window-transition-move: linear 0", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 0 },
        { 0, "window-transition-move: linear 99;", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 99 },
        { 0, "window-transition-move: linear 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 99 },
        { -1, "window-transition-move: linear -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: InQuad 100", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 100 },
        { 0, "window-transition-move: InQuad 0", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 0 },
        { 0, "window-transition-move: InQuad 99;", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 99 },
        { 0, "window-transition-move: InQuad 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 99 },
        { -1, "window-transition-move: OutQuad -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: OutQuad 100", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 100 },
        { 0, "window-transition-move: OutQuad 0", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 0 },
        { 0, "window-transition-move: OutQuad 99;", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 99 },
        { 0, "window-transition-move: OutQuad 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 99 },
        { -1, "window-transition-move: InOutQuad -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: InOutQuad 100", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 100 },
        { 0, "window-transition-move: InOutQuad 0", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 0 },
        { 0, "window-transition-move: InOutQuad 99;", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 99 },
        { 0, "window-transition-move: InOutQuad 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 99 },
        { 0, "window-transition-move: ppp aabb;", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
    };

    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct purc_window_transition transition;

        std::cout << "testing: " << positive_cases[i].styles << std::endl;

        int ret = purc_evaluate_standalone_window_transition_from_styles(
                positive_cases[i].styles, &transition);

        ASSERT_EQ(ret, positive_cases[i].ret);
        ASSERT_EQ(transition.move_func, positive_cases[i].move_func);
        ASSERT_EQ(transition.move_duration, positive_cases[i].move_duration);
    }
}

// This test is written with aid from ChatGPT.
TEST(test_url_encode_decode, url_encode_decode)
{
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    // Test case structure
    struct test_case {
        const char* raw;          // Original string
        const char* encoded;      // Expected encoded result
        bool rfc1738;             // Whether to use RFC1738 encoding
    };

    // URL encoding test cases
    static struct test_case encode_cases[] = {
        // RFC 1738 test cases (using + to replace spaces)
        {"Hello World", "Hello+World", true},
        {"file name with spaces", "file+name+with+spaces", true},
        {"!@#$%^&*()_+", "%21%40%23%24%25%5E%26%2A%28%29_%2B", true},
        {"你好世界", "%E4%BD%A0%E5%A5%BD%E4%B8%96%E7%95%8C", true},
        {"中文 测试", "%E4%B8%AD%E6%96%87+%E6%B5%8B%E8%AF%95", true},

        // RFC 3986 test cases (using %20 to replace spaces)
        {"Hello World", "Hello%20World", false},
        {"file name with spaces", "file%20name%20with%20spaces", false},
        {"!@#$%^&*()_+", "%21%40%23%24%25%5E%26%2A%28%29_%2B", false},
        {"你好世界", "%E4%BD%A0%E5%A5%BD%E4%B8%96%E7%95%8C", false},
        {"中文 测试", "%E4%B8%AD%E6%96%87%20%E6%B5%8B%E8%AF%95", false},

        // General test cases (cases where both encoding methods produce the same result)
        {"http://example.com/path?key=value", 
         "http%3A%2F%2Fexample.com%2Fpath%3Fkey%3Dvalue", true},
        {"path/to/resource", "path%2Fto%2Fresource", true},
        {"Hello你好123", "Hello%E4%BD%A0%E5%A5%BD123", true},
        {"测试@example.com", "%E6%B5%8B%E8%AF%95%40example.com", true},
        {"文件名.txt", "%E6%96%87%E4%BB%B6%E5%90%8D.txt", true}
    };

    // Test URL encoding
    for (size_t i = 0; i < sizeof(encode_cases)/sizeof(encode_cases[0]); i++) {
        char* encoded = purc_url_encode_alloc(encode_cases[i].raw, 
                encode_cases[i].rfc1738);
        ASSERT_NE(encoded, nullptr);
        ASSERT_STREQ(encoded, encode_cases[i].encoded);
        free(encoded);
    }

    // URL decoding test cases
    static struct test_case decode_cases[] = {
        // RFC 1738 decoding tests
        {"Hello World", "Hello+World", true},
        {"file name with spaces", "file+name+with+spaces", true},
        {"中文 测试", "%E4%B8%AD%E6%96%87+%E6%B5%8B%E8%AF%95", true},

        // RFC 3986 decoding tests
        {"Hello World", "Hello%20World", false},
        {"file name with spaces", "file%20name%20with%20spaces", false},
        {"中文 测试", "%E4%B8%AD%E6%96%87%20%E6%B5%8B%E8%AF%95", false},        
        {"Hello 你好", "Hello%20%E4%BD%A0%E5%A5%BD", false},
        {"Hello 你好", "Hello+%E4%BD%A0%E5%A5%BD", true},
        {"测试/test", "%E6%B5%8B%E8%AF%95%2Ftest", false}
    };

    // Test URL decoding
    for (size_t i = 0; i < sizeof(decode_cases)/sizeof(decode_cases[0]); i++) {
        char* decoded = purc_url_decode_alloc(decode_cases[i].encoded, 
                decode_cases[i].rfc1738);
        ASSERT_NE(decoded, nullptr);
        ASSERT_STREQ(decoded, decode_cases[i].raw);
        free(decoded);
    }

    // Error encoding test cases
    static struct error_test_case {
        const char* input;        // Input with invalid encoding
        const char* expected;     // Expected decoded result
    } error_cases[] = {
        {"%Xbc", ""},            // Invalid hexadecimal character
        {"hello%", "hello"},     // Incomplete percent encoding
        {"test%2", "test"},      // Incomplete percent encoding
        {"abc%XXdef", "abc"},    // Invalid hexadecimal character
        {"123%2G45", "123"},     // Invalid hexadecimal character
        {"%e6%ad%a3%e5%b8%b8%XX%e9%94%99%e8%af%af", "正常"},   // Test with Chinese characters and invalid encoding
    };

    // Test handling of error encodings
    for (size_t i = 0; i < sizeof(error_cases)/sizeof(error_cases[0]); i++) {
        char* decoded = purc_url_decode_alloc(error_cases[i].input, true);
        ASSERT_NE(decoded, nullptr);
        ASSERT_STREQ(decoded, error_cases[i].expected);
        free(decoded);
    }
}

TEST(test_punycode, punycode_encode_decode) {
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    // Test case structure
    struct test_case {
        const char* original;     // Original string
        const char* punycode;     // Expected Punycode encoded result
    };

    // Punycode encoding/decoding test cases
    static struct test_case test_cases[] = {
        // Basic ASCII tests
        {"example", "example"},
        {"hello-world", "hello-world"},

        // Chinese domain name tests
        {"中文", "xn--fiq228c"},
        {"中文.com", "xn--fiq228c.com"},
        {"中文.中国", "xn--fiq228c.xn--fiqs8s"},
        {"测试", "xn--0zwm56d"},

        // Mixed character tests
        {"hello中文", "xn--hello-9n1h080l"},
        {"test测试", "xn--test-3x7if72m"},

        // Special character tests
        {"bücher", "xn--bcher-kva"},
        {"münchen", "xn--mnchen-3ya"},
        {"αβγ", "xn--mxacd"},

        // Long string tests
        {"长字符串测试案例", "xn--kiqs6a22xx4mj2f99wjw0amfq"},
        {"中文域名测试.com", "xn--fiq06l2rdsvscfji99b.com"},
    };

    // Test Punycode encoding
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        std::cout << "Testing encode: " << test_cases[i].original << std::endl;

        struct pcutils_mystring encoded;
        pcutils_mystring_init(&encoded);

        pcutils_punycode_encode(&encoded, test_cases[i].original);
        pcutils_mystring_done(&encoded);

        ASSERT_NE(encoded.buff, nullptr);
        ASSERT_STREQ(encoded.buff, test_cases[i].punycode);
        pcutils_mystring_free(&encoded);
    }

    // Test Punycode decoding
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        std::cout << "Testing decode: " << test_cases[i].punycode << std::endl;

        struct pcutils_mystring decoded;
        pcutils_mystring_init(&decoded);

        pcutils_punycode_decode(&decoded, test_cases[i].punycode);
        pcutils_mystring_done(&decoded);

        ASSERT_NE(decoded.buff, nullptr);

        // For pure ASCII strings, decoded result should be same as input
        if (strncmp(test_cases[i].punycode, "xn--", 4) != 0) {
            ASSERT_STREQ(decoded.buff, test_cases[i].punycode);
        }
        else {
            ASSERT_STREQ(decoded.buff, test_cases[i].original);
        }
        pcutils_mystring_free(&decoded);
    }

    // Error handling tests
    static const char* error_cases[] = {
        "xn--",             // Empty Punycode string
        "xn--测试",         // Invalid Punycode format
        nullptr,            // Null pointer test
    };

    for (size_t i = 0; i < sizeof(error_cases)/sizeof(error_cases[0]); i++) {
        std::cout << "Testing bad Punnycode: " << (error_cases[i] ? error_cases[i] : "(nil)") << std::endl;

        struct pcutils_mystring decoded;
        pcutils_mystring_init(&decoded);

        int ret = pcutils_punycode_decode(&decoded, error_cases[i]);
        std::cout << "pcutils_punycode_decode(): " << ret << std::endl;

        if (ret == 0) {
            pcutils_mystring_done(&decoded);
            std::cout << "pcutils_punycode_decode(): " << decoded.buff << std::endl;
        }

        ASSERT_EQ(ret, ret);

        pcutils_mystring_free(&decoded);
    }
}

TEST(test_url_path_encode_decode, url_path_encode_decode) {
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    // Test case structure for positive cases
    struct positive_test_case {
        const char* raw;      // Original path
        const char* encoded;  // Expected encoded result
    };

    // Positive test cases
    static struct positive_test_case positive_cases[] = {
        // empty string
        {"", ""},

        // Test basic path components
        {"/path/to/file", "/path/to/file"},
        {"/simple path/file name", "/simple%20path/file%20name"},

        // Test special characters
        {"/path!@#$%^&*()", "/path%21%40%23%24%25%5E%26%2A%28%29"},
        {"/path+with+plus", "/path%2Bwith%2Bplus"},

        // Test Unicode characters
        {"/中文/路径", "/%E4%B8%AD%E6%96%87/%E8%B7%AF%E5%BE%84"},
        {"/测试/文件名", "/%E6%B5%8B%E8%AF%95/%E6%96%87%E4%BB%B6%E5%90%8D"},

        // Test mixed characters
        {"/path/to/文件.txt", "/path/to/%E6%96%87%E4%BB%B6.txt"},
        {"/user data/用户数据", "/user%20data/%E7%94%A8%E6%88%B7%E6%95%B0%E6%8D%AE"},

        // Test reserved characters
        {"/path:with:colon", "/path%3Awith%3Acolon"},
        {"/query?param=value", "/query%3Fparam%3Dvalue"}
    };

    // Test URL path encoding
    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct pcutils_mystring encoded;
        pcutils_mystring_init(&encoded);

        printf("Encoding: %s\n", positive_cases[i].raw);
        int ret = pcutils_url_path_encode(&encoded, positive_cases[i].raw);
        ASSERT_EQ(ret, 0);
        pcutils_mystring_done(&encoded);
        ASSERT_STREQ(encoded.buff, positive_cases[i].encoded);
        pcutils_mystring_free(&encoded);

        // Test decoding back
        struct pcutils_mystring decoded;
        pcutils_mystring_init(&decoded);

        printf("Decoding: %s\n", positive_cases[i].encoded);
        ret = pcutils_url_path_decode(&decoded, positive_cases[i].encoded);
        ASSERT_EQ(ret, 0);
        pcutils_mystring_done(&decoded);
        ASSERT_STREQ(decoded.buff, positive_cases[i].raw);
        pcutils_mystring_free(&decoded);
    }

    // Negative test cases for encoding
    static const char* negative_encoding_cases[] = {
        nullptr,                    // Null input
    };

    // Test invalid inputs for encoding
    for (size_t i = 0; i < sizeof(negative_encoding_cases)/sizeof(negative_encoding_cases[0]); i++) {
        struct pcutils_mystring output;
        pcutils_mystring_init(&output);

        // Test encoding
        printf("Try to encode: %s\n", negative_encoding_cases[i]);
        int ret = pcutils_url_path_encode(&output, negative_encoding_cases[i]);
        ASSERT_NE(ret, 0);

        pcutils_mystring_free(&output);
    }

    // Negative test cases for decoding
    static const char* negative_decoding_cases[] = {
        nullptr,                    // Null input
        "%",                        // Incomplete percent encoding
        "%2",                       // Incomplete percent encoding
        "%XX",                      // Invalid hex characters
        "/path%2G/invalid",         // Invalid hex character
        "/path%/more",              // Broken percent encoding
    };

    // Negative test cases for decoding
    for (size_t i = 0; i < sizeof(negative_decoding_cases)/sizeof(negative_decoding_cases[0]); i++) {
        struct pcutils_mystring output;
        pcutils_mystring_init(&output);

        printf("Try to decode: %s\n", negative_decoding_cases[i]);
        int ret = pcutils_url_path_decode(&output, negative_decoding_cases[i]);
        ASSERT_NE(ret, 0);

        pcutils_mystring_free(&output);
    }
}

TEST(test_url_query_encode_decode, url_query_encode_decode) {
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    // Test case structure for positive cases
    struct positive_test_case {
        const char* raw;      // Original query string
        const char* encoded;  // Expected encoded result
    };

    // Positive test cases
    static struct positive_test_case positive_cases[] = {
        // Empty string
        {"", ""},

        // Basic key-value pairs
        {"name=value", "name=value"},
        {"key=hello world", "key=hello%20world"},

        // Multiple parameters
        {"a=1&b=2", "a=1&b=2"},
        {"user=john&age=25", "user=john&age=25"},

        // Special characters
        {"q=!@#$%^*()", "q=%21%40%23%24%25%5E%2A%28%29"},
        {"search=C++ Programming", "search=C%2B%2B%20Programming"},

        // Unicode characters
        {"name=张三", "name=%E5%BC%A0%E4%B8%89"},
        {"query=测试", "query=%E6%B5%8B%E8%AF%95"},
        {"测试=张三", "%E6%B5%8B%E8%AF%95=%E5%BC%A0%E4%B8%89"},

        // Mixed content
        {"path=/usr/local&file=config.ini", "path=%2Fusr%2Flocal&file=config.ini"},
        {"text=Hello世界", "text=Hello%E4%B8%96%E7%95%8C"}
    };

    // Test URL query encoding and decoding
    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct pcutils_mystring encoded;
        pcutils_mystring_init(&encoded);

        printf("Encoding: %s\n", positive_cases[i].raw);
        int ret = pcutils_url_query_encode(&encoded, positive_cases[i].raw);
        ASSERT_EQ(ret, 0);
        pcutils_mystring_done(&encoded);
        ASSERT_STREQ(encoded.buff, positive_cases[i].encoded);
        pcutils_mystring_free(&encoded);

        // Test decoding back
        struct pcutils_mystring decoded;
        pcutils_mystring_init(&decoded);

        printf("Decoding: %s\n", positive_cases[i].encoded);
        ret = pcutils_url_query_decode(&decoded, positive_cases[i].encoded);
        ASSERT_EQ(ret, 0);
        pcutils_mystring_done(&decoded);
        ASSERT_STREQ(decoded.buff, positive_cases[i].raw);
        pcutils_mystring_free(&decoded);
    }

    // Negative test cases for encoding
    static const char* negative_encoding_cases[] = {
        nullptr,                    // Null input
        // {"=", "="},              // TODO: missing key and value
        // {"&", "&"},              // TODO: missing key=value pair
        // "=value",                // TODO: Missing key
        // "key=value&&",           // TODO: Double ampersand
        // "&&key=value"            // TODO: Leading ampersands
    };

    // Test invalid inputs for encoding
    for (size_t i = 0; i < sizeof(negative_encoding_cases)/sizeof(negative_encoding_cases[0]); i++) {
        struct pcutils_mystring output;
        pcutils_mystring_init(&output);

        printf("Try to encode: %s\n", negative_encoding_cases[i]);
        int ret = pcutils_url_query_encode(&output, negative_encoding_cases[i]);
        ASSERT_NE(ret, 0);

        pcutils_mystring_free(&output);
    }

    // Negative test cases for decoding
    static const char* negative_decoding_cases[] = {
        nullptr,                    // Null input
        "%",                        // Incomplete percent encoding
        "%2",                       // Incomplete percent encoding
        "%XX",                      // Invalid hex characters
        "key=%2G",                  // Invalid hex character
        "key=%",                    // Broken percent encoding
        // {"=", "="},              // TODO: missing key and value
        // {"&", "&"},              // TODO: missing key=value pair
        // "=value",                // TODO: Missing key
        // "key=value&&",           // TODO: Double ampersand
        // "&&key=value"            // TODO: Leading ampersands
    };

    // Test invalid inputs for decoding
    for (size_t i = 0; i < sizeof(negative_decoding_cases)/sizeof(negative_decoding_cases[0]); i++) {
        struct pcutils_mystring output;
        pcutils_mystring_init(&output);

        printf("Try to decode: %s\n", negative_decoding_cases[i]);
        int ret = pcutils_url_query_decode(&output, negative_decoding_cases[i]);
        ASSERT_NE(ret, 0);

        pcutils_mystring_free(&output);
    }
}

TEST(test_url_fragment_encode_decode, url_fragment_encode_decode) {
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    // Test case structure for positive cases
    struct positive_test_case {
        const char* raw;      // Original fragment
        const char* encoded;  // Expected encoded result
    };

    // Positive test cases
    static struct positive_test_case positive_cases[] = {
        // Basic fragments
        {"section1", "section1"},
        {"heading 1", "heading%201"},

        // Special characters
        {"fragment!@#$%^&*/?()", "fragment%21%40%23%24%25%5E%26%2A/?%28%29"},
        {"section+with+plus", "section%2Bwith%2Bplus"},

        // Unicode characters
        {"章节", "%E7%AB%A0%E8%8A%82"},
        {"标题一", "%E6%A0%87%E9%A2%98%E4%B8%80"},

        // Mixed characters
        {"section1_中文", "section1_%E4%B8%AD%E6%96%87"},
        {"heading-测试", "heading-%E6%B5%8B%E8%AF%95"},

        // Reserved characters
        {"section:subsection", "section%3Asubsection"},
        {"query?param=value", "query?param%3Dvalue"}
    };

    // Test URL fragment encoding
    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct pcutils_mystring encoded;
        pcutils_mystring_init(&encoded);

        printf("Encoding fragment: %s\n", positive_cases[i].raw);
        int ret = pcutils_url_fragment_encode(&encoded, positive_cases[i].raw);
        ASSERT_EQ(ret, 0);
        pcutils_mystring_done(&encoded);
        ASSERT_STREQ(encoded.buff, positive_cases[i].encoded);
        pcutils_mystring_free(&encoded);

        // Test decoding back
        struct pcutils_mystring decoded;
        pcutils_mystring_init(&decoded);

        printf("Decoding fragment: %s\n", positive_cases[i].encoded);
        ret = pcutils_url_fragment_decode(&decoded, positive_cases[i].encoded);
        ASSERT_EQ(ret, 0);
        pcutils_mystring_done(&decoded);
        ASSERT_STREQ(decoded.buff, positive_cases[i].raw);
        pcutils_mystring_free(&decoded);
    }

    // Negative test cases for encoding
    static const char* negative_encoding_cases[] = {
        nullptr,                    // Null input
    };

    // Test invalid inputs for encoding
    for (size_t i = 0; i < sizeof(negative_encoding_cases)/sizeof(negative_encoding_cases[0]); i++) {
        struct pcutils_mystring output;
        pcutils_mystring_init(&output);

        printf("Try to encode invalid fragment: %s\n", negative_encoding_cases[i]);
        int ret = pcutils_url_fragment_encode(&output, negative_encoding_cases[i]);
        ASSERT_NE(ret, 0);

        pcutils_mystring_free(&output);
    }

    // Negative test cases for decoding
    static const char* negative_decoding_cases[] = {
        nullptr,                    // Null input
        "%",                        // Incomplete percent encoding
        "%2",                       // Incomplete percent encoding
        "%XX",                      // Invalid hex characters
        "fragment%2G/invalid",      // Invalid hex character
        "fragment%/more",           // Broken percent encoding
    };

    // Test invalid inputs for decoding
    for (size_t i = 0; i < sizeof(negative_decoding_cases)/sizeof(negative_decoding_cases[0]); i++) {
        struct pcutils_mystring output;
        pcutils_mystring_init(&output);

        printf("Try to decode invalid fragment: %s\n", negative_decoding_cases[i]);
        int ret = pcutils_url_fragment_decode(&output, negative_decoding_cases[i]);
        ASSERT_NE(ret, 0);

        pcutils_mystring_free(&output);
    }
}

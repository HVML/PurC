/*
** Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#include "purc/purc-variant.h"

#include "../helpers.h"
#include <cstdint>
#include <cstdlib>

using namespace std;

typedef purc_variant_t (*purc_variant_operator_t)(purc_variant_t v1, purc_variant_t v2);

static purc_variant_t make_operand_from_string(enum purc_variant_type type, const char* op) {
    purc_variant_t operand = PURC_VARIANT_INVALID;

    double d;
    long long ll;
    unsigned long long ull;
    long double ld;

    switch (type) {
        case PURC_VARIANT_TYPE_NUMBER:
            d = strtod(op, nullptr);
            operand = purc_variant_make_number(d);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            ll = strtoll(op, nullptr, 10);
            operand = purc_variant_make_longint(ll);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            ull = strtoull(op, nullptr, 10);
            operand = purc_variant_make_ulongint(ull);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            ld = strtold(op, nullptr);
            operand = purc_variant_make_longdouble(ld);
            break;
        case PURC_VARIANT_TYPE_BIGINT:
            operand = purc_variant_make_bigint_from_string(op, nullptr, 0);
            break;
        default:
            break;
    }

    return operand;
}

TEST(variant, arithmetic_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);
    // Test case structure for arithmetic operations
    struct arithmetic_test_case {
        enum purc_variant_type type1, type2;
        const char* op1;        // First operand
        const char* op2;        // Second operand
        purc_variant_operator_t op; // Operator function
        const char* expected;   // Expected result after serialization
    };

    // Test cases covering different numeric types and operations
    arithmetic_test_case test_cases[] = {
        // Addition tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "456", "579", purc_variant_operator_add, "1035"},  // number + number
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1.23e308", "4.56e308", purc_variant_operator_add, "Infinity"},  // overflowed double
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "9223372036854775807", "1", purc_variant_operator_add, "9223372036854775808"},  // longint + number
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_NUMBER,
            "18446744073709551615", "1", purc_variant_operator_add, "18446744073709551616"},  // ulongint + number
        { PURC_VARIANT_TYPE_LONGDOUBLE, PURC_VARIANT_TYPE_LONGDOUBLE,
            "1.23e8", "4.56e8", purc_variant_operator_add, "579000000FL"},  // longdouble + longdouble
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "4660046610375530309UL", "7540113804746346429UL",
            purc_variant_operator_add, "12200160415121876738UL"},  // longint + longint
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "1N", "2N", purc_variant_operator_add, "3N"},  // bigint +bigint
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "2880067194370816120N", "4660046610375530309N",
            purc_variant_operator_add, "7540113804746346429N"},  // bigint +bigint
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "4660046610375530309N", "7540113804746346429N",
            purc_variant_operator_add, "12200160415121876738N"},  // longint + longint

        // Subtraction tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "250", purc_variant_operator_sub, "750"},  // number - number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "9223372036854775807", "9223372036854775806", purc_variant_operator_sub, "1L"},  // longint - longint
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "18446744073709551615", "18446744073709551614", purc_variant_operator_sub, "1UL"},  // ulongint - ulongint
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "18446744073709551615N", "18446744073709551614N", purc_variant_operator_sub, "1N"},  // bigint - bigint
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "7540113804746346429N", "4660046610375530309N",
            purc_variant_operator_sub, "2880067194370816120N"},  // bigint - bigint

        // Multiplication tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "123", "456", purc_variant_operator_mul, "56088"},  // number * number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "922337203685477580", "10", purc_variant_operator_mul, "9223372036854775808"},  // longint * number
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_NUMBER,
            "1844674407370955161", "10", purc_variant_operator_mul, "18446744073709551616"},  // ulongint * number
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "18446744073709551615N", "11N", purc_variant_operator_mul, "202914184810805067765N"},  // bigint * bigint

        // True division tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_truediv, "333.33333333333331"},  // number / number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_truediv, "5555.5"},  // longint / number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "9223372036854775807", "2", purc_variant_operator_truediv, "4611686018427387904"},  // longint / number
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "18446744073709551614N", "2N", purc_variant_operator_truediv, "9223372036854775808"},  // bigint - bigint



        // Floor division tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_floordiv, "333"},  // number // number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_floordiv, "5555"},  // longint // number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "9223372036854775807", "2", purc_variant_operator_floordiv, "4611686018427387904"},  // longint // number
        { PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "18446744073709551614N", "2N", purc_variant_operator_floordiv, "9223372036854775807N"},  // bigint - bigint

        // Modulo tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_mod, "1"},  // number % number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_mod, "1"},  // longint // number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "9223372036854775807", "2", purc_variant_operator_mod, "1L"},  // longint % number

        // Power tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "2", "10", purc_variant_operator_pow, "1024"},  // number ** number
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "2", "30", purc_variant_operator_pow, "1073741824"},  // number ** number (larger)
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "9223372036854775807", "16", purc_variant_operator_pow, "2743062034396844336869514018464698837952741034352782431735406935422555235659604611574795800485902102589878063855381220980247414149652079643899138017548027873771831513201398226700753025465497615356604597023149336546797754176993249443973844794089529533475153606348844332504619566761300314793168852746240001N"},  // longint ** longint
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "18446744073709551615", "50", purc_variant_operator_pow, "1976906478982563988295810691554050614399651424488030850762343678931647978618325561474974086879022536362639642807744003756529842940963577394276662468582411572670825043959009899694489661532504088009120877946903639143166499456428438448880195155499253110091516188286694321777197119674227599132338454581472690647559478166682718820606301933576867149263606282479822069977062577699422966456063707753586195851786206483814160770891312582986610957317552176331001104119220689137430267970695253598479625236453588946233916795030393521191859493297484330703657084378498022511947929753899678606763593843427503615639810405259276058581346303867750708563882561183560546452553196090076061939041007446872127913157825040863114845973565294859768831735607624294122356928888311315808677423812406463069338456801790310944262553921204739016074704752181719029665060048844403891050051589949396129274357516548225563425373464972431095290923353938157423423230896109004106619977392256259918212890625N"},  // ulongint ** ulongint
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Checking " << test.op1 << " " << test.op2 << std::endl;
        purc_variant_t op1 = make_operand_from_string(test.type1, test.op1);
        purc_variant_t op2 = make_operand_from_string(test.type2, test.op2);

        ASSERT_NE(op1, nullptr);
        ASSERT_NE(op2, nullptr);

        // Perform the arithmetic operation
        purc_variant_t result = test.op(op1, op2);
        ASSERT_NE(result, nullptr);

        // Serialize and verify
        purc_rwstream_seek(rws, 0, SEEK_SET);
        size_t n = purc_variant_serialize(result, rws, 0,
                PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_NOZERO,
                NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        ASSERT_STREQ(buf, test.expected);

        // Cleanup
        purc_variant_unref(result);
        purc_variant_unref(op1);
        purc_variant_unref(op2);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, contains_operator)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure
    struct contains_test_case {
        const char* container_json;    // eJSON string for container
        const char* element_json;      // eJSON string for element to find
        int expected;                  // Expected result：0 for false, 1 for true, and -1 for failure
        const char* desc;              // Test case description
    };

    // Build test cases
    contains_test_case test_cases[] = {
        // String positive tests
        {
            "\"Hello, World!\"",
            "\"World\"",
            1,
            "String contains substring"
        },
        {
            "\"Hello, World!\"",
            "\"\"",
            1,
            "String contains empty string"
        },

        // String negative tests
        {
            "\"Hello, World!\"",
            "\"xyz\"",
            0,
            "String does not contain substring"
        },
        // Test cases for non-string data types that should not work with string contains operation
        {
            "\"Hello, World!\"",
            "42",
            -1,
            "Number cannot be used for string contains check"
        },
        {
            "\"Hello, World!\"",
            "true",
            -1,
            "Boolean cannot be used for string contains check"
        },
        {
            "\"Hello, World!\"",
            "[1,2,3]",
            -1,
            "Array cannot be used for string contains check"
        },
        {
            "\"Hello, World!\"",
            "{\"key\":\"value\"}",
            -1,
            "Object cannot be used for string contains check"
        },
        {
            "\"Hello, World!\"",
            "null",
            -1,
            "Null cannot be used for string contains check"
        },

        // Byte sequence positive tests (using Base64 encoding)
        {
            "b64SGVsbG8=",
            "b64SGU=",
            1,
            "Byte sequence contains subsequence"
        },
        {
            "b64SGVsbG8=",
            "b64",
            1,
            "Byte sequence contains empty sequence"
        },

        // Array positive tests
        {
            "[1, 2, 3, 4, 5]",
            "3",
            1,
            "Array contains element"
        },

        // Array negative tests
        {
            "[1, 2, 3, 4, 5]",
            "6",
            0,
            "Array does not contain element"
        },

        // Object positive tests
        {
            "{\"name\": \"John\", \"age\": 30}",
            "\"name\"",
            1,
            "Object contains key"
        },

        // Object negative tests
        {
            "{\"name\": \"John\", \"age\": 30}",
            "\"address\"",
            0,
            "Object does not contain key"
        },

        // Nested structure tests
        {
            "[{\"id\": 1, \"data\": [1,2,3]}, {\"id\": 2, \"data\": [4,5,6]}]",
            "{\"id\": 1, \"data\": [1,2,3]}",
            1,
            "Array contains complex object"
        },

        // Type error tests
        {
            "42",
            "2",
            -1,
            "Number type does not support contains operation"
        },
        {
            "true",
            "true",
            -1,
            "Boolean type does not support contains operation"
        }
    };

    // Execute test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;

        // Construct test data from JSON strings
        purc_variant_t container = purc_variant_make_from_json_string(
                test.container_json, strlen(test.container_json));
        purc_variant_t element = purc_variant_make_from_json_string(
                test.element_json, strlen(test.element_json));

        EXPECT_NE(container, nullptr);
        EXPECT_NE(element, nullptr);

        // Perform contains operation
        purc_variant_t result = purc_variant_operator_contains(container, element);

        if (test.expected == 1) {
            EXPECT_NE(result, PURC_VARIANT_INVALID);
            EXPECT_TRUE(purc_variant_booleanize(result));
        }
        else if (test.expected == 0) {
            EXPECT_NE(result, PURC_VARIANT_INVALID);
            EXPECT_FALSE(purc_variant_booleanize(result));
        }
        else {
            EXPECT_EQ(result, PURC_VARIANT_INVALID);
        }

        // Cleanup resources
        if (result != PURC_VARIANT_INVALID) {
            purc_variant_unref(result);
        }
        purc_variant_unref(container);
        purc_variant_unref(element);
    }

    purc_cleanup();
}

TEST(variant, concat_operator)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for concatenation operations
    struct concat_test_case {
        const char* input1;      // First input in JSON format
        const char* input2;      // Second input in JSON format
        const char* expected;    // Expected result after concatenation
        const char* desc;        // Test case description
    };

    // Test cases covering different types and scenarios
    concat_test_case test_cases[] = {
        // String concatenation tests
        {
            "\"Hello, \"",
            "\"World!\"",
            "\"Hello, World!\"",
            "Basic string concatenation"
        },
        {
            "\"\"",
            "\"test\"",
            "\"test\"",
            "Empty string concatenation"
        },
        {
            "\"test\"",
            "\"\"",
            "\"test\"",
            "Concatenation with empty string"
        },

        // Byte sequence concatenation tests (using Base64 encoding)
        {
            "b64SGVsbG8=",
            "b64V29ybGQ=",
            "b64SGVsbG9Xb3JsZA==",
            "Basic byte sequence concatenation"
        },
        {
            "b64",
            "b64SGVsbG8=",
            "b64SGVsbG8=",
            "Empty byte sequence concatenation"
        },
        {
            "b64SGVsbG8=",
            "b64",
            "b64SGVsbG8=",
            "Concatenation with empty byte sequence"
        },

        // Array concatenation tests
        {
            "[1, 2, 3]",
            "[4, 5, 6]",
            "[1,2,3,4,5,6]",
            "Basic array concatenation"
        },
        {
            "[]",
            "[1, 2, 3]",
            "[1,2,3]",
            "Empty array concatenation"
        },
        {
            "[1, 2, 3]",
            "[]",
            "[1,2,3]",
            "Concatenation with empty array"
        },
        {
            "[\"a\", 1, true]",
            "[null, 2.5, \"b\"]",
            "[\"a\",1,true,null,2.5,\"b\"]",
            "Mixed type array concatenation"
        },

        // Tuple concatenation tests
        {
            "[!1, 2, 3]",
            "[!4, 5, 6]",
            "[!1,2,3,4,5,6]",
            "Basic tuple concatenation"
        },
        {
            "[!]",
            "[!1, 2, 3]",
            "[!1,2,3]",
            "Empty tuple concatenation"
        },
        {
            "[!1, 2, 3]",
            "[!]",
            "[!1,2,3]",
            "Concatenation with empty tuple"
        },
        {
            "[!\"a\", 1, true]",
            "[null, 2.5, \"b\"]",
            "[!\"a\",1,true,null,2.5,\"b\"]",
            "Mixed type tuple concatenation"
        }
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;

        // Create variants from JSON strings
        purc_variant_t input1 = purc_variant_make_from_json_string(
                test.input1, strlen(test.input1));
        purc_variant_t input2 = purc_variant_make_from_json_string(
                test.input2, strlen(test.input2));

        ASSERT_NE(input1, nullptr);
        ASSERT_NE(input2, nullptr);

        // Perform concatenation operation
        purc_variant_t result = purc_variant_operator_concat(input1, input2);
        if (test.expected == nullptr) {
            EXPECT_EQ(result, PURC_VARIANT_INVALID);
        }
        else {
            ASSERT_NE(result, nullptr);

            // Serialize and verify result
            purc_rwstream_seek(rws, 0, SEEK_SET);
            size_t n = purc_variant_serialize(result, rws, 0, 
                PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64 | PCVRNT_SERIALIZE_OPT_TUPLE_EJSON, NULL);
            EXPECT_GT(n, 0);
            buf[n] = 0;
            EXPECT_STREQ(buf, test.expected);

            // Cleanup
            purc_variant_unref(result);
        }

        purc_variant_unref(input1);
        purc_variant_unref(input2);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, iconcat_operator)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for in-place concatenation operations
    struct iconcat_test_case {
        const char* input1;      // First input in JSON format
        const char* input2;      // Second input in JSON format
        const char* expected;    // Expected result after concatenation
        const char* desc;        // Test case description
    };

    // Test cases covering different types and scenarios
    iconcat_test_case test_cases[] = {
        // String concatenation tests
        {
            "\"Hello, \"",
            "\"World!\"",
            "\"Hello, World!\"",
            "Basic string in-place concatenation"
        },
        {
            "\"\"",
            "\"test\"",
            "\"test\"",
            "Empty string in-place concatenation"
        },
        {
            "\"test\"",
            "\"\"",
            "\"test\"",
            "In-place concatenation with empty string"
        },

        // Byte sequence concatenation tests
        {
            "b64SGVsbG8=",
            "b64V29ybGQ=",
            "b64SGVsbG9Xb3JsZA==",
            "Basic byte sequence in-place concatenation"
        },
        {
            "b64",
            "b64SGVsbG8=",
            "b64SGVsbG8=",
            "Empty byte sequence in-place concatenation"
        },
        {
            "b64SGVsbG8=",
            "b64",
            "b64SGVsbG8=",
            "In-place concatenation with empty byte sequence"
        },

        // Array concatenation tests
        {
            "[1, 2, 3]",
            "[4, 5, 6]",
            "[1,2,3,4,5,6]",
            "Basic array in-place concatenation"
        },
        {
            "[]",
            "[1, 2, 3]",
            "[1,2,3]",
            "Empty array in-place concatenation"
        },
        {
            "[1, 2, 3]",
            "[]",
            "[1,2,3]",
            "In-place concatenation with empty array"
        },
        {
            "[\"a\", 1, true]",
            "[null, 2.5, \"b\"]",
            "[\"a\",1,true,null,2.5,\"b\"]",
            "Mixed type array in-place concatenation"
        },

        // Array with tuple as second operand
        {
            "[1, 2, 3]",
            "[!4, 5, 6]",
            "[1,2,3,4,5,6]",
            "Array in-place concatenation with tuple"
        },
        {
            "[\"a\", 1, true]",
            "[!null, 2.5, \"b\"]",
            "[\"a\",1,true,null,2.5,\"b\"]",
            "Array in-place concatenation with mixed type tuple"
        },

        // Invalid cases - tuple as first operand
        {
            "[!1, 2, 3]",
            "[4, 5, 6]",
            nullptr,
            "Invalid: tuple as first operand"
        },
        {
            "[!]",
            "[1, 2, 3]",
            nullptr,
            "Invalid: empty tuple as first operand"
        }
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;

        // Create variants from JSON strings
        purc_variant_t input1 = purc_variant_make_from_json_string(
                test.input1, strlen(test.input1));
        purc_variant_t input2 = purc_variant_make_from_json_string(
                test.input2, strlen(test.input2));

        ASSERT_NE(input1, nullptr);
        ASSERT_NE(input2, nullptr);

        // Perform in-place concatenation operation
        int ret = purc_variant_operator_iconcat(input1, input2);
        if (test.expected == nullptr) {
            EXPECT_EQ(ret, -1);
        }
        else {
            EXPECT_EQ(ret, 0);

            // Serialize and verify result
            purc_rwstream_seek(rws, 0, SEEK_SET);
            size_t n = purc_variant_serialize(input1, rws, 0,
                PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64 | PCVRNT_SERIALIZE_OPT_TUPLE_EJSON, NULL);
            EXPECT_GT(n, 0);
            buf[n] = 0;
            EXPECT_STREQ(buf, test.expected);
        }

        // Cleanup resources
        purc_variant_unref(input1);
        purc_variant_unref(input2);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, bitwise_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for bitwise operations
    struct bitwise_test_case {
        enum purc_variant_type type1;
        enum purc_variant_type type2;  // For binary operators
        const char* op1;               // First operand
        const char* op2;               // Second operand (for binary operators)
        purc_variant_t (*op)(purc_variant_t, purc_variant_t);  // Binary operator function
        const char* expected;          // Expected result after serialization
        const char* desc;              // Test description
    };

    // Test cases for binary bitwise operators
    bitwise_test_case binary_test_cases[] = {
        // AND operator tests
        { 
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "15", "7", 
            purc_variant_operator_and,
            "7L",
            "Basic AND operation with longint"
        },
        { 
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "4294967295", "4294967294",
            purc_variant_operator_and,
            "4294967294UL",
            "AND operation with ulongint"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_LONGINT,
            "340282366920938463463374607431768211455", "7",
            purc_variant_operator_and,
            "7N",
            "AND operation with bigint"
        },

        // OR operator tests
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "12", "7",
            purc_variant_operator_or,
            "15L",
            "Basic OR operation with longint"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "4294967290", "5",
            purc_variant_operator_or,
            "4294967295UL",
            "OR operation with ulongint"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_LONGINT,
            "340282366920938463463374607431768211455", "8",
            purc_variant_operator_or,
            "340282366920938463463374607431768211455N",
            "OR operation with bigint"
        },

        // XOR operator tests
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "15", "7",
            purc_variant_operator_xor,
            "8L",
            "Basic XOR operation with longint"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "4294967295", "1",
            purc_variant_operator_xor,
            "4294967294UL",
            "XOR operation with ulongint"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_LONGINT,
            "340282366920938463463374607431768211455", "15",
            purc_variant_operator_xor,
            "340282366920938463463374607431768211440N",
            "XOR operation with bigint"
        },

        // Left shift tests
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "1", "4",
            purc_variant_operator_lshift,
            "16L",
            "Basic left shift operation"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_LONGINT,
            "1", "31",
            purc_variant_operator_lshift,
            "2147483648UL",
            "Large left shift operation"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_LONGINT,
            "340282366920938463463374607431768211455", "1",
            purc_variant_operator_lshift,
            "680564733841876926926749214863536422910N",
            "Left shift operation with bigint"
        },

        // Right shift tests
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "16", "2",
            purc_variant_operator_rshift,
            "4L",
            "Basic right shift operation"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_LONGINT,
            "4294967295", "1",
            purc_variant_operator_rshift,
            "2147483647UL",
            "Right shift with ulongint"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_LONGINT,
            "340282366920938463463374607431768211455", "1",
            purc_variant_operator_rshift,
            "170141183460469231731687303715884105727N",
            "Right shift operation with bigint"
        }
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Test invert operator separately
    {
        std::cout << "Testing invert operator" << std::endl;

        // Test with longint
        purc_variant_t operand = purc_variant_make_longint(5);
        purc_variant_t result = purc_variant_operator_invert(operand);
        ASSERT_NE(result, nullptr);
        
        purc_rwstream_seek(rws, 0, SEEK_SET);
        size_t n = purc_variant_serialize(result, rws, 0, PCVRNT_SERIALIZE_OPT_REAL_EJSON, NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        ASSERT_STREQ(buf, "-6L");  // ~5 = -6 in two's complement

        purc_variant_unref(result);
        purc_variant_unref(operand);

        // Test with bigint
        operand = purc_variant_make_bigint_from_string("340282366920938463463374607431768211455", nullptr, 0);
        ASSERT_NE(operand, nullptr);

        result = purc_variant_operator_invert(operand);
        ASSERT_NE(result, nullptr);
        
        purc_rwstream_seek(rws, 0, SEEK_SET);
        n = purc_variant_serialize(result, rws, 0, PCVRNT_SERIALIZE_OPT_REAL_EJSON, NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        ASSERT_STREQ(buf, "-340282366920938463463374607431768211456N");

        purc_variant_unref(result);
        purc_variant_unref(operand);
    }

    // Execute binary operator test cases
    for (const auto& test : binary_test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;
        
        purc_variant_t op1 = make_operand_from_string(test.type1, test.op1);
        purc_variant_t op2 = make_operand_from_string(test.type2, test.op2);

        ASSERT_NE(op1, nullptr);
        ASSERT_NE(op2, nullptr);

        // Perform the bitwise operation
        purc_variant_t result = test.op(op1, op2);
        ASSERT_NE(result, nullptr);

        // Serialize and verify
        purc_rwstream_seek(rws, 0, SEEK_SET);
        size_t n = purc_variant_serialize(result, rws, 0, PCVRNT_SERIALIZE_OPT_REAL_EJSON, NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        EXPECT_STREQ(buf, test.expected);

        // Cleanup
        purc_variant_unref(result);
        purc_variant_unref(op1);
        purc_variant_unref(op2);
    }

    // Negative test cases
    {
        std::cout << "Testing negative cases" << std::endl;

        // Test with unsupported type (number)
        purc_variant_t num = purc_variant_make_number(42.0);
        purc_variant_t result = purc_variant_operator_invert(num);
        ASSERT_EQ(result, PURC_VARIANT_INVALID);
        purc_variant_unref(num);

        // Test with invalid shift count
        purc_variant_t val = purc_variant_make_longint(1);
        purc_variant_t invalid_shift = purc_variant_make_longint(-1);
        result = purc_variant_operator_lshift(val, invalid_shift);
        ASSERT_EQ(result, PURC_VARIANT_INVALID);
        purc_variant_unref(val);
        purc_variant_unref(invalid_shift);

        // Test with too large shift count
        val = purc_variant_make_longint(1);
        purc_variant_t large_shift = purc_variant_make_longint(INT64_MAX);
        result = purc_variant_operator_rshift(val, large_shift);
        EXPECT_EQ(result, PURC_VARIANT_INVALID);
        purc_variant_unref(val);
        purc_variant_unref(large_shift);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, inplace_arithmetic_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);
    
    // Test case structure for in-place arithmetic operations
    struct inplace_arithmetic_test_case {
        enum purc_variant_type type1, type2;
        const char* op1;        // First operand
        const char* op2;        // Second operand
        int (*op)(purc_variant_t, purc_variant_t); // In-place operator function
        const char* expected;   // Expected result after serialization
        const char* desc;       // Test description
    };

    // Test cases covering different numeric types and operations
    inplace_arithmetic_test_case test_cases[] = {
        // In-place addition tests
        { 
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "456", "579", purc_variant_operator_iadd, "1035",
            "number += number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "9223372036854775807", "1", purc_variant_operator_iadd, "9223372036854775808",
            "longint += number"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "18446744073709551614", "1", purc_variant_operator_iadd, "18446744073709551615UL",
            "ulongint += ulongint"
        },

        // In-place subtraction tests
        {
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "250", purc_variant_operator_isub, "750",
            "number -= number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "9223372036854775807", "9223372036854775806", purc_variant_operator_isub, "1L",
            "longint -= longint"
        },

        // In-place multiplication tests
        {
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "123", "456", purc_variant_operator_imul, "56088",
            "number *= number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "922337203685477580", "10", purc_variant_operator_imul, "9223372036854775808",
            "longint *= number"
        },

        // In-place true division tests
        {
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_itruediv, "333.33333333333331",
            "number /= number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_itruediv, "5555.5",
            "longint /= number"
        },

        // In-place floor division tests
        {
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_ifloordiv, "333",
            "number //= number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_ifloordiv, "5555",
            "longint //= number"
        },

        // In-place modulo tests
        {
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_imod, "1",
            "number %= number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "11111", "2", purc_variant_operator_imod, "1L",
            "longint %= longint"
        },

        // In-place power tests
        {
            PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "2", "10", purc_variant_operator_ipow, "1024",
            "number **= number"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "2", "30", purc_variant_operator_ipow, "1073741824",
            "longint **= number"
        }
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;
        purc_variant_t op1 = make_operand_from_string(test.type1, test.op1);
        purc_variant_t op2 = make_operand_from_string(test.type2, test.op2);

        ASSERT_NE(op1, nullptr);
        ASSERT_NE(op2, nullptr);

        // Perform the in-place arithmetic operation
        int ret = test.op(op1, op2);
        ASSERT_EQ(ret, 0);

        // Serialize and verify
        purc_rwstream_seek(rws, 0, SEEK_SET);
        size_t n = purc_variant_serialize(op1, rws, 0,
                PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_NOZERO,
                NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        EXPECT_STREQ(buf, test.expected);

        // Cleanup
        purc_variant_unref(op1);
        purc_variant_unref(op2);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, inplace_bitwise_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for in-place bitwise operations
    struct inplace_bitwise_test_case {
        enum purc_variant_type type1, type2;
        const char* op1;        // First operand
        const char* op2;        // Second operand
        int (*op)(purc_variant_t, purc_variant_t); // Operator function
        const char* expected;   // Expected result after serialization
        const char* desc;       // Test description
    };

    // Test cases covering different numeric types and operations
    inplace_bitwise_test_case test_cases[] = {
        // In-place AND tests
        { 
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "15", "7", 
            purc_variant_operator_iand, 
            "7L",
            "Basic longint AND operation"
        },
        { 
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "255", "15", 
            purc_variant_operator_iand, 
            "15UL",
            "Basic ulongint AND operation"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "340282366920938463463374607431768211455", "255",
            purc_variant_operator_iand,
            "255N",
            "Large bigint AND operation"
        },

        // In-place XOR tests
        { 
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "15", "7", 
            purc_variant_operator_ixor, 
            "8L",
            "Basic longint XOR operation"
        },
        { 
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "255", "15", 
            purc_variant_operator_ixor, 
            "240UL",
            "Basic ulongint XOR operation"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "340282366920938463463374607431768211455", "255",
            purc_variant_operator_ixor,
            "340282366920938463463374607431768211200N",
            "Large bigint XOR operation"
        },

        // In-place OR tests
        { 
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "12", "7", 
            purc_variant_operator_ior, 
            "15L",
            "Basic longint OR operation"
        },
        { 
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "240", "15", 
            purc_variant_operator_ior, 
            "255UL",
            "Basic ulongint OR operation"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "340282366920938463463374607431768211200", "255",
            purc_variant_operator_ior,
            "340282366920938463463374607431768211455N",
            "Large bigint OR operation"
        },

        // In-place Left Shift tests
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "1", "3",
            purc_variant_operator_ilshift,
            "8L",
            "Basic longint left shift operation"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "4", "2",
            purc_variant_operator_ilshift,
            "16UL",
            "Basic ulongint left shift operation"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "255", "4",
            purc_variant_operator_ilshift,
            "4080N",
            "Basic bigint left shift operation"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "1", "63",
            purc_variant_operator_ilshift,
            "-9223372036854775808L",
            "Maximum longint left shift operation"
        },

        // In-place Right Shift tests
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "8", "2",
            purc_variant_operator_irshift,
            "2L",
            "Basic longint right shift operation"
        },
        {
            PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "16", "3",
            purc_variant_operator_irshift,
            "2UL",
            "Basic ulongint right shift operation"
        },
        {
            PURC_VARIANT_TYPE_BIGINT, PURC_VARIANT_TYPE_BIGINT,
            "4080", "4",
            purc_variant_operator_irshift,
            "255N",
            "Basic bigint right shift operation"
        },
        {
            PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "-8", "2",
            purc_variant_operator_irshift,
            "-2L",
            "Signed right shift operation"
        }
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;
        
        purc_variant_t op1 = make_operand_from_string(test.type1, test.op1);
        purc_variant_t op2 = make_operand_from_string(test.type2, test.op2);

        ASSERT_NE(op1, nullptr);
        ASSERT_NE(op2, nullptr);

        // Perform the in-place operation
        int result = test.op(op1, op2);
        ASSERT_EQ(result, 0);

        // Serialize and verify
        purc_rwstream_seek(rws, 0, SEEK_SET);
        size_t n = purc_variant_serialize(op1, rws, 0,
                PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_NOZERO,
                NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        EXPECT_STREQ(buf, test.expected);

        // Cleanup
        purc_variant_unref(op1);
        purc_variant_unref(op2);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, comparison_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for comparison operations
    struct comparison_test_case {
        const char* v1_json;     // First value in JSON format
        const char* v2_json;     // Second value in JSON format
        bool is_result;          // Expected result for IS operator
        bool is_not_result;      // Expected result for IS NOT operator
        bool lt_result;          // Expected result for < operator
        bool le_result;          // Expected result for <= operator
        bool gt_result;          // Expected result for > operator
        bool ge_result;          // Expected result for >= operator
        bool eq_result;          // Expected result for == operator
        bool ne_result;          // Expected result for != operator
        const char* desc;        // Test case description
    };

    // Test cases covering different types and scenarios
    comparison_test_case test_cases[] = {
        // Number comparisons
        {
            "42", "42",
            true, false, false, true, false, true, true, false,
            "Equal numbers"
        },
        {
            "42", "43",
            false, true, true, true, false, false, false, true,
            "Different numbers"
        },
        {
            "42.0", "42",
            true, false, false, true, false, true, true, false,
            "Integer and float equality"
        },

        // String comparisons
        {
            "\"hello\"", "\"hello\"",
            true, false, false, true, false, true, true, false,
            "Equal strings"
        },
        {
            "\"hello\"", "\"world\"",
            false, true, true, true, false, false, false, true,
            "Different strings"
        },
        {
            "\"\"", "\"\"",
            true, false, false, true, false, true, true, false,
            "Empty strings"
        },

        // Boolean comparisons
        {
            "true", "true",
            true, false, false, true, false, true, true, false,
            "Equal booleans (true)"
        },
        {
            "false", "false",
            true, false, false, true, false, true, true, false,
            "Equal booleans (false)"
        },
        {
            "true", "false",
            false, true, false, false, true, true, false, true,
            "Different booleans"
        },

        // Null comparisons
        {
            "null", "null",
            true, false, false, true, false, true, true, false,
            "Null equality"
        },
        {
            "null", "42",
            false, true, true, true, false, false, false, true,
            "Null and number comparison"
        },

        // Array comparisons
        {
            "[1,2,3]", "[1,2,3]",
            false, true, false, true, false, true, true, false,
            "Equal arrays"
        },
        {
            "[1,2]", "[1,2,3]",
            false, true, true, true, false, false, false, true,
            "Different length arrays"
        },
        {
            "[1,2,4]", "[1,2,3]",
            false, true, false, false, true, true, false, true,
            "Arrays with different elements"
        },

        // Object comparisons
        {
            "{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}",
            false, true, false, true, false, true, true, false,
            "Equal objects"
        },
        {
            "{\"a\":1}", "{\"a\":1,\"b\":2}",
            false, true, true, true, false, false, false, true,
            "Different objects"
        },

        // Mixed type comparisons
        {
            "42", "\"42\"",
            false, true, false, true, false, true, true, false,
            "Number and string"
        },
        {
            "true", "1",
            false, true, false, true, false, true, true, false,
            "Boolean and number"
        },
        {
            "null", "\"null\"",
            false, true, false, true, false, true, true, false,
            "Null and string"
        }
    };

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;

        // Create variants from JSON strings
        purc_variant_t v1 = purc_variant_make_from_json_string(
                test.v1_json, strlen(test.v1_json));
        purc_variant_t v2 = purc_variant_make_from_json_string(
                test.v2_json, strlen(test.v2_json));

        ASSERT_NE(v1, nullptr);
        ASSERT_NE(v2, nullptr);

        // Test IS operator
        bool is_result = purc_variant_operator_is(v1, v2);
        EXPECT_EQ(is_result, test.is_result);

        // Test IS NOT operator
        bool is_not_result = purc_variant_operator_is_not(v1, v2);
        EXPECT_EQ(is_not_result, test.is_not_result);

        // Test < operator
        bool lt_result = purc_variant_operator_lt(v1, v2);
        EXPECT_EQ(lt_result, test.lt_result);

        // Test <= operator
        bool le_result = purc_variant_operator_le(v1, v2);
        EXPECT_EQ(le_result, test.le_result);

        // Test > operator
        bool gt_result = purc_variant_operator_gt(v1, v2);
        EXPECT_EQ(gt_result, test.gt_result);

        // Test >= operator
        bool ge_result = purc_variant_operator_ge(v1, v2);
        EXPECT_EQ(ge_result, test.ge_result);

        // Test == operator
        bool eq_result = purc_variant_operator_eq(v1, v2);
        EXPECT_EQ(eq_result, test.eq_result);

        // Test != operator
        bool ne_result = purc_variant_operator_ne(v1, v2);
        EXPECT_EQ(ne_result, test.ne_result);

        // Cleanup
        purc_variant_unref(v1);
        purc_variant_unref(v2);
    }

    purc_cleanup();
}

TEST(variant, truth_and_not_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for truth and not operations
    struct truth_test_case {
        const char* input;       // Input value in JSON format
        bool expected_truth;     // Expected result for truth operation
        bool expected_not;       // Expected result for not operation
        const char* desc;        // Test case description
    };

    // Test cases covering different types and scenarios
    truth_test_case test_cases[] = {
        // Boolean tests
        {
            "true",
            true,
            false,
            "True boolean value"
        },
        {
            "false",
            false,
            true,
            "False boolean value"
        },

        // Number tests
        {
            "0",
            false,
            true,
            "Zero number"
        },
        {
            "1",
            true,
            false,
            "Positive number"
        },
        {
            "-1",
            true,
            false,
            "Negative number"
        },
        {
            "0.0",
            false,
            true,
            "Zero float"
        },
        {
            "3.14",
            true,
            false,
            "Non-zero float"
        },

        // String tests
        {
            "\"\"",
            false,
            true,
            "Empty string"
        },
        {
            "\"Hello\"",
            true,
            false,
            "Non-empty string"
        },

        // Array tests
        {
            "[]",
            false,
            true,
            "Empty array"
        },
        {
            "[1,2,3]",
            true,
            false,
            "Non-empty array"
        },

        // Object tests
        {
            "{}",
            false,
            true,
            "Empty object"
        },
        {
            "{\"key\":\"value\"}",
            true,
            false,
            "Non-empty object"
        },

        // Null test
        {
            "null",
            false,
            true,
            "Null value"
        },

        // Byte sequence tests
        {
            "b64",
            false,
            true,
            "Empty byte sequence"
        },
        {
            "b64SGVsbG8=",
            true,
            false,
            "Non-empty byte sequence"
        },

        // Long integer tests
        {
            "0L",
            false,
            true,
            "Zero long integer"
        },
        {
            "123L",
            true,
            false,
            "Positive long integer"
        },
        {
            "-123L",
            true,
            false,
            "Negative long integer"
        },

        // Unsigned long integer tests
        {
            "0UL",
            false,
            true,
            "Zero unsigned long integer"
        },
        {
            "123UL",
            true,
            false,
            "Non-zero unsigned long integer"
        },

        // Long double tests
        {
            "0.0FL",
            false,
            true,
            "Zero long double"
        },
        {
            "3.14FL",
            true,
            false,
            "Non-zero long double"
        }
    };

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;

        // Create variant from JSON string
        purc_variant_t input = purc_variant_make_from_json_string(
                test.input, strlen(test.input));
        ASSERT_NE(input, nullptr);

        // Test truth operation
        bool truth_result = purc_variant_operator_truth(input);
        EXPECT_EQ(truth_result, test.expected_truth)
            << "Truth operation failed for: " << test.desc;

        // Test not operation
        bool not_result = purc_variant_operator_not(input);
        EXPECT_EQ(not_result, test.expected_not)
            << "Not operation failed for: " << test.desc;

        // Cleanup
        purc_variant_unref(input);
    }

    purc_cleanup();
}

TEST(variant, unary_operators)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for unary operations
    struct unary_test_case {
        enum purc_variant_type type;
        const char* operand;        // Input operand
        purc_variant_t (*op)(purc_variant_t);  // Operator function
        const char* expected;   // Expected result after serialization
        const char* desc;       // Test description
    };

    // Test cases covering different numeric types and unary operations
    unary_test_case test_cases[] = {
        // Test cases for BIGINT type
        { PURC_VARIANT_TYPE_BIGINT, "123456789012345678901234567890", purc_variant_operator_abs, "123456789012345678901234567890N",
            "abs of positive bigint" },
        { PURC_VARIANT_TYPE_BIGINT, "-123456789012345678901234567890", purc_variant_operator_abs, "123456789012345678901234567890N",
            "abs of negative bigint" },
        { PURC_VARIANT_TYPE_BIGINT, "123456789012345678901234567890", purc_variant_operator_neg, "-123456789012345678901234567890N",
            "negation of positive bigint" },
        { PURC_VARIANT_TYPE_BIGINT, "-123456789012345678901234567890", purc_variant_operator_neg, "123456789012345678901234567890N",
            "negation of negative bigint" },
        { PURC_VARIANT_TYPE_BIGINT, "123456789012345678901234567890", purc_variant_operator_pos, "123456789012345678901234567890N",
            "positive of positive bigint" },
        { PURC_VARIANT_TYPE_BIGINT, "-123456789012345678901234567890", purc_variant_operator_pos, "-123456789012345678901234567890N",
            "positive of negative bigint" },

        // Absolute value tests
        { PURC_VARIANT_TYPE_NUMBER, "-456", purc_variant_operator_abs, "456",
            "abs of negative number" },
        { PURC_VARIANT_TYPE_NUMBER, "456", purc_variant_operator_abs, "456",
            "abs of positive number" },
        { PURC_VARIANT_TYPE_LONGINT, "-9223372036854775807", purc_variant_operator_abs, "9223372036854775807L",
            "abs of negative longint" },
        { PURC_VARIANT_TYPE_LONGINT, "9223372036854775807", purc_variant_operator_abs, "9223372036854775807L",
            "abs of positive longint" },
        { PURC_VARIANT_TYPE_LONGDOUBLE, "-1.5e308", purc_variant_operator_abs, "1.5e+308FL",
            "abs of negative longdouble" },
        { PURC_VARIANT_TYPE_LONGDOUBLE, "1.5e308", purc_variant_operator_abs, "1.5e+308FL",
            "abs of positive longdouble" },

        // Negation tests
        { PURC_VARIANT_TYPE_NUMBER, "456", purc_variant_operator_neg, "-456",
            "negation of positive number" },
        { PURC_VARIANT_TYPE_NUMBER, "-456", purc_variant_operator_neg, "456",
            "negation of negative number" },
        { PURC_VARIANT_TYPE_LONGINT, "9223372036854775807", purc_variant_operator_neg, "-9223372036854775807L",
            "negation of positive longint" },
        { PURC_VARIANT_TYPE_LONGINT, "-9223372036854775807", purc_variant_operator_neg, "9223372036854775807L",
            "negation of negative longint" },
        { PURC_VARIANT_TYPE_LONGDOUBLE, "1.5e308", purc_variant_operator_neg, "-1.5e+308FL",
            "negation of positive longdouble" },
        { PURC_VARIANT_TYPE_LONGDOUBLE, "-1.5e308", purc_variant_operator_neg, "1.5e+308FL",
            "negation of negative longdouble" },

        // Positive tests (should maintain the same value)
        { PURC_VARIANT_TYPE_NUMBER, "456", purc_variant_operator_pos, "456",
            "positive of positive number" },
        { PURC_VARIANT_TYPE_NUMBER, "-456", purc_variant_operator_pos, "-456",
            "positive of negative number" },
        { PURC_VARIANT_TYPE_LONGINT, "9223372036854775807", purc_variant_operator_pos, "9223372036854775807L",
            "positive of positive longint" },
        { PURC_VARIANT_TYPE_LONGINT, "-9223372036854775807", purc_variant_operator_pos, "-9223372036854775807L",
            "positive of negative longint" },
        { PURC_VARIANT_TYPE_LONGDOUBLE, "1.5e308", purc_variant_operator_pos, "1.5e+308FL",
            "positive of positive longdouble" },
        { PURC_VARIANT_TYPE_LONGDOUBLE, "-1.5e308", purc_variant_operator_pos, "-1.5e+308FL",
            "positive of negative longdouble" },
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;
        purc_variant_t operand = make_operand_from_string(test.type, test.operand);
        ASSERT_NE(operand, nullptr);

        // Perform the unary operation
        purc_variant_t result = test.op(operand);
        ASSERT_NE(result, nullptr);

        // Serialize and verify
        purc_rwstream_seek(rws, 0, SEEK_SET);
        size_t n = purc_variant_serialize(result, rws, 0,
                PCVRNT_SERIALIZE_OPT_REAL_EJSON | PCVRNT_SERIALIZE_OPT_NOZERO,
                NULL);
        ASSERT_GT(n, 0);
        buf[n] = 0;
        EXPECT_STREQ(buf, test.expected);

        // Cleanup
        purc_variant_unref(result);
        purc_variant_unref(operand);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

TEST(variant, unary_operators_alt)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    // Test case structure for unary operations
    struct unary_test_case {
        const char* input;       // Input in JSON format
        const char* abs_result;  // Expected result for abs operation
        const char* neg_result;  // Expected result for neg operation
        const char* pos_result;  // Expected result for pos operation
        const char* desc;        // Test case description
    };

    // Test cases for different data types
    unary_test_case test_cases[] = {
        // Test for undefined
        {
            "undefined",
            nullptr,    // abs should return null
            nullptr,    // neg should return null
            nullptr,    // pos should return null
            "Testing undefined value"
        },

        // Test for null
        {
            "null",
            "0",       // abs(null) = 0
            "-0",       // neg(null) = 0
            "0",       // pos(null) = 0
            "Testing null value"
        },

        // Test for true
        {
            "true",
            "1",       // abs(true) = 1
            "-1",      // neg(true) = -1
            "1",       // pos(true) = 1
            "Testing true value"
        },

        // Test for false
        {
            "false",
            "0",       // abs(false) = 0
            "-0",       // neg(false) = 0
            "0",       // pos(false) = 0
            "Testing false value"
        },

        // Test for array
        {
            "[1,2,3]",
            nullptr,    // abs should return null
            nullptr,    // neg should return null
            nullptr,    // pos should return null
            "Testing array"
        },

        // Test for tuple
        {
            "[!1,2,3]",
            nullptr,    // abs should return null
            nullptr,    // neg should return null
            nullptr,    // pos should return null
            "Testing tuple"
        },

        // Test for object
        {
            "{\"key\":\"value\"}",
            nullptr,    // abs should return null
            nullptr,    // neg should return null
            nullptr,    // pos should return null
            "Testing object"
        }
    };

    // Create stream for serialization
    char buf[2048];
    purc_rwstream_t rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(rws, nullptr);

    // Execute all test cases
    for (const auto& test : test_cases) {
        std::cout << "Testing: " << test.desc << std::endl;

        // Create variant from JSON string
        purc_variant_t input = purc_variant_make_from_json_string(
                test.input, strlen(test.input));
        ASSERT_NE(input, nullptr);

        // Test abs operator
        purc_variant_t abs_result = purc_variant_operator_abs(input);
        if (test.abs_result == nullptr) {
            EXPECT_EQ(abs_result, PURC_VARIANT_INVALID);
        }
        else {
            ASSERT_NE(abs_result, nullptr);
            purc_rwstream_seek(rws, 0, SEEK_SET);
            size_t n = purc_variant_serialize(abs_result, rws, 0, 0, NULL);
            EXPECT_GT(n, 0);
            buf[n] = 0;
            EXPECT_STREQ(buf, test.abs_result);
            purc_variant_unref(abs_result);
        }

        // Test neg operator
        purc_variant_t neg_result = purc_variant_operator_neg(input);
        if (test.neg_result == nullptr) {
            EXPECT_EQ(neg_result, PURC_VARIANT_INVALID);
        }
        else {
            ASSERT_NE(neg_result, nullptr);
            purc_rwstream_seek(rws, 0, SEEK_SET);
            size_t n = purc_variant_serialize(neg_result, rws, 0, 0, NULL);
            EXPECT_GT(n, 0);
            buf[n] = 0;
            EXPECT_STREQ(buf, test.neg_result);
            purc_variant_unref(neg_result);
        }

        // Test pos operator
        purc_variant_t pos_result = purc_variant_operator_pos(input);
        if (test.pos_result == nullptr) {
            EXPECT_EQ(pos_result, PURC_VARIANT_INVALID);
        }
        else {
            ASSERT_NE(pos_result, nullptr);
            purc_rwstream_seek(rws, 0, SEEK_SET);
            size_t n = purc_variant_serialize(pos_result, rws, 0, 0, NULL);
            EXPECT_GT(n, 0);
            buf[n] = 0;
            EXPECT_STREQ(buf, test.pos_result);
            purc_variant_unref(pos_result);
        }

        purc_variant_unref(input);
    }

    purc_rwstream_destroy(rws);
    purc_cleanup();
}

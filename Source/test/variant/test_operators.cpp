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

        // Subtraction tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "250", purc_variant_operator_sub, "750"},  // number - number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_LONGINT,
            "9223372036854775807", "9223372036854775806", purc_variant_operator_sub, "1L"},  // longint - longint
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "18446744073709551615", "18446744073709551614", purc_variant_operator_sub, "1UL"},  // ulongint - ulongint

        // Multiplication tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "123", "456", purc_variant_operator_mul, "56088"},  // number * number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "922337203685477580", "10", purc_variant_operator_mul, "9223372036854775808"},  // longint * number
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_NUMBER,
            "1844674407370955161", "10", purc_variant_operator_mul, "18446744073709551616"},  // ulongint * number

        // True division tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_truediv, "333.33333333333331"},  // number / number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_truediv, "5555.5"},  // longint / number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "9223372036854775807", "2", purc_variant_operator_truediv, "4611686018427387904"},  // longint / number

        // Floor division tests
        { PURC_VARIANT_TYPE_NUMBER, PURC_VARIANT_TYPE_NUMBER,
            "1000", "3", purc_variant_operator_floordiv, "333"},  // number // number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "11111", "2", purc_variant_operator_floordiv, "5555"},  // longint // number
        { PURC_VARIANT_TYPE_LONGINT, PURC_VARIANT_TYPE_NUMBER,
            "9223372036854775807", "2", purc_variant_operator_floordiv, "4611686018427387904"},  // longint // number

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
            "9223372036854775807", "16", purc_variant_operator_pow, "2743062034396844336869514018464698837952741034352782431735406935422555235659604611574795800485902102589878063855381220980247414149652079643899138017548027873771831513201398226700753025465497615356604597023149336546797754176993249443973844794089529533475153606348844332504619566761300314793168852746240001n"},  // longint ** longint
        { PURC_VARIANT_TYPE_ULONGINT, PURC_VARIANT_TYPE_ULONGINT,
            "18446744073709551615", "50", purc_variant_operator_pow, "1976906478982563988295810691554050614399651424488030850762343678931647978618325561474974086879022536362639642807744003756529842940963577394276662468582411572670825043959009899694489661532504088009120877946903639143166499456428438448880195155499253110091516188286694321777197119674227599132338454581472690647559478166682718820606301933576867149263606282479822069977062577699422966456063707753586195851786206483814160770891312582986610957317552176331001104119220689137430267970695253598479625236453588946233916795030393521191859493297484330703657084378498022511947929753899678606763593843427503615639810405259276058581346303867750708563882561183560546452553196090076061939041007446872127913157825040863114845973565294859768831735607624294122356928888311315808677423812406463069338456801790310944262553921204739016074704752181719029665060048844403891050051589949396129274357516548225563425373464972431095290923353938157423423230896109004106619977392256259918212890625n"},  // ulongint ** ulongint
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


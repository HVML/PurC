/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
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
**
** Author: Vincent Wei <https://github.com/VincentWei>
*/

// NOTE
// This file is auto-generated by using 'make_hvml_tags_table.py'.
// Please take care when you modify this file mannually.

static const myhvml_tag_context_t myhvml_tag_base_list[MyHVML_TAG_LAST_ENTRY] =
{
    { MyHVML_TAG__UNDEF, "-undef", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG__TEXT, "-text", 5, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG__COMMENT, "_comment", 8, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG__FOREIGN, "_foreign", 8, MyHVML_TOKENIZER_STATE_RAWTEXT,
        MyHVML_TAG_CATEGORIES_FOREIGN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG__DOCTYPE, "!DOCTYPE", 8, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_SPECIAL },
    { MyHVML_TAG_ARCHEDATA, "archedata", 9, MyHVML_TOKENIZER_STATE_RAWTEXT,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_SPECIAL | MyHVML_TAG_CATEGORIES_SCOPE },
    { MyHVML_TAG_ARCHETYPE, "archetype", 9, MyHVML_TOKENIZER_STATE_RAWTEXT,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_SPECIAL | MyHVML_TAG_CATEGORIES_SCOPE },
    { MyHVML_TAG_BACK, "back", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_BODY, "body", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_CALL, "call", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_CATCH, "catch", 5, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_CLOSE, "close", 5, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_DEFINE, "define", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_EMPTY, "empty", 5, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_ERROR, "error", 5, MyHVML_TOKENIZER_STATE_RAWTEXT,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_EXCEPT, "except", 6, MyHVML_TOKENIZER_STATE_RAWTEXT,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_HEAD, "head", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_HVML, "hvml", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_INIT, "init", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_INCLUDE, "include", 7, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_ITERATE, "iterate", 7, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_LISTEN, "listen", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_LOAD, "load", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_MATCH, "match", 5, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_OBSERVE, "observe", 7, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_RAWPART, "rawpart", 7, MyHVML_TOKENIZER_STATE_RAWTEXT,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_SPECIAL },
    { MyHVML_TAG_REMOVE, "remove", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_REQUEST, "request", 7, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_REDUCE, "reduce", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_RETURN, "return", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_SET, "set", 3, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_TEST, "test", 4, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG_UPDATE, "update", 6, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_VERB | MyHVML_TAG_CATEGORIES_ORDINARY },
    { MyHVML_TAG__END_OF_FILE, "-end-of-file", 12, MyHVML_TOKENIZER_STATE_DATA,
        MyHVML_TAG_CATEGORIES_NOUN | MyHVML_TAG_CATEGORIES_ORDINARY },
};

#if SIZEOF_PTR == 8

#define MyHVML_BASE_STATIC_SIZE 35

static const myhvml_tag_static_list_t myhvml_tag_static_list_index[] =
{
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x656069f20e4b131e, slot index: 2
   { &myhvml_tag_base_list[MyHVML_TAG__FOREIGN], 46},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x9ec2699513c0f9c3, slot index: 6
   { &myhvml_tag_base_list[MyHVML_TAG_CLOSE], 0},
   // hash value of this tag: 0x03c43d0057227e28, slot index: 7
   { &myhvml_tag_base_list[MyHVML_TAG_RAWPART], 0},
   // hash value of this tag: 0xe424d04ed760ceb5, slot index: 8
   { &myhvml_tag_base_list[MyHVML_TAG_ITERATE], 0},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x71a3752fc11f9ec3, slot index: 12
   { &myhvml_tag_base_list[MyHVML_TAG__COMMENT], 0},
   // hash value of this tag: 0x2f92df0bac03dce7, slot index: 13
   { &myhvml_tag_base_list[MyHVML_TAG_REDUCE], 0},
   // hash value of this tag: 0xce0eecad70f271e9, slot index: 14
   { &myhvml_tag_base_list[MyHVML_TAG_LOAD], 47},
   // hash value of this tag: 0xc70dfcbcd32b48ac, slot index: 15
   { &myhvml_tag_base_list[MyHVML_TAG_ARCHEDATA], 0},
   // hash value of this tag: 0x98f994ee29d30eb2, slot index: 16
   { &myhvml_tag_base_list[MyHVML_TAG__DOCTYPE], 45},
   // hash value of this tag: 0x9a46ff9baa276602, slot index: 17
   { &myhvml_tag_base_list[MyHVML_TAG_BACK], 0},
   // hash value of this tag: 0xb5167a90e83ba4a9, slot index: 18
   { &myhvml_tag_base_list[MyHVML_TAG_CALL], 38},
   // hash value of this tag: 0x9f7452dd75d54d31, slot index: 19
   { &myhvml_tag_base_list[MyHVML_TAG_ERROR], 36},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x904fefb3d01cb2ae, slot index: 24
   { &myhvml_tag_base_list[MyHVML_TAG_EMPTY], 44},
   // hash value of this tag: 0x5920d140d70abeec, slot index: 25
   { &myhvml_tag_base_list[MyHVML_TAG_ARCHETYPE], 42},
   // hash value of this tag: 0x183719c704a3501f, slot index: 26
   { &myhvml_tag_base_list[MyHVML_TAG__TEXT], 43},
   // hash value of this tag: 0xc1b2e33b13ec076c, slot index: 27
   { &myhvml_tag_base_list[MyHVML_TAG_CATCH], 40},
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x6e815727576f5930, slot index: 30
   { &myhvml_tag_base_list[MyHVML_TAG_EXCEPT], 37},
   // hash value of this tag: 0x4c56130c429d6d92, slot index: 31
   { &myhvml_tag_base_list[MyHVML_TAG_DEFINE], 0},
   // hash value of this tag: 0xa0ad68312a3d0454, slot index: 32
   { &myhvml_tag_base_list[MyHVML_TAG_UPDATE], 0},
   // hash value of this tag: 0x6f4743b8bc5b4eb2, slot index: 33
   { &myhvml_tag_base_list[MyHVML_TAG_REQUEST], 0},
   // hash value of this tag: 0x86b65d6cf338391a, slot index: 34
   { &myhvml_tag_base_list[MyHVML_TAG__UNDEF], 39},
   // hash value of this tag: 0xcd4de79bc6c93295, slot index: 16
   { &myhvml_tag_base_list[MyHVML_TAG_BODY], 0},
   // hash value of this tag: 0x0a8f12cc5f9a0c03, slot index: 19
   { &myhvml_tag_base_list[MyHVML_TAG_HEAD], 0},
   // hash value of this tag: 0x79fc63cc0e796102, slot index: 30
   { &myhvml_tag_base_list[MyHVML_TAG_HVML], 0},
   // hash value of this tag: 0xf5d2afc57ab57213, slot index: 18
   { &myhvml_tag_base_list[MyHVML_TAG_INIT], 0},
   // hash value of this tag: 0xc71bef1c7cd467a7, slot index: 34
   { &myhvml_tag_base_list[MyHVML_TAG_INCLUDE], 0},
   // hash value of this tag: 0xdc8acb313473c026, slot index: 27
   { &myhvml_tag_base_list[MyHVML_TAG_LISTEN], 0},
   // hash value of this tag: 0xc3bfe3a4fe4c13f6, slot index: 2
   { &myhvml_tag_base_list[MyHVML_TAG_MATCH], 0},
   // hash value of this tag: 0xd9760e5c71e67b3f, slot index: 25
   { &myhvml_tag_base_list[MyHVML_TAG_OBSERVE], 0},
   // hash value of this tag: 0xfff83d536a1d457d, slot index: 26
   { &myhvml_tag_base_list[MyHVML_TAG_REMOVE], 0},
   // hash value of this tag: 0xc5c7b983377cad5f, slot index: 24
   { &myhvml_tag_base_list[MyHVML_TAG_RETURN], 0},
   // hash value of this tag: 0x823b87195ce20e23, slot index: 16
   { &myhvml_tag_base_list[MyHVML_TAG_SET], 0},
   // hash value of this tag: 0xf9e6e6ef197c2b25, slot index: 2
   { &myhvml_tag_base_list[MyHVML_TAG_TEST], 0},
   // hash value of this tag: 0xe4a0f45ae7363d42, slot index: 14
   { &myhvml_tag_base_list[MyHVML_TAG__END_OF_FILE], 0},
};

#else   /* SIZEOF_PTR == 4 */

#define MyHVML_BASE_STATIC_SIZE 38

static const myhvml_tag_static_list_t myhvml_tag_static_list_index[] =
{
   // hash value of this tag: 0x000000007e202f96, slot index: 0
   { &myhvml_tag_base_list[MyHVML_TAG_MATCH], 0},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x000000006a9f8552, slot index: 4
   { &myhvml_tag_base_list[MyHVML_TAG_DEFINE], 42},
   { NULL, 0},
   // hash value of this tag: 0x0000000056664acc, slot index: 6
   { &myhvml_tag_base_list[MyHVML_TAG_ARCHETYPE], 38},
   // hash value of this tag: 0x00000000db9215fd, slot index: 7
   { &myhvml_tag_base_list[MyHVML_TAG_REMOVE], 43},
   { NULL, 0},
   // hash value of this tag: 0x0000000086eeeedf, slot index: 9
   { &myhvml_tag_base_list[MyHVML_TAG_OBSERVE], 0},
   { NULL, 0},
   // hash value of this tag: 0x0000000027cb3b23, slot index: 11
   { &myhvml_tag_base_list[MyHVML_TAG_CLOSE], 0},
   // hash value of this tag: 0x00000000baa7f502, slot index: 12
   { &myhvml_tag_base_list[MyHVML_TAG__END_OF_FILE], 0},
   // hash value of this tag: 0x0000000016b1d373, slot index: 13
   { &myhvml_tag_base_list[MyHVML_TAG_INIT], 45},
   // hash value of this tag: 0x0000000069f6105a, slot index: 14
   { &myhvml_tag_base_list[MyHVML_TAG__UNDEF], 40},
   // hash value of this tag: 0x00000000b3f184a9, slot index: 15
   { &myhvml_tag_base_list[MyHVML_TAG_CALL], 46},
   { NULL, 0},
   // hash value of this tag: 0x00000000dbaa7975, slot index: 17
   { &myhvml_tag_base_list[MyHVML_TAG_BODY], 0},
   // hash value of this tag: 0x0000000038adcbf0, slot index: 18
   { &myhvml_tag_base_list[MyHVML_TAG_EXCEPT], 0},
   // hash value of this tag: 0x00000000e60759e9, slot index: 19
   { &myhvml_tag_base_list[MyHVML_TAG_LOAD], 0},
   // hash value of this tag: 0x000000005bb421a2, slot index: 20
   { &myhvml_tag_base_list[MyHVML_TAG_BACK], 0},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   { NULL, 0},
   // hash value of this tag: 0x0000000072ca100c, slot index: 26
   { &myhvml_tag_base_list[MyHVML_TAG_ARCHEDATA], 0},
   { NULL, 0},
   // hash value of this tag: 0x000000004288e94c, slot index: 28
   { &myhvml_tag_base_list[MyHVML_TAG_CATCH], 0},
   // hash value of this tag: 0x00000000b9696f03, slot index: 29
   { &myhvml_tag_base_list[MyHVML_TAG__COMMENT], 39},
   // hash value of this tag: 0x000000009bc6b072, slot index: 30
   { &myhvml_tag_base_list[MyHVML_TAG__DOCTYPE], 41},
   // hash value of this tag: 0x000000004223db9f, slot index: 31
   { &myhvml_tag_base_list[MyHVML_TAG__TEXT], 0},
   // hash value of this tag: 0x000000007b39379e, slot index: 32
   { &myhvml_tag_base_list[MyHVML_TAG__FOREIGN], 0},
   // hash value of this tag: 0x0000000075597a67, slot index: 33
   { &myhvml_tag_base_list[MyHVML_TAG_INCLUDE], 44},
   { NULL, 0},
   // hash value of this tag: 0x0000000032694bc3, slot index: 35
   { &myhvml_tag_base_list[MyHVML_TAG_HEAD], 0},
   // hash value of this tag: 0x000000003b3b76a6, slot index: 36
   { &myhvml_tag_base_list[MyHVML_TAG_LISTEN], 47},
   // hash value of this tag: 0x0000000081998a35, slot index: 37
   { &myhvml_tag_base_list[MyHVML_TAG_ITERATE], 0},
   // hash value of this tag: 0x0000000018a7beee, slot index: 6
   { &myhvml_tag_base_list[MyHVML_TAG_EMPTY], 0},
   // hash value of this tag: 0x0000000021918751, slot index: 29
   { &myhvml_tag_base_list[MyHVML_TAG_ERROR], 0},
   // hash value of this tag: 0x00000000fff3c0e2, slot index: 14
   { &myhvml_tag_base_list[MyHVML_TAG_HVML], 0},
   // hash value of this tag: 0x000000001e2be028, slot index: 30
   { &myhvml_tag_base_list[MyHVML_TAG_RAWPART], 0},
   // hash value of this tag: 0x000000003eaa09b2, slot index: 4
   { &myhvml_tag_base_list[MyHVML_TAG_REQUEST], 0},
   // hash value of this tag: 0x0000000077548ee7, slot index: 7
   { &myhvml_tag_base_list[MyHVML_TAG_REDUCE], 0},
   // hash value of this tag: 0x0000000085ee37bf, slot index: 33
   { &myhvml_tag_base_list[MyHVML_TAG_RETURN], 0},
   // hash value of this tag: 0x00000000c6270703, slot index: 13
   { &myhvml_tag_base_list[MyHVML_TAG_SET], 0},
   // hash value of this tag: 0x00000000afd071e5, slot index: 15
   { &myhvml_tag_base_list[MyHVML_TAG_TEST], 0},
   // hash value of this tag: 0x00000000280f9474, slot index: 36
   { &myhvml_tag_base_list[MyHVML_TAG_UPDATE], 0},
};

#endif  /* SIZEOF_PTR == 4 */


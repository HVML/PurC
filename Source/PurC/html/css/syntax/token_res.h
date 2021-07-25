/**
 * @file token_res.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for resource of css token.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef PCHTML_CSS_SYNTAX_TOKEN_RES_H
#define PCHTML_CSS_SYNTAX_TOKEN_RES_H


#ifdef PCHTML_CSS_SYNTAX_TOKEN_RES_NAME_SHS_MAP
#ifndef PCHTML_CSS_SYNTAX_TOKEN_RES_NAME_SHS_MAP_ENABLED
#define PCHTML_CSS_SYNTAX_TOKEN_RES_NAME_SHS_MAP_ENABLED
static const pchtml_shs_entry_t pchtml_css_syntax_token_res_name_shs_map[] =
{
    {NULL, NULL, 92, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"end-of-file", (void *) PCHTML_CSS_SYNTAX_TOKEN__EOF, 11, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {(char *)"ident", (void *) PCHTML_CSS_SYNTAX_TOKEN_IDENT, 5, 0}, {(char *)"cdo", (void *) PCHTML_CSS_SYNTAX_TOKEN_CDO, 3, 0},
    {NULL, NULL, 0, 0}, {(char *)"left-parenthesis", (void *) PCHTML_CSS_SYNTAX_TOKEN_L_PARENTHESIS, 16, 0},
    {(char *)"right-parenthesis", (void *) PCHTML_CSS_SYNTAX_TOKEN_R_PARENTHESIS, 17, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"percentage", (void *) PCHTML_CSS_SYNTAX_TOKEN_PERCENTAGE, 10, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"at-keyword", (void *) PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD, 10, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {(char *)"string", (void *) PCHTML_CSS_SYNTAX_TOKEN_STRING, 6, 0}, {NULL, NULL, 0, 0},
    {(char *)"bad-url", (void *) PCHTML_CSS_SYNTAX_TOKEN_BAD_URL, 7, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"bad-string", (void *) PCHTML_CSS_SYNTAX_TOKEN_BAD_STRING, 10, 0},
    {(char *)"whitespace", (void *) PCHTML_CSS_SYNTAX_TOKEN_WHITESPACE, 10, 0}, {NULL, NULL, 0, 0},
    {(char *)"undefined", (void *) PCHTML_CSS_SYNTAX_TOKEN_UNDEF, 9, 0}, {NULL, NULL, 0, 0},
    {(char *)"right-curly-bracket", (void *) PCHTML_CSS_SYNTAX_TOKEN_RC_BRACKET, 19, 0}, {(char *)"right-square-bracket", (void *) PCHTML_CSS_SYNTAX_TOKEN_RS_BRACKET, 20, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"number", (void *) PCHTML_CSS_SYNTAX_TOKEN_NUMBER, 6, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {(char *)"semicolon", (void *) PCHTML_CSS_SYNTAX_TOKEN_SEMICOLON, 9, 0}, {NULL, NULL, 0, 0},
    {(char *)"dimension", (void *) PCHTML_CSS_SYNTAX_TOKEN_DIMENSION, 9, 0}, {NULL, NULL, 0, 0},
    {(char *)"colon", (void *) PCHTML_CSS_SYNTAX_TOKEN_COLON, 5, 0}, {(char *)"function", (void *) PCHTML_CSS_SYNTAX_TOKEN_FUNCTION, 8, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"comma", (void *) PCHTML_CSS_SYNTAX_TOKEN_COMMA, 5, 0},
    {(char *)"url", (void *) PCHTML_CSS_SYNTAX_TOKEN_URL, 3, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"cdc", (void *) PCHTML_CSS_SYNTAX_TOKEN_CDC, 3, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"hash", (void *) PCHTML_CSS_SYNTAX_TOKEN_HASH, 4, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {(char *)"comment", (void *) PCHTML_CSS_SYNTAX_TOKEN_COMMENT, 7, 0}, {NULL, NULL, 0, 0},
    {(char *)"delim", (void *) PCHTML_CSS_SYNTAX_TOKEN_DELIM, 5, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0}, {(char *)"left-curly-bracket", (void *) PCHTML_CSS_SYNTAX_TOKEN_LC_BRACKET, 18, 0},
    {(char *)"left-square-bracket", (void *) PCHTML_CSS_SYNTAX_TOKEN_LS_BRACKET, 19, 0}
};
#endif  /* PCHTML_CSS_SYNTAX_TOKEN_RES_NAME_SHS_MAP_ENABLED */
#endif  /* PCHTML_CSS_SYNTAX_TOKEN_RES_NAME_SHS_MAP */


#endif  /* PCHTML_CSS_SYNTAX_TOKEN_RES_H */

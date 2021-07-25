/**
 * @file state_res.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for resource of css state.
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


#ifndef PCHTML_CSS_SYNTAX_STATE_RES_H
#define PCHTML_CSS_SYNTAX_STATE_RES_H


static const pchtml_css_syntax_tokenizer_state_f
pchtml_css_syntax_state_res_map[256] =
{
    pchtml_css_syntax_state_eof, /* 0x00; 'NUL'; NULL */
    pchtml_css_syntax_state_delim, /* 0x01; 'SOH'; Start of Heading */
    pchtml_css_syntax_state_delim, /* 0x02; 'STX'; Start of text */
    pchtml_css_syntax_state_delim, /* 0x03; 'ETX'; End of text */
    pchtml_css_syntax_state_delim, /* 0x04; 'EOT'; End of Transmission */
    pchtml_css_syntax_state_delim, /* 0x05; 'ENQ'; Enquiry */
    pchtml_css_syntax_state_delim, /* 0x06; 'ACK'; Acknowledge */
    pchtml_css_syntax_state_delim, /* 0x07; 'BEL'; Bell */
    pchtml_css_syntax_state_delim, /* 0x08; 'BS'; Backspace */
    pchtml_css_syntax_state_whitespace, /* 0x09; 'TAB'; Horizontal Tab */
    pchtml_css_syntax_state_whitespace, /* 0x0A; 'LF'; Line Feed ('\n') */
    pchtml_css_syntax_state_delim, /* 0x0B; 'VT'; Vertical Tab */
    pchtml_css_syntax_state_whitespace, /* 0x0C; 'FF'; Form Feed */
    pchtml_css_syntax_state_whitespace, /* 0x0D; 'CR'; Carriage Return ('\r') */
    pchtml_css_syntax_state_delim, /* 0x0E; 'SO'; Shift Out */
    pchtml_css_syntax_state_delim, /* 0x0F; 'SI'; Shift In */
    pchtml_css_syntax_state_delim, /* 0x10; 'DLE'; Data Link Escape */
    pchtml_css_syntax_state_delim, /* 0x11; 'DC1'; Device Control #1 */
    pchtml_css_syntax_state_delim, /* 0x12; 'DC2'; Device Control #2 */
    pchtml_css_syntax_state_delim, /* 0x13; 'DC3'; Device Control #3 */
    pchtml_css_syntax_state_delim, /* 0x14; 'DC4'; Device Control #4 */
    pchtml_css_syntax_state_delim, /* 0x15; 'NAK'; Negative Acknowledge */
    pchtml_css_syntax_state_delim, /* 0x16; 'SYN'; Synchronous Idle */
    pchtml_css_syntax_state_delim, /* 0x17; 'ETB'; End of Transmission Block */
    pchtml_css_syntax_state_delim, /* 0x18; 'CAN'; Cancel */
    pchtml_css_syntax_state_delim, /* 0x19; 'EM'; End of Medium */
    pchtml_css_syntax_state_delim, /* 0x1A; 'SUB'; Substitute */
    pchtml_css_syntax_state_delim, /* 0x1B; 'ESC'; Escape */
    pchtml_css_syntax_state_delim, /* 0x1C; 'FS'; File Separator */
    pchtml_css_syntax_state_delim, /* 0x1D; 'GS'; Group Separator */
    pchtml_css_syntax_state_delim, /* 0x1E; 'RS'; Record Separator */
    pchtml_css_syntax_state_delim, /* 0x1F; 'US'; Unit Separator */
    pchtml_css_syntax_state_whitespace, /* 0x20; 'SP'; Space */
    pchtml_css_syntax_state_delim, /* 0x21; '!'; Exclamation mark */
    pchtml_css_syntax_state_string, /* 0x22; '"'; Only quotes above */
    pchtml_css_syntax_state_hash, /* 0x23; '#'; Pound sign */
    pchtml_css_syntax_state_delim, /* 0x24; '$'; Dollar sign */
    pchtml_css_syntax_state_delim, /* 0x25; '%'; Percentage sign */
    pchtml_css_syntax_state_delim, /* 0x26; '&'; Commericial and */
    pchtml_css_syntax_state_string, /* 0x27; '''; Apostrophe */
    pchtml_css_syntax_state_lparenthesis, /* 0x28; '('; Left bracket */
    pchtml_css_syntax_state_rparenthesis, /* 0x29; ')'; Right bracket */
    pchtml_css_syntax_state_delim, /* 0x2A; '*'; Asterisk */
    pchtml_css_syntax_state_plus, /* 0x2B; '+'; Plus symbol */
    pchtml_css_syntax_state_comma, /* 0x2C; ','; Comma */
    pchtml_css_syntax_state_minus, /* 0x2D; '-'; Dash */
    pchtml_css_syntax_state_full_stop, /* 0x2E; '.'; Full stop */
    pchtml_css_syntax_state_comment_begin, /* 0x2F; '/'; Forward slash */
    pchtml_css_syntax_consume_before_numeric, /* 0x30; '0' */
    pchtml_css_syntax_consume_before_numeric, /* 0x31; '1' */
    pchtml_css_syntax_consume_before_numeric, /* 0x32; '2' */
    pchtml_css_syntax_consume_before_numeric, /* 0x33; '3' */
    pchtml_css_syntax_consume_before_numeric, /* 0x34; '4' */
    pchtml_css_syntax_consume_before_numeric, /* 0x35; '5' */
    pchtml_css_syntax_consume_before_numeric, /* 0x36; '6' */
    pchtml_css_syntax_consume_before_numeric, /* 0x37; '7' */
    pchtml_css_syntax_consume_before_numeric, /* 0x38; '8' */
    pchtml_css_syntax_consume_before_numeric, /* 0x39; '9' */
    pchtml_css_syntax_state_colon, /* 0x3A; ':'; Colon */
    pchtml_css_syntax_state_semicolon, /* 0x3B; ';'; Semicolon */
    pchtml_css_syntax_state_less_sign, /* 0x3C; '<'; Small than bracket */
    pchtml_css_syntax_state_delim, /* 0x3D; '='; Equals sign */
    pchtml_css_syntax_state_delim, /* 0x3E; '>'; Bigger than symbol */
    pchtml_css_syntax_state_delim, /* 0x3F; '?'; Question mark */
    pchtml_css_syntax_state_at, /* 0x40; '@'; At symbol */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x41; 'A' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x42; 'B' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x43; 'C' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x44; 'D' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x45; 'E' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x46; 'F' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x47; 'G' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x48; 'H' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x49; 'I' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x4A; 'J' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x4B; 'K' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x4C; 'L' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x4D; 'M' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x4E; 'N' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x4F; 'O' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x50; 'P' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x51; 'Q' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x52; 'R' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x53; 'S' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x54; 'T' */
    pchtml_css_syntax_consume_ident_like, /* 0x55; 'U' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x56; 'V' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x57; 'W' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x58; 'X' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x59; 'Y' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x5A; 'Z' */
    pchtml_css_syntax_state_ls_bracket, /* 0x5B; '['; Left square bracket */
    pchtml_css_syntax_state_rsolidus, /* 0x5C; '\'; Inverse/backward slash */
    pchtml_css_syntax_state_rs_bracket, /* 0x5D; ']'; Right square bracket */
    pchtml_css_syntax_state_delim, /* 0x5E; '^'; Circumflex */
    pchtml_css_syntax_consume_ident_like, /* 0x5F; '_'; Underscore */
    pchtml_css_syntax_state_delim, /* 0x60; '`'; Gravis (backtick) */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x61; 'a' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x62; 'b' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x63; 'c' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x64; 'd' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x65; 'e' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x66; 'f' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x67; 'g' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x68; 'h' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x69; 'i' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x6A; 'j' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x6B; 'k' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x6C; 'l' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x6D; 'm' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x6E; 'n' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x6F; 'o' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x70; 'p' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x71; 'q' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x72; 'r' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x73; 's' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x74; 't' */
    pchtml_css_syntax_consume_ident_like, /* 0x75; 'u' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x76; 'v' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x77; 'w' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x78; 'x' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x79; 'y' */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x7A; 'z' */
    pchtml_css_syntax_state_lc_bracket, /* 0x7B; '{'; Left curly bracket */
    pchtml_css_syntax_state_delim, /* 0x7C; '|'; Vertical line */
    pchtml_css_syntax_state_rc_bracket, /* 0x7D; '}'; Right curly brackets */
    pchtml_css_syntax_state_delim, /* 0x7E; '~'; Tilde */
    pchtml_css_syntax_state_delim, /* 0x7F; 'DEL'; Delete */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x80 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x81 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x82 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x83 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x84 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x85 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x86 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x87 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x88 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x89 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x8A */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x8B */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x8C */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x8D */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x8E */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x8F */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x90 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x91 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x92 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x93 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x94 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x95 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x96 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x97 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x98 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x99 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x9A */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x9B */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x9C */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x9D */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x9E */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0x9F */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA0 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA1 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA2 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA3 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA4 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA5 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA6 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA7 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA8 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xA9 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xAA */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xAB */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xAC */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xAD */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xAE */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xAF */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB0 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB1 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB2 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB3 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB4 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB5 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB6 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB7 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB8 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xB9 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xBA */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xBB */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xBC */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xBD */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xBE */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xBF */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC0 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC1 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC2 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC3 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC4 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC5 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC6 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC7 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC8 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xC9 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xCA */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xCB */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xCC */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xCD */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xCE */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xCF */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD0 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD1 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD2 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD3 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD4 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD5 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD6 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD7 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD8 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xD9 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xDA */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xDB */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xDC */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xDD */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xDE */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xDF */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE0 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE1 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE2 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE3 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE4 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE5 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE6 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE7 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE8 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xE9 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xEA */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xEB */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xEC */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xED */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xEE */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xEF */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF0 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF1 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF2 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF3 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF4 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF5 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF6 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF7 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF8 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xF9 */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xFA */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xFB */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xFC */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xFD */
    pchtml_css_syntax_consume_ident_like_not_url, /* 0xFE */
    pchtml_css_syntax_consume_ident_like_not_url  /* 0xFF */
};


#endif  /* PCHTML_CSS_SYNTAX_STATE_RES_H */

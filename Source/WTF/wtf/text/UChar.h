/*
 * @file UChar.h
 * @author XueShuming
 * @date 2021/11/30
 * @brief define unicode char
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

#pragma once

#if !ENABLE(ICU)
typedef uint16_t UChar;
typedef int32_t UChar32;

typedef int8_t UBool;

#ifndef TRUE
#   define TRUE  1
#endif
#ifndef FALSE
#   define FALSE 0
#endif

#define UCONFIG_NO_COLLATION 0

#define U_SENTINEL (-1)

#define U_IS_SUPPLEMENTARY(c) ((uint32_t)((c)-0x10000)<=0xfffff)

#define U16_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)

#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)

#define U16_LEAD(supplementary) (UChar)(((supplementary)>>10)+0xd7c0)

#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)

#define U16_GET_SUPPLEMENTARY(lead, trail) \
        (((UChar32)(lead)<<10UL)+(UChar32)(trail)-U16_SURROGATE_OFFSET)

#define U16_NEXT_UNSAFE(s, i, c) { \
    (c)=(s)[(i)++]; \
    if(U16_IS_LEAD(c)) { \
        (c)=U16_GET_SUPPLEMENTARY((c), (s)[(i)++]); \
    } \
}

#define U16_NEXT(s, i, length, c) { \
    (c)=(s)[(i)++]; \
    if(U16_IS_LEAD(c)) { \
        uint16_t __c2; \
        if((i)!=(length) && U16_IS_TRAIL(__c2=(s)[(i)])) { \
            ++(i); \
            (c)=U16_GET_SUPPLEMENTARY((c), __c2); \
        } \
    } \
}


#define U16_TRAIL(supplementary) (UChar)(((supplementary)&0x3ff)|0xdc00)

#define U_IS_BMP(c) ((uint32_t)(c)<=0xffff)

#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

#define U_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)

#define U_IS_SURROGATE_TRAIL(c) (((c)&0x400)!=0)

#define U16_IS_SURROGATE(c) U_IS_SURROGATE(c)

#define U16_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)

#define U16_GET(s, start, i, length, c) { \
    (c)=(s)[i]; \
    if(U16_IS_SURROGATE(c)) { \
        uint16_t __c2; \
        if(U16_IS_SURROGATE_LEAD(c)) { \
            if((i)+1!=(length) && U16_IS_TRAIL(__c2=(s)[(i)+1])) { \
                (c)=U16_GET_SUPPLEMENTARY((c), __c2); \
            } \
        } else { \
            if((i)>(start) && U16_IS_LEAD(__c2=(s)[(i)-1])) { \
                (c)=U16_GET_SUPPLEMENTARY(__c2, (c)); \
            } \
        } \
    } \
}

#define U16_FWD_1(s, i, length) { \
    if(U16_IS_LEAD((s)[(i)++]) && (i)!=(length) && U16_IS_TRAIL((s)[i])) { \
        ++(i); \
    } \
}

#define U8_MAX_LENGTH 4

#define U8_APPEND_UNSAFE(s, i, c) { \
    uint32_t __uc=(c); \
    if(__uc<=0x7f) { \
        (s)[(i)++]=(uint8_t)__uc; \
    } else { \
        if(__uc<=0x7ff) { \
            (s)[(i)++]=(uint8_t)((__uc>>6)|0xc0); \
        } else { \
            if(__uc<=0xffff) { \
                (s)[(i)++]=(uint8_t)((__uc>>12)|0xe0); \
            } else { \
                (s)[(i)++]=(uint8_t)((__uc>>18)|0xf0); \
                (s)[(i)++]=(uint8_t)(((__uc>>12)&0x3f)|0x80); \
            } \
            (s)[(i)++]=(uint8_t)(((__uc>>6)&0x3f)|0x80); \
        } \
        (s)[(i)++]=(uint8_t)((__uc&0x3f)|0x80); \
    } \
}

#define U8_APPEND(s, i, capacity, c, isError) { \
    uint32_t __uc=(c); \
    if(__uc<=0x7f) { \
        (s)[(i)++]=(uint8_t)__uc; \
    } else if(__uc<=0x7ff && (i)+1<(capacity)) { \
        (s)[(i)++]=(uint8_t)((__uc>>6)|0xc0); \
        (s)[(i)++]=(uint8_t)((__uc&0x3f)|0x80); \
    } else if((__uc<=0xd7ff || (0xe000<=__uc && __uc<=0xffff)) && (i)+2<(capacity)) { \
        (s)[(i)++]=(uint8_t)((__uc>>12)|0xe0); \
        (s)[(i)++]=(uint8_t)(((__uc>>6)&0x3f)|0x80); \
        (s)[(i)++]=(uint8_t)((__uc&0x3f)|0x80); \
    } else if(0xffff<__uc && __uc<=0x10ffff && (i)+3<(capacity)) { \
        (s)[(i)++]=(uint8_t)((__uc>>18)|0xf0); \
        (s)[(i)++]=(uint8_t)(((__uc>>12)&0x3f)|0x80); \
        (s)[(i)++]=(uint8_t)(((__uc>>6)&0x3f)|0x80); \
        (s)[(i)++]=(uint8_t)((__uc&0x3f)|0x80); \
    } else { \
        (isError)=TRUE; \
    } \
}

#define U8_NEXT_UNSAFE(s, i, c) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if(!U8_IS_SINGLE(c)) { \
        if((c)<0xe0) { \
            (c)=(((c)&0x1f)<<6)|((s)[(i)++]&0x3f); \
        } else if((c)<0xf0) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(UChar)(((c)<<12)|(((s)[i]&0x3f)<<6)|((s)[(i)+1]&0x3f)); \
            (i)+=2; \
        } else { \
            (c)=(((c)&7)<<18)|(((s)[i]&0x3f)<<12)|(((s)[(i)+1]&0x3f)<<6)|((s)[(i)+2]&0x3f); \
            (i)+=3; \
        } \
    } \
}

#define U8_LEAD3_T1_BITS "\x20\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x10\x30\x30"

#define U8_LEAD4_T1_BITS "\x00\x00\x00\x00\x00\x00\x00\x00\x1E\x0F\x0F\x0F\x00\x00\x00\x00"

#define U8_NEXT(s, i, length, c) U8_INTERNAL_NEXT_OR_SUB(s, i, length, c, U_SENTINEL)

#define U8_INTERNAL_NEXT_OR_SUB(s, i, length, c, sub) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if(!U8_IS_SINGLE(c)) { \
        uint8_t __t = 0; \
        if((i)!=(length) && \
                /* fetch/validate/assemble all but last trail byte */ \
                ((c)>=0xe0 ? \
                 ((c)<0xf0 ?  /* U+0800..U+FFFF except surrogates */ \
                  U8_LEAD3_T1_BITS[(c)&=0xf]&(1<<((__t=(s)[i])>>5)) && \
                  (__t&=0x3f, 1) \
                  :  /* U+10000..U+10FFFF */ \
                  ((c)-=0xf0)<=4 && \
                  U8_LEAD4_T1_BITS[(__t=(s)[i])>>4]&(1<<(c)) && \
                  ((c)=((c)<<6)|(__t&0x3f), ++(i)!=(length)) && \
                  (__t=(s)[i]-0x80)<=0x3f) && \
                  /* valid second-to-last trail byte */ \
                  ((c)=((c)<<6)|__t, ++(i)!=(length)) \
                  :  /* U+0080..U+07FF */ \
                  (c)>=0xc2 && ((c)&=0x1f, 1)) && \
                  /* last trail byte */ \
                  (__t=(s)[i]-0x80)<=0x3f && \
                  ((c)=((c)<<6)|__t, ++(i), 1)) { \
        } else { \
            (c)=(sub);  /* ill-formed*/ \
        } \
    } \
}

#define U_IS_UNICODE_NONCHAR(c) \
    ((c)>=0xfdd0 && \
     ((c)<=0xfdef || ((c)&0xfffe)==0xfffe) && (c)<=0x10ffff)

#define U_IS_UNICODE_CHAR(c) \
    ((uint32_t)(c)<0xd800 || \
     (0xdfff<(c) && (c)<=0x10ffff && !U_IS_UNICODE_NONCHAR(c)))

#define U16_APPEND_UNSAFE(s, i, c) { \
    if((uint32_t)(c)<=0xffff) { \
        (s)[(i)++]=(uint16_t)(c); \
    } else { \
        (s)[(i)++]=(uint16_t)(((c)>>10)+0xd7c0); \
        (s)[(i)++]=(uint16_t)(((c)&0x3ff)|0xdc00); \
    } \
}

#define U16_APPEND(s, i, capacity, c, isError) { \
    if((uint32_t)(c)<=0xffff) { \
        (s)[(i)++]=(uint16_t)(c); \
    } else if((uint32_t)(c)<=0x10ffff && (i)+1<(capacity)) { \
        (s)[(i)++]=(uint16_t)(((c)>>10)+0xd7c0); \
        (s)[(i)++]=(uint16_t)(((c)&0x3ff)|0xdc00); \
    } else /* c>0x10ffff or not enough space */ { \
        (isError)=TRUE; \
    } \
}

#define U16_IS_SINGLE(c) !U_IS_SURROGATE(c)

#define U8_IS_SINGLE(c) (((c)&0x80)==0)


#endif

/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright (C) 2021 ~ 2023 Beijing FMSoft Technologies Co., Ltd.
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

css_error css__parse_transform_impl(css_language *c,
        const parserutils_vector *vector, int *ctx,
        css_style *result, int np)
{
    (void)np;
    int orig_ctx = *ctx;
    css_error error;
    const css_token *token;
    char buff[1024];
    lwc_string * trans = NULL;
    size_t num;
    uint32_t trans_snumber;
    size_t i;
    
    num = 0;
    parserutils_vector_get_length((parserutils_vector *)vector,&num);
    
    memset(buff,0,sizeof(buff));
    for (i = 0; i < num; i++) {
        token = parserutils_vector_iterate(vector, ctx);
        if (!token)
            break;
        
        switch (token->type) {
            case CSS_TOKEN_FUNCTION:
                strcat(buff,lwc_string_data(token->idata));
                strcat(buff,"(");
                break;
            case CSS_TOKEN_S:
                strcat(buff," ");
                break;
            default:
                if(token->idata)
                    strcat(buff, lwc_string_data(token->idata));
                break;
        }
	}
	
	if (lwc_intern_string(buff, strlen(buff), &trans) == lwc_error_oom) {
	    *ctx = orig_ctx;
	    return CSS_INVALID;
	}
	
	error = css__stylesheet_string_add(c->sheet, trans, &trans_snumber);
	
	if (error != CSS_OK) {
        *ctx = orig_ctx;
        lwc_string_unref(trans);
        return error;
    }
    
    error = css__stylesheet_style_appendOPV(result,
        CSS_PROP_TRANSFORM, 0, TRANSFORM_URI);
        
    if (error != CSS_OK) {
        *ctx = orig_ctx;
        return error;
    }
           
	error = css__stylesheet_style_append(result, trans_snumber);
	if (error != CSS_OK)
        *ctx = orig_ctx;
    
    return error;
}

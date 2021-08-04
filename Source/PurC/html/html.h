/**
 * @file html.h
 * @author 
 * @date 2021/07/02
 * @brief Include hearder files for html parser.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache 
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_H
#define PCHTML_HTML_H

#include "config.h"

#include "html/base.h"
#include "html/tree/error.h"
#include "html/tree/insertion_mode.h"
#include "html/tree/template_insertion.h"
#include "html/tree/active_formatting.h"
#include "html/tree/open_elements.h"
#include "html/node.h"
#include "html/html_tag.h"
#include "html/parser.h"
#include "html/tokenizer/error.h"
#include "html/tokenizer/state_rawtext.h"
#include "html/tokenizer/state_script.h"
#include "html/tokenizer/state_comment.h"
#include "html/tokenizer/state_doctype.h"
#include "html/tokenizer/state.h"
#include "html/tokenizer/state_rcdata.h"
#include "html/tree.h"
#include "html/tokenizer.h"
#include "html/html_interface.h"
#include "html/token_attr.h"
#include "html/token.h"
#include "html/serialize.h"
#include "html/interfaces/video_element.h"
#include "html/interfaces/data_list_element.h"
#include "html/interfaces/picture_element.h"
#include "html/interfaces/field_set_element.h"
#include "html/interfaces/quote_element.h"
#include "html/interfaces/li_element.h"
#include "html/interfaces/progress_element.h"
#include "html/interfaces/iframe_element.h"
#include "html/interfaces/style_element.h"
#include "html/interfaces/select_element.h"
#include "html/interfaces/details_element.h"
#include "html/interfaces/div_element.h"
#include "html/interfaces/d_list_element.h"
#include "html/interfaces/html_element.h"
#include "html/interfaces/map_element.h"
#include "html/interfaces/br_element.h"
#include "html/interfaces/text_area_element.h"
#include "html/interfaces/legend_element.h"
#include "html/interfaces/slot_element.h"
#include "html/interfaces/body_element.h"
#include "html/interfaces/param_element.h"
#include "html/interfaces/track_element.h"
#include "html/interfaces/frame_element.h"
#include "html/interfaces/media_element.h"
#include "html/interfaces/span_element.h"
#include "html/interfaces/meta_element.h"
#include "html/interfaces/hr_element.h"
#include "html/interfaces/marquee_element.h"
#include "html/interfaces/data_element.h"
#include "html/interfaces/window.h"
#include "html/interfaces/heading_element.h"
#include "html/interfaces/template_element.h"
#include "html/interfaces/source_element.h"
#include "html/interfaces/canvas_element.h"
#include "html/interfaces/embed_element.h"
#include "html/interfaces/title_element.h"
#include "html/interfaces/o_list_element.h"
#include "html/interfaces/output_element.h"
#include "html/interfaces/frame_set_element.h"
#include "html/interfaces/directory_element.h"
#include "html/interfaces/mod_element.h"
#include "html/interfaces/unknown_element.h"
#include "html/interfaces/menu_element.h"
#include "html/interfaces/button_element.h"
#include "html/interfaces/time_element.h"
#include "html/interfaces/element.h"
#include "html/interfaces/base_element.h"
#include "html/interfaces/meter_element.h"
#include "html/interfaces/table_section_element.h"
#include "html/interfaces/head_element.h"
#include "html/interfaces/input_element.h"
#include "html/interfaces/label_element.h"
#include "html/interfaces/u_list_element.h"
#include "html/interfaces/paragraph_element.h"
#include "html/interfaces/document.h"
#include "html/interfaces/audio_element.h"
#include "html/interfaces/image_element.h"
#include "html/interfaces/link_element.h"
#include "html/interfaces/opt_group_element.h"
#include "html/interfaces/table_col_element.h"
#include "html/interfaces/object_element.h"
#include "html/interfaces/dialog_element.h"
#include "html/interfaces/option_element.h"
#include "html/interfaces/pre_element.h"
#include "html/interfaces/form_element.h"
#include "html/interfaces/table_caption_element.h"
#include "html/interfaces/anchor_element.h"
#include "html/interfaces/script_element.h"
#include "html/interfaces/font_element.h"
#include "html/interfaces/table_cell_element.h"
#include "html/interfaces/table_element.h"
#include "html/interfaces/table_row_element.h"
#include "html/interfaces/area_element.h"

#endif  /* PCHTML_HTML_H */

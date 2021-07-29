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
 */

#ifndef PCHTML_PARSER_H
#define PCHTML_PARSER_H

#include "config.h"
#include "html/parser/base.h"
#include "html/parser/tree/error.h"
#include "html/parser/tree/insertion_mode.h"
#include "html/parser/tree/template_insertion.h"
#include "html/parser/tree/active_formatting.h"
#include "html/parser/tree/open_elements.h"
#include "html/parser/node.h"
#include "html/parser/tag.h"
#include "html/parser/parser.h"
#include "html/parser/tokenizer/error.h"
#include "html/parser/tokenizer/state_rawtext.h"
#include "html/parser/tokenizer/state_script.h"
#include "html/parser/tokenizer/state_comment.h"
#include "html/parser/tokenizer/state_doctype.h"
#include "html/parser/tokenizer/state.h"
#include "html/parser/tokenizer/state_rcdata.h"
#include "html/parser/tree.h"
#include "html/parser/tokenizer.h"
#include "html/parser/interface.h"
#include "html/parser/token_attr.h"
#include "html/parser/token.h"
#include "html/parser/serialize.h"
#include "html/parser/interfaces/video_element.h"
#include "html/parser/interfaces/data_list_element.h"
#include "html/parser/interfaces/picture_element.h"
#include "html/parser/interfaces/field_set_element.h"
#include "html/parser/interfaces/quote_element.h"
#include "html/parser/interfaces/li_element.h"
#include "html/parser/interfaces/progress_element.h"
#include "html/parser/interfaces/iframe_element.h"
#include "html/parser/interfaces/style_element.h"
#include "html/parser/interfaces/select_element.h"
#include "html/parser/interfaces/details_element.h"
#include "html/parser/interfaces/div_element.h"
#include "html/parser/interfaces/d_list_element.h"
#include "html/parser/interfaces/html_element.h"
#include "html/parser/interfaces/map_element.h"
#include "html/parser/interfaces/br_element.h"
#include "html/parser/interfaces/text_area_element.h"
#include "html/parser/interfaces/legend_element.h"
#include "html/parser/interfaces/slot_element.h"
#include "html/parser/interfaces/body_element.h"
#include "html/parser/interfaces/param_element.h"
#include "html/parser/interfaces/track_element.h"
#include "html/parser/interfaces/frame_element.h"
#include "html/parser/interfaces/media_element.h"
#include "html/parser/interfaces/span_element.h"
#include "html/parser/interfaces/meta_element.h"
#include "html/parser/interfaces/hr_element.h"
#include "html/parser/interfaces/marquee_element.h"
#include "html/parser/interfaces/data_element.h"
#include "html/parser/interfaces/window.h"
#include "html/parser/interfaces/heading_element.h"
#include "html/parser/interfaces/template_element.h"
#include "html/parser/interfaces/source_element.h"
#include "html/parser/interfaces/canvas_element.h"
#include "html/parser/interfaces/embed_element.h"
#include "html/parser/interfaces/title_element.h"
#include "html/parser/interfaces/o_list_element.h"
#include "html/parser/interfaces/output_element.h"
#include "html/parser/interfaces/frame_set_element.h"
#include "html/parser/interfaces/directory_element.h"
#include "html/parser/interfaces/mod_element.h"
#include "html/parser/interfaces/unknown_element.h"
#include "html/parser/interfaces/menu_element.h"
#include "html/parser/interfaces/button_element.h"
#include "html/parser/interfaces/time_element.h"
#include "html/parser/interfaces/element.h"
#include "html/parser/interfaces/base_element.h"
#include "html/parser/interfaces/meter_element.h"
#include "html/parser/interfaces/table_section_element.h"
#include "html/parser/interfaces/head_element.h"
#include "html/parser/interfaces/input_element.h"
#include "html/parser/interfaces/label_element.h"
#include "html/parser/interfaces/u_list_element.h"
#include "html/parser/interfaces/paragraph_element.h"
#include "html/parser/interfaces/document.h"
#include "html/parser/interfaces/audio_element.h"
#include "html/parser/interfaces/image_element.h"
#include "html/parser/interfaces/link_element.h"
#include "html/parser/interfaces/opt_group_element.h"
#include "html/parser/interfaces/table_col_element.h"
#include "html/parser/interfaces/object_element.h"
#include "html/parser/interfaces/dialog_element.h"
#include "html/parser/interfaces/option_element.h"
#include "html/parser/interfaces/pre_element.h"
#include "html/parser/interfaces/form_element.h"
#include "html/parser/interfaces/table_caption_element.h"
#include "html/parser/interfaces/anchor_element.h"
#include "html/parser/interfaces/script_element.h"
#include "html/parser/interfaces/font_element.h"
#include "html/parser/interfaces/table_cell_element.h"
#include "html/parser/interfaces/table_element.h"
#include "html/parser/interfaces/table_row_element.h"
#include "html/parser/interfaces/area_element.h"

#endif  /* PCHTML_PARSER_H */

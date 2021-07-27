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

#ifndef PCHTML_HTML_H
#define PCHTML_HTML_H

#include "config.h"
#include "html/html/base.h"
#include "html/html/tree/error.h"
#include "html/html/tree/insertion_mode.h"
#include "html/html/tree/template_insertion.h"
#include "html/html/tree/active_formatting.h"
#include "html/html/tree/open_elements.h"
#include "html/html/node.h"
#include "html/html/tag.h"
#include "html/html/parser.h"
#include "html/html/tokenizer/error.h"
#include "html/html/tokenizer/state_rawtext.h"
#include "html/html/tokenizer/state_script.h"
#include "html/html/tokenizer/state_comment.h"
#include "html/html/tokenizer/state_doctype.h"
#include "html/html/tokenizer/state.h"
#include "html/html/tokenizer/state_rcdata.h"
#include "html/html/tree.h"
#include "html/html/tokenizer.h"
#include "html/html/interface.h"
#include "html/html/token_attr.h"
#include "html/html/token.h"
#include "html/html/serialize.h"
#include "html/html/interfaces/video_element.h"
#include "html/html/interfaces/data_list_element.h"
#include "html/html/interfaces/picture_element.h"
#include "html/html/interfaces/field_set_element.h"
#include "html/html/interfaces/quote_element.h"
#include "html/html/interfaces/li_element.h"
#include "html/html/interfaces/progress_element.h"
#include "html/html/interfaces/iframe_element.h"
#include "html/html/interfaces/style_element.h"
#include "html/html/interfaces/select_element.h"
#include "html/html/interfaces/details_element.h"
#include "html/html/interfaces/div_element.h"
#include "html/html/interfaces/d_list_element.h"
#include "html/html/interfaces/html_element.h"
#include "html/html/interfaces/map_element.h"
#include "html/html/interfaces/br_element.h"
#include "html/html/interfaces/text_area_element.h"
#include "html/html/interfaces/legend_element.h"
#include "html/html/interfaces/slot_element.h"
#include "html/html/interfaces/body_element.h"
#include "html/html/interfaces/param_element.h"
#include "html/html/interfaces/track_element.h"
#include "html/html/interfaces/frame_element.h"
#include "html/html/interfaces/media_element.h"
#include "html/html/interfaces/span_element.h"
#include "html/html/interfaces/meta_element.h"
#include "html/html/interfaces/hr_element.h"
#include "html/html/interfaces/marquee_element.h"
#include "html/html/interfaces/data_element.h"
#include "html/html/interfaces/window.h"
#include "html/html/interfaces/heading_element.h"
#include "html/html/interfaces/template_element.h"
#include "html/html/interfaces/source_element.h"
#include "html/html/interfaces/canvas_element.h"
#include "html/html/interfaces/embed_element.h"
#include "html/html/interfaces/title_element.h"
#include "html/html/interfaces/o_list_element.h"
#include "html/html/interfaces/output_element.h"
#include "html/html/interfaces/frame_set_element.h"
#include "html/html/interfaces/directory_element.h"
#include "html/html/interfaces/mod_element.h"
#include "html/html/interfaces/unknown_element.h"
#include "html/html/interfaces/menu_element.h"
#include "html/html/interfaces/button_element.h"
#include "html/html/interfaces/time_element.h"
#include "html/html/interfaces/element.h"
#include "html/html/interfaces/base_element.h"
#include "html/html/interfaces/meter_element.h"
#include "html/html/interfaces/table_section_element.h"
#include "html/html/interfaces/head_element.h"
#include "html/html/interfaces/input_element.h"
#include "html/html/interfaces/label_element.h"
#include "html/html/interfaces/u_list_element.h"
#include "html/html/interfaces/paragraph_element.h"
#include "html/html/interfaces/document.h"
#include "html/html/interfaces/audio_element.h"
#include "html/html/interfaces/image_element.h"
#include "html/html/interfaces/link_element.h"
#include "html/html/interfaces/opt_group_element.h"
#include "html/html/interfaces/table_col_element.h"
#include "html/html/interfaces/object_element.h"
#include "html/html/interfaces/dialog_element.h"
#include "html/html/interfaces/option_element.h"
#include "html/html/interfaces/pre_element.h"
#include "html/html/interfaces/form_element.h"
#include "html/html/interfaces/table_caption_element.h"
#include "html/html/interfaces/anchor_element.h"
#include "html/html/interfaces/script_element.h"
#include "html/html/interfaces/font_element.h"
#include "html/html/interfaces/table_cell_element.h"
#include "html/html/interfaces/table_element.h"
#include "html/html/interfaces/table_row_element.h"
#include "html/html/interfaces/area_element.h"

#endif  /* PCHTML_HTML_H */

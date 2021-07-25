/**
 * @file interface.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser interface.
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

#ifndef PCHTML_HTML_INTERFACES_H
#define PCHTML_HTML_INTERFACES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/html/base.h"
#include "html/tag/const.h"
#include "html/ns/const.h"
#include "html/dom/interface.h"


#define pchtml_html_interface_document(obj) ((pchtml_html_document_t *) (obj))
#define pchtml_html_interface_anchor(obj) ((pchtml_html_anchor_element_t *) (obj))
#define pchtml_html_interface_area(obj) ((pchtml_html_area_element_t *) (obj))
#define pchtml_html_interface_audio(obj) ((pchtml_html_audio_element_t *) (obj))
#define pchtml_html_interface_br(obj) ((pchtml_html_br_element_t *) (obj))
#define pchtml_html_interface_base(obj) ((pchtml_html_base_element_t *) (obj))
#define pchtml_html_interface_body(obj) ((pchtml_html_body_element_t *) (obj))
#define pchtml_html_interface_button(obj) ((pchtml_html_button_element_t *) (obj))
#define pchtml_html_interface_canvas(obj) ((pchtml_html_canvas_element_t *) (obj))
#define pchtml_html_interface_d_list(obj) ((pchtml_html_d_list_element_t *) (obj))
#define pchtml_html_interface_data(obj) ((pchtml_html_data_element_t *) (obj))
#define pchtml_html_interface_data_list(obj) ((pchtml_html_data_list_element_t *) (obj))
#define pchtml_html_interface_details(obj) ((pchtml_html_details_element_t *) (obj))
#define pchtml_html_interface_dialog(obj) ((pchtml_html_dialog_element_t *) (obj))
#define pchtml_html_interface_directory(obj) ((pchtml_html_directory_element_t *) (obj))
#define pchtml_html_interface_div(obj) ((pchtml_html_div_element_t *) (obj))
#define pchtml_html_interface_element(obj) ((pchtml_html_element_t *) (obj))
#define pchtml_html_interface_embed(obj) ((pchtml_html_embed_element_t *) (obj))
#define pchtml_html_interface_field_set(obj) ((pchtml_html_field_set_element_t *) (obj))
#define pchtml_html_interface_font(obj) ((pchtml_html_font_element_t *) (obj))
#define pchtml_html_interface_form(obj) ((pchtml_html_form_element_t *) (obj))
#define pchtml_html_interface_frame(obj) ((pchtml_html_frame_element_t *) (obj))
#define pchtml_html_interface_frame_set(obj) ((pchtml_html_frame_set_element_t *) (obj))
#define pchtml_html_interface_hr(obj) ((pchtml_html_hr_element_t *) (obj))
#define pchtml_html_interface_head(obj) ((pchtml_html_head_element_t *) (obj))
#define pchtml_html_interface_heading(obj) ((pchtml_html_heading_element_t *) (obj))
#define pchtml_html_interface_html(obj) ((pchtml_html_html_element_t *) (obj))
#define pchtml_html_interface_iframe(obj) ((pchtml_html_iframe_element_t *) (obj))
#define pchtml_html_interface_image(obj) ((pchtml_html_image_element_t *) (obj))
#define pchtml_html_interface_input(obj) ((pchtml_html_input_element_t *) (obj))
#define pchtml_html_interface_li(obj) ((pchtml_html_li_element_t *) (obj))
#define pchtml_html_interface_label(obj) ((pchtml_html_label_element_t *) (obj))
#define pchtml_html_interface_legend(obj) ((pchtml_html_legend_element_t *) (obj))
#define pchtml_html_interface_link(obj) ((pchtml_html_link_element_t *) (obj))
#define pchtml_html_interface_map(obj) ((pchtml_html_map_element_t *) (obj))
#define pchtml_html_interface_marquee(obj) ((pchtml_html_marquee_element_t *) (obj))
#define pchtml_html_interface_media(obj) ((pchtml_html_media_element_t *) (obj))
#define pchtml_html_interface_menu(obj) ((pchtml_html_menu_element_t *) (obj))
#define pchtml_html_interface_meta(obj) ((pchtml_html_meta_element_t *) (obj))
#define pchtml_html_interface_meter(obj) ((pchtml_html_meter_element_t *) (obj))
#define pchtml_html_interface_mod(obj) ((pchtml_html_mod_element_t *) (obj))
#define pchtml_html_interface_o_list(obj) ((pchtml_html_o_list_element_t *) (obj))
#define pchtml_html_interface_object(obj) ((pchtml_html_object_element_t *) (obj))
#define pchtml_html_interface_opt_group(obj) ((pchtml_html_opt_group_element_t *) (obj))
#define pchtml_html_interface_option(obj) ((pchtml_html_option_element_t *) (obj))
#define pchtml_html_interface_output(obj) ((pchtml_html_output_element_t *) (obj))
#define pchtml_html_interface_paragraph(obj) ((pchtml_html_paragraph_element_t *) (obj))
#define pchtml_html_interface_param(obj) ((pchtml_html_param_element_t *) (obj))
#define pchtml_html_interface_picture(obj) ((pchtml_html_picture_element_t *) (obj))
#define pchtml_html_interface_pre(obj) ((pchtml_html_pre_element_t *) (obj))
#define pchtml_html_interface_progress(obj) ((pchtml_html_progress_element_t *) (obj))
#define pchtml_html_interface_quote(obj) ((pchtml_html_quote_element_t *) (obj))
#define pchtml_html_interface_script(obj) ((pchtml_html_script_element_t *) (obj))
#define pchtml_html_interface_select(obj) ((pchtml_html_select_element_t *) (obj))
#define pchtml_html_interface_slot(obj) ((pchtml_html_slot_element_t *) (obj))
#define pchtml_html_interface_source(obj) ((pchtml_html_source_element_t *) (obj))
#define pchtml_html_interface_span(obj) ((pchtml_html_span_element_t *) (obj))
#define pchtml_html_interface_style(obj) ((pchtml_html_style_element_t *) (obj))
#define pchtml_html_interface_table_caption(obj) ((pchtml_html_table_caption_element_t *) (obj))
#define pchtml_html_interface_table_cell(obj) ((pchtml_html_table_cell_element_t *) (obj))
#define pchtml_html_interface_table_col(obj) ((pchtml_html_table_col_element_t *) (obj))
#define pchtml_html_interface_table(obj) ((pchtml_html_table_element_t *) (obj))
#define pchtml_html_interface_table_row(obj) ((pchtml_html_table_row_element_t *) (obj))
#define pchtml_html_interface_table_section(obj) ((pchtml_html_table_section_element_t *) (obj))
#define pchtml_html_interface_template(obj) ((pchtml_html_template_element_t *) (obj))
#define pchtml_html_interface_text_area(obj) ((pchtml_html_text_area_element_t *) (obj))
#define pchtml_html_interface_time(obj) ((pchtml_html_time_element_t *) (obj))
#define pchtml_html_interface_title(obj) ((pchtml_html_title_element_t *) (obj))
#define pchtml_html_interface_track(obj) ((pchtml_html_track_element_t *) (obj))
#define pchtml_html_interface_u_list(obj) ((pchtml_html_u_list_element_t *) (obj))
#define pchtml_html_interface_unknown(obj) ((pchtml_html_unknown_element_t *) (obj))
#define pchtml_html_interface_video(obj) ((pchtml_html_video_element_t *) (obj))
#define pchtml_html_interface_window(obj) ((pchtml_html_window_t *) (obj))


typedef struct pchtml_html_document pchtml_html_document_t;
typedef struct pchtml_html_anchor_element pchtml_html_anchor_element_t;
typedef struct pchtml_html_area_element pchtml_html_area_element_t;
typedef struct pchtml_html_audio_element pchtml_html_audio_element_t;
typedef struct pchtml_html_br_element pchtml_html_br_element_t;
typedef struct pchtml_html_base_element pchtml_html_base_element_t;
typedef struct pchtml_html_body_element pchtml_html_body_element_t;
typedef struct pchtml_html_button_element pchtml_html_button_element_t;
typedef struct pchtml_html_canvas_element pchtml_html_canvas_element_t;
typedef struct pchtml_html_d_list_element pchtml_html_d_list_element_t;
typedef struct pchtml_html_data_element pchtml_html_data_element_t;
typedef struct pchtml_html_data_list_element pchtml_html_data_list_element_t;
typedef struct pchtml_html_details_element pchtml_html_details_element_t;
typedef struct pchtml_html_dialog_element pchtml_html_dialog_element_t;
typedef struct pchtml_html_directory_element pchtml_html_directory_element_t;
typedef struct pchtml_html_div_element pchtml_html_div_element_t;
typedef struct pchtml_html_element pchtml_html_element_t;
typedef struct pchtml_html_embed_element pchtml_html_embed_element_t;
typedef struct pchtml_html_field_set_element pchtml_html_field_set_element_t;
typedef struct pchtml_html_font_element pchtml_html_font_element_t;
typedef struct pchtml_html_form_element pchtml_html_form_element_t;
typedef struct pchtml_html_frame_element pchtml_html_frame_element_t;
typedef struct pchtml_html_frame_set_element pchtml_html_frame_set_element_t;
typedef struct pchtml_html_hr_element pchtml_html_hr_element_t;
typedef struct pchtml_html_head_element pchtml_html_head_element_t;
typedef struct pchtml_html_heading_element pchtml_html_heading_element_t;
typedef struct pchtml_html_html_element pchtml_html_html_element_t;
typedef struct pchtml_html_iframe_element pchtml_html_iframe_element_t;
typedef struct pchtml_html_image_element pchtml_html_image_element_t;
typedef struct pchtml_html_input_element pchtml_html_input_element_t;
typedef struct pchtml_html_li_element pchtml_html_li_element_t;
typedef struct pchtml_html_label_element pchtml_html_label_element_t;
typedef struct pchtml_html_legend_element pchtml_html_legend_element_t;
typedef struct pchtml_html_link_element pchtml_html_link_element_t;
typedef struct pchtml_html_map_element pchtml_html_map_element_t;
typedef struct pchtml_html_marquee_element pchtml_html_marquee_element_t;
typedef struct pchtml_html_media_element pchtml_html_media_element_t;
typedef struct pchtml_html_menu_element pchtml_html_menu_element_t;
typedef struct pchtml_html_meta_element pchtml_html_meta_element_t;
typedef struct pchtml_html_meter_element pchtml_html_meter_element_t;
typedef struct pchtml_html_mod_element pchtml_html_mod_element_t;
typedef struct pchtml_html_o_list_element pchtml_html_o_list_element_t;
typedef struct pchtml_html_object_element pchtml_html_object_element_t;
typedef struct pchtml_html_opt_group_element pchtml_html_opt_group_element_t;
typedef struct pchtml_html_option_element pchtml_html_option_element_t;
typedef struct pchtml_html_output_element pchtml_html_output_element_t;
typedef struct pchtml_html_paragraph_element pchtml_html_paragraph_element_t;
typedef struct pchtml_html_param_element pchtml_html_param_element_t;
typedef struct pchtml_html_picture_element pchtml_html_picture_element_t;
typedef struct pchtml_html_pre_element pchtml_html_pre_element_t;
typedef struct pchtml_html_progress_element pchtml_html_progress_element_t;
typedef struct pchtml_html_quote_element pchtml_html_quote_element_t;
typedef struct pchtml_html_script_element pchtml_html_script_element_t;
typedef struct pchtml_html_select_element pchtml_html_select_element_t;
typedef struct pchtml_html_slot_element pchtml_html_slot_element_t;
typedef struct pchtml_html_source_element pchtml_html_source_element_t;
typedef struct pchtml_html_span_element pchtml_html_span_element_t;
typedef struct pchtml_html_style_element pchtml_html_style_element_t;
typedef struct pchtml_html_table_caption_element pchtml_html_table_caption_element_t;
typedef struct pchtml_html_table_cell_element pchtml_html_table_cell_element_t;
typedef struct pchtml_html_table_col_element pchtml_html_table_col_element_t;
typedef struct pchtml_html_table_element pchtml_html_table_element_t;
typedef struct pchtml_html_table_row_element pchtml_html_table_row_element_t;
typedef struct pchtml_html_table_section_element pchtml_html_table_section_element_t;
typedef struct pchtml_html_template_element pchtml_html_template_element_t;
typedef struct pchtml_html_text_area_element pchtml_html_text_area_element_t;
typedef struct pchtml_html_time_element pchtml_html_time_element_t;
typedef struct pchtml_html_title_element pchtml_html_title_element_t;
typedef struct pchtml_html_track_element pchtml_html_track_element_t;
typedef struct pchtml_html_u_list_element pchtml_html_u_list_element_t;
typedef struct pchtml_html_unknown_element pchtml_html_unknown_element_t;
typedef struct pchtml_html_video_element pchtml_html_video_element_t;
typedef struct pchtml_html_window pchtml_html_window_t;


pchtml_dom_interface_t *
pchtml_html_interface_create(pchtml_html_document_t *document, pchtml_tag_id_t tag_id,
                          pchtml_ns_id_t ns) WTF_INTERNAL;

pchtml_dom_interface_t *
pchtml_html_interface_destroy(pchtml_dom_interface_t *intrfc) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_INTERFACES_H */

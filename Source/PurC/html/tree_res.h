/**
 * @file tree_res.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for resource of tree.
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


#ifndef PCHTML_HTML_TREE_RES_H
#define PCHTML_HTML_TREE_RES_H


typedef struct {
    const char *from;
    const char *to;
    size_t     len;
}
pchtml_html_tree_res_attr_adjust_t;

typedef struct {
    const char       *name;
    const char       *prefix;
    const char       *local_name;

    size_t           name_len;
    size_t           prefix_len;

    pchtml_ns_id_t      ns;
}
pchtml_html_tree_res_attr_adjust_foreign_t;


static const pchtml_html_tree_res_attr_adjust_t
pchtml_html_tree_res_attr_adjust_svg_map[] =
{
    {"attributename", "attributeName", 13},
    {"attributetype", "attributeType", 13},
    {"basefrequency", "baseFrequency", 13},
    {"baseprofile", "baseProfile", 11},
    {"calcmode", "calcMode", 8},
    {"clippathunits", "clipPathUnits", 13},
    {"diffuseconstant", "diffuseConstant", 15},
    {"edgemode", "edgeMode", 8},
    {"filterunits", "filterUnits", 11},
    {"glyphref", "glyphRef", 8},
    {"gradienttransform", "gradientTransform", 17},
    {"gradientunits", "gradientUnits", 13},
    {"kernelmatrix", "kernelMatrix", 12},
    {"kernelunitlength", "kernelUnitLength", 16},
    {"keypoints", "keyPoints", 9},
    {"keysplines", "keySplines", 10},
    {"keytimes", "keyTimes", 8},
    {"lengthadjust", "lengthAdjust", 12},
    {"limitingconeangle", "limitingConeAngle", 17},
    {"markerheight", "markerHeight", 12},
    {"markerunits", "markerUnits", 11},
    {"markerwidth", "markerWidth", 11},
    {"maskcontentunits", "maskContentUnits", 16},
    {"maskunits", "maskUnits", 9},
    {"numoctaves", "numOctaves", 10},
    {"pathlength", "pathLength", 10},
    {"patterncontentunits", "patternContentUnits", 19},
    {"patterntransform", "patternTransform", 16},
    {"patternunits", "patternUnits", 12},
    {"pointsatx", "pointsAtX", 9},
    {"pointsaty", "pointsAtY", 9},
    {"pointsatz", "pointsAtZ", 9},
    {"preservealpha", "preserveAlpha", 13},
    {"preserveaspectratio", "preserveAspectRatio", 19},
    {"primitiveunits", "primitiveUnits", 14},
    {"refx", "refX", 4},
    {"refy", "refY", 4},
    {"repeatcount", "repeatCount", 11},
    {"repeatdur", "repeatDur", 9},
    {"requiredextensions", "requiredExtensions", 18},
    {"requiredfeatures", "requiredFeatures", 16},
    {"specularconstant", "specularConstant", 16},
    {"specularexponent", "specularExponent", 16},
    {"spreadmethod", "spreadMethod", 12},
    {"startoffset", "startOffset", 11},
    {"stddeviation", "stdDeviation", 12},
    {"stitchtiles", "stitchTiles", 11},
    {"surfacescale", "surfaceScale", 12},
    {"systemlanguage", "systemLanguage", 14},
    {"tablevalues", "tableValues", 11},
    {"targetx", "targetX", 7},
    {"targety", "targetY", 7},
    {"textlength", "textLength", 10},
    {"viewbox", "viewBox", 7},
    {"viewtarget", "viewTarget", 10},
    {"xchannelselector", "xChannelSelector", 16},
    {"ychannelselector", "yChannelSelector", 16},
    {"zoomandpan", "zoomAndPan", 10},
};

static const pchtml_html_tree_res_attr_adjust_foreign_t
pchtml_html_tree_res_attr_adjust_foreign_map[] =
{
    {"xlink:actuate", "xlink", "actuate", 13, 5, PCHTML_NS_XLINK},
    {"xlink:arcrole", "xlink", "arcrole", 13, 5, PCHTML_NS_XLINK},
    {"xlink:href", "xlink", "href", 10, 5, PCHTML_NS_XLINK},
    {"xlink:role", "xlink", "role", 10, 5, PCHTML_NS_XLINK},
    {"xlink:show", "xlink", "show", 10, 5, PCHTML_NS_XLINK},
    {"xlink:title", "xlink", "title", 11, 5, PCHTML_NS_XLINK},
    {"xlink:type", "xlink", "type", 10, 5, PCHTML_NS_XLINK},
    {"xml:lang", "xml", "lang", 8, 3, PCHTML_NS_XML},
    {"xml:space", "xml", "space", 9, 3, PCHTML_NS_XML},
    {"xmlns", "", "xmlns", 5, 0, PCHTML_NS_XMLNS},
    {"xmlns:xlink", "xmlns", "xlink", 11, 5, PCHTML_NS_XMLNS}
};


#endif  /* PCHTML_HTML_TREE_RES_H */

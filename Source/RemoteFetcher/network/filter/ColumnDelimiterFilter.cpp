/* 
 * Copyright (C) 2020 Beijing FMSoft Technologies Co., Ltd.
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
 * Or,
 * 
 * As this component is a program released under LGPLv3, which claims
 * explicitly that the program could be modified by any end user
 * even if the program is conveyed in non-source form on the system it runs.
 * Generally, if you distribute this program in embedded devices,
 * you might not satisfy this condition. Under this situation or you can
 * not accept any condition of LGPLv3, you need to get a commercial license
 * from FMSoft, along with a patent license for the patents owned by FMSoft.
 * 
 * If you have got a commercial/patent license of this program, please use it
 * under the terms and conditions of the commercial license.
 * 
 * For more information about the commercial license and patent license,
 * please refer to
 * <https://hybridos.fmsoft.cn/blog/hybridos-licensing-policy/>.
 * 
 * Also note that the LGPLv3 license does not apply to any entity in the
 * Exception List published by Beijing FMSoft Technologies Co., Ltd.
 * 
 * If you are or the entity you represent is listed in the Exception List,
 * the above open source or free software license does not apply to you
 * or the entity you represent. Regardless of the purpose, you should not
 * use the software in any way whatsoever, including but not limited to
 * downloading, viewing, copying, distributing, compiling, and running.
 * If you have already downloaded it, you MUST destroy all of its copies.
 * 
 * The Exception List is published by FMSoft and may be updated
 * from time to time. For more information, please see
 * <https://www.fmsoft.cn/exception-list>.
 */ 

#include "config.h"
#include <stdio.h>
#include "ColumnDelimiterFilter.h"

namespace PurCFetcher {
using namespace PurCFetcher;

ColumnDelimiterFilter::ColumnDelimiterFilter()
{
}

ColumnDelimiterFilter::~ColumnDelimiterFilter()
{
}

Vector<String> ColumnDelimiterFilter::filterInnter(Vector<String> lines, String param)
{
    if (lines.size() == 0)
        return lines;

    Vector<String> result;
    size_t size = lines.size();
    for (size_t i = 0; i < size; i++)
    {
        result.appendVector(splitLine(lines[i], param));
    }
    return result;
}

Vector<String> ColumnDelimiterFilter::splitLine(String line, String param)
{
    if (param.length() > 1)
    {
        Vector<String> result = line.split(param[0]);
        return filterInnter(result, param.substring(1, param.length() - 1));
    }
    else
    {
        return  line.split(param);
    }
}

Vector<Row> ColumnDelimiterFilter::doFilter(Vector<Row> rowVec, String param)
{
    if (param.isEmpty() || rowVec.size() == 0)
        return rowVec;

    Vector<Row> result;
    size_t size = rowVec.size();
    for (size_t i = 0; i < size; i++)
    {
        result.appendVector(splitRow(rowVec[i], param));
    }
    return result;
}

Vector<Row> ColumnDelimiterFilter::splitRow(Row& row, String param)
{
    Vector<Row> result;
    result.append(filterInnter(row, param));
    return result;
}

} // namespace PurCFetcher


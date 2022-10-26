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
#include "CmdFilterManager.h"

#include "LineSplitFilter.h"
#include "LineCharsFilter.h"
#include "LineLettersFilter.h"
#include "LineWordsFilter.h"
#include "LineSentencesFilter.h"
#include "LineHeadFilter.h"
#include "LineTailFilter.h"
#include "LineIgnoreFilter.h"
#include "LinePickFilter.h"
#include "LineCutFilter.h"
#include "ColumnDelimiterFilter.h"
#include "ColumnCharsFilter.h"
#include "ColumnLettersFilter.h"
#include "ColumnWordsFilter.h"
#include "ColumnSentencesFilter.h"
#include "ColumnHeadFilter.h"
#include "ColumnCutFilter.h"
#include "ColumnIgnoreFilter.h"
#include "ColumnPickFilter.h"
#include "ColumnIgnoreFilter.h"
#include "ColumnTailFilter.h"

#include "FormatKeys.h"
#include "FormatArray.h"

namespace PurCFetcher {

CmdFilterManager::CmdFilterManager()
{
    initFilterVec();
    initNameFilterMap();
}

CmdFilterManager::~CmdFilterManager()
{
}

bool CmdFilterManager::addFilter(String name, String param)
{
    if (name.isEmpty())
        return false;

    String nameLowerCase = name.convertToASCIILowercase().stripWhiteSpace();
    auto findResult = m_nameFilterMap.find(nameLowerCase);
    if(findResult == m_nameFilterMap.end())
        return false;

    RefPtr<FilterBase> filter = findResult->value;
    switch(filter->type())
    {
        case FilterTypeLineSplit:
        case FilterTypeLineCut:
        case FilterTypeColumnSplit:
        case FilterTypeColumnCut:
            m_filterNameVec.append(nameLowerCase);
            m_filterParamVec.append(param);
            break;

        case FilterTypeFormat:
            m_formatFilter.append(nameLowerCase);
            m_formatParamFilter.append(param);
            break;

        default:
            return false;
    }

    return true;
}

Vector<Ref<JSON::Value>> CmdFilterManager::doFilter(Vector<String> lines)
{
    Vector<Ref<JSON::Value>> result;

    int linesSize = lines.size();
    Vector<Vector<String>> lineListVec;
    for (int i = 0; i < linesSize; i++) 
    {
        Vector<String> columnVec;
        columnVec.append(lines[i]);
        lineListVec.append(columnVec);
    }

    int size = m_filterNameVec.size();
    for (int i = 0; i < size; i++)
    {
        lineListVec = doFilterInner(lineListVec, m_filterNameVec[i], m_filterParamVec[i]);
    }

    size = lineListVec.size();
    for (int i = 0; i < size; i++)
    {
        Ref<JSON::Value> formatResult = doFormat(lineListVec[i]);
        result.append(WTFMove(formatResult));
    }

    return result;
}

Vector<Vector<String>> CmdFilterManager::doFilterInner(Vector<Vector<String>>& lineListVec, String filterName, String filterParam)
{
    printf(".....................................doFilterInner|name=%s|param=%s|\n", filterName.characters8(), filterParam.characters8());
    auto findResult  = m_nameFilterMap.find(filterName);
    if(findResult == m_nameFilterMap.end())
        return lineListVec;

    return findResult->value->doFilter(lineListVec, filterParam);
}

Ref<JSON::Value> CmdFilterManager::doFormat(Vector<String> lineColumns)
{
    String name;
    String param;
    int formatSize =m_formatFilter.size(); 
    if (formatSize)
    {
        name = m_formatFilter[formatSize - 1];
        param = m_formatParamFilter[formatSize - 1];
    }
    else
    {
        name = "keys";
        param = "";
    }

    auto findResult  = m_nameFilterMap.find(name);
    if(findResult == m_nameFilterMap.end())
        return JSON::Value::null();

    FormatBase* format = (FormatBase*)findResult->value.get();
    return format->doFormat(lineColumns, param);
}

void CmdFilterManager::initFilterVec()
{
    m_filterVec.append(adoptRef(*new LineCharsFilter()));
    m_filterVec.append(adoptRef(*new LineCutFilter()));
    m_filterVec.append(adoptRef(*new LineHeadFilter()));
    m_filterVec.append(adoptRef(*new LineIgnoreFilter()));
    m_filterVec.append(adoptRef(*new LineLettersFilter()));
    m_filterVec.append(adoptRef(*new LinePickFilter()));
    m_filterVec.append(adoptRef(*new LineSentencesFilter()));
    m_filterVec.append(adoptRef(*new LineSplitFilter()));
    m_filterVec.append(adoptRef(*new LineTailFilter()));
    m_filterVec.append(adoptRef(*new LineWordsFilter()));

    m_filterVec.append(adoptRef(*new ColumnCharsFilter()));
    m_filterVec.append(adoptRef(*new ColumnCutFilter()));
    m_filterVec.append(adoptRef(*new ColumnDelimiterFilter()));
    m_filterVec.append(adoptRef(*new ColumnHeadFilter()));
    m_filterVec.append(adoptRef(*new ColumnIgnoreFilter()));
    m_filterVec.append(adoptRef(*new ColumnLettersFilter()));
    m_filterVec.append(adoptRef(*new ColumnPickFilter()));
    m_filterVec.append(adoptRef(*new ColumnSentencesFilter()));
    m_filterVec.append(adoptRef(*new ColumnTailFilter()));
    m_filterVec.append(adoptRef(*new ColumnWordsFilter()));

    m_filterVec.append(adoptRef(*new FormatArray()));
    m_filterVec.append(adoptRef(*new FormatKeys()));
}

void CmdFilterManager::initNameFilterMap()
{
    int size = m_filterVec.size();
    for (int i = 0; i < size; i++)
    {
        m_nameFilterMap.set(m_filterVec[i]->name().convertToASCIILowercase(), m_filterVec[i]);
    }
}

} // namespace PurCFetcher

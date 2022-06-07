/*
 * @file GFdMonitor.cpp
 * @author XueShuming
 * @date 2022/04/06
 * @brief The fd monitor for RunLoop.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include "config.h"
#include "GFdMonitor.h"

#include <gio/gio.h>
#include <glib-unix.h>
#include <wtf/glib/RunLoopSourcePriority.h>

namespace PurCWTF {

GFdMonitor::~GFdMonitor()
{
    stop();
}

gboolean GFdMonitor::fdSourceCallback(gint fd, GIOCondition condition,
        GFdMonitor *monitor)
{
    return monitor->m_callback(fd, condition);
}

void GFdMonitor::start(gint fd, GIOCondition condition, GMainContext* gctxt,
        Function<gboolean(gint, GIOCondition)>&& callback)
{
    stop();

    m_callback = WTFMove(callback);
    m_source = adoptGRef(g_unix_fd_source_new(fd, condition));
    g_source_set_callback(m_source.get(),
            reinterpret_cast<GSourceFunc>(
                reinterpret_cast<GCallback>(fdSourceCallback)), this, nullptr);
    g_source_set_priority(m_source.get(),
            RunLoopSourcePriority::RunLoopDispatcher);
    g_source_attach(m_source.get(), gctxt);
}

void GFdMonitor::stop()
{
    if (!m_source)
        return;

    g_source_destroy(m_source.get());
    m_source = nullptr;
    m_callback = nullptr;
}

} // namespace PurCWTF

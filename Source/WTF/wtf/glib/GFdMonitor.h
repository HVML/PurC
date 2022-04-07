/*
 * @file GFdMonitor.h
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

#pragma once

#include <glib.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>
#include <wtf/glib/GRefPtr.h>

namespace WTF {

class GFdMonitor : public RefCounted<GFdMonitor> {
    WTF_MAKE_NONCOPYABLE(GFdMonitor);
public:
    GFdMonitor() = default;
    WTF_EXPORT_PRIVATE ~GFdMonitor();

    WTF_EXPORT_PRIVATE void start(gint fd, GIOCondition condition,
            GMainContext* gctxt,
            Function<gboolean(gint, GIOCondition)>&& callback);
    WTF_EXPORT_PRIVATE void stop();
    bool isActive() const { return !!m_source; }

private:
    static gboolean fdSourceCallback(gint, GIOCondition, GFdMonitor*);

    GRefPtr<GSource> m_source;
    Function<gboolean(gint, GIOCondition)> m_callback;
};

} // namespace WTF

using WTF::GFdMonitor;

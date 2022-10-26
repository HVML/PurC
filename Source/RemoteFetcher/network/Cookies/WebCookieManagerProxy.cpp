/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebCookieManagerProxy.h"

#include "NetworkProcessMessages.h"
#include "WebCookieManagerMessages.h"
#include "WebCookieManagerProxyMessages.h"
#include "Cookie.h"
#include "SecurityOriginData.h"

namespace PurCFetcher {
using namespace PurCFetcher;

const char* WebCookieManagerProxy::supplementName()
{
    return "WebCookieManagerProxy";
}

Ref<WebCookieManagerProxy> WebCookieManagerProxy::create()
{
    return adoptRef(*new WebCookieManagerProxy());
}

WebCookieManagerProxy::WebCookieManagerProxy()
{
}

WebCookieManagerProxy::~WebCookieManagerProxy()
{
    ASSERT(m_cookieObservers.isEmpty());
}

void WebCookieManagerProxy::initializeClient()
{
}

// WebContextSupplement

void WebCookieManagerProxy::processPoolDestroyed()
{
    Vector<Observer*> observers;
    for (auto& observerSet : m_cookieObservers.values()) {
        for (auto* observer : observerSet)
            observers.append(observer);
    }

    for (auto* observer : observers)
        observer->managerDestroyed();

    ASSERT(m_cookieObservers.isEmpty());
}

void WebCookieManagerProxy::processDidClose()
{
}

void WebCookieManagerProxy::refWebContextSupplement()
{
    API::Object::ref();
}

void WebCookieManagerProxy::derefWebContextSupplement()
{
    API::Object::deref();
}

void WebCookieManagerProxy::getHostnamesWithCookies(PAL::SessionID, CompletionHandler<void(Vector<String>&&)>&&)
{
}

void WebCookieManagerProxy::deleteCookiesForHostnames(PAL::SessionID, const Vector<String>&)
{
}

void WebCookieManagerProxy::deleteAllCookies(PAL::SessionID)
{
}

void WebCookieManagerProxy::deleteCookie(PAL::SessionID, const Cookie&, CompletionHandler<void()>&&)
{
}

void WebCookieManagerProxy::deleteAllCookiesModifiedSince(PAL::SessionID, WallTime, CompletionHandler<void()>&&)
{
}

void WebCookieManagerProxy::setCookies(PAL::SessionID, const Vector<Cookie>&, CompletionHandler<void()>&&)
{
}

void WebCookieManagerProxy::setCookies(PAL::SessionID, const Vector<Cookie>&, const URL&, const URL&, CompletionHandler<void()>&&)
{
}

void WebCookieManagerProxy::getAllCookies(PAL::SessionID, CompletionHandler<void(Vector<Cookie>&&)>&&)
{
}

void WebCookieManagerProxy::getCookies(PAL::SessionID, const URL&, CompletionHandler<void(Vector<Cookie>&&)>&&)
{
}

void WebCookieManagerProxy::startObservingCookieChanges(PAL::SessionID)
{
}

void WebCookieManagerProxy::stopObservingCookieChanges(PAL::SessionID)
{
}

void WebCookieManagerProxy::setCookieObserverCallback(PAL::SessionID sessionID, PurCWTF::Function<void ()>&& callback)
{
    if (callback)
        m_legacyCookieObservers.set(sessionID, WTFMove(callback));
    else
        m_legacyCookieObservers.remove(sessionID);
}

void WebCookieManagerProxy::registerObserver(PAL::SessionID sessionID, Observer& observer)
{
    auto result = m_cookieObservers.set(sessionID, HashSet<Observer*>());
    result.iterator->value.add(&observer);

    if (result.isNewEntry)
        startObservingCookieChanges(sessionID);
}

void WebCookieManagerProxy::unregisterObserver(PAL::SessionID sessionID, Observer& observer)
{
    auto iterator = m_cookieObservers.find(sessionID);
    if (iterator == m_cookieObservers.end())
        return;

    iterator->value.remove(&observer);
    if (!iterator->value.isEmpty())
        return;

    m_cookieObservers.remove(iterator);
    stopObservingCookieChanges(sessionID);
}

void WebCookieManagerProxy::cookiesDidChange(PAL::SessionID sessionID)
{
    auto legacyIterator = m_legacyCookieObservers.find(sessionID);
    if (legacyIterator != m_legacyCookieObservers.end())
        ((*legacyIterator).value)();

    auto iterator = m_cookieObservers.find(sessionID);
    if (iterator == m_cookieObservers.end())
        return;

    for (auto* observer : iterator->value)
        observer->cookiesDidChange();
}

} // namespace PurCFetcher

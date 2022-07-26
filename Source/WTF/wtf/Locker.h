/*
 * Copyright (C) 2008-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Assertions.h>
#include <wtf/Atomics.h>
#include <wtf/Compiler.h>
#include <wtf/Noncopyable.h>

namespace PurCWTF {

enum NoLockingNecessaryTag { NoLockingNecessary };

class AbstractLocker {
    WTF_MAKE_NONCOPYABLE(AbstractLocker);
public:
    AbstractLocker(NoLockingNecessaryTag)
    {
    }
    
protected:
    AbstractLocker()
    {
    }
};

template <typename T> class Locker : public AbstractLocker {
public:
    explicit Locker(T& lockable) : m_lockable(&lockable) { lock(); }
    explicit Locker(T* lockable) : m_lockable(lockable) { lock(); }

    // You should be wary of using this constructor. It's only applicable
    // in places where there is a locking protocol for a particular object
    // but it's not necessary to engage in that protocol yet. For example,
    // this often happens when an object is newly allocated and it can not
    // be accessed concurrently.
    Locker(NoLockingNecessaryTag) : m_lockable(nullptr) { }
    
    Locker(int) = delete;

    ~Locker()
    {
        compilerFence();
        if (m_lockable)
            m_lockable->unlock();
    }
    
    static Locker tryLock(T& lockable)
    {
        Locker result(NoLockingNecessary);
        if (lockable.tryLock()) {
            result.m_lockable = &lockable;
            return result;
        }
        return result;
    }
    
    explicit operator bool() const { return !!m_lockable; }
    
    void unlockEarly()
    {
        m_lockable->unlock();
        m_lockable = 0;
    }
    
    // It's great to be able to pass lockers around. It enables custom locking adaptors like
    // JSC::LockDuringMarking.
    Locker(Locker&& other)
        : m_lockable(other.m_lockable)
    {
        other.m_lockable = nullptr;
    }
    
    Locker& operator=(Locker&& other)
    {
        if (m_lockable)
            m_lockable->unlock();
        m_lockable = other.m_lockable;
        other.m_lockable = nullptr;
        return *this;
    }
    
private:
    void lock()
    {
        if (m_lockable)
            m_lockable->lock();
        compilerFence();
    }
    
    T* m_lockable;
};

// Use this lock scope like so:
// auto locker = holdLock(lock);
template<typename LockType>
Locker<LockType> holdLock(LockType&) WARN_UNUSED_RETURN;
template<typename LockType>
Locker<LockType> holdLock(LockType& lock)
{
    return Locker<LockType>(lock);
}

template<typename LockType>
Locker<LockType> holdLockIf(LockType&, bool predicate) WARN_UNUSED_RETURN;
template<typename LockType>
Locker<LockType> holdLockIf(LockType& lock, bool predicate)
{
    return Locker<LockType>(predicate ? &lock : nullptr);
}

template<typename LockType>
Locker<LockType> tryHoldLock(LockType&) WARN_UNUSED_RETURN;
template<typename LockType>
Locker<LockType> tryHoldLock(LockType& lock)
{
    return Locker<LockType>::tryLock(lock);
}

}

using PurCWTF::AbstractLocker;
using PurCWTF::Locker;
using PurCWTF::NoLockingNecessaryTag;
using PurCWTF::NoLockingNecessary;
using PurCWTF::holdLock;
using PurCWTF::holdLockIf;

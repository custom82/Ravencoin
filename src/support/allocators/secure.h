// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_SUPPORT_ALLOCATORS_SECURE_H
#define RAVEN_SUPPORT_ALLOCATORS_SECURE_H

#include "support/lockedpool.h"
#include "support/cleanse.h"

#include <memory>
#include <string>
#include <vector>

//
// Allocator that locks its contents from being paged
// out of memory and clears its contents before deletion.
//
template <typename T>
struct secure_allocator : public std::allocator<T> {
    // MSVC8 default copy constructor is broken
    typedef std::allocator<T> base;
    typedef typename std::allocator_traits<base>::size_type size_type;
    typedef typename std::allocator_traits<base>::difference_type difference_type;
    typedef typename std::allocator_traits<base>::pointer pointer;
    typedef typename std::allocator_traits<base>::const_pointer const_pointer;
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    secure_allocator() noexcept {}
    secure_allocator(const secure_allocator& a) noexcept : base(a) {}
    template <typename U>
    secure_allocator(const secure_allocator<U>& a) noexcept : base(a)
    {
    }
    ~secure_allocator() noexcept {}
    template <typename _Other>
    struct rebind {
        typedef secure_allocator<_Other> other;
    };

    pointer allocate(size_type n, const void* hint = 0)
    {
        return static_cast<pointer>(LockedPoolManager::Instance().alloc(sizeof(T) * n));
    }

    void deallocate(pointer p, size_type n)
    {
        if (p != nullptr) {
            memory_cleanse(p, sizeof(T) * n);
        }
        LockedPoolManager::Instance().free(p);
    }
};

// This is exactly like std::string, but with a custom allocator.
typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char> > SecureString;
typedef std::vector<unsigned char, secure_allocator<unsigned char> >             SecureVector;

#endif // RAVEN_SUPPORT_ALLOCATORS_SECURE_H

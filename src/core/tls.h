// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "common/logging/log.h"
#include "common/types.h"
#ifdef _WIN32
#include <malloc.h>
#endif

namespace Xbyak {
class CodeGenerator;
}

namespace Libraries::Fiber {
struct OrbisFiberContext;
}

namespace Core {

union DtvEntry {
    std::size_t counter;
    u8* pointer;
};

struct Tcb {
    Tcb* tcb_self;
    DtvEntry* tcb_dtv;
    void* tcb_thread;
    void* tcb_spare[2];
    u64 tcb_canary;
    ::Libraries::Fiber::OrbisFiberContext* tcb_fiber;
};

#ifdef _WIN32
/// Gets the thread local storage key for the TCB block.
u32 GetTcbKey();
#endif

/// Sets the data pointer to the TCB block.
void SetTcbBase(void* image_address);

/// Retrieves Tcb structure for the calling thread.
Tcb* GetTcbBase();

/// Makes sure TLS is initialized for the thread before entering guest.
void InitializeTLS();

template <auto f>
struct HostCallWrapperImpl;

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// Guest callers occasionally pass small integers (e.g. rdx=0x1) where the
// host signature expects a pointer; detect this and short-circuit to avoid
// a SIGSEGV before the real function can validate the argument.
inline bool IsBadGuestPointer(const void* p) {
    if (p == nullptr) return true;
    const auto v = reinterpret_cast<std::uintptr_t>(p);
    return v < 0x100000000ULL;
}

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// Some libc preinit calls pass small integers (rdx=0x1) where a pointer is
// expected; dereferencing crashes. Filter out bad pointers in pointer-typed
// args before forwarding, returning a benign default so the guest can recover.
// Only enabled when D1_PRESERVATION_BAD_GUEST_PTR_GUARD is set (compile-time
// toggle to keep upstream builds unaffected).
#ifdef D1_PRESERVATION_BAD_GUEST_PTR_GUARD
inline bool AnyBadPointer() {
    return false;
}
template <class T>
inline bool AnyBadPointer(const T& v) {
    if constexpr (std::is_pointer_v<T>) {
        return IsBadGuestPointer(v);
    } else {
        return false;
    }
}
template <class T, class... Rest>
inline bool AnyBadPointer(const T& v, const Rest&... rest) {
    return AnyBadPointer(v) || AnyBadPointer(rest...);
}
#endif

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// Process-wide HLE bridge call counter. Shared by every
// HostCallWrapperImpl instantiation so the count reflects total
// HLE traffic, not per-function. Returned by reference so callers
// can initialise a static local with it.
inline std::atomic<u64>& HleBridgeCallCounter() {
    static std::atomic<u64> counter{0};
    return counter;
}

template <class ReturnType, class... Args, PS4_SYSV_ABI ReturnType (*func)(Args...)>
struct HostCallWrapperImpl<func> {
    static ReturnType PS4_SYSV_ABI wrap(Args... args) {
#ifdef D1_PRESERVATION_HLE_TRACE
        // D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
        // Diagnostic: count every HLE bridge call and log the first N with the
        // templated function pointer address. After N, log a heartbeat every
        // HEARTBEAT calls. Lets us see what user code is doing when the guest
        // is silent (libc is past preinit, no further log output). Toggled
        // by compile define so upstream builds are unaffected.
        static std::atomic<u64>& call_count = HleBridgeCallCounter();
        const u64 n = call_count.fetch_add(1, std::memory_order_relaxed) + 1;
        const std::uintptr_t func_addr = reinterpret_cast<std::uintptr_t>(func);
        if (n <= 50) {
            LOG_INFO(HLE_Bridge, "HLE call #{:>3} -> func=0x{:016x}", n, func_addr);
        } else if ((n % 5000ULL) == 0ULL) {
            LOG_INFO(HLE_Bridge, "HLE heartbeat: {} calls so far (last func=0x{:016x})", n,
                     func_addr);
        }
#endif
#ifdef D1_PRESERVATION_BAD_GUEST_PTR_GUARD
        if (AnyBadPointer(args...)) {
            // D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
            // Return a benign default so the libc can continue past the bogus call.
            if constexpr (std::is_void_v<ReturnType>) {
                return;
            } else if constexpr (std::is_pointer_v<ReturnType>) {
                return nullptr;
            } else {
                return static_cast<ReturnType>(0);
            }
        }
#endif
        return func(args...);
    }
};

#define HOST_CALL(func) (Core::HostCallWrapperImpl<func>::wrap)

} // namespace Core

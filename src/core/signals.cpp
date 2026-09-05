// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "common/decoder.h"
#include "common/signal_context.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/cpu_patches.h" // Windows static guest red-zone protection
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/linker.h"
#include "core/signals.h"
#include "emulator.h"

#ifdef _WIN32
#include <windows.h>
#ifdef ARCH_X86_64
#include <Zydis/Formatter.h>
#endif
static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
#else
#include <csignal>
#include <pthread.h>
#ifdef ARCH_X86_64
#include <Zydis/Formatter.h>
#endif
#endif

namespace Core {

#if defined(_WIN32)

static LONG WINAPI SignalHandler(EXCEPTION_POINTERS* pExp) noexcept {
    using namespace Libraries::Kernel;
    const auto* signals = Signals::Instance();
    // Windows static guest red-zone protection
    const bool use_static_windows_guest_red_zone_protection =
        WindowsGuestRedZoneProtection::IsStaticPatchingEnabled();
    DWORD code = 0;
    PVOID address = nullptr;

    if (pExp != nullptr && pExp->ExceptionRecord != nullptr) {
        code = pExp->ExceptionRecord->ExceptionCode;
        address = pExp->ExceptionRecord->ExceptionAddress;
    }

    Ucontext guest_context{pExp->ContextRecord};
    Siginfo guest_info{
        ._si_signo = 0,
        ._si_errno = 0,
        ._si_code = POSIX_SI_NOINFO,
        ._si_addr = (void*)guest_context.uc_mcontext.mc_rip,
    };

    bool handled = false;
    bool static_protection_exception = false; // Windows static guest red-zone protection
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        guest_info._si_signo = POSIX_SIGSEGV;
        guest_info._si_code = POSIX_SEGV_MAPERR;
        static_protection_exception = true; // Windows static guest red-zone protection
        handled = signals->DispatchAccessViolation(
            pExp, reinterpret_cast<void*>(pExp->ExceptionRecord->ExceptionInformation[1]));
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        guest_info._si_signo = POSIX_SIGILL;
        guest_info._si_code = POSIX_ILL_ILLOPC;
        static_protection_exception = true; // Windows static guest red-zone protection
        handled = signals->DispatchIllegalInstruction(pExp);
        break;
    case EXCEPTION_PRIV_INSTRUCTION: // Windows static guest red-zone protection
        if (use_static_windows_guest_red_zone_protection) {
            static_protection_exception = true;
            handled = signals->DispatchIllegalInstruction(pExp);
        }
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        guest_info._si_signo = POSIX_SIGBUS;
        guest_info._si_code = POSIX_BUS_ADRALN;
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_INTDIV;
        break;
    case EXCEPTION_INT_OVERFLOW:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_INTOVF;
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTDIV;
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTINV;
        break;
    case EXCEPTION_FLT_OVERFLOW:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTOVF;
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTUND;
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTSUB; // i am not sure about this one
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTRES;
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        guest_info._si_signo = POSIX_SIGILL;
        guest_info._si_code = POSIX_ILL_BADSTK; // i am not sure about this one either
        break;
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        guest_info._si_signo = POSIX_SIGTRAP;
        guest_info._si_code = POSIX_TRAP_BRKPT;
        break;
    case DBG_PRINTEXCEPTION_C:
    case DBG_PRINTEXCEPTION_WIDE_C:
        // Used by OutputDebugString functions.
        return EXCEPTION_CONTINUE_EXECUTION;
    case MS_VC_EXCEPTION:
        LOG_DEBUG(Debug, "Pass MS_VC_EXCEPTION at {} to handler", address);
        return EXCEPTION_EXECUTE_HANDLER;
    default:
        break;
    }

    if (handled) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (guest_info._si_signo != 0) {
        if (g_curthread &&
            g_curthread->DispatchSignal(guest_info._si_signo, &guest_info, &guest_context)) {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    // Windows static guest red-zone protection
    const bool report_unhandled = use_static_windows_guest_red_zone_protection
                                      ? static_protection_exception
                                      : code != EXCEPTION_BREAKPOINT;
    if (report_unhandled) { // Windows static guest red-zone protection
        // D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
        // Identify the module owning the PC and disassemble the faulting instr so a SIGSEGV in the user region leaves a breadcrumb.
        const VAddr pc = static_cast<VAddr>(guest_context.uc_mcontext.mc_rip);
        const char* mod_name = "<unmapped>";
        if (auto* linker = Common::Singleton<Core::Linker>::Instance(); linker) {
            if (auto* mod = linker->FindByAddress(pc); mod) {
                mod_name = mod->name.c_str();
            }
        }
        // Disassemble using the same decoder the Linux branch uses.
        char disasm[256] = "<unable to decode>";
#ifdef ARCH_X86_64
        ZydisDecodedInstruction instr;
        ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
        if (ZYAN_SUCCESS(Common::Decoder::Instance()->decodeInstruction(instr, ops,
                                                                       reinterpret_cast<void*>(pc)))) {
            ZydisFormatter formatter;
            ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
            ZydisFormatterFormatInstruction(&formatter, &instr, ops, instr.operand_count_visible,
                                            disasm, sizeof(disasm), pc, ZYAN_NULL);
        }
        // Also walk the guest stack to find the return address (the call site
        // of the host stub that just returned). This is what identifies the
        // missing HLE function by the import that was stubbed.
        const VAddr rsp = static_cast<VAddr>(guest_context.uc_mcontext.mc_rsp);
        VAddr ret_addr = 0;
        std::memcpy(&ret_addr, reinterpret_cast<const void*>(rsp), sizeof(ret_addr));
        const char* ret_mod = "<unmapped>";
        if (auto* linker = Common::Singleton<Core::Linker>::Instance(); linker) {
            if (auto* mod = linker->FindByAddress(ret_addr); mod) {
                ret_mod = mod->name.c_str();
            }
        }
        const VAddr ret_off = ret_addr; // base subtraction done by reader
        // D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
        // Dump a 6-frame call stack and the registers that point at the bad destination; rdi is the dest of `mov [rdi], rax` and almost always the cause, rsi/rdx tell which call just returned.
        std::string stack_trace = "\n  return-to " + fmt::format("{:#x} in module {}", ret_addr, ret_mod);
        // D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
        // Walk the call chain by rbp frame pointers; PS4 SysV prologue is `push rbp; mov rbp, rsp` so each frame has [rbp]=saved rbp and [rbp+8]=return address.
        VAddr frame_rbp = guest_context.uc_mcontext.mc_rbp;
        for (int frame = 1; frame <= 6 && frame_rbp != 0; ++frame) {
            VAddr frame_ret = 0;
            if (!std::memcpy(&frame_ret, reinterpret_cast<const void*>(frame_rbp + 8),
                             sizeof(frame_ret))) {
                break;
            }
            const char* frame_mod = "<unmapped>";
            if (auto* linker = Common::Singleton<Core::Linker>::Instance(); linker) {
                if (auto* mod = linker->FindByAddress(frame_ret); mod) {
                    frame_mod = mod->name.c_str();
                }
            }
            stack_trace += "\n  frame " + std::to_string(frame) + ": " +
                           fmt::format("{:#x} in module {}", frame_ret, frame_mod);
            VAddr next_rbp = 0;
            if (!std::memcpy(&next_rbp, reinterpret_cast<const void*>(frame_rbp),
                             sizeof(next_rbp))) {
                break;
            }
            if (next_rbp <= frame_rbp) {
                break; // rbp must strictly grow
            }
            frame_rbp = next_rbp;
        }
        const u64 rdi_val = guest_context.uc_mcontext.mc_rdi;
        const u64 rsi_val = guest_context.uc_mcontext.mc_rsi;
        const u64 rdx_val = guest_context.uc_mcontext.mc_rdx;
        const u64 rcx_val = guest_context.uc_mcontext.mc_rcx;
        const u64 rax_val = guest_context.uc_mcontext.mc_rax;
        const u64 rbp_val = guest_context.uc_mcontext.mc_rbp;
        const u64 r8_val = guest_context.uc_mcontext.mc_r8;
        const u64 r9_val = guest_context.uc_mcontext.mc_r9;
        // D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
        // Dump 16 bytes at the host alias of the faulting PC and 32 bytes at rsp+0; IsBadReadPtr is fault-safe so an unmapped read does not re-fault.
        u8 pc_bytes[16] = {0};
        u8 rsp_bytes[32] = {0};
        if (!::IsBadReadPtr(reinterpret_cast<const void*>(pc), sizeof(pc_bytes))) {
            std::memcpy(pc_bytes, reinterpret_cast<const void*>(pc), sizeof(pc_bytes));
        }
        if (!::IsBadReadPtr(reinterpret_cast<const void*>(rsp), sizeof(rsp_bytes))) {
            std::memcpy(rsp_bytes, reinterpret_cast<const void*>(rsp), sizeof(rsp_bytes));
        }
        std::string pc_hex;
        std::string rsp_hex;
        for (u8 b : pc_bytes) {
            pc_hex += fmt::format("{:02x} ", b);
        }
        for (u8 b : rsp_bytes) {
            rsp_hex += fmt::format("{:02x} ", b);
        }
        LOG_CRITICAL(Debug,
                     "Unhandled Exception code {:#x} at {} in module {}: {}{}\n  "
                     "rax={:#x} rcx={:#x} rdx={:#x} rsi={:#x} rdi={:#x} rbp={:#x} r8={:#x} r9={:#x} rsp={:#x}\n  "
                     "pc_bytes: {}\n  rsp_bytes: {}",
                     code, address, mod_name, disasm, stack_trace, rax_val, rcx_val, rdx_val,
                     rsi_val, rdi_val, rbp_val, r8_val, r9_val, rsp, pc_hex, rsp_hex);
#else
        LOG_CRITICAL(Debug, "Unhandled Exception code {:#x} at {} in module {}", code, address,
                     mod_name);
#endif
        Common::Singleton<Core::Emulator>::Instance()->Shutdown();
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#else

static std::string DisassembleInstruction(void* code_address) {
    char buffer[256] = "<unable to decode>";

#ifdef ARCH_X86_64
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    const auto status =
        Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_address);
    if (ZYAN_SUCCESS(status)) {
        ZydisFormatter formatter;
        ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
        ZydisFormatterFormatInstruction(&formatter, &instruction, operands,
                                        instruction.operand_count_visible, buffer, sizeof(buffer),
                                        reinterpret_cast<u64>(code_address), ZYAN_NULL);
    }
#endif

    return buffer;
}

static s32 NativeSiCodeToGuest(s32 sig, s32 code) {
    using namespace Libraries::Kernel;
    switch (sig) {
    case SIGUSR1:
        return POSIX_SI_LWP;
    case SIGSEGV:
        switch (code) {
        case SEGV_MAPERR:
            return POSIX_SEGV_MAPERR;
        case SEGV_ACCERR:
            return POSIX_SEGV_ACCERR;
        }
    case SIGBUS:
        switch (code) {
        case BUS_ADRALN:
            return POSIX_BUS_ADRALN;
        case BUS_ADRERR:
            return POSIX_BUS_ADRERR;
        case BUS_OBJERR:
            return POSIX_BUS_OBJERR;
        }
    case SIGILL:
        switch (code) {
        case ILL_ILLOPC:
            return POSIX_ILL_ILLOPC;
        case ILL_ILLOPN:
            return POSIX_ILL_ILLOPN;
        case ILL_ILLADR:
            return POSIX_ILL_ILLADR;
        case ILL_ILLTRP:
            return POSIX_ILL_ILLTRP;
        case ILL_PRVOPC:
            return POSIX_ILL_PRVOPC;
        case ILL_PRVREG:
            return POSIX_ILL_PRVREG;
        case ILL_COPROC:
            return POSIX_ILL_COPROC;
        case ILL_BADSTK:
            return POSIX_ILL_BADSTK;
        }
    case SIGFPE:
        switch (code) {
        case FPE_INTOVF:
            return POSIX_FPE_INTOVF;
        case FPE_INTDIV:
            return POSIX_FPE_INTDIV;
        case FPE_FLTDIV:
            return POSIX_FPE_FLTDIV;
        case FPE_FLTOVF:
            return POSIX_FPE_FLTOVF;
        case FPE_FLTUND:
            return POSIX_FPE_FLTUND;
        case FPE_FLTRES:
            return POSIX_FPE_FLTRES;
        case FPE_FLTINV:
            return POSIX_FPE_FLTINV;
        case FPE_FLTSUB:
            return POSIX_FPE_FLTSUB;
        }
    case SIGTRAP:
        switch (code) {
        case TRAP_BRKPT:
            return POSIX_TRAP_BRKPT;
        case TRAP_TRACE:
            return POSIX_TRAP_TRACE;
#ifdef __FreeBSD__
        case TRAP_DTRACE:
            return POSIX_TRAP_DTRACE;
#endif
        }

    default:
        return POSIX_SI_NOINFO;
    }
}

void SignalHandler(int sig, siginfo_t* info, void* raw_context) {
    using namespace Libraries::Kernel;
    auto* thread = g_curthread;
    const auto* signals = Signals::Instance();

    auto* code_address = Common::GetRip(raw_context);

    Ucontext context{info, reinterpret_cast<ucontext_t*>(raw_context)};
    Siginfo guest_info{};
    if (info) {
        guest_info = *reinterpret_cast<Siginfo*>(info);
        guest_info._si_signo = sig == SIGUSR1 ? 0 : NativeToOrbisSignal(info->si_signo);
        guest_info._si_errno = NativeToPosixErrno(info->si_errno);
        guest_info._si_code = NativeSiCodeToGuest(sig, info->si_code);
        guest_info._si_addr = (void*)context.uc_mcontext.mc_rip;
    }
    Siginfo* info_p = info ? &guest_info : nullptr;
    Ucontext* context_p = raw_context ? &context : nullptr;

    switch (sig) {
    case SIGSEGV:
    case SIGBUS: {
        const bool is_write = Common::IsWriteError(raw_context);
        if (!signals->DispatchAccessViolation(raw_context, info->si_addr)) {
            if (thread && thread->DispatchSignal(NativeToOrbisSignal(sig), info_p, context_p)) {
                return;
            }
            UNREACHABLE_MSG("Unhandled access violation at code address {}: {} address {}",
                            fmt::ptr(code_address), is_write ? "Write to" : "Read from",
                            fmt::ptr(info->si_addr));
        }
        break;
    }
    case SIGILL:
        if (signals->DispatchIllegalInstruction(raw_context)) {
            return;
        }
    case SIGFPE:
    case SIGTRAP:
    case SIGSYS: {
        if (thread && thread->DispatchSignal(NativeToOrbisSignal(sig), info_p, context_p)) {
            return;
        }

        UNREACHABLE_MSG("Unhandled signal {} at code address {}", sig, fmt::ptr(code_address));
    }
    case SIGSLEEP: {
        // Sleep thread until signal is received again
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGSLEEP);
        sigwait(&sigset, &sig);
        break;
    }
    case SIGUSR1:
        if (thread) {
            thread->DispatchPendingSignals(info_p, context_p);
        }
        break;
    default:
        UNREACHABLE_MSG("Unhandled signal {} at code address {}", sig, fmt::ptr(code_address));
    }
}

#endif

SignalDispatch::SignalDispatch() {
#if defined(_WIN32)
    ASSERT_MSG(handle = AddVectoredExceptionHandler(0, SignalHandler),
               "Failed to register exception handler.");
#else
    struct sigaction action{};
    action.sa_sigaction = SignalHandler;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);

    ASSERT_MSG(
        sigaction(SIGSEGV, &action, nullptr) == 0 && sigaction(SIGBUS, &action, nullptr) == 0 &&
            sigaction(SIGILL, &action, nullptr) == 0 && sigaction(SIGFPE, &action, nullptr) == 0 &&
            sigaction(SIGTRAP, &action, nullptr) == 0 && sigaction(SIGSYS, &action, nullptr) == 0 &&
            sigaction(SIGUSR1, &action, nullptr) == 0 && sigaction(SIGSLEEP, &action, nullptr) == 0,
        "Failed to register signal handlers.");
#endif
}

void SignalDispatch::RemoveHandlers() {
    // asserting here would get into an infinite loop until too
    // many nested exceptions makes the OS kill the process
#if defined(_WIN32)
    if (!(RemoveVectoredExceptionHandler(handle))) {
        LOG_CRITICAL(Core, "Failed to remove exception handler.");
        std::quick_exit(1);
    }
#else
    struct sigaction action{};
    action.sa_handler = SIG_DFL;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);

    if (!(sigaction(SIGSEGV, &action, nullptr) == 0 && sigaction(SIGBUS, &action, nullptr) == 0 &&
          sigaction(SIGILL, &action, nullptr) == 0 && sigaction(SIGFPE, &action, nullptr) == 0 &&
          sigaction(SIGTRAP, &action, nullptr) == 0 && sigaction(SIGSYS, &action, nullptr) == 0 &&
          sigaction(SIGUSR1, &action, nullptr) == 0 &&
          sigaction(SIGSLEEP, &action, nullptr) == 0)) {
        LOG_CRITICAL(Core, "Failed to remove signal handlers.");
        std::quick_exit(1);
    }
#endif
}

SignalDispatch::~SignalDispatch() {
    RemoveHandlers();
}

bool SignalDispatch::DispatchAccessViolation(void* context, void* fault_address) const {
    for (const auto& [handler, _] : access_violation_handlers) {
        if (handler(context, fault_address)) {
            return true;
        }
    }
    return false;
}

bool SignalDispatch::DispatchIllegalInstruction(void* context) const {
    for (const auto& [handler, _] : illegal_instruction_handlers) {
        if (handler(context)) {
            return true;
        }
    }
    return false;
}

} // namespace Core

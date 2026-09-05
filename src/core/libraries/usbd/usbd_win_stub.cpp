// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// Windows-only usbd stubs: real usbd.cpp is excluded on Windows, but ipc.cpp and libs.cpp still
// reference the `usb_backend` global and `RegisterLib` from usbd.h. We must avoid pulling in
// usb_backend.h (uses pthread_mutex_t outside of <pthread.h>); provide the symbols via usbd.h
// only, which already forward-declares UsbBackend.

#include <memory>
#include "core/libraries/usbd/usbd.h"

namespace Libraries::Usbd {

std::shared_ptr<UsbBackend> usb_backend;

void RegisterLib(Core::Loader::SymbolsResolver*) {}

} // namespace Libraries::Usbd

// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// libusb_* stubs returning LIBUSB_ERROR_NOT_SUPPORTED: link-only, unreachable at runtime (usbd.cpp excluded on Windows).

#include "libusb.h"

extern "C" {

int libusb_init(libusb_context** ctx) { (void)ctx; return LIBUSB_ERROR_NOT_SUPPORTED; }
void libusb_exit(libusb_context* ctx) { (void)ctx; }

ssize_t libusb_get_device_list(libusb_context* ctx, libusb_device*** list) {
    (void)ctx; (void)list; return LIBUSB_ERROR_NOT_SUPPORTED;
}
void libusb_free_device_list(libusb_device** list, int unref_devices) {
    (void)list; (void)unref_devices;
}
void libusb_ref_device(libusb_device* dev) { (void)dev; }
void libusb_unref_device(libusb_device* dev) { (void)dev; }

int libusb_get_configuration(libusb_device_handle* dev, int* config) {
    (void)dev; (void)config; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_get_device_descriptor(libusb_device* dev, libusb_device_descriptor* desc) {
    (void)dev; (void)desc; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_get_active_config_descriptor(libusb_device* dev, libusb_config_descriptor** config) {
    (void)dev; (void)config; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_get_config_descriptor(libusb_device* dev, uint8_t idx, libusb_config_descriptor** cfg) {
    (void)dev; (void)idx; (void)cfg; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_get_config_descriptor_by_value(libusb_device* dev, uint8_t v, libusb_config_descriptor** cfg) {
    (void)dev; (void)v; (void)cfg; return LIBUSB_ERROR_NOT_SUPPORTED;
}
void libusb_free_config_descriptor(libusb_config_descriptor* config) { (void)config; }

uint8_t libusb_get_bus_number(libusb_device* dev) { (void)dev; return 0; }
uint8_t libusb_get_device_address(libusb_device* dev) { (void)dev; return 0; }
int libusb_get_device_speed(libusb_device* dev) {
    (void)dev; return LIBUSB_SPEED_UNKNOWN;
}
int libusb_get_max_packet_size(libusb_device* dev, uint8_t endpoint) {
    (void)dev; (void)endpoint; return 0;
}

int libusb_open(libusb_device* dev, libusb_device_handle** handle) {
    (void)dev; (void)handle; return LIBUSB_ERROR_NOT_SUPPORTED;
}
void libusb_close(libusb_device_handle* handle) { (void)handle; }
libusb_device* libusb_get_device(libusb_device_handle* handle) {
    (void)handle; return nullptr;
}

int libusb_set_configuration(libusb_device_handle* dev, int configuration) {
    (void)dev; (void)configuration; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_claim_interface(libusb_device_handle* dev, int iface) {
    (void)dev; (void)iface; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_release_interface(libusb_device_handle* dev, int iface) {
    (void)dev; (void)iface; return LIBUSB_ERROR_NOT_SUPPORTED;
}
libusb_device_handle* libusb_open_device_with_vid_pid(libusb_context* ctx, uint16_t vid, uint16_t pid) {
    (void)ctx; (void)vid; (void)pid; return nullptr;
}
int libusb_reset_device(libusb_device_handle* dev) { (void)dev; return LIBUSB_ERROR_NOT_SUPPORTED; }

int libusb_kernel_driver_active(libusb_device_handle* dev, int iface) {
    (void)dev; (void)iface; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_detach_kernel_driver(libusb_device_handle* dev, int iface) {
    (void)dev; (void)iface; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_attach_kernel_driver(libusb_device_handle* dev, int iface) {
    (void)dev; (void)iface; return LIBUSB_ERROR_NOT_SUPPORTED;
}

int libusb_control_transfer(libusb_device_handle* dev, uint8_t rtype, uint8_t req,
                            uint16_t val, uint16_t idx, unsigned char* data,
                            uint16_t length, unsigned int timeout) {
    (void)dev; (void)rtype; (void)req; (void)val; (void)idx;
    (void)data; (void)length; (void)timeout;
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

libusb_transfer* libusb_alloc_transfer(int iso_packets) { (void)iso_packets; return nullptr; }
int libusb_submit_transfer(libusb_transfer* transfer) { (void)transfer; return LIBUSB_ERROR_NOT_SUPPORTED; }
int libusb_cancel_transfer(libusb_transfer* transfer) { (void)transfer; return LIBUSB_ERROR_NOT_SUPPORTED; }
void libusb_free_transfer(libusb_transfer* transfer) { (void)transfer; }

void libusb_fill_control_transfer(libusb_transfer* t, libusb_device_handle* d,
                                  unsigned char* buf, void (*cb)(libusb_transfer*), void* ud,
                                  unsigned int timeout) {
    (void)t; (void)d; (void)buf; (void)cb; (void)ud; (void)timeout;
}
void libusb_fill_bulk_transfer(libusb_transfer* t, libusb_device_handle* d, uint8_t ep,
                               unsigned char* buf, int length,
                               void (*cb)(libusb_transfer*), void* ud, unsigned int timeout) {
    (void)t; (void)d; (void)ep; (void)buf; (void)length; (void)cb; (void)ud; (void)timeout;
}
void libusb_fill_interrupt_transfer(libusb_transfer* t, libusb_device_handle* d, uint8_t ep,
                                    unsigned char* buf, int length,
                                    void (*cb)(libusb_transfer*), void* ud, unsigned int timeout) {
    (void)t; (void)d; (void)ep; (void)buf; (void)length; (void)cb; (void)ud; (void)timeout;
}
void libusb_fill_iso_transfer(libusb_transfer* t, libusb_device_handle* d, uint8_t ep,
                              unsigned char* buf, int length, int niso,
                              void (*cb)(libusb_transfer*), void* ud, unsigned int timeout) {
    (void)t; (void)d; (void)ep; (void)buf; (void)length; (void)niso; (void)cb; (void)ud; (void)timeout;
}

uint8_t* libusb_control_transfer_get_data(libusb_transfer* t) { (void)t; return nullptr; }
libusb_control_setup* libusb_control_transfer_get_setup(libusb_transfer* t) { (void)t; return nullptr; }

void libusb_fill_control_setup(unsigned char* buf, uint8_t rtype, uint8_t req,
                               uint16_t val, uint16_t idx, uint16_t length) {
    (void)buf; (void)rtype; (void)req; (void)val; (void)idx; (void)length;
}

int libusb_try_lock_events(libusb_context* ctx) { (void)ctx; return LIBUSB_ERROR_NOT_SUPPORTED; }
void libusb_lock_events(libusb_context* ctx) { (void)ctx; }
void libusb_unlock_events(libusb_context* ctx) { (void)ctx; }
int libusb_event_handling_ok(libusb_context* ctx) { (void)ctx; return 0; }
int libusb_event_handler_active(libusb_context* ctx) { (void)ctx; return 0; }
void libusb_lock_event_waiters(libusb_context* ctx) { (void)ctx; }
void libusb_unlock_event_waiters(libusb_context* ctx) { (void)ctx; }
int libusb_wait_for_event(libusb_context* ctx, struct timeval* tv) {
    (void)ctx; (void)tv; return LIBUSB_ERROR_NOT_SUPPORTED;
}

int libusb_handle_events_timeout(libusb_context* ctx, struct timeval* tv) {
    (void)ctx; (void)tv; return LIBUSB_ERROR_NOT_SUPPORTED;
}
int libusb_handle_events(libusb_context* ctx) { (void)ctx; return LIBUSB_ERROR_NOT_SUPPORTED; }
int libusb_handle_events_locked(libusb_context* ctx, struct timeval* tv) {
    (void)ctx; (void)tv; return LIBUSB_ERROR_NOT_SUPPORTED;
}

} // extern "C"

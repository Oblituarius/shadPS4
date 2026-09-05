// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// Type shim: libusb is POSIX-only, fork has no Windows port; provides opaque structs + enums + minimal struct bodies.

#pragma once

// Forward-declared `struct timeval` to avoid pulling <Winsock2.h>.
// usb_backend.h uses `timeval*` in method signatures only; no member
// access. The real timeval layout comes from sys/time.h on POSIX and
// from <Winsock2.h> on Windows; nothing in the shadps4 path reads
// timeval fields on Windows, so the layout is irrelevant.
struct timeval;

#include <cstddef>
#include <cstdint>

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;
struct libusb_config_descriptor;
struct libusb_interface;
struct libusb_interface_descriptor;
struct libusb_endpoint_descriptor;
struct libusb_transfer;
struct libusb_control_setup;

enum libusb_speed {
    LIBUSB_SPEED_UNKNOWN = 0,
    LIBUSB_SPEED_LOW = 1,
    LIBUSB_SPEED_FULL = 2,
    LIBUSB_SPEED_HIGH = 3,
    LIBUSB_SPEED_SUPER = 4,
    LIBUSB_SPEED_SUPER_PLUS = 5,
};

enum libusb_transfer_status {
    LIBUSB_TRANSFER_COMPLETED = 0,
    LIBUSB_TRANSFER_ERROR = 1,
    LIBUSB_TRANSFER_TIMED_OUT = 2,
    LIBUSB_TRANSFER_CANCELLED = 3,
    LIBUSB_TRANSFER_STALL = 4,
    LIBUSB_TRANSFER_NO_DEVICE = 5,
    LIBUSB_TRANSFER_OVERFLOW = 6,
};

enum libusb_transfer_type {
    LIBUSB_TRANSFER_TYPE_CONTROL = 0,
    LIBUSB_TRANSFER_TYPE_ISOCHRONOUS = 1,
    LIBUSB_TRANSFER_TYPE_BULK = 2,
    LIBUSB_TRANSFER_TYPE_INTERRUPT = 3,
};

#define LIBUSB_SUCCESS 0
#define LIBUSB_ERROR_IO -1
#define LIBUSB_ERROR_INVALID_PARAM -2
#define LIBUSB_ERROR_ACCESS -3
#define LIBUSB_ERROR_NO_DEVICE -4
#define LIBUSB_ERROR_NOT_FOUND -5
#define LIBUSB_ERROR_BUSY -6
#define LIBUSB_ERROR_TIMEOUT -7
#define LIBUSB_ERROR_OVERFLOW -8
#define LIBUSB_ERROR_PIPE -9
#define LIBUSB_ERROR_INTERRUPTED -10
#define LIBUSB_ERROR_NO_MEM -11
#define LIBUSB_ERROR_NOT_SUPPORTED -12
#define LIBUSB_ERROR_OTHER -99

#define LIBUSB_TRANSFER_FREE_TRANSFER 0x01
#define LIBUSB_TRANSFER_ZERO_PACKET 0x02

struct libusb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
};

struct libusb_endpoint_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
    uint8_t  bRefresh;
    uint8_t  bSynchAddress;
    uint8_t  *extra;
    int       extra_length;
};

struct libusb_interface_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
    struct libusb_endpoint_descriptor *endpoint;
    uint8_t  *extra;
    int       extra_length;
};

struct libusb_interface {
    struct libusb_interface_descriptor *altsetting;
    int num_altsetting;
};

struct libusb_config_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  MaxPower;
    struct libusb_interface *interface;
    uint8_t  *extra;
    int       extra_length;
};

struct libusb_control_setup {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

struct libusb_iso_packet_descriptor {
    unsigned int length;
    unsigned int actual_length;
    int          status;
};

struct libusb_transfer {
    struct libusb_device_handle *dev_handle;
    uint8_t                      flags;
    unsigned char               *endpoint;
    uint8_t                      type;
    unsigned int                 timeout;
    int                          status;
    int                          actual_length;
    int                          length;
    unsigned char               *buffer;
    int                          num_iso_packets;
    struct libusb_iso_packet_descriptor *iso_packet_desc;
    void                        *user_data;
    void                      (*callback)(struct libusb_transfer *);
    uint8_t                    **buffer_by;
    void                         *priv;
};

typedef long ssize_t;  // MSVC has no ssize_t in <sys/types.h>; libusb returns it.

// D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
// libusb_* C declarations: needed so usb_backend.h inline bodies find the identifiers (C3861).
extern "C" {

int              libusb_init(libusb_context** ctx);
void             libusb_exit(libusb_context* ctx);

ssize_t          libusb_get_device_list(libusb_context* ctx, libusb_device*** list);
void             libusb_free_device_list(libusb_device** list, int unref_devices);
void             libusb_ref_device(libusb_device* dev);
void             libusb_unref_device(libusb_device* dev);

int              libusb_get_configuration(libusb_device_handle* dev, int* config);
int              libusb_get_device_descriptor(libusb_device* dev, libusb_device_descriptor* desc);
int              libusb_get_active_config_descriptor(libusb_device* dev, libusb_config_descriptor** config);
int              libusb_get_config_descriptor(libusb_device* dev, uint8_t config_index, libusb_config_descriptor** config);
int              libusb_get_config_descriptor_by_value(libusb_device* dev, uint8_t bConfigurationValue, libusb_config_descriptor** config);
void             libusb_free_config_descriptor(libusb_config_descriptor* config);

uint8_t          libusb_get_bus_number(libusb_device* dev);
uint8_t          libusb_get_device_address(libusb_device* dev);
int              libusb_get_device_speed(libusb_device* dev);
int              libusb_get_max_packet_size(libusb_device* dev, uint8_t endpoint);

int              libusb_open(libusb_device* dev, libusb_device_handle** handle);
void             libusb_close(libusb_device_handle* handle);
libusb_device*   libusb_get_device(libusb_device_handle* handle);

int              libusb_set_configuration(libusb_device_handle* dev, int configuration);
int              libusb_claim_interface(libusb_device_handle* dev, int interface_number);
int              libusb_release_interface(libusb_device_handle* dev, int interface_number);
libusb_device_handle* libusb_open_device_with_vid_pid(libusb_context* ctx, uint16_t vendor_id, uint16_t product_id);
int              libusb_reset_device(libusb_device_handle* dev);

int              libusb_kernel_driver_active(libusb_device_handle* dev, int interface_number);
int              libusb_detach_kernel_driver(libusb_device_handle* dev, int interface_number);
int              libusb_attach_kernel_driver(libusb_device_handle* dev, int interface_number);

int              libusb_control_transfer(libusb_device_handle* dev, uint8_t bmRequestType,
                                        uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                        unsigned char* data, uint16_t wLength, unsigned int timeout);

libusb_transfer* libusb_alloc_transfer(int iso_packets);
int              libusb_submit_transfer(libusb_transfer* transfer);
int              libusb_cancel_transfer(libusb_transfer* transfer);
void             libusb_free_transfer(libusb_transfer* transfer);

void             libusb_fill_control_transfer(libusb_transfer* transfer, libusb_device_handle* dev,
                                             unsigned char* buffer, void (*callback)(libusb_transfer*),
                                             void* user_data, unsigned int timeout);
void             libusb_fill_bulk_transfer(libusb_transfer* transfer, libusb_device_handle* dev,
                                          unsigned char endpoint, unsigned char* buffer, int length,
                                          void (*callback)(libusb_transfer*), void* user_data,
                                          unsigned int timeout);
void             libusb_fill_interrupt_transfer(libusb_transfer* transfer, libusb_device_handle* dev,
                                               unsigned char endpoint, unsigned char* buffer, int length,
                                               void (*callback)(libusb_transfer*), void* user_data,
                                               unsigned int timeout);
void             libusb_fill_iso_transfer(libusb_transfer* transfer, libusb_device_handle* dev,
                                         unsigned char endpoint, unsigned char* buffer, int length,
                                         int num_iso_packets, void (*callback)(libusb_transfer*),
                                         void* user_data, unsigned int timeout);

uint8_t*         libusb_control_transfer_get_data(libusb_transfer* transfer);
libusb_control_setup* libusb_control_transfer_get_setup(libusb_transfer* transfer);

void             libusb_fill_control_setup(unsigned char* buffer, uint8_t bmRequestType,
                                           uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                           uint16_t wLength);

int              libusb_try_lock_events(libusb_context* ctx);
void             libusb_lock_events(libusb_context* ctx);
void             libusb_unlock_events(libusb_context* ctx);
int              libusb_event_handling_ok(libusb_context* ctx);
int              libusb_event_handler_active(libusb_context* ctx);
void             libusb_lock_event_waiters(libusb_context* ctx);
void             libusb_unlock_event_waiters(libusb_context* ctx);
int              libusb_wait_for_event(libusb_context* ctx, struct timeval* tv);

int              libusb_handle_events_timeout(libusb_context* ctx, struct timeval* tv);
int              libusb_handle_events(libusb_context* ctx);
int              libusb_handle_events_locked(libusb_context* ctx, struct timeval* tv);

} // extern "C"

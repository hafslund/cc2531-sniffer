#include <libusb.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "log.h"
#include "cc2531.h"

#define CC2531_INTERFACE 0
#define CC2531_TIMEOUT 1000
#define CC2531_BUFFER_SIZE 256

#define try_null(expr) if (expr < 0) return NULL
#define try(expr) if (expr < 0) return -1

enum _cc2531_request {
  CC2531_REQUEST_CONTROL = 9
};


struct cc2531 {
  struct log *log;
  libusb_device_handle *dh;
  unsigned char buf[CC2531_BUFFER_SIZE];
  uint16_t device_id;
};

struct  __attribute__ ((__packed__)) _cc2531_usb_header {
  unsigned char info;
  uint16_t length;
  uint32_t timestamp;
};

int
_cc2531_handle_transfer(struct cc2531 *cc2531, int r) {
  if (r >= 0) {
    log_msg(cc2531->log, LOG_LEVEL_DEBUG, "USB transfer: %d bytes.", r);
    return r;
  } else {
    switch(r) {
    case LIBUSB_ERROR_TIMEOUT:
      return log_err(cc2531->log, "USB transfer timeout.");
    case LIBUSB_ERROR_PIPE:
      return log_err(cc2531->log, "Control request not supported by USB device.");
    case LIBUSB_ERROR_NO_DEVICE:
      return log_err(cc2531->log, "USB device has been disconnected.");
    default:
      return log_err(cc2531->log, "USB error during transfer: ", r);
    }
  }
}

int
_cc2531_set_config(struct cc2531 *cc2531, unsigned short c) {
  return _cc2531_handle_transfer(cc2531, libusb_control_transfer(cc2531->dh, LIBUSB_ENDPOINT_OUT, CC2531_REQUEST_CONTROL, c, 0, NULL, 0, CC2531_TIMEOUT));
}

int
_cc2531_get_ctrl(struct cc2531 *cc2531, unsigned short c, unsigned short length) {
  return _cc2531_handle_transfer(cc2531, libusb_control_transfer(cc2531->dh, 0xC0, c, 0, 0, cc2531->buf, length, CC2531_TIMEOUT));
}

int
_cc2531_set_ctrl(struct cc2531 *cc2531, unsigned short c, unsigned short length, unsigned short index) {
  return _cc2531_handle_transfer(cc2531, libusb_control_transfer(cc2531->dh, 0x40, c, 0, index, cc2531->buf, length, CC2531_TIMEOUT));
}

int
_cc2531_wait_for_198(struct cc2531 *cc2531) {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 62400000;
  int cnt = 1;
  
  _cc2531_get_ctrl(cc2531, 198, 1);
  while (cc2531->buf[0] != 4) {
    nanosleep(&ts, NULL);
    _cc2531_get_ctrl(cc2531, 198, 1);
    if (++cnt > 20) return log_err(cc2531->log, "Didn't receive acknowledgment from CC2531 dongle.");    
  }

  return 0;
}

struct cc2531 *
cc2531_create(struct log *log) {
  libusb_device *dev = NULL;
  libusb_device_handle *dh = NULL;
  libusb_device **dev_list = NULL;
  struct libusb_device_descriptor desc;
  struct cc2531 *c = NULL;
  unsigned char desc_manufacturer[256];
  unsigned char desc_product[256];
  unsigned char desc_serial[256];
  ssize_t num_devices = 0;
  ssize_t i = 0;

  int r = libusb_init(NULL);
  if (r != LIBUSB_SUCCESS)
    return log_err_null(log, "Failed to initialize libusb: %d", r);

  num_devices = libusb_get_device_list(NULL, &dev_list);
  while (i < num_devices) {
    dev = dev_list[i];


    r = libusb_get_device_descriptor(dev, &desc);
    if (r != LIBUSB_SUCCESS)
      return log_err_null(log, "Failed to get the USB device descriptor: %d", r);

    if (desc.idVendor == USB_VID && desc.idProduct == USB_PID) break;

    i++;
  }

  if (i >= num_devices)
    return log_err_null(log, "Failed to find CC2531.");

  switch (libusb_open(dev, &dh)) {
  case LIBUSB_SUCCESS:
    break;
  case LIBUSB_ERROR_NO_MEM:
    return log_err_null(log, "No memeory to open USB device.");
  case LIBUSB_ERROR_ACCESS:
    return log_err_null(log, "Insufficient privileges to open USB device.");
  case LIBUSB_ERROR_NO_DEVICE:
    return log_err_null(log, "Device has been disconnected.");
  default:
    return log_err_null(log, "Error opening USB device: ", r);
  }
  switch (r)
  if (r != LIBUSB_SUCCESS)
    return log_err_null(log, "Failed to open USB device: %d", r);

  r = libusb_get_string_descriptor_ascii(dh, desc.iManufacturer, desc_manufacturer, 256);
  if (r < 0)
    return log_err_null(log, "Failed to get the manufacturer: %d", r);

  r = libusb_get_string_descriptor_ascii(dh, desc.iProduct, desc_product, 256);
  if (r < 0)
    return log_err_null(log, "Failed to get the product: %d", r);

  log_msg(log, LOG_LEVEL_INFO, "Using CC2531 on USB bus %03d device %03d. Manufacturer: \"%s\" Product: \"%s\" Serial: %04x", libusb_get_bus_number(dev),
          libusb_get_device_address(dev), desc_manufacturer, desc_product, desc.bcdDevice);

  /* Try and claim the device */
  r = libusb_claim_interface(dh, CC2531_INTERFACE);
  switch (r) {
  case LIBUSB_SUCCESS: break;
  case LIBUSB_ERROR_BUSY:
    return log_err_null(log, "The USB device is in use by another program or driver.");
  case LIBUSB_ERROR_NO_DEVICE:
    return log_err_null(log, "The USB device has been disconnected.");
  case LIBUSB_ERROR_NOT_FOUND:
    return log_err_null(log, "The USB device was not found.");
  default:
    return log_err_null(log, "Failed to claim USB device: %d", r);
  }

  c = malloc(sizeof(struct cc2531));
  c->log = log;
  c->dh = dh;
  c->device_id = desc.bcdDevice;
  try_null(_cc2531_set_config(c, 0));
  try_null(_cc2531_get_ctrl(c, 192, 256));

  return c;
}

int
cc2531_set_channel(struct cc2531 *cc2531, unsigned char channel) {
  log_msg(cc2531->log, LOG_LEVEL_INFO, "Setting CC2531 to channel %d", channel);

  try(_cc2531_set_config(cc2531, 1));
  try(_cc2531_set_ctrl(cc2531, 197, 0, 4));
  try(_cc2531_wait_for_198(cc2531));
  try(_cc2531_set_ctrl(cc2531, 201, 0, 0));

  cc2531->buf[0] = channel;
  try(_cc2531_set_ctrl(cc2531, 210, 1, 0));
  
  return 0;
}

int
_cc2531_read_frame(struct cc2531 *cc2531) {
  int transferred = 0;
  int r = 0;
  
  r = libusb_bulk_transfer(cc2531->dh, 0x83 , cc2531->buf, CC2531_BUFFER_SIZE, &transferred, 0);
  switch (r) {
  case LIBUSB_SUCCESS:
    log_msg(cc2531->log, LOG_LEVEL_DEBUG, "Bulk read %d bytes.", transferred);
    break;
  case LIBUSB_ERROR_TIMEOUT:
    return log_err(cc2531->log, "Timeout reading data from CC2531 dongle.");
  case LIBUSB_ERROR_PIPE:
    return log_err(cc2531->log, "CC2531 Endpoint halted.");
  case LIBUSB_ERROR_OVERFLOW:
    return log_err(cc2531->log, "CC2531 overflow.");
  case LIBUSB_ERROR_NO_DEVICE:
    return log_err(cc2531->log, "CC2531 has been disconnected.");
  default:
    return log_err(cc2531->log, "CC2531 bulk read failed: %d", r);
  }

  if (transferred > sizeof(struct _cc2531_usb_header)) return transferred; else return 0;
}

int
cc2531_start_capture(struct cc2531 *cc2531) {

  cc2531->buf[0] = 0;
  try(_cc2531_set_ctrl(cc2531, 210, 1, 1));
  try(_cc2531_set_ctrl(cc2531, 208, 0, 0));

  return 0;
}

int
cc2531_get_next_packet(struct cc2531 *cc2531, struct cc2531_frame *frame) {
  int len = 0;

  len = _cc2531_read_frame(cc2531);
  if (len < 0) return -1;

  struct _cc2531_usb_header *header = (struct _cc2531_usb_header *) cc2531->buf;
  /* Skip everything other than 0 frames */
  if (header->info != 0) return cc2531_get_next_packet(cc2531, frame);

  log_msg(cc2531->log, LOG_LEVEL_DEBUG, "Received frame with info %02x and length %d and timestamp %d", header->info, le16toh(header->length), le32toh(header->timestamp));

  frame->length = cc2531->buf[7];
  frame->data = &cc2531->buf[8];
  frame->rssi = frame->data[frame->length] - 73;
  frame->device_id = cc2531->device_id;

  return 0;
}

void
cc2531_free(struct cc2531 *cc2531) {
  libusb_release_interface(cc2531->dh, CC2531_INTERFACE);
  libusb_exit(NULL);
  free(cc2531);
}

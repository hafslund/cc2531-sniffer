#include <stdio.h>
#include <string.h>

#include "common.h"
#include "ieee802154.h"

static const char * const _frame_types[] = {"Beacon", "Data", "Ack"};
static const char * _unknown = "";
static const char * const _cmd_types[] = {"", "Association Request", "Association Response", 
					  "Disassociation Notification", "Data Request",
					  "PAN ID Conflict", "Orphan Notification", "Beacon Request", "Coordinator Realignment",
					  "GTS Request"};
static const char * _broadcast = "Broadcast";

#define ADDR_MODE_NONE 0
#define ADDR_MODE_SHORT 2
#define ADDR_MODE_EXT 3


uint8_t
ieee802154_fmt_address(char *buf, uint8_t addr_mode, uint8_t *data) {
  uint16_t addr;
  if (addr_mode == ADDR_MODE_SHORT) {
    addr = *((uint16_t *) &data[0]);
    if (addr == 0xffff) {
      strcpy(buf, _broadcast);
    } else {
      snprintf(buf, 16, "0x%04X", le16toh(addr));
    }
    return 2;
  } else {
    strncpy(buf, _unknown, 24);
    return 0;
  }
}

void
ieee802154_decode(uint8_t channel, uint8_t length, 
		       uint8_t *data, int8_t rssi, uint8_t device_id, 
		       struct ieee802154_packet *packet) {
  uint8_t frame_type;
  uint8_t src_addr_mode;
  uint8_t dst_addr_mode;

  packet->channel = channel;
  packet->device = device_id;
  packet->length = length;
  packet->lqi = rssi;
  packet->data = data;

  /* Get the frame description */
  frame_type = data[0] & 7;
  if (frame_type <= 2) {
    strncpy(packet->desc, _frame_types[frame_type], 256);
  } else {
    strncpy(packet->desc, _unknown, 256);
  }

  src_addr_mode = (data[1] >> 6) & 3;
  dst_addr_mode = (data[1] >> 2) & 3;

  data += 2;
  packet->seq = data++[0];

  /* PAN */
  if (src_addr_mode == ADDR_MODE_SHORT || src_addr_mode == ADDR_MODE_EXT) {
    data += ieee802154_fmt_address(packet->pan_addr, ADDR_MODE_SHORT, data);
  } else {
    data += ieee802154_fmt_address(packet->pan_addr, ADDR_MODE_NONE, NULL);
  }

  /* Destination address */
  data += ieee802154_fmt_address(packet->dst_addr, dst_addr_mode, data);
  
  /* Source address */
  data += ieee802154_fmt_address(packet->src_addr, src_addr_mode, data);

  /* Command */
  if (frame_type == 3 && (data[0] <= 9)) {
    strncpy(packet->desc, _cmd_types[data[0]], 256);
  }
}


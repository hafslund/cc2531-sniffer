#ifndef CC2531_H
#define CC2531_H

#define USB_VID 0x0451
#define USB_PID 0x16ae

struct cc2531;

struct cc2531 * cc2531_create(struct log *log);

struct cc2531_frame {
  unsigned char length;
  unsigned char *data;
  char rssi;  
  uint16_t device_id;
};

int cc2531_set_channel(struct cc2531 *cc2531, unsigned char channel);
int cc2531_start_capture(struct cc2531 *cc2531);
int cc2531_get_next_packet(struct cc2531 *cc2531, struct cc2531_frame *frame);
void cc2531_free(struct cc2531 *cc2531);

#endif /* CC2531_H */


#include <stdlib.h>
#include <time.h>
#include <endian.h>

struct zep;

struct zep_packet {
  unsigned char channel;
  unsigned short device;
  unsigned char length;
  unsigned char lqi;
  unsigned char *data;
};

int zep_send_packet(struct zep *zep, struct zep_packet *packet);

struct zep *zep_create(struct log *log, char *remote_address);
void zep_free(struct zep *zep);

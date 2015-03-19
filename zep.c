#include <endian.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "log.h"
#include "zep.h"

#define ZEP_BUFFER_SIZE 256

#define ZEP_PREAMBLE "EX"
#define ZEP_V2_TYPE_DATA 1
#define ZEP_V2_TYPE_ACK 2

#define ZEP_PORT 17754

const unsigned long long EPOCH = 2208988800ULL;

struct zep {
  struct log *log;
  unsigned char buf[ZEP_BUFFER_SIZE];
  uint32_t sequence;
  struct sockaddr_in socket_addr;
  int socket_fd;
};

struct __attribute__((__packed__)) _zep_header_data {
  char preamble[2];
  unsigned char version; 
  unsigned char type;
  unsigned char channel_id;
  uint16_t device_id;
  unsigned char lqi_mode;
  unsigned char lqi;
  int32_t timestamp_s;
  uint32_t timestamp_ns;  
  uint32_t sequence;
  unsigned char reserved[10];
  unsigned char length; 
};

int
zep_send_packet(struct zep *zep, struct zep_packet *packet) {
  struct timeval tv;
  int length = 0;
  
  log_msg(zep->log, LOG_LEVEL_DEBUG, "Sending ZEP packet for channel %d, device %04x, length %d", packet->channel, packet->device, packet->length);

  struct _zep_header_data *header = (struct _zep_header_data *) zep->buf; 
  strncpy(header->preamble, ZEP_PREAMBLE, 2);
  header->version = 2;
  header->type = ZEP_V2_TYPE_DATA;
  header->channel_id = packet->channel;
  header->device_id = htobe32(packet->device);
  header->lqi_mode = 0;
  header->lqi = packet->lqi;

  gettimeofday(&tv, NULL);
  header->timestamp_s = htobe32(tv.tv_sec + EPOCH);
  header->timestamp_ns = htobe32(tv.tv_usec * 1000);

  header->sequence = htobe32(zep->sequence++); 
  header->length = packet->length;
  
  memcpy(&zep->buf[sizeof(struct _zep_header_data)], packet->data, packet->length);
  length = sizeof(struct _zep_header_data) + packet->length;

  if (sendto(zep->socket_fd, zep->buf, length, 0, (struct sockaddr *) &zep->socket_addr, sizeof(zep->socket_addr)) == -1) {
    switch (errno) {
    case EACCES:
      return log_err(zep->log, "Access denied while trying to send to UDP socket");
    case EBADF:
      return log_err(zep->log, "Invalid descriptor for UDP socket");
    case EINTR:
      return log_err(zep->log, "Signal occured before transmitting data via UDP socket");
    case EINVAL:
      return log_err(zep->log, "Invalid argument");
    case EMSGSIZE:
      return log_err(zep->log, "UDP packet too large to send automatically.");
    case ENOMEM:
      return log_err(zep->log, "No memory to send UDP packet");
    case ENOTCONN:
      return log_err(zep->log, "UDP socket not connected");
    default:
      return log_err(zep->log, "Error sending UDP packet: %d", errno);
    }
  }
  
  return 0;
}

struct zep *
zep_create(struct log *log, char *remote_address) {
  struct zep *zep = NULL;
  int s, socket_fd;

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    return log_err_null(log, "Unable to open UDP socket.");

  zep = malloc(sizeof(struct zep));
  zep->log = log;
  zep->sequence = 0;
  zep->socket_fd = socket_fd;

  memset((char *) &zep->socket_addr, 0, sizeof(zep->socket_addr));
  zep->socket_addr.sin_family = AF_INET;
  zep->socket_addr.sin_port = htons(ZEP_PORT);
  if (inet_aton(remote_address, &zep->socket_addr.sin_addr) == 0) {
    free(zep);
    return log_err_null(log, "inet_aton() failed\n");
  }
  
  return zep;
}

void
zep_free(struct zep *zep) {
  close(zep->socket_fd);
  free(zep);
}

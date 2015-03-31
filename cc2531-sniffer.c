#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "cc2531.h"
#include "zep.h"

int 
sniff(struct log *log, unsigned char channel, char *remote_address)
{
  struct cc2531 *cc2531;
  struct zep *zep;

  struct cc2531_frame frame;
  struct zep_packet packet;
  int r;
  
  log_msg(log, LOG_LEVEL_DEBUG, "cc2531-sniffer initializing.");

  cc2531 = cc2531_create(log);
  if (cc2531 == NULL) return -1;
  if (cc2531_set_channel(cc2531, channel) < 0) return -1;
  if (cc2531_start_capture(cc2531) < 0) return -1;

  zep = zep_create(log, remote_address);

  while (1) {
    r = cc2531_get_next_packet(cc2531, &frame);
    if (r < 0) return -1;

    packet.channel = channel;
    packet.device = frame.device_id;
    packet.length = frame.length;
    packet.lqi = frame.rssi;
    packet.data = frame.data;

    r = zep_send_packet(zep, &packet);
    if (r < 0) {
      /* If sending fails, then re-open and move on */
      zep_free(zep);
      zep = zep_create(log, remote_address);
    }
  }
  
  /* Clean up */
  zep_free(zep);
  cc2531_free(cc2531);
  log_free(log);
 
  return 0;
}

void 
print_usage()
{
  printf("Usage: cc22531-sniffer -c CHANNEL -r REMOTE\n");
}


int main(int argc, char* argv[])
{
  int c;
  unsigned char channel = 0;
  char remote_address[16];

  struct log *log = log_stdio_create();
  memset(remote_address, 0, 16);
  
  while (1) {
    int cur_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"channel", required_argument, 0, 'c' },
      {"remote",  required_argument, 0, 'r' },
      {0,         0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "c:r:", long_options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 'c':
      channel = atoi(optarg);
      if (channel < 11 || channel > 26)
	return log_err(log, "Channel out of range. Must be from 11 to 26.");
      break;
    case 'r':
      strncpy(remote_address, optarg, 15);
      break;
    default:
      print_usage();
      return(EXIT_FAILURE);
    }
  }

  if (channel == 0 || remote_address == NULL) {
    print_usage();
    return(EXIT_FAILURE);
  }

  sniff(log, channel, remote_address);
}

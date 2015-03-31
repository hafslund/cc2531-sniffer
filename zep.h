#ifndef ZEP_H
#define ZEP_H

#include "ieee802154.h"

struct zep;

int zep_send_packet(struct zep *zep, struct ieee802154_packet *packet);

struct zep *zep_create(struct log *log, char *remote_address);
void zep_free(struct zep *zep);

#endif /* ZEP_H */

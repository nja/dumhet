#ifndef _dhtclient_h
#define _dhtclient_h

#include <dht.h>

typedef struct DhtClient {
  DhtNode node;
  DhtTable *table;
  int socket;
} DhtClient;

DhtClient *DhtClient_Create(DhtHash id, uint32_t addr, uint16_t port);
void DhtClient_Destroy(DhtClient *client);

#endif

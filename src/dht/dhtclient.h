#ifndef _dhtclient_h
#define _dhtclient_h

#include <dht/message.h>
#include <dht/table.h>
#include <dht/pendingresponses.h>

typedef struct DhtClient {
    DhtNode node;
    DhtTable *table;
    int socket;
    HashmapPendingResponses *pending;
    char *buf;
    int next_t;
} DhtClient;

DhtClient *DhtClient_Create(DhtHash id, uint32_t addr, uint16_t port);
void DhtClient_Destroy(DhtClient *client);

Message *Ping_Create(DhtClient *client);

#endif

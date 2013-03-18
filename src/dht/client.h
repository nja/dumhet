#ifndef _dhtclient_h
#define _dhtclient_h

#include <dht/message.h>
#include <dht/table.h>
#include <dht/pendingresponses.h>

#define SECRETS_LEN 2

typedef DhtHash Token;

typedef struct DhtClient {
    DhtNode node;
    DhtTable *table;
    int socket;
    HashmapPendingResponses *pending;
    char *buf;
    int next_t;
    DhtHash secrets[SECRETS_LEN];
} DhtClient;

DhtClient *DhtClient_Create(DhtHash id, uint32_t addr, uint16_t port);
void DhtClient_Destroy(DhtClient *client);

Message *Ping_Create(DhtClient *client);

Token DhtClient_MakeToken(DhtClient *client, DhtNode *from);
int DhtClient_IsValidToken(DhtClient *client, DhtNode *from,
                           char *token, size_t token_len);
int DhtClient_NewSecret(DhtClient *client);

#endif

#ifndef _dhtclient_h
#define _dhtclient_h

#include <dht/table.h>
#include <dht/protocol.h>
#include <lcthw/hashmap.h>

#define SECRETS_LEN 2

typedef DhtHash Token;

typedef struct DhtClient {
    DhtNode node;
    DhtTable *table;
    int socket;
    uint16_t peer_port;
    struct PendingResponses *pending;
    char *buf;
    int next_t;
    DhtHash secrets[SECRETS_LEN];
    Hashmap *peers;
} DhtClient;

DhtClient *DhtClient_Create(DhtHash id,
                            uint32_t addr,
                            uint16_t port,
                            uint16_t peer_port);
void DhtClient_Destroy(DhtClient *client);

Token DhtClient_MakeToken(DhtClient *client, DhtNode *from);
int DhtClient_IsValidToken(DhtClient *client, DhtNode *from,
                           char *token, size_t token_len);
int DhtClient_NewSecret(DhtClient *client);

int DhtClient_GetPeers(DhtClient *client, DhtHash *id, DArray **peers);
int DhtClient_AddPeer(DhtClient *client, Peer *peer);

#endif

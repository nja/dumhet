#ifndef _client_h
#define _client_h

#include <dht/table.h>
#include <dht/protocol.h>
#include <lcthw/hashmap.h>

#define SECRETS_LEN 2

typedef Hash Token;

typedef struct Client {
    Node node;
    DhtTable *table;
    int socket;
    uint16_t peer_port;
    struct PendingResponses *pending;
    char *buf;
    int next_t;
    Hash secrets[SECRETS_LEN];
    Hashmap *peers;
} Client;

Client *Client_Create(Hash id,
                      uint32_t addr,
                      uint16_t port,
                      uint16_t peer_port);
void Client_Destroy(Client *client);

Token Client_MakeToken(Client *client, Node *from);
int Client_IsValidToken(Client *client,
                        Node *from,
                        char *token,
                        size_t token_len);
int Client_NewSecret(Client *client);

int Client_GetPeers(Client *client, Hash *info_hash, DArray **peers);
int Client_AddPeer(Client *client, Hash *info_hash, Peer *peer);

#endif

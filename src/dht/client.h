#ifndef _dht_client_h
#define _dht_client_h

#include <dht/messagequeue.h>
#include <dht/table.h>
#include <dht/protocol.h>
#include <lcthw/hashmap.h>

/* Past secrets are kept to give a grace period for slow announcers */
#define SECRETS_LEN 2

/* Our own tokens have a known fixed length. See also FToken. */
typedef Hash Token;

typedef struct Client {
    Node node;                  /* Client's own Node */
    Table *table;               /* DHT routing table of Nodes */
    int socket;
    uint16_t peer_port;         /* Port of the Client's Node's Peer */
    /* tid and message type of queries for which we're expecting replies */
    struct PendingResponses *pending;
    char *buf;                  /* Used for sending and receiving */
    int next_t;                 /* Next transaction id */
    Hash secrets[SECRETS_LEN];  /* Current and past secrets */
    Hashmap *peers;             /* All the Peers announced to us, by info_hash */
    MessageQueue *incoming;
    MessageQueue *queries;
    MessageQueue *replies;
    DArray *searches;
} Client;

Client *Client_Create(Hash id,
                      uint32_t addr,
                      uint16_t port,
                      uint16_t peer_port);
void Client_Destroy(Client *client);

/* Make the Token required for a valid announce by the from Node */
Token Client_MakeToken(Client *client, Node *from);
/* Validate the Node's token against the Client's secrets */
int Client_IsValidToken(Client *client,
                        Node *from,
                        char *token,
                        size_t token_len);
/* Create a new secret and shift the past ones */
int Client_NewSecret(Client *client);

/* Sets *peers to a new DArray of all Peers announced to client for info_hash.
 * Returns 0 on success, -1 on failure. */
int Client_GetPeers(Client *client, Hash *info_hash, DArray **peers);
/* Adds peer as announced on info_hash to client or updates the entry time
 * when already present.
 * Returns 0 on success, -1 on failure. */
int Client_AddPeer(Client *client, Hash *info_hash, Peer *peer);

/* Notes an invalid message from the node. For blacklisting. */
int Client_MarkInvalidMessage(Client *client, Node *from);

#endif

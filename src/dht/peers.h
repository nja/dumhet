#ifndef _peers_h
#define _peers_h

#include <dht/hash.h>
#include <dht/message.h>
#include <lcthw/darray.h>
#include <lcthw/hashmap.h>

#define MAXPEERS 0x1F00

typedef struct Peers {
    DhtHash info_hash;
    Hashmap *hashmap;
    int count;
} Peers;

Peers *Peers_Create(DhtHash *info_hash);
void Peers_Destroy(Peers *peers);

int Peers_GetPeers(Peers *peers, DArray *result);
int Peers_AddPeer(Peers *peers, Peer *peer);

#endif

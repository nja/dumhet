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

Hashmap *PeersHashmap_Create();
void PeersHashmap_Destroy(Hashmap *hashmap);

int PeersHashmap_GetPeers(Hashmap *hashmap, DhtHash *info_hash, DArray *result);
int PeersHashmap_AddPeer(Hashmap *hashmap, DhtHash *info_hash, Peer *peer);

#endif

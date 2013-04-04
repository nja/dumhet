#ifndef _peers_h
#define _peers_h

#include <dht/hash.h>
#include <dht/message.h>
#include <lcthw/darray.h>
#include <lcthw/hashmap.h>

#define MAXPEERS 0x1F00

typedef struct Peers {
    Hash info_hash;
    Hashmap *hashmap;
    int count;
} Peers;

Peers *Peers_Create(Hash *info_hash);
void Peers_Destroy(Peers *peers);

int Peers_GetPeers(Peers *peers, DArray *result);
int Peers_AddPeer(Peers *peers, Peer *peer);

Hashmap *PeersHashmap_Create();
void PeersHashmap_Destroy(Hashmap *hashmap);

int PeersHashmap_GetPeers(Hashmap *hashmap, Hash *info_hash, DArray *result);
int PeersHashmap_AddPeer(Hashmap *hashmap, Hash *info_hash, Peer *peer);

#endif

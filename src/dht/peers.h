#ifndef _dht_peers_h
#define _dht_peers_h

#include <time.h>

#include <dht/hash.h>
#include <lcthw/darray.h>
#include <lcthw/hashmap.h>

#define MAXPEERS 0x1F00

typedef struct Peer {
    uint32_t addr;
    uint16_t port;
} Peer;

typedef struct Peers {
    Hash info_hash;
    Hashmap *hashmap;
    int count;
    time_t (*GetTime)();
} Peers;

Peers *Peers_Create(Hash *info_hash);
void Peers_Destroy(Peers *peers);

int Peers_GetPeers(Peers *peers, DArray *result);
int Peers_AddPeer(Peers *peers, Peer *peer);

int Peers_Clean(Peers *peers, time_t cutoff);

Hashmap *PeersHashmap_Create();
void PeersHashmap_Destroy(Hashmap *hashmap);

int PeersHashmap_GetPeers(Hashmap *hashmap, Hash *info_hash, DArray *result);
int PeersHashmap_AddPeer(Hashmap *hashmap, Hash *info_hash, Peer *peer);

#endif

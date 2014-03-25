#ifndef _dht_peers_h
#define _dht_peers_h

#include <time.h>

#include <dht/dht.h>
#include <dht/hash.h>
#include <lcthw/darray.h>
#include <lcthw/hashmap.h>

#define MAXPEERS 0x1F00

/* Stores announced peers for a single info_hash.  */
typedef struct Peers {
    Hash info_hash;
    Hashmap *hashmap;
    int count;
    time_t (*GetTime)();
} Peers;

Peers *Peers_Create(Hash *info_hash);
void Peers_Destroy(Peers *peers);

/* Pushes all peers on the result array. */
int Peers_GetPeers(Peers *peers, DArray *result);
/* Add the peer or update the time if already present. */
int Peers_AddPeer(Peers *peers, Peer *peer);

/* Clean out all peers added or updated before the cutoff time. */
int Peers_Clean(Peers *peers, time_t cutoff);

Hashmap *PeersHashmap_Create();
void PeersHashmap_Destroy(Hashmap *hashmap);

/* From a hashmap of Peers structs by info_hash, push all peers for the
 * given info_hash on the result array. */
int PeersHashmap_GetPeers(Hashmap *hashmap, Hash *info_hash, DArray *result);
/* To a hashmap of Peers structs by info_hash, add the peer to the
 * Peers of the info_hash, creating the Peers struct when needed. */
int PeersHashmap_AddPeer(Hashmap *hashmap, Hash *info_hash, Peer *peer);

#endif

#include <string.h>
#include <time.h>

#include <dht/hash.h>
#include <dht/peers.h>
#include <lcthw/dbg.h>

struct PeerEntry {
    Peer peer;
    time_t time;
};

int Peer_Compare(struct PeerEntry *a, struct PeerEntry *b)
{
    assert(a != NULL && "NULL Peer pointer");
    assert(b != NULL && "NULL Peer pointer");

    return memcmp(&a->peer, &b->peer, sizeof(Peer));
}

uint32_t Peer_Hash(void *key)
{
    assert(key != NULL && "NULL Peer pointer");

    return ((Peer *)key)->addr;
}

time_t GetTime()
{
    return time(NULL);
}

Peers *Peers_Create(Hash *info_hash)
{
    assert(info_hash != NULL && "NULL Hash pointer");

    Peers *peers = malloc(sizeof(Peers));
    check_mem(peers);

    peers->count = 0;

    peers->hashmap = Hashmap_create((Hashmap_compare)Peer_Compare, Peer_Hash);
    check(peers->hashmap != NULL, "Hashmap_create failed");

    peers->info_hash = *info_hash;
    peers->GetTime = GetTime;

    return peers;
error:
    free(peers);
    return NULL;
}

void Peers_Destroy(Peers *peers)
{
    if (peers == NULL)
        return;

    Hashmap_traverse(peers->hashmap, NULL, Hashmap_freeNodeData);
    Hashmap_destroy(peers->hashmap);
    free(peers);
}

int Peers_AddPeer(Peers *peers, Peer *peer)
{
    assert(peers != NULL && "NULL Peers pointer");
    assert(peer != NULL && "NULL Peer pointer");
    assert(0 <= peers->count && peers->count <= MAXPEERS && "Bad peers count");

    if (peers->count == MAXPEERS)
        return -1;

    struct PeerEntry *entry = Hashmap_delete(peers->hashmap, peer);

    if (entry == NULL)
    {
        entry = malloc(sizeof(struct PeerEntry));
        check_mem(entry);
        entry->peer = *peer;
    }
    else
    {
        peers->count--;
    }

    entry->time = peers->GetTime();

    int rc = Hashmap_set(peers->hashmap, &entry->peer, entry);
    check(rc == 0, "Hashmap_set failed");

    peers->count++;

    return 0;
error:
    free(entry);
    return -1;
}

int TraverseAddPeer(DArray *array, HashmapNode *node)
{
    assert(array != NULL && "NULL DArray pointer");
    assert(node != NULL && "NULL HashmapNode pointer");

    int rc = DArray_push(array, node->data);
    check(rc == 0, "DArray_push failed");

    return 0;
error:
    return -1;
}

int Peers_GetPeers(Peers *peers, DArray *result)
{
    assert(peers != NULL && "NULL Peers pointer");
    assert(result != NULL && "NULL DArray pointer");

    int rc = Hashmap_traverse(peers->hashmap,
                              result,
                              (Hashmap_traverse_cb)TraverseAddPeer);
    check(rc == 0, "Hashmap_traverse failed");

    return 0;
error:
    return -1;
}

int FreePeersHashmapEntry(void *context, HashmapNode *node)
{
    (void)(context);
    Peers_Destroy(node->data);

    return 0;
}

Hashmap *PeersHashmap_Create()
{
    Hashmap *hashmap = Hashmap_create(
        (Hashmap_compare)Distance_Compare,
        (Hashmap_hash)Hash_Hash);
    check_mem(hashmap);

    return hashmap;
error:
    return NULL;
}

void PeersHashmap_Destroy(Hashmap *hashmap)
{
    if (hashmap == NULL)
        return;

    Hashmap_traverse(hashmap, NULL, FreePeersHashmapEntry);
    Hashmap_destroy(hashmap);
}

Peers *PeersHashmap_GetSetPeers(Hashmap *hashmap, Hash *info_hash)
{
    assert(hashmap != NULL && "NULL Hashmap pointer");
    assert(info_hash != NULL && "NULL Hash pointer");

    Peers *peers = Hashmap_get(hashmap, info_hash);
    Peers *new_peers = NULL;

    if (peers == NULL)
    {
        peers = new_peers = Peers_Create(info_hash);
        check(peers != NULL, "Peers_Create failed");

        int rc = Hashmap_set(hashmap, info_hash, peers);
        check(rc == 0, "Hashmap_set failed");
    }

    return peers;
error:
    Peers_Destroy(new_peers);
    return NULL;
}

int PeersHashmap_GetPeers(Hashmap *hashmap, Hash *info_hash, DArray *result)
{
    assert(hashmap != NULL && "NULL Hashmap pointer");
    assert(info_hash != NULL && "NULL Hash pointer");
    assert(result != NULL && "NULL DArray pointer");

    Peers *peers = PeersHashmap_GetSetPeers(hashmap, info_hash);

    int rc = Peers_GetPeers(peers, result);
    check(rc == 0, "Peers_GetPeers failed");

    return 0;
error:
    return -1;
}

int PeersHashmap_AddPeer(Hashmap *hashmap, Hash *info_hash, Peer *peer)
{
    assert(hashmap != NULL && "NULL Hashmap pointer");
    assert(info_hash != NULL && "NULL Hash pointer");
    assert(peer != NULL && "NULL Peer pointer");

    Peers *peers = PeersHashmap_GetSetPeers(hashmap, info_hash);

    int rc = Peers_AddPeer(peers, peer);
    check(rc == 0, "Peers_AddPeer failed");

    return 0;
error:
    return -1;
}

int Peers_Clean(Peers *peers, time_t cutoff)
{
    assert(peers != NULL && "NULL Peers pointer");
    assert(0 <= peers->count && peers->count <= MAXPEERS && "Bad Peers count");

    DArray *tmp = DArray_create(sizeof(struct PeerEntry *), 128);
    int rc = Peers_GetPeers(peers, tmp);
    check(rc == 0, "Peers_GetPeers failed");

    int i = 0;
    for (i = 0; i < DArray_count(tmp); i++)
    {
        struct PeerEntry *entry = DArray_get(tmp, i);
        check(entry != NULL, "NULL PeerEntry");

        if (entry->time < cutoff)
        {
            Hashmap_delete(peers->hashmap, &entry->peer);
            peers->count--;
            free(entry);
        }
    }

    assert(0 <= peers->count && peers->count <= MAXPEERS && "Bad Peers count");

    DArray_destroy(tmp);
    return 0;
error:
    DArray_destroy(tmp);
    return -1;
}

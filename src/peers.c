#include <string.h>

#include <dht/hash.h>
#include <dht/peers.h>
#include <lcthw/dbg.h>

int Peer_Compare(Peer *a, Peer *b)
{
    assert(a != NULL && "NULL Peer pointer");
    assert(b != NULL && "NULL Peer pointer");

    return memcmp(a, b, sizeof(Peer));
}

uint32_t Peer_Hash(void *key)
{
    assert(key != NULL && "NULL Peer pointer");

    return ((Peer *)key)->addr;
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

Peer *Peer_Copy(Peer *peer)
{
    assert(peer != NULL && "NULL Peer pointer");

    Peer *copy = malloc(sizeof(Peer));
    check_mem(copy);

    *copy = *peer;

    return copy;
error:
    return NULL;
}

int Peers_AddPeer(Peers *peers, Peer *peer)
{
    assert(peers != NULL && "NULL Peers pointer");
    assert(peer != NULL && "NULL Peer pointer");
    assert(0 <= peers->count && peers->count <= MAXPEERS && "Bad peers count");

    if (peers->count == MAXPEERS)
        return -1;

    Peer *to_add = Hashmap_delete(peers->hashmap, peer);

    if (to_add == NULL)
    {
        to_add = Peer_Copy(peer);
        check(to_add != NULL, "Peer_copy failed");
    }
    else
    {
        peers->count--;
    }

    int rc = Hashmap_set(peers->hashmap, to_add, to_add);
    check(rc == 0, "Hashmap_set failed");

    peers->count++;

    return 0;
error:
    free(to_add);
    return -1;
}

int TraverseAddPeer(void *context, HashmapNode *node)
{
    assert(context != NULL && "NULL Peers pointer");
    assert(node != NULL && "NULL HashmapNode pointer");

    int rc = DArray_push((DArray *)context, node->data);
    check(rc == 0, "DArray_push failed");

    return 0;
error:
    return -1;
}

int Peers_GetPeers(Peers *peers, DArray *result)
{
    assert(peers != NULL && "NULL Peers pointer");
    assert(result != NULL && "NULL DArray pointer");

    int rc = Hashmap_traverse(peers->hashmap, result, TraverseAddPeer);
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
        (Hashmap_compare)DhtDistance_Compare,
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


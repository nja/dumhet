#include <dht/peers.h>
#include <lcthw/dbg.h>

int Peer_Compare(void *a, void *b)
{
    assert(a != NULL && "NULL Peer pointer");
    assert(b != NULL && "NULL Peer pointer");

    if (((Peer *)a)->addr < ((Peer *)b)->addr)
        return -1;
    else if (((Peer *)b)->addr < ((Peer *)a)->addr)
        return 1;
    else if (((Peer *)a)->port < ((Peer *)b)->port)
        return -1;
    else if (((Peer *)b)->port < ((Peer *)a)->port)
        return 1;
    else
        return 0;
}

uint32_t Peer_Hash(void *key)
{
    assert(key != NULL && "NULL Peer pointer");

    return ((Peer *)key)->addr;
}

Peers *Peers_Create(DhtHash *info_hash)
{
    assert(info_hash != NULL && "NULL DhtHash pointer");

    Peers *peers = malloc(sizeof(Peers));
    check_mem(peers);

    peers->count = 0;

    peers->hashmap = Hashmap_create(Peer_Compare, Peer_Hash);
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

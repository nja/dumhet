#include "minunit.h"
#include <dht/client.h>
#include <dht/message_create.h>
#include <dht/network.h>
#include <dht/peers.h>

char *test_Peers_CreateDestroy()
{
    Hash info_hash = { "info_hash" };
    Peers *peers = Peers_Create(&info_hash);
    mu_assert(peers != NULL, "Peers_Create failed");

    int i = 0;
    for (i = 0; i < 32; i++)
    {
        Peer peer = { .addr = i, .port = ~i };
        int rc = Peers_AddPeer(peers, &peer);
        mu_assert(rc == 0, "Peers_AddPeer failed");
    }

    Peers_Destroy(peers);

    return NULL;
}

char *test_MaxPeersInRGetPeersEncoded()
{
    Hash id = { "id" };
    Client *client = Client_Create(id, 0, 0, 0);

    Message *query = Message_CreateQGetPeers(client, &id);
    Token token = {{ 0 }};

    char buffer[UDPBUFLEN];

    DArray *peers = DArray_create(sizeof(Peer *), MAXPEERS + 1);

    while (DArray_count(peers) < MAXPEERS)
    {
        Peer *peer = malloc(sizeof(Peer));
        peer->addr = DArray_count(peers);
        peer->port = ~DArray_count(peers);
        DArray_push(peers, peer);
    }

    Message *response = Message_CreateRGetPeers(client,
                                                query,
                                                peers,
                                                NULL,
                                                &token);

    debug("Encoding %d peers in RGetPeers", DArray_count(peers));
    int rc = Message_Encode(response, buffer, UDPBUFLEN);
    mu_assert(rc > 0, "Message_Encode failed");

    while (DArray_count(peers) > 0)
        free(DArray_pop(peers));

    DArray_destroy(peers);

    Message_Destroy(response);

    Client_Destroy(client);
    Message_Destroy(query);

    return NULL;
}

char *test_Peers_RepeatAdd()
{
    Hash info_hash = { "info_hash" };
    Peers *peers = Peers_Create(&info_hash);

    Peer a = { 0 };
    Peer b = { 0 };
    b.addr = 1;

    int rc = Peers_AddPeer(peers, &a);
    mu_assert(rc == 0, "Peers_AddPeer failed");
    rc = Peers_AddPeer(peers, &b);
    mu_assert(rc == 0, "Peers_AddPeer failed");

    mu_assert(peers->count == 2, "Wrong count");

    rc = Peers_AddPeer(peers, &a);
    mu_assert(rc == 0, "Repeated Peers_AddPeer failed");
    rc = Peers_AddPeer(peers, &b);
    mu_assert(rc == 0, "Repeated Peers_AddPeer failed");

    mu_assert(peers->count == 2, "Wrong count");

    Peers_Destroy(peers);

    return NULL;
}

char *test_Peers_GetPeers()
{
    Hash info_hash = { "info_hash" };
    Peers *peers = Peers_Create(&info_hash);

    while (peers->count < MAXPEERS)
    {
        Peer peer = { 0 };
        peer.addr = peers->count;
        Peers_AddPeer(peers, &peer);
    }

    DArray *result = DArray_create(sizeof(Peer *), MAXPEERS + 1);

    int rc = Peers_GetPeers(peers, result);
    mu_assert(rc == 0, "Peers_GetPeers failed");
    mu_assert(DArray_count(result) == MAXPEERS, "Wrong result count");

    char *seen = calloc(1, MAXPEERS);

    while (DArray_count(result) > 0)
    {
        Peer *peer = DArray_pop(result);
        mu_assert(seen[peer->addr] == 0, "Already seen peer");
        seen[peer->addr] = 1;
    }

    DArray_destroy(result);
    Peers_Destroy(peers);
    free(seen);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Peers_CreateDestroy);
    mu_run_test(test_MaxPeersInRGetPeersEncoded);
    mu_run_test(test_Peers_RepeatAdd);
    mu_run_test(test_Peers_GetPeers);

    return NULL;
}

RUN_TESTS(all_tests);

#include "minunit.h"
#include <dht/client.h>
#include <dht/handle.h>
#include <dht/message.h>
#include <dht/message_create.h>

int SameT(Message *a, Message *b)
{
    if (a->t_len != b->t_len)
        return 0;

    unsigned int i = 0;
    for (i = 0; i < a->t_len; i++)
    {
        if (a->t[i] != b->t[i])
            return 0;
    }

    return 1;
}

int HasRecentQuery(DhtClient *client, DhtHash id)
{
    DhtNode *node = DhtTable_FindNode(client->table, &id);
    check(node != NULL, "No node in client table");
    check(node->query_time > 0, "query_time not set");

    return 1;
error:
    return 0;
}

char *test_HandleQPing()
{
    DhtHash id = { "client id" }, from_id = { "from id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 1, 1);

    DhtTable_InsertNode(client->table, &from->node);

    Message *qping = Message_CreateQPing(from);

    Message *reply = HandleQPing(client, qping);

    mu_assert(reply != NULL, "HandleQPing failed");
    mu_assert(reply->type == RPing, "Wrong type");
    mu_assert(SameT(qping, reply), "Wrong t");
    mu_assert(HasRecentQuery(client, from_id), "Node query_time not set");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(qping);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQGetPeers_nodes()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash target_id = { "target id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 1, 1);

    DhtTable_InsertNode(client->table, &from->node);

    Message *qgetpeers = Message_CreateQGetPeers(from, &target_id);

    Message *reply = HandleQGetPeers(client, qgetpeers, &from->node);

    mu_assert(reply != NULL, "HandleQGetPeers failed");
    mu_assert(reply->type == RGetPeers, "Wrong type");
    mu_assert(SameT(qgetpeers, reply), "Wrong t");
    mu_assert(HasRecentQuery(client, from_id), "Node query_time not set");
    mu_assert(DhtClient_IsValidToken(client,
                                     &from->node,
                                     reply->data.rgetpeers.token,
                                     reply->data.rgetpeers.token_len),
              "Invalid token");
    mu_assert(reply->data.rgetpeers.nodes != NULL, "No nodes");
    mu_assert(reply->data.rgetpeers.count == 1, "Wrong count");
    mu_assert(reply->data.rgetpeers.values == NULL, "Unwanted peers");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(qgetpeers);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQGetPeers_peers()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash target_id = { "target id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 1, 1);
    Peer peer = { .addr = 2, .port = 3 };

    DhtTable_InsertNode(client->table, &from->node);
    DhtClient_AddPeer(client, &target_id, &peer);

    Message *qgetpeers = Message_CreateQGetPeers(from, &target_id);

    Message *reply = HandleQGetPeers(client, qgetpeers, &from->node);

    mu_assert(reply != NULL, "HandleQGetPeers failed");
    mu_assert(reply->type == RGetPeers, "Wrong type");
    mu_assert(SameT(qgetpeers, reply), "Wrong t");
    mu_assert(HasRecentQuery(client, from_id), "Node query_time not set");
    mu_assert(DhtClient_IsValidToken(client,
                                     &from->node,
                                     reply->data.rgetpeers.token,
                                     reply->data.rgetpeers.token_len),
              "Invalid token");
    mu_assert(reply->data.rgetpeers.values != NULL, "No peers");
    mu_assert(reply->data.rgetpeers.count == 1, "Wrong count");
    mu_assert(reply->data.rgetpeers.nodes == NULL, "Unwanted nodes");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(qgetpeers);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQAnnouncePeer()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash target_id = { "target id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 2, 3);

    DhtTable_InsertNode(client->table, &from->node);
    Token token = DhtClient_MakeToken(client, &from->node);

    Message *query = Message_CreateQAnnouncePeer(from, &target_id, &token);

    Message *reply = HandleQAnnouncePeer(client, query, &from->node);

    mu_assert(reply != NULL, "HandleQAnnouncePeer failed");
    mu_assert(reply->type == RAnnouncePeer, "Wrong type");
    mu_assert(SameT(query, reply), "Wrong t");
    mu_assert(HasRecentQuery(client, from_id), "Node query_time not set");

    DArray *peers = NULL;

    int rc = DhtClient_GetPeers(client, &target_id, &peers);
    mu_assert(rc == 0, "DhtClient_GetPeers failed");
    mu_assert(peers != NULL, "NULL DArray");
    mu_assert(DArray_count(peers) == 1, "Wrong peers count");

    Peer *peer = DArray_get(peers, 0);
    mu_assert(peer != NULL, "NULL Peer");
    mu_assert(peer->addr == from->node.addr.s_addr, "Wrong peer addr");
    mu_assert(peer->port == from->peer_port, "Wrong peer port");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);
    DArray_destroy(peers);

    return NULL;
}

char *test_HandleQAnnouncePeer_badtoken()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash target_id = { "target id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 2, 3);

    DhtTable_InsertNode(client->table, &from->node);
    Token token = { "bad token" };

    Message *query = Message_CreateQAnnouncePeer(from, &target_id, &token);

    Message *reply = HandleQAnnouncePeer(client, query, &from->node);

    mu_assert(reply != NULL, "HandleQAnnouncePeer failed");
    mu_assert(reply->type == RError, "Wrong type");
    mu_assert(reply->data.rerror.code = RERROR_PROTOCOL, "Wrong error code");
    mu_assert(SameT(query, reply), "Wrong t");
    mu_assert(HasRecentQuery(client, from_id), "Node query_time not set");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_HandleQPing);
    mu_run_test(test_HandleQGetPeers_nodes);
    mu_run_test(test_HandleQGetPeers_peers);
    mu_run_test(test_HandleQAnnouncePeer);
    mu_run_test(test_HandleQAnnouncePeer_badtoken);

    return NULL;
}

RUN_TESTS(all_tests);

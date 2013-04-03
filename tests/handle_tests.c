#include "minunit.h"
#include <dht/client.h>
#include <dht/handle.h>
#include <dht/message.h>
#include <dht/message_create.h>
#include <dht/search.h>

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

int HasRecentQuery(DhtTable *table, DhtHash id)
{
    DhtNode *node = DhtTable_FindNode(table, &id);
    check(node != NULL, "No node in client table");

    return node->query_time > 0;
error:
    return 0;
}

int HasRecentReply(DhtTable *table, DhtHash id)
{
    DhtNode *node = DhtTable_FindNode(table, &id);
    check(node != NULL, "No node in table");

    return node->reply_time > 0;
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
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

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
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");
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
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");
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
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

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
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQFindNode()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash target_id = { "target id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 2, 3);

    DhtTable_InsertNode(client->table, &from->node);

    Message *query = Message_CreateQFindNode(from, &target_id);

    Message *reply = HandleQFindNode(client, query);

    mu_assert(reply != NULL, "HandleQFindNode failed");
    mu_assert(reply->type == RFindNode, "Wrong type");
    mu_assert(reply->data.rfindnode.nodes != NULL, "No nodes");
    mu_assert(reply->data.rfindnode.count == 1, "Wrong count");
    mu_assert(SameT(query, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleRFindNode()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash target_id = { "target id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 2, 3);
    DhtNode found_node = { .id = { "found id" },
                           .addr = { .s_addr = 2345 },
                           .port = 2345};

    DArray *found = DArray_create(sizeof(DhtNode *), 2);
    DArray_push(found, &found_node);

    Search *search = Search_Create(&target_id);

    /* Insert to tables so replies can be marked by handle */
    DhtNode *client_from_node = DhtNode_Copy(&from->node);
    client_from_node->pending_queries = 1;
    DhtTable_InsertNode(client->table, client_from_node);
    DhtNode *search_from_node = DhtNode_Copy(&from->node);
    search_from_node->pending_queries = 1;
    DhtTable_InsertNode(search->table, search_from_node);

    Message *query = Message_CreateQFindNode(client, &target_id);

    Message *rfindnode = Message_CreateRFindNode(from, query, found);
    rfindnode->context = search; /* Would be set when decoding */

    int rc = HandleRFindNode(client, rfindnode);
    mu_assert(rc == 0, "HandleRFindNode failed");

    mu_assert(search->peers->count == 0, "Wrong peers count on search");

    mu_assert(HasRecentReply(client->table, from->node.id),
              "Reply not marked in client->table");
    mu_assert(HasRecentReply(search->table, from->node.id),
              "Reply not marked in search->table");

    DArray_destroy(found);
    Search_Destroy(search);
    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    DhtNode_Destroy(client_from_node);
    DhtNode_Destroy(search_from_node);
    Message_Destroy(query);
    Message_Destroy(rfindnode);

    return NULL;
}

char *test_HandleRPing()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 2, 3);

    DhtNode *from_node = DhtNode_Copy(&from->node);
    from_node->pending_queries = 1;
    DhtTable_InsertNode(client->table, from_node);

    Message *query = Message_CreateQPing(client);

    Message *reply = Message_CreateRPing(from, query);

    int rc = HandleReply(client, reply);

    mu_assert(rc == 0, "HandleReply failed");
    mu_assert(HasRecentReply(client->table, from_node->id), "Reply not marked");
    mu_assert(reply->type == RPing, "Wrong type");
    mu_assert(SameT(query, reply), "Wrong t");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    DhtNode_Destroy(from_node);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleRAnnouncePeer()
{
    DhtHash id = { "client id" };
    DhtHash from_id = { "from id" };
    DhtHash info_hash = { "info_hash" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 2, 3);

    DhtNode *from_node = DhtNode_Copy(&from->node);
    from_node->pending_queries = 1;
    DhtTable_InsertNode(client->table, from_node);

    Token token = DhtClient_MakeToken(client, &from->node);

    Message *query = Message_CreateQAnnouncePeer(client, &info_hash, &token);

    Message *reply = Message_CreateRAnnouncePeer(from, query);

    int rc = HandleReply(client, reply);

    mu_assert(rc == 0, "HandleReply failed");
    mu_assert(HasRecentReply(client->table, from_node->id), "Reply not marked");
    mu_assert(reply->type == RAnnouncePeer, "Wrong type");
    mu_assert(SameT(query, reply), "Wrong t");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    DhtNode_Destroy(from_node);
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
    mu_run_test(test_HandleQFindNode);

    mu_run_test(test_HandleRFindNode);
    mu_run_test(test_HandleRPing);

    return NULL;
}

RUN_TESTS(all_tests);

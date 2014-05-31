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

    return memcmp(a->t, b->t, a->t_len) == 0;
}

int HasRecentQuery(Table *table, Hash id)
{
    Node *node = Table_FindNode(table, &id);
    check(node != NULL, "No node in client table");

    return node->query_time > 0;
error:
    return 0;
}

int HasRecentReply(Table *table, Hash id)
{
    Node *node = Table_FindNode(table, &id);
    check(node != NULL, "No node in table");

    return node->reply_time > 0;
error:
    return 0;
}

char *test_HandleQPing()
{
    Hash id = { "client id" }, from_id = { "from id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 1, 1);

    Table_InsertNode(client->table, &from->node);

    Message *qping = Message_CreateQPing(from, &from->node);

    Message *reply = (GetQueryHandler(qping->type))(client, qping);

    mu_assert(reply != NULL, "HandleQPing failed");
    mu_assert(reply->type == RPing, "Wrong type");
    mu_assert(SameT(qping, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(qping);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQGetPeers_nodes()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 1, 1);

    Table_InsertNode(client->table, &from->node);

    Message *qgetpeers = Message_CreateQGetPeers(from, &from->node, &target_id);

    Message *reply = (GetQueryHandler(qgetpeers->type))(client, qgetpeers);

    mu_assert(reply != NULL, "HandleQGetPeers failed");
    mu_assert(reply->type == RGetPeers, "Wrong type");
    mu_assert(SameT(qgetpeers, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");
    mu_assert(Client_IsValidToken(client,
                                  &from->node,
                                  reply->data.rgetpeers.token.data,
                                  reply->data.rgetpeers.token.len),
              "Invalid token");
    mu_assert(reply->data.rgetpeers.nodes != NULL, "No nodes");
    mu_assert(reply->data.rgetpeers.count == 1, "Wrong count");
    mu_assert(reply->data.rgetpeers.values == NULL, "Unwanted peers");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(qgetpeers);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQGetPeers_peers()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 1, 1);
    Peer peer = { .addr = 23, .port = 32 };

    Table_InsertNode(client->table, &from->node);
    Client_AddPeer(client, &target_id, &peer);

    Message *qgetpeers = Message_CreateQGetPeers(from, &from->node, &target_id);

    Message *reply = (GetQueryHandler(qgetpeers->type))(client, qgetpeers);

    mu_assert(reply != NULL, "HandleQGetPeers failed");
    mu_assert(reply->type == RGetPeers, "Wrong type");
    mu_assert(SameT(qgetpeers, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");
    mu_assert(Client_IsValidToken(client,
                                  &from->node,
                                  reply->data.rgetpeers.token.data,
                                  reply->data.rgetpeers.token.len),
              "Invalid token");
    mu_assert(reply->data.rgetpeers.values != NULL, "No peers");
    mu_assert(reply->data.rgetpeers.count == 1, "Wrong count");
    mu_assert(reply->data.rgetpeers.nodes == NULL, "Unwanted nodes");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(qgetpeers);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQAnnouncePeer()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 2, 3);

    Table_InsertNode(client->table, &from->node);
    Token token = Client_MakeToken(client, &from->node);

    Message *query = Message_CreateQAnnouncePeer(from,
                                                 &from->node,
                                                 &target_id,
                                                 token.value,
                                                 HASH_BYTES);

    Message *reply = (GetQueryHandler(query->type))(client, query);

    mu_assert(reply != NULL, "HandleQAnnouncePeer failed");
    mu_assert(reply->type == RAnnouncePeer, "Wrong type");
    mu_assert(SameT(query, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

    DArray *peers = NULL;

    int rc = Client_GetPeers(client, &target_id, &peers);
    mu_assert(rc == 0, "Client_GetPeers failed");
    mu_assert(peers != NULL, "NULL DArray");
    mu_assert(DArray_count(peers) == 1, "Wrong peers count");

    Peer *peer = DArray_get(peers, 0);
    mu_assert(peer != NULL, "NULL Peer");
    mu_assert(peer->addr == from->node.addr.s_addr, "Wrong peer addr");
    mu_assert(peer->port == from->peer_port, "Wrong peer port");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);
    DArray_destroy(peers);

    return NULL;
}

char *test_HandleQAnnouncePeer_badtoken()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 2, 3);

    Table_InsertNode(client->table, &from->node);
    Token token = { "bad token" };

    Message *query = Message_CreateQAnnouncePeer(from,
                                                 &from->node,
                                                 &target_id,
                                                 token.value,
                                                 HASH_BYTES);

    Message *reply = (GetQueryHandler(query->type))(client, query);

    mu_assert(reply != NULL, "HandleQAnnouncePeer failed");
    mu_assert(reply->type == RError, "Wrong type");
    mu_assert(reply->data.rerror.code = RERROR_PROTOCOL, "Wrong error code");
    mu_assert(SameT(query, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleQFindNode()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 2, 3);

    Table_InsertNode(client->table, &from->node);

    Message *query = Message_CreateQFindNode(from, &from->node, &target_id);

    Message *reply = (GetQueryHandler(query->type))(client, query);

    mu_assert(reply != NULL, "HandleQFindNode failed");
    mu_assert(reply->type == RFindNode, "Wrong type");
    mu_assert(reply->data.rfindnode.nodes != NULL, "No nodes");
    mu_assert(reply->data.rfindnode.count == 1, "Wrong count");
    mu_assert(SameT(query, reply), "Wrong t");
    mu_assert(HasRecentQuery(client->table, from_id), "Node query_time not set");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleRFindNode()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Hash found_id = { "found_id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 2, 3);

    Node* found_node = Node_Create(&found_id);
    found_node->addr.s_addr = 2345;
    found_node->port = 2345;

    debug("found_node %p", found_node);

    DArray *found = DArray_create(sizeof(Node *), 2);
    DArray_push(found, found_node);

    Search *search = Search_Create(&target_id);

    /* Insert to tables so replies can be marked by handle */
    Node *client_from_node = Node_Copy(&from->node);
    client_from_node->pending_queries = 1;
    Table_InsertNode(client->table, client_from_node);
    Node *search_from_node = Node_Copy(&from->node);
    search_from_node->pending_queries = 1;
    Table_InsertNode(search->table, search_from_node);

    Message *query = Message_CreateQFindNode(client, &from->node, &target_id);

    Message *rfindnode = Message_CreateRFindNode(from, query, found);
    rfindnode->context = search; /* Would be set when decoding */

    int rc = (GetReplyHandler(rfindnode->type))(client, rfindnode);
    mu_assert(rc == 0, "HandleRFindNode failed");

    mu_assert(search->peers->count == 0, "Wrong peers count on search");

    mu_assert(HasRecentReply(client->table, from->node.id),
              "Reply not marked in client->table");
    mu_assert(HasRecentReply(search->table, from->node.id),
              "Reply not marked in search->table");
    DArray_destroy(found);
    Search_Destroy(search);
    Client_Destroy(client);
    Client_Destroy(from);
    Node_Destroy(client_from_node);
    Message_Destroy(query);
    Message_Destroy(rfindnode);

    return NULL;
}

char *test_HandleRPing()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 2, 3);

    Node *from_node = Node_Copy(&from->node);
    from_node->pending_queries = 1;
    Table_InsertNode(client->table, from_node);

    Message *query = Message_CreateQPing(client, &from->node);

    Message *reply = Message_CreateRPing(from, query);

    int rc = (GetReplyHandler(reply->type))(client, reply);

    mu_assert(rc == 0, "HandleReply failed");
    mu_assert(HasRecentReply(client->table, from_node->id), "Reply not marked");
    mu_assert(reply->type == RPing, "Wrong type");
    mu_assert(SameT(query, reply), "Wrong t");

    Client_Destroy(client);
    Client_Destroy(from);
    Node_Destroy(from_node);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleRAnnouncePeer()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash info_hash = { "info_hash" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 2, 3);

    Node *from_node = Node_Copy(&from->node);
    from_node->pending_queries = 1;
    Table_InsertNode(client->table, from_node);

    Token token = Client_MakeToken(client, &from->node);

    Message *query = Message_CreateQAnnouncePeer(client,
                                                 &from->node,
                                                 &info_hash,
                                                 token.value,
                                                 HASH_BYTES);

    Message *reply = Message_CreateRAnnouncePeer(from, query);

    int rc = (GetReplyHandler(reply->type))(client, reply);

    mu_assert(rc == 0, "HandleReply failed");
    mu_assert(HasRecentReply(client->table, from_node->id), "Reply not marked");
    mu_assert(reply->type == RAnnouncePeer, "Wrong type");
    mu_assert(SameT(query, reply), "Wrong t");

    Client_Destroy(client);
    Client_Destroy(from);
    Node_Destroy(from_node);
    Message_Destroy(query);
    Message_Destroy(reply);

    return NULL;
}

char *test_HandleRGetPeers_nodes()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 1, 1);
    const int nodes_count = 3;
    Node *found_nodes[nodes_count];

    from->node.pending_queries = 1;
    Table_InsertNode(client->table, &from->node);

    int i = 0;
    for (i = 0; i < nodes_count; i++)
    {
        Hash id = { "  found node id" };
        id.value[0] = '0' + i;
        found_nodes[i] = Node_Create(&id);
        Table_InsertNode(from->table, found_nodes[i]);
    }

    Message *qgetpeers = Message_CreateQGetPeers(client,
                                                 &client->node,
                                                 &target_id);

    Message *rgetpeers = HandleQGetPeers(from, qgetpeers);

    Search *search = Search_Create(&target_id);
    rgetpeers->context = search;

    /* rgetpeers is addressed to client, now change it as if it had
     * arrived from from */
    rgetpeers->node = from->node;
    rgetpeers->node.reply_time = time(NULL);
    int rc = (GetReplyHandler(rgetpeers->type))(client, rgetpeers);

    mu_assert(rc == 0, "HandleRGetPeers failed");

    mu_assert(search->table->buckets[0]->count == nodes_count + 1, /* +1 for from node */
              "Found nodes missing.");
    mu_assert(search->peers->count == 0, "No peers expected");

    Client_Destroy(client);
    Client_Destroy(from);

    Message_Destroy(qgetpeers);
    Message_Destroy(rgetpeers);
    Search_Destroy(search);

    return NULL;
}

char *test_HandleRGetPeers_peers()
{
    Hash id = { "client id" };
    Hash from_id = { "from id" };
    Hash target_id = { "target id" };
    Client *client = Client_Create(id, 2, 4, 8);
    Client *from = Client_Create(from_id, 1, 1, 1);
    const int peers_count = 3;

    from->node.pending_queries = 1;
    Table_InsertNode(client->table, &from->node);

    int i = 0;
    for (i = 0; i < peers_count; i++)
    {
        Peer peer = { .addr = 100 + i, .port = 100 + i };
        Client_AddPeer(from, &target_id, &peer);
    }

    Message *qgetpeers = Message_CreateQGetPeers(client,
                                                 &from->node,
                                                 &target_id);

    qgetpeers->node = client->node;
    Message *rgetpeers = HandleQGetPeers(from, qgetpeers);

    Search *search = Search_Create(&target_id);
    rgetpeers->context = search;

    rgetpeers->node = from->node;
    rgetpeers->node.reply_time = time(NULL);

    int rc = (GetReplyHandler(rgetpeers->type))(client, rgetpeers);

    mu_assert(rc == 0, "HandleRGetPeers failed");
    mu_assert(search->table->buckets[0]->count == 1, "Only from node expected.");
    mu_assert(search->peers->count == peers_count, "Missing peers");

    Client_Destroy(client);
    Client_Destroy(from);
    Message_Destroy(qgetpeers);
    Message_Destroy(rgetpeers);
    Search_Destroy(search);

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
    mu_run_test(test_HandleRAnnouncePeer);
    mu_run_test(test_HandleRGetPeers_nodes);
    mu_run_test(test_HandleRGetPeers_peers);

    return NULL;
}

RUN_TESTS(all_tests);

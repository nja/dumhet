#include "minunit.h"
#include <dht/client.h>
#include <dht/message.h>
#include <dht/message_create.h>

int DoMessageStr(Message *message)
{
    bstring str = Dht_MessageStr(message);
    check(str != NULL, "Dht_MessageStr failed");

    debug("MessageStr:\n%s\n", str->data);

    bdestroy(str);

    return 0;
error:
    return -1;
}

char *test_CreateDestroy_QPing()
{
    Hash id = { "qping" };
    Client *client = Client_Create(id, 2, 4, 8);
    Node to = {{ "node id" }, {1234}};

    Message *message = Message_CreateQPing(client, &to);

    mu_assert(message != NULL, "Message_CreateQPing failed");
    mu_assert(message->type == QPing, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(Node_Same(&to, &message->node), "Wrong node");

    int rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_QFindNode()
{
    Hash id = { "qfindnode" };
    Hash target = { "target" };
    Client *client = Client_Create(id, 2, 4, 8);
    Node to = {{ "node id" }, {1234}};

    Message *message = Message_CreateQFindNode(client, &to, &target);

    mu_assert(message != NULL, "Message_CreateQFindNode failed");
    mu_assert(message->type == QFindNode, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(Hash_Equals(&target, message->data.qfindnode.target),
              "Wrong target");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(Node_Same(&to, &message->node), "Wrong node");

    int rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_QGetPeers()
{
    Hash id = { "qgetpeers" };
    Hash info_hash = { "info_hash" };
    Client *client = Client_Create(id, 2, 4, 8);
    Node to = {{ "node id" }, {1234}};

    Message *message = Message_CreateQGetPeers(client, &to, &info_hash);

    mu_assert(message != NULL, "Message_CreateQGetPeers failed");
    mu_assert(message->type == QGetPeers, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(Hash_Equals(&info_hash, message->data.qgetpeers.info_hash),
              "Wrong target");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(Node_Same(&to, &message->node), "Wrong node");

    int rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_QAnnouncePeer()
{
    const int peer_port = 1234;
    Hash id = { "qannouncepeer" };
    Hash info_hash = { "info_hash" };
    Client *client = Client_Create(id, 2, 4, peer_port);
    Node to = {{ "node id" }, {1234}};

    Token token = Client_MakeToken(client, &to);

    Message *message = Message_CreateQAnnouncePeer(client,
                                                   &to,
                                                   &info_hash,
                                                   token.value,
                                                   HASH_BYTES);

    mu_assert(message != NULL, "Message_CreateQGetPeers failed");
    mu_assert(message->type == QAnnouncePeer, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(Hash_Equals(&info_hash, message->data.qannouncepeer.info_hash),
              "Wrong target");
    mu_assert(message->data.qannouncepeer.port == peer_port, "Wrong peer port");
    mu_assert(message->data.qannouncepeer.token.len == HASH_BYTES, "Wrong token_len");
    mu_assert(Hash_Equals(&token, (Hash *)message->data.qannouncepeer.token.data),
              "Wrong token");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(Node_Same(&to, &message->node), "Wrong node");

    int rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

Node test_node = {{ "test from id"}, {2222}};

Message *CreateTestQuery(MessageType type)
{
    const int t_len = 3;
    char *t = "abc";
    Message *message = calloc(1, sizeof(Message));
    message->t = malloc(t_len);
    memcpy(message->t, t, t_len);
    message->t_len = t_len;
    message->type = type;
    message->node = test_node;

    return message;
}

int SameT(Message *query, Message *response)
{
    check(query->t_len == response->t_len, "Wrong t_len");

    unsigned int i = 0;
    for (i = 0; i < query->t_len; i++)
        check(query->t[i] == response->t[i], "Wrong t at %d", i);

    return 1;
error:
    return 0;
}

char *test_CreateDestroy_RPing()
{
    Hash id = { "rping" };
    Client *client = Client_Create(id, 2, 4, 8);

    Message *query = CreateTestQuery(QPing);
    Message *message = Message_CreateRPing(client, query);

    mu_assert(message != NULL, "Message_CreateRPing failed");
    mu_assert(message->type == RPing, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(SameT(query, message), "Wrong t");
    mu_assert(Node_Same(&test_node, &message->node), "Wrong node");

    int rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(query);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_RFindNode()
{
    Hash id = { "rfindnode" };
    Client *client = Client_Create(id, 2, 4, 8);
    Message *query = CreateTestQuery(RFindNode);

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        DArray *found = DArray_create(sizeof(Node *), i + 1);

        int j = 0;

        while (DArray_count(found) < i)
        {
            Hash found_id = { "found" };
            found_id.value[5] = j;
            Node *node = Node_Create(&found_id);
            node->addr.s_addr = j;
            node->port = ~j;
            DArray_push(found, node);

            j++;
        }

        Message *message = Message_CreateRFindNode(client, query, found);

        mu_assert(message->data.rfindnode.count == (unsigned int)i,
                  "Wrong node count");

        for (j = 0; j < DArray_count(found); j++)
        {
            mu_assert(Node_Same(DArray_get(found, j),
                                message->data.rfindnode.nodes[j]),
                      "Wrong node in message");
        }

        mu_assert(message != NULL, "Message_CreateRFindNode failed");
        mu_assert(message->type == RFindNode, "Wrong message type");
        mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
        mu_assert(message->t != NULL, "No message t");
        mu_assert(message->t_len > 0, "No t_len");
        mu_assert(SameT(query, message), "Wrong t");
        mu_assert(Node_Same(&test_node, &message->node), "Wrong node");

        int rc = DoMessageStr(message);
        mu_assert(rc == 0, "Dht_MessageStr failed");

        Message_Destroy(message);

        while (DArray_count(found) > 0)
            Node_Destroy(DArray_pop(found));

        DArray_destroy(found);
    }

    int rc = DoMessageStr(query);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Message_Destroy(query);
    Client_Destroy(client);

    return NULL;
}

char *test_CreateDestroy_RAnnouncePeer()
{
    Hash id = { "rAnnounceData" };
    Client *client = Client_Create(id, 2, 4, 8);

    Message *query = CreateTestQuery(RAnnouncePeer);
    Message *message = Message_CreateRAnnouncePeer(client, query);

    mu_assert(message != NULL, "Message_CreateRAnnouncePeer failed");
    mu_assert(message->type == RAnnouncePeer, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(SameT(query, message), "Wrong t");
    mu_assert(Node_Same(&test_node, &message->node), "Wrong node");

    int rc = DoMessageStr(query);
    mu_assert(rc == 0, "Dht_MessageStr failed");
    rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(query);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_RGetPeers_Nodes()
{
    Hash id = { "rgetpeers_nodes" };
    Client *client = Client_Create(id, 2, 4, 8);
    Message *query = CreateTestQuery(RGetPeers);

    int i = 0;

    for (i = 0; i < BUCKET_K; i++)
    {
        DArray *found = DArray_create(sizeof(Node *), i + 1);

        int j = 0;

        while (DArray_count(found) < i)
        {
            Hash found_id = { "found" };
            found_id.value[5] = j;
            Node *node = Node_Create(&found_id);
            node->addr.s_addr = j;
            node->port = ~j;
            DArray_push(found, node);

            j++;
        }

        Token token = { "token" };
        token.value[5] = i;

        Message *message = Message_CreateRGetPeers(client,
                                                   query,
                                                   NULL,
                                                   found,
                                                   &token);

        mu_assert(message->data.rgetpeers.values == NULL, "Unwanted peers");
        mu_assert(message->data.rgetpeers.count == (unsigned int)i,
                  "Wrong node count");

        for (j = 0; j < DArray_count(found); j++)
        {
            mu_assert(Node_Same(DArray_get(found, j),
                                message->data.rgetpeers.nodes[j]),
                      "Wrong node in message");
        }

        mu_assert(message != NULL, "Message_CreateRGetPeers failed");
        mu_assert(message->type == RGetPeers, "Wrong message type");
        mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
        mu_assert(message->data.rgetpeers.token.len == HASH_BYTES,
                  "Wrong token_len");
        mu_assert(Hash_Equals(&token, (Hash *)message->data.rgetpeers.token.data),
                  "Wrong token");
        mu_assert(message->t != NULL, "No message t");
        mu_assert(message->t_len > 0, "No t_len");
        mu_assert(SameT(query, message), "Wrong t");
        mu_assert(Node_Same(&test_node, &message->node), "Wrong node");

        int rc = DoMessageStr(message);
        mu_assert(rc == 0, "Dht_MessageStr failed");

        Message_Destroy(message);

        while (DArray_count(found) > 0)
            Node_Destroy(DArray_pop(found));

        DArray_destroy(found);
    }

    int rc = DoMessageStr(query);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(query);

    return NULL;

}

char *test_CreateDestroy_RGetPeers_Peers()
{
    Hash id = { "rgetpeers_peers" };
    Client *client = Client_Create(id, 2, 4, 8);
    Message *query = CreateTestQuery(RGetPeers);

    int i = 0;

    for (i = 0; i < BUCKET_K; i++)
    {
        DArray *found = DArray_create(sizeof(Peer *), i + 1);

        while (DArray_count(found) < i)
        {
            Peer *peer = malloc(sizeof(Peer));
            peer->addr = DArray_count(found);
            peer->port = ~DArray_count(found);
            DArray_push(found, peer);
        }

        Token token = { "token" };
        token.value[5] = i;

        Message *message = Message_CreateRGetPeers(client,
                                                   query,
                                                   found,
                                                   NULL,
                                                   &token);

        mu_assert(message->data.rgetpeers.nodes == NULL, "Unwanted nodes");
        mu_assert(message->data.rgetpeers.count == (unsigned int)i,
                  "Wrong node count");

        int j = 0;
        for (j = 0; j < DArray_count(found); j++)
        {
            Peer *expected = DArray_get(found, j);
            Peer *actual = &message->data.rgetpeers.values[j];
            mu_assert(expected->addr == actual->addr, "Wrong peer addr");
            mu_assert(expected->port == actual->port, "Wrong peer port");
        }

        mu_assert(message != NULL, "Message_CreateRGetPeers failed");
        mu_assert(message->type == RGetPeers, "Wrong message type");
        mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
        mu_assert(message->data.rgetpeers.token.len == HASH_BYTES,
                  "Wrong token_len");
        mu_assert(Hash_Equals(&token, (Hash *)message->data.rgetpeers.token.data),
                  "Wrong token");
        mu_assert(message->t != NULL, "No message t");
        mu_assert(message->t_len > 0, "No t_len");
        mu_assert(SameT(query, message), "Wrong t");
        mu_assert(Node_Same(&test_node, &message->node), "Wrong node");

        int rc = DoMessageStr(message);
        mu_assert(rc == 0, "Dht_MessageStr failed");

        Message_Destroy(message);

        while (DArray_count(found) > 0)
            free(DArray_pop(found));

        DArray_destroy(found);
    }

    int rc = DoMessageStr(query);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(query);

    return NULL;
}

char *test_CreateDestroy_RError()
{
    Hash id = { "rerror" };
    Client *client = Client_Create(id, 2, 4, 8);
    Message *query = CreateTestQuery(RAnnouncePeer);

    Message *message = Message_CreateRErrorBadToken(client, query);
    mu_assert(message != NULL, "Message_CreateRErrorBadToken failed");
    mu_assert(message->type == RError, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(SameT(query, message), "Wrong t");
    mu_assert(Node_Same(&test_node, &message->node), "Wrong node");

    mu_assert(message->data.rerror.code == RERROR_PROTOCOL, "Wrong error code");
    mu_assert(blength(message->data.rerror.message) != 0, "No message");

    int rc = DoMessageStr(query);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    rc = DoMessageStr(message);
    mu_assert(rc == 0, "Dht_MessageStr failed");

    Client_Destroy(client);
    Message_Destroy(query);
    Message_Destroy(message);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_CreateDestroy_QPing);
    mu_run_test(test_CreateDestroy_QFindNode);
    mu_run_test(test_CreateDestroy_QGetPeers);
    mu_run_test(test_CreateDestroy_QAnnouncePeer);

    mu_run_test(test_CreateDestroy_RPing);
    mu_run_test(test_CreateDestroy_RAnnouncePeer);
    mu_run_test(test_CreateDestroy_RFindNode);
    mu_run_test(test_CreateDestroy_RGetPeers_Peers);
    mu_run_test(test_CreateDestroy_RGetPeers_Nodes);

    mu_run_test(test_CreateDestroy_RError);

    return NULL;
}

RUN_TESTS(all_tests);

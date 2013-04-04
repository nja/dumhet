#include "minunit.h"
#include <dht/client.h>
#include <dht/message.h>
#include <dht/message_create.h>

char *test_CreateDestroy_QPing()
{
    Hash id = { "qping" };
    Client *client = Client_Create(id, 0, 0, 0);

    Message *message = Message_CreateQPing(client);

    mu_assert(message != NULL, "Message_CreateQPing failed");
    mu_assert(message->type == QPing, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_QFindNode()
{
    Hash id = { "qfindnode" };
    Hash target = { "target" };
    Client *client = Client_Create(id, 0, 0, 0);

    Message *message = Message_CreateQFindNode(client, &target);

    mu_assert(message != NULL, "Message_CreateQFindNode failed");
    mu_assert(message->type == QFindNode, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(Hash_Equals(&target, message->data.qfindnode.target),
              "Wrong target");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_QGetPeers()
{
    Hash id = { "qgetpeers" };
    Hash info_hash = { "info_hash" };
    Client *client = Client_Create(id, 0, 0, 0);

    Message *message = Message_CreateQGetPeers(client, &info_hash);

    mu_assert(message != NULL, "Message_CreateQGetPeers failed");
    mu_assert(message->type == QGetPeers, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(Hash_Equals(&info_hash, message->data.qgetpeers.info_hash),
              "Wrong target");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_QAnnouncePeer()
{
    const int peer_port = 1234;
    Hash id = { "qannouncepeer" };
    Hash info_hash = { "info_hash" };
    Client *client = Client_Create(id, 0, 0, peer_port);
    Node from = {{{ 0 }}};

    Token token = Client_MakeToken(client, &from);

    Message *message = Message_CreateQAnnouncePeer(client, &info_hash, &token);

    mu_assert(message != NULL, "Message_CreateQGetPeers failed");
    mu_assert(message->type == QAnnouncePeer, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(Hash_Equals(&info_hash, message->data.qannouncepeer.info_hash),
              "Wrong target");
    mu_assert(message->data.qannouncepeer.port == peer_port, "Wrong peer port");
    mu_assert(Hash_Equals(&token, (Hash *)message->data.qannouncepeer.token),
              "Wrong token");
    mu_assert(message->data.qannouncepeer.token_len == HASH_BYTES, "Wrong token_len");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");

    Client_Destroy(client);
    Message_Destroy(message);

    return NULL;
}

Message *CreateTestQuery(MessageType type)
{
    const int t_len = 3;
    char *t = "abc";
    Message *message = calloc(1, sizeof(Message));
    message->t = malloc(t_len);
    memcpy(message->t, t, t_len);
    message->t_len = t_len;
    message->type = type;

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
    Client *client = Client_Create(id, 0, 0, 0);

    Message *query = CreateTestQuery(QPing);
    Message *message = Message_CreateRPing(client, query);

    mu_assert(message != NULL, "Message_CreateRPing failed");
    mu_assert(message->type == RPing, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(SameT(query, message), "Wrong t");

    Client_Destroy(client);
    Message_Destroy(query);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_RFindNode()
{
    Hash id = { "rfindnode" };
    Client *client = Client_Create(id, 0, 0, 0);
    Message *query = CreateTestQuery(RFindNode);

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        DArray *found = DArray_create(sizeof(Node *), i + 1);

        while (DArray_count(found) < i)
        {
            Hash found_id = { "found" };
            found_id.value[5] = i;
            Node *node = Node_Create(&found_id);
            node->addr.s_addr = i;
            node->port = ~i;
            DArray_push(found, node);
        }

        Message *message = Message_CreateRFindNode(client, query, found);

        mu_assert(message->data.rfindnode.count == (unsigned int)i,
                  "Wrong node count");

        int j = 0;
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

        Message_Destroy(message);

        while (DArray_count(found) > 0)
            Node_Destroy(DArray_pop(found));

        DArray_destroy(found);
    }

    Message_Destroy(query);
    Client_Destroy(client);

    return NULL;
}

char *test_CreateDestroy_RAnnouncePeer()
{
    Hash id = { "rAnnounceData" };
    Client *client = Client_Create(id, 0, 0, 0);

    Message *query = CreateTestQuery(RAnnouncePeer);
    Message *message = Message_CreateRAnnouncePeer(client, query);

    mu_assert(message != NULL, "Message_CreateQAnnouncePeer failed");
    mu_assert(message->type == RAnnouncePeer, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(SameT(query, message), "Wrong t");

    Client_Destroy(client);
    Message_Destroy(query);
    Message_Destroy(message);

    return NULL;
}

char *test_CreateDestroy_RGetPeers_Nodes()
{
    Hash id = { "rgetpeers_nodes" };
    Client *client = Client_Create(id, 0, 0, 0);
    Message *query = CreateTestQuery(RGetPeers);

    int i = 0;

    for (i = 0; i < BUCKET_K; i++)
    {
        DArray *found = DArray_create(sizeof(Node *), i + 1);

        while (DArray_count(found) < i)
        {
            Hash found_id = { "found" };
            found_id.value[5] = i;
            Node *node = Node_Create(&found_id);
            node->addr.s_addr = i;
            node->port = ~i;
            DArray_push(found, node);
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

        int j = 0;
        for (j = 0; j < DArray_count(found); j++)
        {
            mu_assert(Node_Same(DArray_get(found, j),
                                message->data.rgetpeers.nodes[j]),
                      "Wrong node in message");
        }

        mu_assert(message != NULL, "Message_CreateRGetPeers failed");
        mu_assert(message->type == RGetPeers, "Wrong message type");
        mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
        mu_assert(Hash_Equals(&token, (Hash *)message->data.rgetpeers.token),
                  "Wrong token");
        mu_assert(message->data.rgetpeers.token_len == HASH_BYTES,
                  "Wrong token_len");
        mu_assert(message->t != NULL, "No message t");
        mu_assert(message->t_len > 0, "No t_len");
        mu_assert(SameT(query, message), "Wrong t");

        Message_Destroy(message);

        while (DArray_count(found) > 0)
            Node_Destroy(DArray_pop(found));

        DArray_destroy(found);
    }

    Client_Destroy(client);
    Message_Destroy(query);

    return NULL;

}

char *test_CreateDestroy_RGetPeers_Peers()
{
    Hash id = { "rgetpeers_peers" };
    Client *client = Client_Create(id, 0, 0, 0);
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
        mu_assert(Hash_Equals(&token, (Hash *)message->data.rgetpeers.token),
                  "Wrong token");
        mu_assert(message->data.rgetpeers.token_len == HASH_BYTES,
                  "Wrong token_len");
        mu_assert(message->t != NULL, "No message t");
        mu_assert(message->t_len > 0, "No t_len");
        mu_assert(SameT(query, message), "Wrong t");

        Message_Destroy(message);

        while (DArray_count(found) > 0)
            free(DArray_pop(found));

        DArray_destroy(found);
    }

    Client_Destroy(client);
    Message_Destroy(query);

    return NULL;
}

char *test_CreateDestroy_RError()
{
    Hash id = { "rerror" };
    Client *client = Client_Create(id, 0, 0, 0);
    Message *query = CreateTestQuery(RAnnouncePeer);

    Message *message = Message_CreateRErrorBadToken(client, query);
    mu_assert(message != NULL, "Message_CreateRErrorBadToken failed");
    mu_assert(message->type == RError, "Wrong message type");
    mu_assert(Hash_Equals(&id, &message->id), "Wrong message id");
    mu_assert(message->t != NULL, "No message t");
    mu_assert(message->t_len > 0, "No t_len");
    mu_assert(SameT(query, message), "Wrong t");

    mu_assert(message->data.rerror.code == RERROR_PROTOCOL, "Wrong error code");
    mu_assert(blength(message->data.rerror.message) != 0, "No message");

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

#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQuery(DhtClient *client, MessageType type)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(MessageType_IsQuery(type) && "MessageType not a query");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    message->type = type;

    message->t = malloc(sizeof(tid_t));
    check_mem(message->t);
    *(tid_t *)message->t = client->next_t++;
    message->t_len = sizeof(tid_t);

    message->id = client->node.id;

    return message;
error:
    return NULL;
}    

Message *Message_CreateQPing(DhtClient *client)
{
    return Message_CreateQuery(client, QPing);
}

Message *Message_CreateQFindNode(DhtClient *client, DhtHash *id)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(id != NULL && "NULL DhtHash pointer");

    Message *message = Message_CreateQuery(client, QFindNode);
    check(message != NULL, "Message_Create failed");

    message->data.qfindnode.target = malloc(HASH_BYTES);
    check_mem(message->data.qfindnode.target);

    memcpy(message->data.qfindnode.target, id->value, HASH_BYTES);

    return message;
error:
    return NULL;
}

Message *Message_CreateQGetPeers(DhtClient *client, DhtHash *info_hash)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(info_hash != NULL && "NULL DhtHash pointer");

    Message *message = Message_CreateQuery(client, QGetPeers);
    check(message != NULL, "Message_Create failed");

    message->data.qgetpeers.info_hash = malloc(HASH_BYTES);
    check_mem(message->data.qgetpeers.info_hash);

    memcpy(message->data.qgetpeers.info_hash, info_hash->value, HASH_BYTES);

    return message;
error:
    return NULL;
}

Message *Message_CreateQAnnouncePeer(DhtClient *client,
                                     DhtHash *info_hash,
                                     Token *token)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(info_hash != NULL && "NULL DhtHash pointer");
    assert(token != NULL && "NULL Token pointer");

    QAnnouncePeerData data = { 0 };

    Message *message = Message_CreateQuery(client, QAnnouncePeer);
    check(message != NULL, "Message_Create failed");

    data.info_hash = malloc(HASH_BYTES);
    check_mem(data.info_hash);

    memcpy(data.info_hash, info_hash->value, HASH_BYTES);

    data.token = malloc(HASH_BYTES);
    check_mem(data.token);

    memcpy(data.token, token->value, HASH_BYTES);
    data.token_len = HASH_BYTES;

    data.port = client->peer_port;

    message->data.qannouncepeer = data;

    return message;
error:
    free(data.info_hash);
    return NULL;
}

Message *Message_CreateResponse(DhtClient *client, Message *query, MessageType type)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->t != NULL && "NULL char token pointer in query");
    assert(query->t_len > 0 && "Bad t_len in query");
    assert(!MessageType_IsQuery(type) && "MessageType not a response");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    message->type = type;

    message->t_len = query->t_len;
    message->t = malloc(query->t_len);
    check_mem(message->t);
    memcpy(message->t, query->t, query->t_len);

    message->id = client->node.id;

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRFindNode(DhtClient *client, Message *query, DArray *found)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(found != NULL && "NULL DArray pointer");

    Message *message = Message_CreateResponse(client, query, RFindNode);
    check(message != NULL, "Message_Create failed");

    RFindNodeData data;
    data.count = DArray_count(found);
    data.nodes = malloc(data.count * sizeof(DhtNode *));

    unsigned int i = 0;
    for (i = 0; i < data.count; i++)
        data.nodes[i] = DArray_get(found, i);

    message->data.rfindnode = data;

    return message;
error:
    return NULL;
}

Message *Message_CreateRPing(DhtClient *client, Message *query)
{
    return Message_CreateResponse(client, query, RPing);
}

Message *Message_CreateRAnnouncePeer(DhtClient *client, Message *query)
{
    return Message_CreateResponse(client, query, RAnnouncePeer);
}

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRGetPeers(DhtClient *client,
                                 Message *query,
                                 DArray *peers,
                                 DArray *nodes, 
                                 Token *token)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(token != NULL && "NULL Token pointer");

    RGetPeersData data = { 0 };

    check(peers != NULL || nodes != NULL, "Neither peers nor nodes in RGetPeers");
    check(peers == NULL || nodes == NULL, "Both peers and nodes in RGetPeers");

    Message *message = Message_CreateResponse(client, query, RGetPeers);
    check(message != NULL, "Message_Create failed");

    data.token = malloc(HASH_BYTES);
    check_mem(data.token);
    memcpy(data.token, token->value, HASH_BYTES);
    data.token_len = HASH_BYTES;

    unsigned int i = 0;

    if (peers != NULL)
    {
        data.count = DArray_count(peers);
        data.values = malloc(sizeof(Peer) * data.count);
        check_mem(data.values);

        for (i = 0; i < data.count; i++)
        {
            data.values[i] = *(Peer *)DArray_get(peers, i);
        }
    }
    else
    {
        data.count = DArray_count(nodes);
        data.nodes = malloc(data.count * sizeof(DhtNode *));
        check_mem(data.nodes);

        for (i = 0; i < data.count; i++)
        {
            data.nodes[i] = DArray_get(nodes, i);
        }
    }

    message->data.rgetpeers = data;

    return message;
error:
    free(data.token);
    return NULL;
}

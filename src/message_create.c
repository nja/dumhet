#include <dht/client.h>
#include <dht/message.h>

Message *Message_Create(DhtClient *client, MessageType type)
{
    assert(client != NULL && "NULL DhtClient pointer");

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
    return Message_Create(client, QPing);
}

Message *Message_CreateQFindNode(DhtClient *client, DhtHash *id)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(id != NULL && "NULL DhtHash pointer");

    Message *message = Message_Create(client, QFindNode);
    check(message != NULL, "Message_Create failed");

    message->data.qfindnode.target = malloc(HASH_BYTES);
    check_mem(message->data.qfindnode.target);

    memcpy(message->data.qfindnode.target, id->value, HASH_BYTES);

    return message;
error:
    return NULL;
}

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRFindNode(DhtClient *client, DArray *found)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(found != NULL && "NULL DArray pointer");

    Message *message = Message_Create(client, RFindNode);
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

Message *Message_CreateRPing(DhtClient *client)
{
    return Message_Create(client, RPing);
}

Message *Message_CreateRAnnouncePeer(DhtClient *client)
{
    return Message_Create(client, RAnnouncePeer);
}

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRGetPeers(DhtClient *client,
                                 DArray *peers,
                                 DArray *nodes, 
                                 Token *token)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(token != NULL && "NULL Token pointer");

    RGetPeersData data = { 0 };

    check(peers != NULL || nodes != NULL, "Neither peers nor nodes in RGetPeers");
    check(peers == NULL || nodes == NULL, "Both peers and nodes in RGetPeers");

    Message *message = Message_Create(client, RGetPeers);
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

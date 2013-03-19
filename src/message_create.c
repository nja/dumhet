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

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRFindNode(DhtClient *client, Message *query, DArray *found)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(found != NULL && "NULL DArray pointer");

    Message *message = Message_Create(client, RFindNode);
    check(message != NULL, "Message_Create failed");

    RFindNodeData data;
    data.count = DArray_count(found);
    data.nodes = calloc(data.count, sizeof(DhtNode *));

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

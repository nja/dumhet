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

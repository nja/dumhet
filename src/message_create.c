#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQPing(DhtClient *client)
{
    assert(client != NULL && "NULL DhtClient pointer");

    Message *ping = calloc(1, sizeof(Message));
    check_mem(ping);

    ping->type = QPing;

    ping->t = malloc(sizeof(tid_t));
    check_mem(ping->t);
    *(tid_t *)ping->t = client->next_t++;
    ping->t_len = sizeof(tid_t);

    ping->id = client->node.id;

    return ping;
error:
    return NULL;
}

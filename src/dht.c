#include <lcthw/dbg.h>
#include <dht/dht.h>
#include <dht/client.h>
#include <dht/hooks.h>
#include <dht/message_create.h>
#include <dht/network.h>
#include <dht/work.h>

void *Dht_CreateClient(Hash id, uint32_t addr, uint16_t port, uint16_t peer_port)
{
    Client *client = Client_Create(id, addr, port, peer_port);
    return client;
}

void Dht_DestroyClient(void *client)
{
    Client_Destroy((Client *)client);
}

int Dht_AddNode(void *client_, uint32_t addr, uint16_t port)
{
    Message *qping = NULL;
    Client *client = (Client *)client_;
    check(client != NULL, "NULL client pointer");

    Node node = { .addr.s_addr = addr, .port = port, .is_new = 1 };

    qping = Message_CreateQPing(client, &node);
    check(qping != NULL, "Message_CreateQPing failed");

    int rc = MessageQueue_Push(client->queries, qping);
    check(rc == 0, "Messagequeue_Push failed");

    return 0;
error:
    Message_Destroy(qping);
    return -1;
}

int Dht_Start(void *client_)
{
    Client *client = (Client *)client_;
    check(client != NULL, "NULL client pointer");

    int rc = Client_AddSearch(client, &client->table->id);
    check(rc == 0, "Client_AddSearch failed");

    rc = NetworkUp(client);
    check(rc == 0, "NetworkUp failed");

    return 0;
error:
    return -1;
}

int Dht_Stop(void *client)
{
    check(client != NULL, "NULL client pointer");

    return NetworkDown((Client *)client);
error:
    return -1;
}

int Dht_Process(void *client_)
{
    Client *client = (Client *)client_;
    check(client != NULL, "NULL client pointer");

    int rc = Client_HandleSearches(client);
    check(rc == 0, "Client_HandleSearches failed");

    rc = Client_Send(client, client->queries);
    check(rc == 0, "Client_Send failed");

    rc = Client_Receive(client);
    check(rc == 0, "Client_Receive failed");

    rc = Client_HandleMessages(client);
    check(rc == 0, "Client_Handle failed");

    rc = Client_Send(client, client->replies);
    check(rc == 0, "Client_Send failed");

    return 0;
error:
    return -1;
}

int Dht_AddHook(void *client, Hook *hook)
{
    check(client != NULL, "NULL client pointer");

    return Client_AddHook((Client *)client, hook);
error:
    return -1;
}

int Dht_RemoveHook(void *client, Hook *hook)
{
    check(client != NULL, "NULL client pointer");

    return Client_RemoveHook((Client *)client, hook);
error:
    return -1;
}

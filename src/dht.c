#include <lcthw/dbg.h>
#include <dht/dht.h>
#include <dht/client.h>
#include <dht/hooks.h>
#include <dht/network.h>
#include <dht/work.h>

void *Dht_CreateClient(Hash id, uint32_t addr, uint16_t port, uint16_t peer_port)
{
    Client *client = Client_Create(id, addr, port, peer_port);
    return client;
}

int Dht_AddNode(void *client, Hash id, uint32_t addr, uint16_t port)
{
    check(client != NULL, "NULL client pointer");

    Node *node = Node_Create(&id);
    check(node != NULL, "Node_Create failed");

    node->addr.s_addr = addr;
    node->port = port;

    Table_InsertNodeResult result = Table_InsertNode(((Client *)client)->table, node);

    switch (result.rc)
    {
    case OKAdded:
        break;
    case OKFull:
    case OKAlreadyAdded:
        Node_Destroy(node);
        break;
    case OKReplaced:
        Node_Destroy(result.replaced);
    default:
        sentinel("Table_InsertNode failed");
    }

    return 0;
error:
    Node_Destroy(node);
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

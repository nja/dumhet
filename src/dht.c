#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

/* bstring */

bstring HexStr(char *data, size_t len)
{
    assert(data != NULL && "NULL data pointer");

    bstring str = bfromcstralloc(len * 2, "");
    check_mem(str);

    char *end = data + len;

    while (data < end)
    {
        int rc = bformata(str, "%02X", *data++);
        check(rc == BSTR_OK, "bformata failed");
    }

    return str;
error:
    bdestroy(str);
    return NULL;
}

bstring Dht_HashStr(Hash *hash)
{
    if (hash == NULL)
        return bformat("%*s", HASH_BYTES * 2, "(NULL HASH)");

    return HexStr(hash->value, HASH_BYTES);
}

bstring Dht_NodeStr(Node *node)
{
    if (node == NULL)
        return bfromcstr("(NULL Node)");

    bstring id = Dht_HashStr(&node->id);
    char *addr = inet_ntoa(node->addr);

    bstring str = bformat("%15s:%-5d %s", addr, htons(node->port), id->data);

    bdestroy(id);

    return str;
}

bstring Dht_PeerStr(Peer *peer)
{
    if (peer == NULL)
        return bfromcstr("(NULL Peer)");

    struct in_addr addr = { .s_addr = peer->addr };

    return bformat("%15s:%-5d", inet_ntoa(addr), peer->port);
}

bstring Dht_FTokenStr(struct FToken *ftoken)
{
    if (ftoken == NULL)
        return bfromcstr("(NULL FToken)");

    return HexStr(ftoken->data, ftoken->len);
}

bstring TokenStr(char *data, size_t len)
{
    if (data == NULL)
        return bfromcstr("(NULL token)");

    return HexStr(data, len);
}

bstring Dht_MessageTypeStr(MessageType type)
{
    switch (type)
    {
    case MUnknown:      return bfromcstr("MUnknown");
    case QPing:         return bfromcstr("QPing");
    case QFindNode:     return bfromcstr("QFindNode");
    case QGetPeers:     return bfromcstr("QGetPeers");
    case QAnnouncePeer: return bfromcstr("QAnnouncePeer");
    case RPing:         return bfromcstr("RPing");
    case RFindNode:     return bfromcstr("RFindNode");
    case RGetPeers:     return bfromcstr("RGetPeers");
    case RAnnouncePeer: return bfromcstr("RAnnouncePeer");
    case RError:        return bfromcstr("RError");
    default:            return bfromcstr("(Invalid MessageType");
    }
}

bstring Dht_MessageStr(Message *message)
{
    if (message == NULL)
        return bfromcstr("(NULL Message)");

    bstring type = NULL, node = NULL, token = NULL, id = NULL,
        str = NULL, data = NULL;

    type = Dht_MessageTypeStr(message->type);
    check_mem(type);

    node = Dht_NodeStr(&message->node);
    check_mem(node);

    token = TokenStr(message->t, message->t_len);
    check_mem(token);

    id = Dht_HashStr(&message->id);
    check_mem(id);

    str = bformat(
        "%-13s Errors:%02X\n"
        "%s\n"
        "Token %s\n"
        "Id %s",
        type->data,
        message->errors,
        node->data,
        token->data,
        id->data);
    check_mem(str);

    if (1)
    {
        data = bfromcstr("Data todo");
        bconchar(str, '\n');
        bconcat(str, data);
        bdestroy(data);
    }

    bdestroy(type);
    bdestroy(node);
    bdestroy(token);
    bdestroy(id);

    return str;
error:
    bdestroy(type);
    bdestroy(node);
    bdestroy(token);
    bdestroy(id);
    bdestroy(str);

    return NULL;
}

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

    Search *search = Client_AddSearch(client, &client->table->id);
    check(search != NULL, "Client_AddSearch failed");

    int rc = NetworkUp(client);
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
        int rc = bformata(str, "%02hhX", *data++);
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

bstring Dht_FTokenStr(struct FToken ftoken)
{
    if (ftoken.data == NULL)
        return bfromcstr("(NULL ftoken)");

    return HexStr(ftoken.data, ftoken.len);
}

bstring TidStr(char *data, size_t len)
{
    if (data == NULL)
        return bfromcstr("(NULL tid)");

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

bstring Dht_RERROR_Str(int code)
{
    switch (code)
    {
    case RERROR_GENERIC:       return bfromcstr("GENERIC");
    case RERROR_SERVER:        return bfromcstr("SERVER");
    case RERROR_PROTOCOL:      return bfromcstr("PROTOCOL");
    case RERROR_METHODUNKNOWN: return bfromcstr("METHOD UNKNOWN");
    default:                   return bfromcstr("(UNKNOWN CODE)");
    }
}

bstring DataStr(Message *message);

bstring Dht_MessageStr(Message *message)
{
    if (message == NULL)
        return bfromcstr("(NULL Message)");

    bstring type = NULL, node = NULL, tid = NULL, id = NULL,
        str = NULL, data = NULL;

    type = Dht_MessageTypeStr(message->type);
    check_mem(type);

    node = Dht_NodeStr(&message->node);
    check_mem(node);

    tid = TidStr(message->t, message->t_len);
    check_mem(tid);

    id = Dht_HashStr(&message->id);
    check_mem(id);

    str = bformat(
        "%-13s Errors:%02X\n"
        "%s\n"
        "tid %s\n"
        "Id %s",
        type->data,
        message->errors,
        node->data,
        tid->data,
        id->data);
    check_mem(str);

    data = DataStr(message);
    check_mem(data);
    bconchar(str, '\n');
    bconcat(str, data);
    bdestroy(data);

    bdestroy(type);
    bdestroy(node);
    bdestroy(tid);
    bdestroy(id);

    return str;
error:
    bdestroy(type);
    bdestroy(node);
    bdestroy(tid);
    bdestroy(id);
    bdestroy(str);
    bdestroy(data);

    return NULL;
}

bstring DataQFindNodeStr(Message *message);
bstring DataQGetPeersStr(Message *message);
bstring DataQAnnouncePeerStr(Message *message);
bstring DataRFindNodeStr(Message *message);
bstring DataRGetPeersStr(Message *message);
bstring DataRErrorStr(Message *message);

bstring DataStr(Message *message)
{
    if (message == NULL)
        return bfromcstr("(NULL Message)");

    switch (message->type)
    {
    case MUnknown:
    case QPing:
    case RPing:
    case RAnnouncePeer:
        return bfromcstr("");   /* No data. */
    case QFindNode: return DataQFindNodeStr(message);
    case QGetPeers: return DataQGetPeersStr(message);
    case QAnnouncePeer: return DataQAnnouncePeerStr(message);
    case RFindNode: return DataRFindNodeStr(message);
    case RGetPeers: return DataRGetPeersStr(message);
    case RError: return DataRErrorStr(message);
    default: return bfromcstr("(Invalid MessageType)");
    }
}

bstring DataQFindNodeStr(Message *message)
{
    bstring target = Dht_HashStr(message->data.qfindnode.target);

    bstring str = bformat("QFindNode target: %s", target->data);

    bdestroy(target);

    return str;
}

bstring DataQGetPeersStr(Message *message)
{
    bstring info_hash = Dht_HashStr(message->data.qgetpeers.info_hash);

    bstring str =  bformat("QGetPeers info_hash: %s", info_hash->data);

    bdestroy(info_hash);

    return str;
}

bstring DataQAnnouncePeerStr(Message *message)
{
    bstring info_hash = Dht_HashStr(message->data.qannouncepeer.info_hash);
    bstring ftoken = Dht_FTokenStr(message->data.qannouncepeer.token);

    bstring str = bformat("QAnnouncePeer info_hash: %s\n"
                          "                  token: %s\n"
                          "                   port: %hd",
                          info_hash->data,
                          ftoken->data,
                          message->data.qannouncepeer.port);

    bdestroy(info_hash);
    bdestroy(ftoken);

    return str;
}

bstring DataRFindNodeStr(Message *message)
{
    if (message->data.rfindnode.nodes == NULL)
        return bfromcstr("(NULL rfindnode.nodes)");

    if (message->data.rfindnode.count == 0)
        return bfromcstr("(Zero nodes)");

    size_t i = 0;
    Node **nodes = message->data.rfindnode.nodes;

    bstring str = bfromcstr("");

    for (i = 0; i < message->data.rfindnode.count; i++)
    {
        bstring node = Dht_NodeStr(nodes[i]);

        bformata(str, "%sNode %02zd: %s",
                 i > 0 ? "\n" : "",
                 i,
                 node->data);

        bdestroy(node);
    }

    return str;
}
        
bstring DataRGetPeersStr(Message *message)
{
    if (message->data.rgetpeers.nodes != NULL)
    {
        return DataRFindNodeStr(message);
    }

    bstring ftoken = Dht_FTokenStr(message->data.rgetpeers.token);

    bstring str = bformat("Token: %s", ftoken->data);

    bdestroy(ftoken);

    size_t i = 0;

    if (message->data.rgetpeers.values == NULL)
    {
        bformata(str, "\n(NULL rgetpeers.values)");
        return str;
    }

    for (i = 0; i < message->data.rgetpeers.count; i++)
    {
        bstring peer = Dht_PeerStr(message->data.rgetpeers.values + i);

        bformata(str, "\nPeer %03zd: %s", i, peer->data);

        bdestroy(peer);
    }

    return str;
}

bstring DataRErrorStr(Message *message)
{
    bstring error = Dht_RERROR_Str(message->data.rerror.code);

    bstring str =  bformat("%03d %s: %s",
                           message->data.rerror.code,
                           error->data,
                           message->data.rerror.message->data);

    bdestroy(error);

    return str;
}

void *Dht_AddSearch(void *client, Hash info_hash)
{
    check(client != NULL, "NULL client pointer");

    return Client_AddSearch((Client *)client, &info_hash);
error:
    return NULL;
}

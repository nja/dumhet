#include <dht/close.h>
#include <dht/message.h>
#include <dht/message_create.h>
#include <dht/client.h>
#include <dht/search.h>
#include <dht/table.h>

int HandleReply(DhtClient *client, Message *message)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(message != NULL && "NULL Message pointer");
    assert((message->type == RPing
            || message->type == RAnnouncePeer) && "Wrong message type");
    assert(message->context == NULL && "Non-NULL message context");

    int rc = DhtTable_MarkReply(client->table, &message->id);
    check(rc == 0, "Table_MarkReply failed");

    return 0;
error:
    return -1;
}

int AddSearchNodes(Search *search, DhtNode **nodes, size_t count);

int HandleRFindNode(DhtClient *client, Message *message)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RFindNode && "Wrong message type");
    assert(message->context != NULL && "NULL message context");

    Search *search = (Search *)message->context;
    check(search != NULL, "Missing Search context");

    int rc = DhtTable_MarkReply(client->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (client->table)");

    rc = DhtTable_MarkReply(search->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (search->table)");

    rc = AddSearchNodes(search,
                        message->data.rfindnode.nodes,
                        message->data.rfindnode.count);
    check(rc == 0, "AddSearchNodes failed");
 
    return 0;
error:
    return -1;
}

int HandleRGetPeers(DhtClient *client, Message *message)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RGetPeers && "Wrong message type");
    assert(message->context != NULL && "NULL message context");

    RGetPeersData *data = &message->data.rgetpeers;

    check((data->values == NULL || data->nodes == NULL),
          "Both peers and nodes in RGetPeers");

    Search *search = (Search *)message->context;

    int rc = DhtTable_MarkReply(client->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (client->table)");

    rc = DhtTable_MarkReply(search->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (search->table)");

    if (data->nodes != NULL)
    {
        rc = AddSearchNodes(search, data->nodes, data->count);
        check(rc == 0, "AddSearchNodes failed");
    }
    else if (data->values != NULL)
    {
        rc = Search_AddPeers(search, data->values, data->count);
        check(rc == 0, "Search_AddPeers failed");
    }
    else
    {
        sentinel("No peers or nodes in RGetPeers");
    }

    return 0;
error:
    return -1;
}    

int AddSearchNodes(Search *search, DhtNode **nodes, size_t count)
{
    assert(search != NULL && "NULL Search pointer");
    assert(nodes != NULL && "NULL pointer to DhtNodes pointer");

    DhtNode **node = nodes;
    DhtNode **end = node + count;

    while (node < end)
    {
        DhtTable_InsertNodeResult result
            = DhtTable_InsertNode(search->table, *node);
        check(result.rc != ERROR, "DhtTable_InsertNode failed");

        if (result.rc == OKAdded || result.rc == OKReplaced)
        {
            *node = NULL;      /* Don't free it later */
        }

        DhtNode_Destroy(result.replaced);

        node++;
    }

    return 0;
error:
    return -1;
}

Message *HandleQFindNode(DhtClient *client, Message *query)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QFindNode && "Wrong message type");

    DhtTable_MarkQuery(client->table, &query->id);

    DArray *found = DhtTable_GatherClosest(client->table,
                                           query->data.qfindnode.target);
    check(found != NULL, "DhtTable_GatherClosest failed");

    Message *reply = Message_CreateRFindNode(client, query, found);
    check(reply != NULL, "Message_CreateRFindNode failed");

    DArray_destroy(found);
    return reply;
error:
    DArray_destroy(found);
    return NULL;
}

Message *HandleQPing(DhtClient *client, Message *query)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QPing && "Wrong message type");

    DhtTable_MarkQuery(client->table, &query->id);

    Message *reply = Message_CreateRPing(client, query);
    check(reply != NULL, "Message_CreateRPing failed");

    return reply;
error:
    return NULL;
}

Message *HandleQAnnouncePeer(DhtClient *client, Message *query, DhtNode *from)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QAnnouncePeer && "Wrong message type");
    assert(from != NULL && "NULL DhtNode pointer");

    DhtTable_MarkQuery(client->table, &query->id);

    if (!DhtClient_IsValidToken(client,
                                from,
                                query->data.qannouncepeer.token,
                                query->data.qannouncepeer.token_len))
    {
        Message *error = Message_CreateRErrorBadToken(client, query);
        check(error != NULL, "Message_CreateRErrorBadToken failed");
        return error;
    }

    Peer peer = { .addr = from->addr.s_addr, .port = query->data.qannouncepeer.port };

    int rc = DhtClient_AddPeer(client, query->data.qannouncepeer.info_hash, &peer);
    check(rc == 0, "Client_AddPeer failed");

    Message *reply = Message_CreateRAnnouncePeer(client, query);
    check(reply != NULL, "Message_CreateRAnnouncePeer failed");

    return reply;
error:
    return NULL;
}

Message *HandleQGetPeers(DhtClient *client, Message *query, DhtNode *from)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QGetPeers && "Wrong message type");
    assert(from != NULL && "NULL DhtNode pointer");

    DhtTable_MarkQuery(client->table, &query->id);

    DArray *peers = NULL;
    DArray *nodes = NULL;

    int rc = DhtClient_GetPeers(client, query->data.qgetpeers.info_hash, &peers);
    check(rc == 0, "DhtClient_GetPeers failed");

    if (DArray_count(peers) == 0)
    {
        DArray_destroy(peers);
        peers = NULL;

        nodes = DhtTable_GatherClosest(client->table,
                                       query->data.qgetpeers.info_hash);
        check(nodes != NULL, "DhtTable_GatherClosest failed");
    }

    Token token = DhtClient_MakeToken(client, from);

    Message *reply = Message_CreateRGetPeers(client, query, peers, nodes, &token);
    check(reply != NULL, "Message_CreateRGetPeers failed");

    DArray_destroy(peers);
    DArray_destroy(nodes);

    return reply;
error:
    DArray_destroy(peers);
    DArray_destroy(nodes);

    return NULL;
}

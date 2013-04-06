#include <dht/close.h>
#include <dht/message.h>
#include <dht/message_create.h>
#include <dht/client.h>
#include <dht/search.h>
#include <dht/table.h>

int HandleReply(Client *client, Message *message)
{
    assert(client != NULL && "NULL Client pointer");
    assert(message != NULL && "NULL Message pointer");
    assert((message->type == RPing
            || message->type == RAnnouncePeer) && "Wrong message type");
    assert(message->context == NULL && "Non-NULL message context");

    int rc = Table_MarkReply(client->table, &message->id);
    check(rc == 0, "Table_MarkReply failed");

    return 0;
error:
    return -1;
}

int AddSearchNodes(Search *search, Node **nodes, size_t count);

int HandleRFindNode(Client *client, Message *message)
{
    assert(client != NULL && "NULL Client pointer");
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RFindNode && "Wrong message type");
    assert(message->context != NULL && "NULL message context");

    Search *search = (Search *)message->context;
    check(search != NULL, "Missing Search context");

    int rc = Table_MarkReply(client->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (client->table)");

    rc = Table_MarkReply(search->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (search->table)");

    rc = AddSearchNodes(search,
                        message->data.rfindnode.nodes,
                        message->data.rfindnode.count);
    check(rc == 0, "AddSearchNodes failed");
 
    return 0;
error:
    return -1;
}

int HandleRGetPeers(Client *client, Message *message)
{
    assert(client != NULL && "NULL Client pointer");
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RGetPeers && "Wrong message type");
    assert(message->context != NULL && "NULL message context");

    RGetPeersData *data = &message->data.rgetpeers;

    check((data->values == NULL || data->nodes == NULL),
          "Both peers and nodes in RGetPeers");

    Search *search = (Search *)message->context;

    int rc = Table_MarkReply(client->table, &message->id);
    check(rc == 0, "Table_MarkReply failed (client->table)");

    rc = Table_MarkReply(search->table, &message->id);
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

int AddSearchNodes(Search *search, Node **nodes, size_t count)
{
    assert(search != NULL && "NULL Search pointer");
    assert(nodes != NULL && "NULL pointer to Nodes pointer");

    Node **node = nodes;
    Node **end = node + count;

    while (node < end)
    {
        Table_InsertNodeResult result
            = Table_InsertNode(search->table, *node);
        check(result.rc != ERROR, "Table_InsertNode failed");

        if (result.rc == OKAdded || result.rc == OKReplaced)
        {
            *node = NULL;      /* Don't free it later */
        }

        Node_Destroy(result.replaced);

        node++;
    }

    return 0;
error:
    return -1;
}

Message *HandleQFindNode(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QFindNode && "Wrong message type");

    Table_MarkQuery(client->table, &query->id);

    DArray *found = Table_GatherClosest(client->table,
                                        query->data.qfindnode.target);
    check(found != NULL, "Table_GatherClosest failed");

    Message *reply = Message_CreateRFindNode(client, query, found);
    check(reply != NULL, "Message_CreateRFindNode failed");

    DArray_destroy(found);
    return reply;
error:
    DArray_destroy(found);
    return NULL;
}

Message *HandleQPing(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QPing && "Wrong message type");

    Table_MarkQuery(client->table, &query->id);

    Message *reply = Message_CreateRPing(client, query);
    check(reply != NULL, "Message_CreateRPing failed");

    return reply;
error:
    return NULL;
}

Message *HandleQAnnouncePeer(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QAnnouncePeer && "Wrong message type");

    Table_MarkQuery(client->table, &query->id);

    if (!Client_IsValidToken(client,
                             &query->node,
                             query->data.qannouncepeer.token,
                             query->data.qannouncepeer.token_len))
    {
        Message *error = Message_CreateRErrorBadToken(client, query);
        check(error != NULL, "Message_CreateRErrorBadToken failed");
        return error;
    }

    Peer peer = { .addr = query->node.addr.s_addr,
                  .port = query->data.qannouncepeer.port };

    int rc = Client_AddPeer(client, query->data.qannouncepeer.info_hash, &peer);
    check(rc == 0, "Client_AddPeer failed");

    Message *reply = Message_CreateRAnnouncePeer(client, query);
    check(reply != NULL, "Message_CreateRAnnouncePeer failed");

    return reply;
error:
    return NULL;
}

Message *HandleQGetPeers(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->type == QGetPeers && "Wrong message type");

    Table_MarkQuery(client->table, &query->id);

    DArray *peers = NULL;
    DArray *nodes = NULL;

    int rc = Client_GetPeers(client, query->data.qgetpeers.info_hash, &peers);
    check(rc == 0, "Client_GetPeers failed");

    if (DArray_count(peers) == 0)
    {
        DArray_destroy(peers);
        peers = NULL;

        nodes = Table_GatherClosest(client->table,
                                    query->data.qgetpeers.info_hash);
        check(nodes != NULL, "Table_GatherClosest failed");
    }

    Token token = Client_MakeToken(client, &query->node);

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

#include <arpa/inet.h>

#include <dht/close.h>
#include <dht/handle.h>
#include <dht/message.h>
#include <dht/message_create.h>
#include <dht/client.h>
#include <dht/search.h>
#include <dht/table.h>

ReplyHandler GetReplyHandler(MessageType type)
{
    switch (type)
    {
    case RError: return HandleRError;
    case RPing: return HandleRPing;
    case RAnnouncePeer: return HandleReply;
    case RFindNode: return HandleRFindNode;
    case RGetPeers: return HandleRGetPeers;
    default:
        log_err("No reply handler for type %d", type);
        return NULL;
    }
}

QueryHandler GetQueryHandler(MessageType type)
{
    switch (type)
    {
    case QPing: return HandleQPing;
    case QAnnouncePeer: return HandleQAnnouncePeer;
    case QFindNode: return HandleQFindNode;
    case QGetPeers: return HandleQGetPeers;
    default:
        log_err("No query handler for type %d", type);
        return NULL;
    }
}

int HandleReply(Client *client, Message *message)
{
    assert(client != NULL && "NULL Client pointer");
    assert(message != NULL && "NULL Message pointer");
    assert((message->type == RPing
            || message->type == RAnnouncePeer) && "Wrong message type");
    assert(message->context == NULL && "Non-NULL message context");

    int rc = Table_MarkReply(client->table, message);
    check(rc == 0, "Table_MarkReply failed");

    return 0;
error:
    return -1;
}

int HandleRError(Client *client, Message *message)
{
    log_err("Error reply %s:%d %d %s",
            inet_ntoa(message->node.addr),
            message->node.port,
            message->data.rerror.code,
            bdata(message->data.rerror.message));
    return HandleReply(client, message);
}

int HandleUnknown(Client *client, Message *message)
{
    (void)client;

    log_err("Unknown message type from %s:%d",
            inet_ntoa(message->node.addr),
            message->node.port);
    return 0;
}

int AddSearchNodes(Client *client, Search *search, Node **nodes, size_t count);

int HandleRPing(Client *client, Message *message)
{
    assert(client != NULL && "NULL Client pointer");
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RPing && "Wrong message type");

    int rc = HandleReply(client, message);
    check(rc == 0, "HandleReply failed");

    int i;
    for (i = 0; i < DArray_end(client->searches); i++)
    {
        Search *search = (Search *)DArray_get(client->searches, i);
        /* Only good nodes are copied, so we set the reply_time */
        message->node.reply_time = time(NULL);
        rc = Table_CopyAndAddNode(search->table, &message->node);
        check(rc == 0, "Table_CopyAndAddNode failed");
    }

    return 0;
error:
    return -1;
}

int HandleRFindNode(Client *client, Message *message)
{
    assert(client != NULL && "NULL Client pointer");
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RFindNode && "Wrong message type");
    assert(message->context != NULL && "NULL message context");

    Search *search = (Search *)message->context;
    check(search != NULL, "Missing Search context");

    int rc = Table_MarkReply(client->table, message);
    check(rc == 0, "Table_MarkReply failed (client->table)");

    rc = Table_MarkReply(search->table, message);
    check(rc == 0, "Table_MarkReply failed (search->table)");

    rc = AddSearchNodes(client,
                        search,
                        message->data.rfindnode.nodes,
                        message->data.rfindnode.count);
    check(rc == 0, "AddSearchNodes failed");

    /* Free nodes not added to search */
    Node_DestroyBlock(message->data.rfindnode.nodes,
                      message->data.rfindnode.count);

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

    int rc = Table_MarkReply(client->table, message);
    check(rc == 0, "Table_MarkReply failed (client->table)");

    rc = Table_MarkReply(search->table, message);
    check(rc == 0, "Table_MarkReply failed (search->table)");

    rc = Search_SetToken(search, &message->id, data->token);
    check(rc == 0, "Search_SetToken failed");

    if (data->nodes != NULL)
    {
        rc = AddSearchNodes(client, search, data->nodes, data->count);
        check(rc == 0, "AddSearchNodes failed");

        /* Free nodes not added to search */
        Node_DestroyBlock(data->nodes, data->count);
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

/* AddSearchNodes NULLs the added nodes. */
int AddSearchNodes(Client *client, Search *search, Node **nodes, size_t count)
{
    assert(search != NULL && "NULL Search pointer");
    assert(nodes != NULL && "NULL pointer to Nodes pointer");

    Node **node = nodes;
    Node **end = node + count;

    while (node < end)
    {
        if (client->node.addr.s_addr == (*node)->addr.s_addr
            && client->node.port == (*node)->port)
        {
            /* Don't add ourselves */
            node++;
            continue;
        }

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

    DArray *found = NULL;

    int rc = Table_MarkQuery(client->table, &query->node);
    check(rc == 0, "Table_MarkQuery failed");

    found = Table_GatherClosest(client->table,
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

    int rc = Table_MarkQuery(client->table, &query->node);
    check(rc == 0, "Table_MarkQuery failed");

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

    int rc = Table_MarkQuery(client->table, &query->node);
    check(rc == 0, "Table_MarkQuery failed");

    if (!Client_IsValidToken(client,
                             &query->node,
                             query->data.qannouncepeer.token.data,
                             query->data.qannouncepeer.token.len))
    {
        Message *error = Message_CreateRErrorBadToken(client, query);
        check(error != NULL, "Message_CreateRErrorBadToken failed");
        return error;
    }

    Peer peer = { .addr = query->node.addr.s_addr,
                  .port = query->data.qannouncepeer.port };

    rc = Client_AddPeer(client, query->data.qannouncepeer.info_hash, &peer);
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

    DArray *peers = NULL;
    DArray *nodes = NULL;

    int rc = Table_MarkQuery(client->table, &query->node);
    check(rc == 0, "Table_MarkQuery failed");

    rc = Client_GetPeers(client, query->data.qgetpeers.info_hash, &peers);
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

Message *HandleInvalidQuery(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(MessageType_IsQuery(query->type) && "Not a query");
    assert(query->errors && "No errors in query");

    Client_MarkInvalidMessage(client, &query->node);

    Message *reply = Message_CreateRError(client, query);
    check(reply != NULL, "Message_CreateRError failed");

    return reply;
error:
    return NULL;
}

int HandleInvalidReply(Client *client, Message *reply)
{
    assert(client != NULL && "NULL Client pointer");
    assert(reply != NULL && "NULL Message pointer");
    assert(MessageType_IsReply(reply->type) && "Not a reply");
    assert(reply->errors && "No errors in reply");

    int rc = Client_MarkInvalidMessage(client, &reply->node);
    check(rc == 0, "Client_MarkInvalidMessage failed");

    rc = Table_MarkReply(client->table, reply);
    check(rc == 0, "Table_MarkReply failed");

    return 0;
error:
    return -1;
}

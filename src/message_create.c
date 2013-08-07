#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQuery(Client *client, Node *to, MessageType type)
{
    assert(client != NULL && "NULL Client pointer");
    assert(MessageType_IsQuery(type) && "MessageType not a query");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    message->type = type;
    message->node = *to;

    message->t = malloc(sizeof(tid_t));
    check_mem(message->t);
    *(tid_t *)message->t = client->next_t++;
    message->t_len = sizeof(tid_t);

    message->id = client->node.id;

    return message;
error:
    return NULL;
}    

Message *Message_CreateQPing(Client *client, Node *to)
{
    return Message_CreateQuery(client, to, QPing);
}

Message *Message_CreateQFindNode(Client *client, Node *to, Hash *id)
{
    assert(client != NULL && "NULL Client pointer");
    assert(id != NULL && "NULL Hash pointer");

    Message *message = Message_CreateQuery(client, to, QFindNode);
    check(message != NULL, "Message_Create failed");

    message->data.qfindnode.target = malloc(HASH_BYTES);
    check_mem(message->data.qfindnode.target);

    memcpy(message->data.qfindnode.target, id->value, HASH_BYTES);

    return message;
error:
    return NULL;
}

Message *Message_CreateQGetPeers(Client *client, Node *to, Hash *info_hash)
{
    assert(client != NULL && "NULL Client pointer");
    assert(info_hash != NULL && "NULL Hash pointer");

    Message *message = Message_CreateQuery(client, to, QGetPeers);
    check(message != NULL, "Message_Create failed");

    message->data.qgetpeers.info_hash = malloc(HASH_BYTES);
    check_mem(message->data.qgetpeers.info_hash);

    memcpy(message->data.qgetpeers.info_hash, info_hash->value, HASH_BYTES);

    return message;
error:
    return NULL;
}

Message *Message_CreateQAnnouncePeer(Client *client,
                                     Node *to,
                                     Hash *info_hash,
                                     Token *token)
{
    assert(client != NULL && "NULL Client pointer");
    assert(info_hash != NULL && "NULL Hash pointer");
    assert(token != NULL && "NULL Token pointer");

    QAnnouncePeerData data = { 0 };

    Message *message = Message_CreateQuery(client, to, QAnnouncePeer);
    check(message != NULL, "Message_Create failed");

    data.info_hash = malloc(HASH_BYTES);
    check_mem(data.info_hash);

    memcpy(data.info_hash, info_hash->value, HASH_BYTES);

    data.token.data = malloc(HASH_BYTES);
    check_mem(data.token.data);

    memcpy(data.token.data, token->value, HASH_BYTES);
    data.token.len = HASH_BYTES;

    data.port = client->peer_port;

    message->data.qannouncepeer = data;

    return message;
error:
    free(data.info_hash);
    return NULL;
}

Message *Message_CreateResponse(Client *client, Message *query, MessageType type)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(query->t != NULL && "NULL char token pointer in query");
    assert(query->t_len > 0 && "Bad t_len in query");
    assert(!MessageType_IsQuery(type) && "MessageType not a response");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    message->type = type;
    message->node = query->node;

    message->t_len = query->t_len;
    message->t = malloc(query->t_len);
    check_mem(message->t);
    memcpy(message->t, query->t, query->t_len);

    message->id = client->node.id;

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRFindNode(Client *client, Message *query, DArray *found)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(found != NULL && "NULL DArray pointer");

    Message *message = Message_CreateResponse(client, query, RFindNode);
    check(message != NULL, "Message_Create failed");

    RFindNodeData data;
    data.count = DArray_count(found);
    data.nodes = malloc(data.count * sizeof(Node *));

    unsigned int i = 0;
    for (i = 0; i < data.count; i++)
        data.nodes[i] = DArray_get(found, i);

    message->data.rfindnode = data;

    return message;
error:
    return NULL;
}

Message *Message_CreateRPing(Client *client, Message *query)
{
    return Message_CreateResponse(client, query, RPing);
}

Message *Message_CreateRAnnouncePeer(Client *client, Message *query)
{
    return Message_CreateResponse(client, query, RAnnouncePeer);
}

/* This does not copy the found nodes, so the message must be sent before
 * they can be destroyed. */
Message *Message_CreateRGetPeers(Client *client,
                                 Message *query,
                                 DArray *peers,
                                 DArray *nodes, 
                                 Token *token)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");
    assert(token != NULL && "NULL Token pointer");

    RGetPeersData data = { 0 };

    check(peers != NULL || nodes != NULL, "Neither peers nor nodes in RGetPeers");
    check(peers == NULL || nodes == NULL, "Both peers and nodes in RGetPeers");

    Message *message = Message_CreateResponse(client, query, RGetPeers);
    check(message != NULL, "Message_Create failed");

    data.token.data = malloc(HASH_BYTES);
    check_mem(data.token.data);
    memcpy(data.token.data, token->value, HASH_BYTES);
    data.token.len = HASH_BYTES;

    unsigned int i = 0;

    if (peers != NULL)
    {
        data.count = DArray_count(peers);
        data.values = malloc(sizeof(Peer) * data.count);
        check_mem(data.values);

        for (i = 0; i < data.count; i++)
        {
            data.values[i] = *(Peer *)DArray_get(peers, i);
        }
    }
    else
    {
        data.count = DArray_count(nodes);
        data.nodes = malloc(data.count * sizeof(Node *));
        check_mem(data.nodes);

        for (i = 0; i < data.count; i++)
        {
            data.nodes[i] = DArray_get(nodes, i);
        }
    }

    message->data.rgetpeers = data;

    return message;
error:
    free(data.token.data);
    return NULL;
}

Message *Message_CreateRErrorBadToken(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query != NULL && "NULL Message pointer");

    Message *message = Message_CreateResponse(client, query, RError);
    check(message != NULL, "Message_CreateResponse failed");

    message->data.rerror.code = RERROR_PROTOCOL;
    message->data.rerror.message = bfromcstr("Bad token");
    check_mem(message->data.rerror.message);

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

struct ErrorMessage {
    merror_t errors;
    int code;
    char *message;
};

struct ErrorMessage Errors[] = {
    { MERROR_UNKNOWN_TYPE, RERROR_PROTOCOL, "Bad message type" },
    { MERROR_INVALID_QUERY_TYPE, RERROR_METHODUNKNOWN, "Unknown method" },
    { MERROR_INVALID_TID, RERROR_PROTOCOL, "Bad transaction id" },
    { MERROR_INVALID_NODE_ID, RERROR_PROTOCOL, "Bad node id" },
    { MERROR_INVALID_DATA, RERROR_PROTOCOL, "Bad data" },
    { MERROR_PROGRAM, RERROR_SERVER, "Oops" },
    { ~0, RERROR_GENERIC, "GENERAL ERROR" }
};

Message *Message_CreateRError(Client *client, Message *query)
{
    assert(client != NULL && "NULL Client pointer");
    assert(query->errors && "No errors in query");

    Message *message = Message_CreateResponse(client, query, RError);
    check(message != NULL, "Message_CreateResponse failed");

    struct ErrorMessage *error = Errors;

    while (!(error->errors & query->errors))
    {
        error++;
    }

    message->data.rerror.code = error->code;
    message->data.rerror.message = bfromcstr(error->message);
    check_mem(message->data.rerror.message);

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

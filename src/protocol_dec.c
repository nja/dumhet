#include <arpa/inet.h>
#include <assert.h>

#include <dht/bencode.h>
#include <dht/message.h>
#include <dht/protocol.h>
#include <lcthw/dbg.h>

char GetMessageType(BNode *dict);

int DecodeQuery(Message *message, BNode *dict);
int DecodeResponse(Message *message, BNode *dict, struct PendingResponses *pending);
int DecodeError(Message *message, BNode *dict);

Message *Message_Decode(char *data, size_t len, struct PendingResponses *pending)
{
    assert(data != NULL && "NULL data pointer");

    BNode *dict = NULL;
    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    dict = BDecode(data, len);

    if (dict == NULL || dict->type != BDictionary)
    {
        BNode_Destroy(dict);
        message->type = MUnknown;
        message->errors |= MERROR_UNKNOWN_TYPE;
        return message;
    }

    char y = GetMessageType(dict);

    int rc = 0;

    switch(y)
    {
    case 'q':
        rc = DecodeQuery(message, dict);
        check(rc == 0, "DecodeQuery failed");
	break;
    case 'r':
        rc = DecodeResponse(message, dict, pending);
        check(rc == 0, "DecodeResponse failed");
	break;
    case 'e':
        rc = DecodeError(message, dict);
        check(rc == 0, "DecodeError failed");
	break;
    default:
        message->errors |= MERROR_UNKNOWN_TYPE;
	break;
    }

    BNode_Destroy(dict);

    return message;
error:
    BNode_Destroy(dict);
    return NULL;
}

char GetMessageType(BNode *dict)
{
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a BDictionary");

    BNode *yVal = NULL;

    yVal = BNode_GetValue(dict, "y", 1);

    if (yVal == NULL || yVal->type != BString || yVal->count != 1)
    {
        return 0;
    }

    return (char)*yVal->value.string;
}    

void SetQueryType(Message *message, BNode *dict);
int SetTransactionId(Message *message, BNode *dict);
void SetQueryId(Message *message, BNode *dict);
int SetQueryData(Message *message, BNode *dict);

int DecodeQuery(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    SetQueryType(message, dict);
    SetQueryId(message, dict);

    int rc = SetTransactionId(message, dict);
    check(rc == 0, "SetTransactionId failed");

    rc = SetQueryData(message, dict);
    check(rc == 0, "SetQueryData failed");

    return 0;
error:
    return -1;
}

void SetQueryType(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    BNode *qVal = BNode_GetValue(dict, "q", 1);

    if (qVal == NULL)
    {
        goto invalid;
    }

    if (BNode_StringEquals("ping", qVal))
    {
	message->type = QPing;
	return;
    }

    if (BNode_StringEquals("find_node", qVal))
    {
	message->type = QFindNode;
	return;
    }

    if (BNode_StringEquals("get_peers", qVal))
    {
	message->type = QGetPeers;
	return;
    }

    if (BNode_StringEquals("announce_peer", qVal))
    {
	message->type = QAnnouncePeer;
	return;
    }

invalid:
    message->type = MUnknown;
    message->errors |= MERROR_INVALID_QUERY_TYPE;
    return;
}    

int SetTransactionId(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    BNode *tVal = BNode_GetValue(dict, "t", 1);

    if (tVal == NULL || tVal->type != BString)
    {
        message->t = NULL;
        message->t_len = 0;
        message->errors |= MERROR_INVALID_TID;

        return 0;
    }

    message->t = BNode_CopyString(tVal);
    check(message->t != NULL, "BString copy failed");
    message->t_len = tVal->count;

    return 0;
error:
    return -1;
}

void SetQueryId(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    BNode *arguments = BNode_GetValue(dict, "a", 1);

    if (arguments == NULL || arguments->type != BDictionary)
    {
        message->errors |= MERROR_INVALID_NODE_ID;
        return;
    }

    BNode *idVal = BNode_GetValue(arguments, "id", 2);

    if (idVal == NULL || idVal->type != BString || idVal->count != HASH_BYTES)
    {
        message->errors |= MERROR_INVALID_NODE_ID;
        return;
    }

    memcpy(message->id.value, idVal->value.string, HASH_BYTES);

    return;
}

int SetQueryFindNodeData(Message *message, BNode *arguments);
int SetQueryGetPeersData(Message *message, BNode *arguments);
int SetQueryAnnouncePeerData(Message *message, BNode *arguments);

int SetQueryData(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(dict->type == BDictionary && "Not a dictionary");
    assert(MessageType_IsQuery(message->type) && "Not a query");

    if (message->errors)
    {
        return 0;
    }

    if (message->type == QPing)
    {
	return 0;
    }

    BNode *arguments = BNode_GetValue(dict, "a", 1);

    if (arguments == NULL || arguments->type != BDictionary)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    switch (message->type)
    {
    case QFindNode:
	return SetQueryFindNodeData(message, arguments);
        break;
    case QGetPeers:
	return SetQueryGetPeersData(message, arguments);
        break;
    case QAnnouncePeer:
	return SetQueryAnnouncePeerData(message, arguments);
        break;
    default:
        break;
    }

    log_err("Unhandled Query MessageType");
    message->errors |= MERROR_INVALID_DATA;
    return -1;
}

int SetQueryFindNodeData(Message *message, BNode *arguments)
{
    assert(message != NULL && "NULL Message pointer");
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(arguments->type == BDictionary && "Not a dictionary");

    BNode *target = BNode_GetValue(arguments, "target", 6);

    if (target == NULL || target->type != BString || target->count != HASH_BYTES)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    QFindNodeData *data = &message->data.qfindnode;
    data->target = malloc(HASH_BYTES);
    check_mem(data->target);

    memcpy(data->target->value, target->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int IsInfoHashNode(BNode *node)
{
    return node != NULL
        && node->type == BString
        && node->count == HASH_BYTES;
}

int SetQueryGetPeersData(Message *message, BNode *arguments)
{
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == QGetPeers && "Wrong Message type");
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(arguments->type == BDictionary && "Not a dictionary");

    BNode *info_hash = BNode_GetValue(arguments, "info_hash", 9);

    if (!IsInfoHashNode(info_hash))
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    QGetPeersData *data = &message->data.qgetpeers;

    data->info_hash = malloc(HASH_BYTES);
    check_mem(data->info_hash);

    memcpy(data->info_hash->value, info_hash->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int SetQueryAnnouncePeerData(Message *message, BNode *arguments)
{
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == QAnnouncePeer && "Wrong Message type");
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(arguments->type == BDictionary && "Not a dictionary");

    BNode *info_hash = BNode_GetValue(arguments, "info_hash", 9);

    if (!IsInfoHashNode(info_hash))
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    BNode *port = BNode_GetValue(arguments, "port", 4);

    if (port == NULL
        || port->type != BInteger
        || port->value.integer < 0
        || port->value.integer > 0xffff)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    BNode *token = BNode_GetValue(arguments, "token", 5);

    if (token == NULL || token->type != BString)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    QAnnouncePeerData *data = &message->data.qannouncepeer;

    data->info_hash = malloc(HASH_BYTES);
    check_mem(data->info_hash);

    memcpy(data->info_hash->value, info_hash->value.string, HASH_BYTES);

    data->port = port->value.integer;

    data->token.data = BNode_CopyString(token);
    check(data->token.data != NULL, "Failed to copy token");

    data->token.len = token->count;

    return 0;
error:
    Hash_Destroy(data->info_hash);

    return -1;
}

int SetResponseData(Message *message, BNode *dict);
void SetResponseId(Message *message, BNode *dict);

int DecodeResponse(Message *message, BNode *dict, struct PendingResponses *pending)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a dictionary");
    assert(pending != NULL && "NULL struct PendingResponses pointer");

    int rc = SetTransactionId(message, dict);
    check(rc == 0, "SetTransactionId failed");

    if (message->t_len != sizeof(tid_t))
    {
        message->errors |= MERROR_INVALID_TID;
    }

    SetResponseId(message, dict);

    PendingResponse entry = pending->getPendingResponse(pending, message->t, &rc);

    if (rc == 0)
    {
        if (!Hash_Equals(&message->id, &entry.id))
        {
            message->errors |= MERROR_INVALID_NODE_ID;
        }

        message->type = entry.type;
        message->context = entry.context;
    }
    else
    {
        message->errors |= MERROR_INVALID_TID;
    }

    rc = SetResponseData(message, dict);
    check(rc == 0, "SetResponseData failed");

    return 0;
error:
    return -1;
}

void SetResponseId(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    BNode *arguments = BNode_GetValue(dict, "r", 1);

    if (arguments == NULL || arguments->type != BDictionary)
    {
        message->errors |= MERROR_INVALID_NODE_ID;
        return;
    }

    BNode *idVal = BNode_GetValue(arguments, "id", 2);

    if (idVal == NULL || idVal->type != BString || idVal->count != HASH_BYTES)
    {
        message->errors |= MERROR_INVALID_NODE_ID;
        return;
    }

    memcpy(message->id.value, idVal->value.string, HASH_BYTES);

    return;
}

int SetResponseFindNodeData(Message *message, BNode *arguments);
int SetResponseGetPeersData(Message *message, BNode *arguments);

int SetResponseData(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(MessageType_IsReply(message->type) && "Not a reply");
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    if (message->type == RPing || message->type == RAnnouncePeer)
    {
        message->errors |= MERROR_INVALID_DATA;
	return 0;
    }

    BNode *arguments = BNode_GetValue(dict, "r", 1);

    if (arguments == NULL || arguments->type != BDictionary)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    switch (message->type)
    {
    case RFindNode:
	return SetResponseFindNodeData(message, arguments);
    case RGetPeers:
	return SetResponseGetPeersData(message, arguments);
    default:
        log_err("Unhandled response message type %d", message->type);
        return -1;
    }
}

int SetCompactNodeInfo(Message *message, BNode *string);

int SetResponseFindNodeData(Message *message, BNode *arguments)
{
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RFindNode && "Wrong message type");
    assert(arguments != NULL && "NULL BNode pointer");
    assert(arguments->type == BDictionary && "Not a dictionary");

    BNode *nodes = BNode_GetValue(arguments, "nodes", 5);

    if (nodes == NULL || nodes->type != BString)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    return SetCompactNodeInfo(message, nodes);
}

#define COMPACTNODE_BYTES (HASH_BYTES + sizeof(uint32_t) + sizeof(uint16_t))

int SetCompactNodeInfo(Message *message, BNode *string)
{
    assert(message != NULL && "NULL Message pointer");
    assert((message->type == RFindNode || message->type == RGetPeers)
           && "Wrong message type");
    assert(string != NULL && "NULL BNode string pointer");
    assert(string->type == BString && "Not a BString");

    if (string->count % COMPACTNODE_BYTES != 0)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    RFindNodeData *data = &message->data.rfindnode;
    char *nodes = string->value.string;
    
    data->count = string->count / COMPACTNODE_BYTES;
    data->nodes = calloc(data->count, sizeof(Node *));
    check_mem(data->nodes);

    unsigned int i = 0;
    for (i = 0; i < data->count; i++, nodes += COMPACTNODE_BYTES)
    {
        data->nodes[i] = Node_Create((Hash *)nodes);
        check_mem(data->nodes[i]);
        data->nodes[i]->addr.s_addr = ntohl(*(uint32_t *)(nodes
                                                         + HASH_BYTES));
	data->nodes[i]->port = ntohs(*(uint16_t *)(nodes
                                                  + HASH_BYTES
                                                  + sizeof(uint32_t)));
    }

    return 0;
error:
    if (data->nodes != NULL)
    {
        Node_DestroyBlock(data->nodes, string->count);
        free(nodes);
        data->nodes = NULL;
    }

    data->count = 0;

    return -1;
}

int SetCompactPeerInfo(Message *message, BNode *list);

int SetResponseGetPeersData(Message *message, BNode *arguments)
{
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RGetPeers && "Wrong message type");
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(arguments->type == BDictionary && "Not a dictionary");

    BNode *token = BNode_GetValue(arguments, "token", 5);

    if (token == NULL || token->type != BString)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    RGetPeersData *data = &message->data.rgetpeers;

    data->token.data = BNode_CopyString(token);
    check(data->token.data != NULL, "Failed to copy token");

    data->token.len = token->count;

    data->nodes = NULL;
    data->values = NULL;
    data->count = 0;

    /* TODO: BNode_TryGetValue */
    BNode *values = BNode_GetValue(arguments, "values", 6);
    BNode *nodes = BNode_GetValue(arguments, "nodes", 5);

    if (values != NULL && nodes != NULL)
    {
        message->errors |= MERROR_INVALID_DATA;
    }
    else if (values != NULL && values->type == BList)
    {
	return SetCompactPeerInfo(message, values);
    }
    else if (nodes != NULL && nodes->type == BString)
    {
        int rc = SetCompactNodeInfo(message, nodes);
	check(rc == 0, "Failed to get compact node info");
    }
    else
    {
	message->errors |= MERROR_INVALID_DATA;
    }

    return 0;
error:
    free(data->token.data);
    data->token.data = NULL;

    return -1;
}

#define COMPACTPEER_BYTES (sizeof(uint32_t) + sizeof(uint16_t))

int SetCompactPeerInfo(Message *message, BNode *list)
{
    assert(message != NULL && "NULL Message pointer");
    assert(message->type == RGetPeers && "Wrong Message type");
    assert(list != NULL && "NULL BNode string pointer");
    assert(list->type == BList && "Not a BList");

    RGetPeersData *data = &message->data.rgetpeers;

    data->count = list->count;
    data->values = calloc(data->count, sizeof(Peer));
    check_mem(data->values);

    Peer *peer = data->values;
    size_t i = 0;

    for (i = 0; i < data->count; i++, peer++)
    {
	BNode *string = list->value.nodes[i];

        if (string->type != BString || string->count != COMPACTPEER_BYTES)
        {
            message->errors |= MERROR_INVALID_DATA;
            return 0;
        }

	peer->addr = ntohl(*(uint32_t *)string->value.string);
	peer->port = ntohs(*(uint16_t *)(string->value.string + sizeof(uint32_t)));
    }

    return 0;
error:
    free(data->values);
    data->values = NULL;
    data->count = 0;

    return -1;
}

int DecodeError(Message *message, BNode *dict)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Not a dictionary");

    message->type = RError;

    int rc = SetTransactionId(message, dict);
    check(rc == 0, "SetTransactionId failed");

    BNode *eVal = BNode_GetValue(dict, "e", 1);
    check(eVal != NULL, "No 'e' value");
    check(eVal->type == BList, "Value not a BList");

    if (eVal == NULL || eVal->type != BList || eVal->count != 2)
    {
        message->errors |= MERROR_INVALID_DATA;
        return 0;
    }

    BNode *code = eVal->value.nodes[0],
        *error_msg = eVal->value.nodes[1];

    if (code->type == BInteger)
    {
        message->data.rerror.code = code->value.integer;
    }
    else
    {
        message->errors |= MERROR_INVALID_DATA;
    }

    if (error_msg->type == BString)
    {
        message->data.rerror.message = BNode_bstring(error_msg);
        check(message->data.rerror.message != NULL, "Failed to create bstring");
    }
    else
    {
        message->errors |= MERROR_INVALID_DATA;
    }

    return 0;
error:
    return -1;
}

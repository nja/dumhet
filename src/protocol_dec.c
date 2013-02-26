#include <arpa/inet.h>
#include <assert.h>

#include <dht/bencode.h>
#include <dht/message.h>
#include <dht/protocol.h>
#include <lcthw/dbg.h>

int HasTransactionId(BNode *dict);
int HasNodeId(BNode *dict);
char GetMessageType(BNode *dict);

Message *DecodeQuery(BNode *dict);
Message *DecodeResponse(BNode *dict, struct PendingResponses *pending);
Message *DecodeError(BNode *dict);

Message *Message_Decode(char *data, size_t len, struct PendingResponses *pending)
{
    assert(data != NULL && "NULL data pointer");

    Message *message = NULL;
    BNode *dict = BDecode(data, len);

    check(dict != NULL, "BDecode failed");
    check(dict->type == BDictionary, "Wrong bencode type: %s", BType_Name(dict->type));

    check(HasTransactionId(dict), "Message missing transaction id");

    char y = GetMessageType(dict);

    switch(y)
    {
    case 'q': message = DecodeQuery(dict);
	break;
    case 'r': message = DecodeResponse(dict, pending);
	break;
    case 'e': message = DecodeError(dict);
	break;
    default: sentinel("Invalid or missing message type");
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

    BNode *yVal = NULL;

    check(dict->type == BDictionary, "Not a dictionary");

    yVal = BNode_GetValue(dict, "y", 1);

    check(yVal != NULL, "No 'y' key");
    check(yVal->type == BString, "Wrong 'y' value type");
    check(yVal->count == 1, "Bad 'y' value length");

    return (char)*yVal->value.string;

error:
    return '\0';
}    

int HasTransactionId(BNode *dict)
{
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Wrong BNode type");

    BNode *tValue = BNode_GetValue(dict, "t", 1);

    return tValue != NULL
	&& tValue->type == BString
	&& tValue->count > 0;
}    

int HasNodeId(BNode *dict)
{
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Wrong BNode type");

    BNode *arguments = BNode_GetValue(dict, "a", 1);

    if (arguments == NULL || arguments->type != BDictionary)
	return 0;

    BNode *id = BNode_GetValue(arguments, "id", 2);

    return id != NULL
	&& id->type == BString
	&& id->count == 20;
}

int GetQueryType(BNode *dict, MessageType *type);
int GetTransactionId(BNode *dict, char **t, size_t *t_len);
int GetQueryId(BNode *dict, DhtHash **id);
int GetQueryData(MessageType type, BNode *dict, Message *message);

Message *DecodeQuery(BNode *dict)
{
    assert(dict != NULL && "NULL BNode pointer");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    int rc = 0;

    check(dict->type == BDictionary, "Not a dictionary");

    rc = GetQueryType(dict, &message->type);
    check(rc == 0, "Bad query type");

    rc = GetTransactionId(dict, &message->t, &message->t_len);
    check(rc == 0, "Bad transaction id");

    rc = GetQueryId(dict, &message->id);
    check(rc == 0, "Bad query node id");

    rc = GetQueryData(message->type, dict, message);
    check(rc == 0, "Bad query arguments");

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

int GetQueryType(BNode *dict, MessageType *type)
{
    assert(dict != NULL && "NULL BNode pointer");
    assert(type != NULL && "NULL MessageType pointer");

    check(dict->type == BDictionary, "Not a dictionary");

    BNode *qVal = BNode_GetValue(dict, "q", 1);
    check(qVal != NULL, "No 'q' value");
    check(qVal->type == BString, "Value not a BString");

    if (BNode_StringEquals("ping", qVal))
    {
	*type = QPing;
	return 0;
    }

    if (BNode_StringEquals("find_node", qVal))
    {
	*type = QFindNode;
	return 0;
    }

    if (BNode_StringEquals("get_peers", qVal))
    {
	*type = QGetPeers;
	return 0;
    }

    if (BNode_StringEquals("announce_peer", qVal))
    {
	*type = QAnnouncePeer;
	return 0;
    }

error:
    return -1;
}    

int GetTransactionId(BNode *dict, char **t, size_t *t_len)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(t != NULL && "NULL pointer to transaction id pointer");
    assert(t_len != NULL && "NULL pointer to transaction id length");

    check(dict->type == BDictionary, "Not a dictionary");

    BNode *tVal = BNode_GetValue(dict, "t", 1);
    check(tVal != NULL, "No 't' value");

    char *tmp = BNode_CopyString(tVal);
    check(tmp != NULL, "BString copy failed");

    *t = tmp;
    *t_len = tVal->count;

    return 0;
error:
    return -1;
}

int GetQueryId(BNode *dict, DhtHash **id)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(id != NULL && "NULL pointer to DhtHash pointer");

    check(dict->type == BDictionary, "Not a dictionary");

    BNode *arguments = BNode_GetValue(dict, "a", 1);
    check(arguments != NULL, "No arguments");
    check(arguments->type == BDictionary, "Arguments not a dictionary");

    BNode *idVal = BNode_GetValue(arguments, "id", 2);
    check(idVal != NULL, "No id argument");
    check(idVal->type == BString, "Wrong id type");
    check(idVal->count == HASH_BYTES, "Wrong id length");

    *id = malloc(HASH_BYTES);
    check_mem(id);

    memcpy(&((*id)->value), idVal->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int GetQueryFindNodeData(BNode *arguments, QFindNodeData *data);
int GetQueryGetPeersData(BNode *arguments, QGetPeersData *data);
int GetQueryAnnouncePeerData(BNode *arguments, QAnnouncePeerData *data);

int GetQueryData(MessageType type, BNode *dict, Message *message)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    if (type == QPing)
	return 0;

    BNode *arguments = BNode_GetValue(dict, "a", 1);

    if (arguments == NULL || arguments->type != BDictionary)
	return -1;

    switch (type)
    {
    case QFindNode:
	return GetQueryFindNodeData(arguments, &message->data.qfindnode);
    case QGetPeers:
	return GetQueryGetPeersData(arguments, &message->data.qgetpeers);
    case QAnnouncePeer:
	return GetQueryAnnouncePeerData(arguments, &message->data.qannouncepeer);
    default:
	log_err("Bad query MessageType");
	return -1;
    }
}

int GetQueryFindNodeData(BNode *arguments, QFindNodeData *data)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(data != NULL && "NULL QFindNodeData pointer");

    BNode *target = BNode_GetValue(arguments, "target", 6);

    check(target != NULL
	  && target->type == BString
	  && target->count == HASH_BYTES,
	  "Missing or bad target id");

    data->target = malloc(HASH_BYTES);
    check_mem(data->target);

    memcpy(data->target->value, target->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int GetQueryGetPeersData(BNode *arguments, QGetPeersData *data)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(data != NULL && "NULL QGetPeersData pointer");

    BNode *info_hash = BNode_GetValue(arguments, "info_hash", 9);

    check(info_hash != NULL
	  && info_hash->type == BString
	  && info_hash->count == HASH_BYTES,
	  "Missing or bad info_hash id");

    data->info_hash = malloc(HASH_BYTES);
    check_mem(data->info_hash);

    memcpy(data->info_hash->value, info_hash->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int GetQueryAnnouncePeerData(BNode *arguments, QAnnouncePeerData *data)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(data != NULL && "NULL QAnnouncePeerData pointer");

    data->info_hash = NULL;

    BNode *info_hash = BNode_GetValue(arguments, "info_hash", 9);

    check(info_hash != NULL
	  && info_hash->type == BString
	  && info_hash->count == HASH_BYTES,
	  "Missing or bad info_hash id");

    BNode *port = BNode_GetValue(arguments, "port", 4);

    check(port != NULL
	  && port->type == BInteger
	  && port->value.integer >= 0
	  && port->value.integer <= 0xffff,
	  "Missing or bad port");

    BNode *token = BNode_GetValue(arguments, "token", 5);

    check(token != NULL
	  && token->type == BString,
	  "Missing or bad token");

    data->info_hash = malloc(HASH_BYTES);
    check_mem(data->info_hash);

    memcpy(data->info_hash->value, info_hash->value.string, HASH_BYTES);

    data->port = port->value.integer;

    data->token = BNode_CopyString(token);
    check(data->token != NULL, "Failed to copy token");

    data->token_len = token->count;

    return 0;
error:
    DhtHash_Destroy(data->info_hash);

    return -1;
}

int GetResponseData(MessageType type, BNode *dict, Message *message);
int GetResponseId(BNode *dict, DhtHash **id);

Message *DecodeResponse(BNode *dict, struct PendingResponses *pending)
{
    assert(dict != NULL && "NULL BNode pointer");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    int rc = 0;

    check(dict->type == BDictionary, "Not a dictionary");

    rc = GetTransactionId(dict, &message->t, &message->t_len);
    check(rc == 0, "Bad transaction id");
    check(message->t_len == sizeof(tid_t), "Bad response transaction id length");

    rc = GetResponseId(dict, &message->id);
    check(rc == 0, "Bad response node id");

    rc = pending->getResponseType(pending, message->t, &message->type);
    check(rc == 0, "Bad response type");

    rc = GetResponseData(message->type, dict, message);
    check(rc == 0, "Bad response arguments");

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

int GetResponseId(BNode *dict, DhtHash **id)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(id != NULL && "NULL pointer to DhtHash pointer");

    check(dict->type == BDictionary, "Not a dictionary");

    BNode *arguments = BNode_GetValue(dict, "r", 1);
    check(arguments != NULL, "No response arguments");
    check(arguments->type == BDictionary, "Arguments not a dictionary");

    BNode *idVal = BNode_GetValue(arguments, "id", 2);
    check(idVal != NULL, "No id argument");
    check(idVal->type == BString, "Wrong id type");
    check(idVal->count == HASH_BYTES, "Wrong id length");

    *id = malloc(HASH_BYTES);
    check_mem(id);

    memcpy(&((*id)->value), idVal->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int GetResponseFindNodeData(BNode *arguments, RFindNodeData *data);
int GetResponseGetPeersData(BNode *arguments, RGetPeersData *data);

int GetResponseData(MessageType type, BNode *dict, Message *message)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    if (type == RPing || type == RAnnouncePeer)
	return 0;

    BNode *arguments = BNode_GetValue(dict, "r", 1);

    if (arguments == NULL || arguments->type != BDictionary)
	return -1;

    switch (type)
    {
    case RFindNode:
	return GetResponseFindNodeData(arguments, &message->data.rfindnode);
    case RGetPeers:
	return GetResponseGetPeersData(arguments, &message->data.rgetpeers);
    default: return -1;
    }
}

int GetCompactNodeInfo(BNode *string, DhtNode **nodes, size_t *count);

int GetResponseFindNodeData(BNode *arguments, RFindNodeData *data)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(data != NULL && "NULL RFindNodeData pointer");

    BNode *nodes = BNode_GetValue(arguments, "nodes", 5);

    check(nodes != NULL, "Missing nodes");

    int rc = GetCompactNodeInfo(nodes,
				&data->nodes,
				&data->count);
    check(rc == 0, "Error reading compact node info");

    return 0;
error:
    return -1;
}

#define COMPACTNODE_BYTES (HASH_BYTES + sizeof(uint32_t) + sizeof(uint16_t))

int GetCompactNodeInfo(BNode *string, DhtNode **nodes, size_t *count)
{
    assert(string != NULL && "NULL BNode string pointer");
    assert(nodes != NULL && "NULL pointer to DhtNode pointer");
    assert(count != NULL && "NULL pointer to size_t count");

    check(string->type == BString, "Not a BString");
    check(string->count % COMPACTNODE_BYTES == 0, "Bad compact node info length");
    
    *count = string->count / COMPACTNODE_BYTES;
    *nodes = calloc(*count, sizeof(DhtNode));
    check_mem(*nodes);
    
    DhtNode *node = *nodes;
    char *data = string->value.string;

    while (node < *nodes + *count)
    {
	memcpy(node->id.value, data, HASH_BYTES);
	node->addr.s_addr = ntohl(*(uint32_t *)(data + HASH_BYTES));
	node->port = ntohs(*(uint16_t *)(data + HASH_BYTES + sizeof(uint32_t)));

	data += COMPACTNODE_BYTES;
	node++;
    }

    return 0;
error:
    return -1;
}

int GetCompactPeerInfo(BNode *list, Peer **values, size_t *count);

int GetResponseGetPeersData(BNode *arguments, RGetPeersData *data)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(data != NULL && "NULL RGetPeersData pointer");

    BNode *token = BNode_GetValue(arguments, "token", 5);
    check(token != NULL, "Missing token");
    check(token->type == BString, "Bad token type");

    data->token = BNode_CopyString(token);
    check(data->token != NULL, "Failed to copy token");

    data->token_len = token->count;

    int rc = 0;

    BNode *values = BNode_GetValue(arguments, "values", 6);
    BNode *nodes = BNode_GetValue(arguments, "nodes", 5);

    if (values != NULL && nodes != NULL)
    {
	sentinel("Both values and nodes in response");
    }
    else if (values != NULL)
    {
	rc = GetCompactPeerInfo(values,
				&data->values,
				&data->count);
	check(rc == 0, "Failed to get peer values compact info");
    }
    else if (nodes != NULL)
    {
	rc = GetCompactNodeInfo(nodes,
				&data->nodes,
				&data->count);
	check(rc == 0, "Failed to get compact node info");
    }
    else
    {
	sentinel("Missing values or nodes");
    }

    return 0;
error:
    free(data->token);
    data->token = NULL;

    free(data->values);
    data->values = NULL;

    if (data->nodes != NULL)
	DhtNode_DestroyBlock(data->nodes, data->count);
    data->nodes = NULL;

    data->count = 0;

    return -1;
}

#define COMPACTPEER_BYTES (sizeof(uint32_t) + sizeof(uint16_t))

int GetCompactPeerInfo(BNode *list, Peer **peers, size_t *count)
{
    assert(list != NULL && "NULL BNode string pointer");
    assert(peers != NULL && "NULL pointer to Peer pointer");
    assert(count != NULL && "NULL pointer to size_t count");

    *peers = NULL;

    check(list->type == BList, "Not a BList");

    *count = list->count;
    *peers = calloc(*count, sizeof(Peer));
    check_mem(*peers);

    Peer *peer = *peers;
    size_t i = 0;

    for (i = 0; i < *count; i++)
    {
	BNode *string = list->value.nodes[i];

	check(string->type == BString, "Wrong peer type");
	check(string->count == COMPACTPEER_BYTES, "Bad compact peer info length");
	peer->addr = ntohl(*(uint32_t *)string->value.string);
	peer->port = ntohs(*(uint16_t *)(string->value.string + sizeof(uint32_t)));

	peer++;
    }

    return 0;
error:
    free(*peers);
    *peers = NULL;
    *count = 0;

    return -1;
}

Message *DecodeError(BNode *dict)
{
    assert(dict != NULL && "NULL BNode dict pointer");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    message->type = RError;

    check(dict->type == BDictionary, "Not a dictionary");

    int rc = GetTransactionId(dict, &message->t, &message->t_len);
    check(rc == 0, "Bad transaction id");

    BNode *eVal = BNode_GetValue(dict, "e", 1);
    check(eVal != NULL, "No 'e' value");
    check(eVal->type == BList, "Value not a BList");

    check(eVal->count == 2, "Wrong size 'e' list");

    BNode *code = eVal->value.nodes[0], *error_msg = eVal->value.nodes[1];

    check(code->type == BInteger, "Wrong type in error code position");
    message->data.rerror.code = code->value.integer;

    check(error_msg->type == BString, "Wrong type in error message position");
    message->data.rerror.message = BNode_bstring(error_msg);
    check(message->data.rerror.message != NULL, "Failed to create bstring");

    return message;
error:
    Message_Destroy(message);
    return NULL;
}

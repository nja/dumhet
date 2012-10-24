#include <assert.h>
#include <dbg.h>
#include <protocol.h>
#include <bencode.h>
#include <message.h>

int HasTransactionId(BNode *dict);
int HasNodeId(BNode *dict);
char GetMessageType(BNode *dict);

Message *DecodeQuery(BNode *dict);
Message *DecodeResponse(BNode *dict);
Message *DecodeError(BNode *dict);

Message *Decode(uint8_t *data, size_t len)
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
    case 'r': message = DecodeResponse(dict);
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

    yVal = BNode_GetValue(dict, (uint8_t *)"y", 1);

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

    BNode *tValue = BNode_GetValue(dict, (uint8_t *)"t", 1);

    return tValue != NULL
	&& tValue->type == BString
	&& tValue->count > 0;
}    

int HasNodeId(BNode *dict)
{
    assert(dict != NULL && "NULL BNode pointer");
    assert(dict->type == BDictionary && "Wrong BNode type");

    BNode *arguments = BNode_GetValue(dict, (uint8_t *)"a", 1);

    if (arguments == NULL || arguments->type != BDictionary)
	return 0;

    BNode *id = BNode_GetValue(arguments, (uint8_t *)"id", 2);

    return id != NULL
	&& id->type == BString
	&& id->count == 20;
}

int GetQueryType(BNode *dict, MessageType *type);
int GetTransactionId(BNode *dict, uint8_t **t, size_t *t_len);
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

    BNode *qVal = BNode_GetValue(dict, (uint8_t *)"q", 1);
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

int GetTransactionId(BNode *dict, uint8_t **t, size_t *t_len)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(t != NULL && "NULL pointer to transaction id pointer");
    assert(t_len != NULL && "NULL pointer to transaction id length");

    check(dict->type == BDictionary, "Not a dictionary");

    BNode *tVal = BNode_GetValue(dict, (uint8_t *)"t", 1);
    check(tVal != NULL, "No 't' value");

    uint8_t *tmp = BNode_CopyString(tVal);
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

    BNode *arguments = BNode_GetValue(dict, (uint8_t *)"a", 1);
    check(arguments != NULL, "No arguments");
    check(arguments->type == BDictionary, "Arguments not a dictionary");

    BNode *idVal = BNode_GetValue(arguments, (uint8_t *)"id", 2);
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

int GetQueryFindNodeData(BNode *arguments, Message *message);
int GetQueryGetPeersData(BNode *arguments, Message *message);
int GetQueryAnnouncePeerData(BNode *arguments, Message *message);

int GetQueryData(MessageType type, BNode *dict, Message *message)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    if (type == QPing)
	return 0;

    BNode *arguments = BNode_GetValue(dict, (uint8_t *)"a", 1);

    if (arguments == NULL || arguments->type != BDictionary)
	return -1;

    switch(type)
    {
    case QPing: return 0;
    case QFindNode: return GetQueryFindNodeData(arguments, message);
    case QGetPeers: return GetQueryGetPeersData(arguments, message);
    case QAnnouncePeer: return GetQueryAnnouncePeerData(arguments, message);
    default: return -1;
    }
}

int GetQueryFindNodeData(BNode *arguments, Message *message)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    BNode *target = BNode_GetValue(arguments, (uint8_t *)"target", 6);

    check(target != NULL
	  && target->type == BString
	  && target->count == HASH_BYTES,
	  "Missing or bad target id");

    message->data.qfindnode.target = malloc(HASH_BYTES);
    check_mem(message->data.qfindnode.target);

    memcpy(&message->data.qfindnode.target->value, target->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int GetQueryGetPeersData(BNode *arguments, Message *message)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    BNode *info_hash = BNode_GetValue(arguments, (uint8_t *)"info_hash", 9);

    check(info_hash != NULL
	  && info_hash->type == BString
	  && info_hash->count == HASH_BYTES,
	  "Missing or bad info_hash id");

    message->data.qgetpeers.info_hash = malloc(HASH_BYTES);
    check_mem(message->data.qgetpeers.info_hash);

    memcpy(&message->data.qgetpeers.info_hash->value, info_hash->value.string, HASH_BYTES);

    return 0;
error:
    return -1;
}

int GetQueryAnnouncePeerData(BNode *arguments, Message *message)
{
    assert(arguments != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    message->data.qannouncepeer.info_hash = NULL;

    BNode *info_hash = BNode_GetValue(arguments, (uint8_t *)"info_hash", 9);

    check(info_hash != NULL
	  && info_hash->type == BString
	  && info_hash->count == HASH_BYTES,
	  "Missing or bad info_hash id");

    BNode *port = BNode_GetValue(arguments, (uint8_t *)"port", 4);

    check(port != NULL
	  && port->type == BInteger
	  && port->value.integer > 0,
	  "Missing or bad port");

    BNode *token = BNode_GetValue(arguments, (uint8_t *)"token", 5);

    check(token != NULL
	  && token->type == BString,
	  "Missing or bad token");

    message->data.qannouncepeer.info_hash = malloc(HASH_BYTES);
    check_mem(message->data.qannouncepeer.info_hash);

    memcpy(message->data.qannouncepeer.info_hash->value, info_hash->value.string, HASH_BYTES);

    message->data.qannouncepeer.port = port->value.integer;

    message->data.qannouncepeer.token = BNode_CopyString(token);
    check(message->data.qannouncepeer.token != NULL, "Failed to copy token");

    message->data.qannouncepeer.token_len = token->count;

    return 0;
error:
    DhtHash_Destroy(message->data.qannouncepeer.info_hash);

    return -1;
}

Message *DecodeResponse(BNode *dict)
{
    assert(dict == (BNode *)"TODO");
    return NULL;
}

Message *DecodeError(BNode *dict)
{
    assert(dict == (BNode *)"TODO");
    return NULL;
}

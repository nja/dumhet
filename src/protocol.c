#include <assert.h>
#include <dbg.h>
#include <protocol.h>
#include <bencode.h>
#include <message.h>
#include <arpa/inet.h>

int HasTransactionId(BNode *dict);
int HasNodeId(BNode *dict);
char GetMessageType(BNode *dict);

Message *DecodeQuery(BNode *dict);
Message *DecodeResponse(BNode *dict, GetResponseType_fp getResponseType);
Message *DecodeError(BNode *dict);

Message *Decode(uint8_t *data, size_t len, GetResponseType_fp getResponseType)
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
    case 'r': message = DecodeResponse(dict, getResponseType);
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

int GetQueryFindNodeData(BNode *arguments, QFindNodeData *data);
int GetQueryGetPeersData(BNode *arguments, QGetPeersData *data);
int GetQueryAnnouncePeerData(BNode *arguments, QAnnouncePeerData *data);

int GetQueryData(MessageType type, BNode *dict, Message *message)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    if (type == QPing)
	return 0;

    BNode *arguments = BNode_GetValue(dict, (uint8_t *)"a", 1);

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

    BNode *target = BNode_GetValue(arguments, (uint8_t *)"target", 6);

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

    BNode *info_hash = BNode_GetValue(arguments, (uint8_t *)"info_hash", 9);

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

    BNode *info_hash = BNode_GetValue(arguments, (uint8_t *)"info_hash", 9);

    check(info_hash != NULL
	  && info_hash->type == BString
	  && info_hash->count == HASH_BYTES,
	  "Missing or bad info_hash id");

    BNode *port = BNode_GetValue(arguments, (uint8_t *)"port", 4);

    check(port != NULL
	  && port->type == BInteger
	  && port->value.integer >= 0
	  && port->value.integer <= 0xffff,
	  "Missing or bad port");

    BNode *token = BNode_GetValue(arguments, (uint8_t *)"token", 5);

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

Message *DecodeResponse(BNode *dict, GetResponseType_fp getResponseType)
{
    assert(dict != NULL && "NULL BNode pointer");

    Message *message = calloc(1, sizeof(Message));
    check_mem(message);

    int rc = 0;

    check(dict->type == BDictionary, "Not a dictionary");

    rc = GetTransactionId(dict, &message->t, &message->t_len);
    check(rc == 0, "Bad transaction id");

    rc = GetResponseId(dict, &message->id);
    check(rc == 0, "Bad response node id");

    rc = getResponseType(message->t, message->t_len, &message->type);
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

    BNode *arguments = BNode_GetValue(dict, (uint8_t *)"r", 1);
    check(arguments != NULL, "No response arguments");
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

int GetResponseFindNodeData(BNode *arguments, RFindNodeData *data);
int GetResponseGetPeersData(BNode *arguments, RGetPeersData *data);

int GetResponseData(MessageType type, BNode *dict, Message *message)
{
    assert(dict != NULL && "NULL BNode dictionary pointer");
    assert(message != NULL && "NULL Message pointer");

    if (type == RPing || type == RAnnouncePeer)
	return 0;

    BNode *arguments = BNode_GetValue(dict, (uint8_t *)"r", 1);

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

    BNode *nodes = BNode_GetValue(arguments, (uint8_t *)"nodes", 5);

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
    uint8_t *data = string->value.string;

    while (node < *nodes + *count)
    {
	memcpy(node->id.value, data, HASH_BYTES);
	node->addr = ntohl(*(uint32_t *)(data + HASH_BYTES));
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

    BNode *token = BNode_GetValue(arguments, (uint8_t *)"token", 5);
    check(token != NULL, "Missing token");
    check(token->type == BString, "Bad token type");

    data->token = BNode_CopyString(token);
    check(data->token != NULL, "Failed to copy token");

    data->token_len = token->count;

    int rc = 0;

    BNode *values = BNode_GetValue(arguments, (uint8_t *)"values", 6);
    BNode *nodes = BNode_GetValue(arguments, (uint8_t *)"nodes", 5);

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

int EncodeQueryPing(Message *message, uint8_t *dest, size_t len);
int EncodeQueryFindNode(Message *message, uint8_t *dest, size_t len);
int EncodeQueryGetPeers(Message *message, uint8_t *dest, size_t len);
int EncodeQueryAnnouncePeer(Message *message, uint8_t *dest, size_t len);

int EncodeResponsePing(Message *message, uint8_t *dest, size_t len);
int EncodeResponseFindNode(Message *message, uint8_t *dest, size_t len);
int EncodeResponseGetPeers(Message *message, uint8_t *dest, size_t len);
int EncodeResponseAnnouncePeer(Message *message, uint8_t *dest, size_t len);

int Message_Encode(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    switch (message->type)
    {
    case QPing: return EncodeQueryPing(message, dest, len);
    case QFindNode: return EncodeQueryFindNode(message, dest, len);
    case QGetPeers: return EncodeQueryGetPeers(message, dest, len);
    case QAnnouncePeer: return EncodeQueryAnnouncePeer(message, dest, len);
    case RPing: return EncodeResponsePing(message, dest, len);
    case RFindNode: return EncodeResponseFindNode(message, dest, len);
    case RGetPeers: return EncodeResponseGetPeers(message, dest, len);
    case RAnnouncePeer: return EncodeResponseAnnouncePeer(message, dest, len);
    default: log_err("Can't encode unknown message type");
	return -1;
    }
}

int digits(size_t l)
{
    assert(l < 100000 && "Too many digits");
    
    if (l < 10)
	return 1;
    if (l < 100)
	return 2;
    if (l < 1000)
	return 3;
    if (l < 10000)
	return 4;
    if (l < 100000)
	return 5;

    return 20;
}

int StringHeaderLen(size_t string_len)
{
    return digits(string_len) + 1;
}

int BStringLen(size_t data_len)
{
    return StringHeaderLen(data_len) + data_len;
}

#define SLen(S) (sizeof(S) - 1)

int TLen(Message *message)
{
    return SLen("1:t") + BStringLen(message->t_len);
}

#define SCpy(D, S) memcpy(D, S, SLen(S)); D += SLen(S)

void StringHeaderCpy(uint8_t **dest, size_t string_len)
{
    int len_digits = digits(string_len);
    snprintf((char *)*dest, len_digits + 1, "%ld", string_len);
    (*dest)[len_digits] = ':';
    *dest += len_digits + 1;
}

void BStringCpy(uint8_t **dest, uint8_t *string, size_t len)
{
    StringHeaderCpy(dest, len);
    memcpy(*dest, string, len);
    *dest += len;
}

void TCpy(uint8_t **dest, Message *message)
{
    SCpy(*dest, "1:t");
    BStringCpy(dest, message->t, message->t_len);
}

#define HCpy(D, H) BStringCpy(&D, H, HASH_BYTES)
#define HASHLEN BStringLen(HASH_BYTES)

#define QPINGA "d1:ad2:id"
#define QPINGB "e1:q4:ping"
#define QPINGC "1:y1:qe"

int EncodeQueryPing(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == QPing, "Not a ping query");

    uint8_t *orig_dest = dest;

    check(SLen(QPINGA)
	  + HASHLEN
	  + SLen(QPINGB)
	  + TLen(message)
	  + SLen(QPINGC)
	  <= len,
	  "ping query would overflow dest");

    SCpy(dest, QPINGA);
    HCpy(dest, message->id->value);
    SCpy(dest, QPINGB);
    TCpy(&dest, message);
    SCpy(dest, QPINGC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

#define QFINDNODEA "d1:ad2:id"
#define QFINDNODEB "6:target"
#define QFINDNODEC "e1:q9:find_node"
#define QFINDNODED "1:y1:qe"

int EncodeQueryFindNode(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == QFindNode, "Not a find_node query");

    uint8_t *orig_dest = dest;

    check(SLen(QFINDNODEA)
	  + HASHLEN
	  + SLen(QFINDNODEB)
	  + HASHLEN
	  + SLen(QFINDNODEC)
	  + TLen(message)
	  + SLen(QFINDNODED)
	  <= len,
	  "find_node query would overflow dest");

    SCpy(dest, QFINDNODEA);
    HCpy(dest, message->id->value);
    SCpy(dest, QFINDNODEB);
    HCpy(dest, message->data.qfindnode.target->value);
    SCpy(dest, QFINDNODEC);
    TCpy(&dest, message);
    SCpy(dest, QFINDNODED);
    
    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

#define QGETPEERSA "d1:ad2:id"
#define QGETPEERSB "9:info_hash"
#define QGETPEERSC "e1:q9:get_peers"
#define QGETPEERSD "1:y1:qe"

int EncodeQueryGetPeers(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == QGetPeers, "Not a get_peers query");

    uint8_t *orig_dest = dest;

    check(SLen(QGETPEERSA)
	  + HASHLEN
	  + SLen(QGETPEERSB)
	  + HASHLEN
	  + SLen(QGETPEERSC)
	  + TLen(message)
	  + SLen(QGETPEERSD)
	  <= len,
	  "get_peers query would overflow dest");

    SCpy(dest, QGETPEERSA);
    HCpy(dest, message->id->value);
    SCpy(dest, QGETPEERSB);
    HCpy(dest, message->data.qgetpeers.info_hash->value);
    SCpy(dest, QGETPEERSC);
    TCpy(&dest, message);
    SCpy(dest, QGETPEERSD);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

int ILen(int i)
{
    return digits(i) + 2;
}

void ICpy(uint8_t **dest, int i)
{
    int len = ILen(i);
    snprintf((char *)*dest, len, "i%d", i);
    (*dest)[len - 1] = 'e';
    *dest += len;
}

#define QANNOUNCEPEERA "d1:ad2:id"
#define QANNOUNCEPEERB "9:info_hash"
#define QANNOUNCEPEERC "4:port"
#define QANNOUNCEPEERD "5:token"
#define QANNOUNCEPEERE "e1:q13:announce_peer"
#define QANNOUNCEPEERF "1:y1:qe"

int EncodeQueryAnnouncePeer(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == QAnnouncePeer, "Not a announce_peer query");

    uint8_t *orig_dest = dest;
    QAnnouncePeerData *data = &message->data.qannouncepeer;

    check(SLen(QANNOUNCEPEERA)
	  + HASHLEN
	  + SLen(QANNOUNCEPEERB)
	  + HASHLEN
	  + SLen(QANNOUNCEPEERC)
	  + ILen(data->port)
	  + SLen(QANNOUNCEPEERD)
	  + BStringLen(data->token_len)
	  + SLen(QANNOUNCEPEERE)
	  + TLen(message)
	  + SLen(QANNOUNCEPEERF)
	  <= len,
	  "announce_peer query would overflow dest");

    SCpy(dest, QANNOUNCEPEERA);
    HCpy(dest, message->id->value);
    SCpy(dest, QANNOUNCEPEERB);
    HCpy(dest, data->info_hash->value);
    SCpy(dest, QANNOUNCEPEERC);
    ICpy(&dest, data->port);
    SCpy(dest, QANNOUNCEPEERD);
    BStringCpy(&dest, data->token, data->token_len);
    SCpy(dest, QANNOUNCEPEERE);
    TCpy(&dest, message);
    SCpy(dest, QANNOUNCEPEERF);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

#define RPINGA "d1:rd2:id"
#define RPINGB "e"
#define RPINGC "1:y1:re"

int EncodeResponsePing(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == RPing, "Not a ping response");

    uint8_t *orig_dest = dest;

    check(SLen(RPINGA)
	  + HASHLEN
	  + SLen(RPINGB)
	  + TLen(message)
	  + SLen(RPINGC)
	  <= len,
	  "ping response would overflow dest");

    SCpy(dest, RPINGA);
    HCpy(dest, message->id->value);
    SCpy(dest, RPINGB);
    TCpy(&dest, message);
    SCpy(dest, RPINGC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

#define COMPACTNODEBYTES 26

int NodesLen(size_t nodes_count)
{
    return SLen("5:nodes") + BStringLen(nodes_count * COMPACTNODEBYTES);
}

void NodesCpy(uint8_t **dest, DhtNode *nodes, size_t count)
{
    SCpy(*dest, "5:nodes");

    int len = count * COMPACTNODEBYTES;

    StringHeaderCpy(dest, len);

    size_t i = 0;
    for (i = 0; i < count; i++)
    {
	memcpy(*dest, nodes[i].id.value, HASH_BYTES);
	*dest += HASH_BYTES;

	(*dest)[0] = nodes[i].addr >> 24;
	(*dest)[1] = nodes[i].addr >> 16;
	(*dest)[2] = nodes[i].addr >> 8;
	(*dest)[3] = nodes[i].addr;

	(*dest)[4] = nodes[i].port >> 8;
	(*dest)[5] = nodes[i].port;

	*dest += 6;
    }
}    

#define RFINDNODEA "d1:rd2:id"
#define RFINDNODEB "e"
#define RFINDNODEC "1:y1:re"

int EncodeResponseFindNode(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == RFindNode, "Not a find_node response");

    uint8_t *orig_dest = dest;
    RFindNodeData *data = &message->data.rfindnode;

    check(SLen(RFINDNODEA)
	  + HASHLEN
	  + NodesLen(data->count)
	  + SLen(RFINDNODEB)
	  + TLen(message)
	  + SLen(RFINDNODEC)
	  <= len,
	  "find_node response would overflow dest");

    SCpy(dest, RFINDNODEA);
    HCpy(dest, message->id->value);
    NodesCpy(&dest, data->nodes, data->count);
    SCpy(dest, RFINDNODEB);
    TCpy(&dest, message);
    SCpy(dest, RFINDNODEC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
    
error:
    return -1;
}

int ValuesLen(int count)
{
    return count * 8 + 2;
}

void ValuesCpy(uint8_t **dest, Peer *values, int count)
{
    assert(dest != NULL && "NULL pointer to uint8_t dest pointer");
    assert(values != NULL && "NULL Peer values pointer");

    *(*dest)++ = 'l';

    int i = 0;
    for (i = 0; i < count; i++)
    {
	(*dest)[0] = '6';
	(*dest)[1] = ':';
	(*dest)[2] = values[i].addr >> 24;
	(*dest)[3] = values[i].addr >> 16;
	(*dest)[4] = values[i].addr >> 8;
	(*dest)[5] = values[i].addr;

	(*dest)[6] = values[i].port >> 8;
	(*dest)[7] = values[i].port;

	*dest += 8;
    }

    *(*dest)++ = 'e';
}

#define RGETPEERSA "d1:rd2:id"
#define RGETPEERSB "5:token"
#define RGETPEERSC "1:y1:re"

int EncodeResponseGetPeers_values(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == RGetPeers, "Not a get_peers response");

    uint8_t *orig_dest = dest;
    RGetPeersData *data = &message->data.rgetpeers;

    assert(data->values != NULL && "NULL Peer values pointer");

    check(SLen(RGETPEERSA)
	  + HASHLEN
	  + SLen(RGETPEERSB)
	  + BStringLen(data->token_len)
	  + SLen("6:values")
	  + ValuesLen(data->count)
	  + SLen("e")
	  + TLen(message)
	  + SLen(RGETPEERSC)
	  <= len,
	  "get_peers response would overflow dest");

    SCpy(dest, RGETPEERSA);
    HCpy(dest, message->id->value);
    SCpy(dest, RGETPEERSB);
    BStringCpy(&dest, data->token, data->token_len);
    SCpy(dest, "6:values");
    ValuesCpy(&dest, data->values, data->count);
    SCpy(dest, "e");
    TCpy(&dest, message);
    SCpy(dest, RGETPEERSC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

int EncodeResponseGetPeers_nodes(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == RGetPeers, "Not a get_peers response");

    uint8_t *orig_dest = dest;
    RGetPeersData *data = &message->data.rgetpeers;

    assert(data->nodes != NULL && "NULL Peer values pointer");

    check(SLen(RGETPEERSA)
	  + HASHLEN
	  + NodesLen(data->count)
	  + SLen(RGETPEERSB)
	  + BStringLen(data->token_len)
	  + SLen("e")
	  + TLen(message)
	  + SLen(RGETPEERSC)
	  <= len,
	  "get_peers response would overflow dest");

    SCpy(dest, RGETPEERSA);
    HCpy(dest, message->id->value);
    NodesCpy(&dest, data->nodes, data->count);
    SCpy(dest, RGETPEERSB);
    BStringCpy(&dest, data->token, data->token_len);
    SCpy(dest, "e");
    TCpy(&dest, message);
    SCpy(dest, RGETPEERSC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

int EncodeResponseGetPeers(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    if (message->data.rgetpeers.values != NULL)
	return EncodeResponseGetPeers_values(message, dest, len);
    else if (message->data.rgetpeers.nodes != NULL)
	return EncodeResponseGetPeers_nodes(message, dest, len);

    sentinel("get_peers message without values and nodes");
error:
    return -1;
}

#define RANNOUNCEPEERA "d1:rd2:id"
#define RANNOUNCEPEERB "e"
#define RANNOUNCEPEERC "1:y1:re"

int EncodeResponseAnnouncePeer(Message *message, uint8_t *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL uint8_t dest pointer");

    check(message->type == RAnnouncePeer, "Not a announce_peer response");

    uint8_t *orig_dest = dest;

    check(SLen(RANNOUNCEPEERA)
	  + HASHLEN
	  + SLen(RANNOUNCEPEERB)
	  + TLen(message)
	  + SLen(RANNOUNCEPEERC)
	  <= len,
	  "announce_peer response would overflow dest");

    SCpy(dest, RANNOUNCEPEERA);
    HCpy(dest, message->id->value);
    SCpy(dest, RANNOUNCEPEERB);
    TCpy(&dest, message);
    SCpy(dest, RANNOUNCEPEERC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

Message *DecodeError(BNode *dict)
{
    assert(dict == (BNode *)"TODO");
    dict = dict;		/* Unused */
    return NULL;
}

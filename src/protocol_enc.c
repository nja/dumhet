#include <assert.h>

#include <dht/bencode.h>
#include <dht/message.h>
#include <dht/protocol.h>
#include <lcthw/dbg.h>

int EncodeQueryPing(Message *message, char *dest, size_t len);
int EncodeQueryFindNode(Message *message, char *dest, size_t len);
int EncodeQueryGetPeers(Message *message, char *dest, size_t len);
int EncodeQueryAnnouncePeer(Message *message, char *dest, size_t len);

int EncodeResponsePing(Message *message, char *dest, size_t len);
int EncodeResponseFindNode(Message *message, char *dest, size_t len);
int EncodeResponseGetPeers(Message *message, char *dest, size_t len);
int EncodeResponseAnnouncePeer(Message *message, char *dest, size_t len);
int EncodeResponseError(Message *message, char *dest, size_t len);

int Message_Encode(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

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
    case RError: return EncodeResponseError(message, dest, len);
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

void StringHeaderCpy(char **dest, size_t string_len)
{
    int len_digits = digits(string_len);
    snprintf((char *)*dest, len_digits + 1, "%zd", string_len);
    (*dest)[len_digits] = ':';
    *dest += len_digits + 1;
}

void BStringCpy(char **dest, char *string, size_t len)
{
    StringHeaderCpy(dest, len);
    memcpy(*dest, string, len);
    *dest += len;
}

void TCpy(char **dest, Message *message)
{
    SCpy(*dest, "1:t");
    BStringCpy(dest, message->t, message->t_len);
}

#define HCpy(D, H) BStringCpy(&D, H, HASH_BYTES)
#define HASHLEN BStringLen(HASH_BYTES)

#define QPINGA "d1:ad2:id"
#define QPINGB "e1:q4:ping"
#define QPINGC "1:y1:qe"

int EncodeQueryPing(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == QPing, "Not a ping query");

    char *orig_dest = dest;

    check(SLen(QPINGA)
	  + HASHLEN
	  + SLen(QPINGB)
	  + TLen(message)
	  + SLen(QPINGC)
	  <= len,
	  "ping query would overflow dest");

    SCpy(dest, QPINGA);
    HCpy(dest, message->id.value);
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

int EncodeQueryFindNode(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == QFindNode, "Not a find_node query");

    char *orig_dest = dest;

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
    HCpy(dest, message->id.value);
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

int EncodeQueryGetPeers(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == QGetPeers, "Not a get_peers query");

    char *orig_dest = dest;

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
    HCpy(dest, message->id.value);
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

void ICpy(char **dest, int i)
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

int EncodeQueryAnnouncePeer(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == QAnnouncePeer, "Not a announce_peer query");

    char *orig_dest = dest;
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
    HCpy(dest, message->id.value);
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

int EncodeResponsePing(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == RPing, "Not a ping response");

    char *orig_dest = dest;

    check(SLen(RPINGA)
	  + HASHLEN
	  + SLen(RPINGB)
	  + TLen(message)
	  + SLen(RPINGC)
	  <= len,
	  "ping response would overflow dest");

    SCpy(dest, RPINGA);
    HCpy(dest, message->id.value);
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

void NodesCpy(char **dest, Node **nodes, size_t count)
{
    SCpy(*dest, "5:nodes");

    int len = count * COMPACTNODEBYTES;

    StringHeaderCpy(dest, len);

    unsigned int i = 0;
    for (i = 0; i < count; i++)
    {
	memcpy(*dest, nodes[i]->id.value, HASH_BYTES);
	*dest += HASH_BYTES;

	(*dest)[0] = nodes[i]->addr.s_addr >> 24;
	(*dest)[1] = nodes[i]->addr.s_addr >> 16;
	(*dest)[2] = nodes[i]->addr.s_addr >> 8;
	(*dest)[3] = nodes[i]->addr.s_addr;

	(*dest)[4] = nodes[i]->port >> 8;
	(*dest)[5] = nodes[i]->port;

	*dest += 6;
    }
}    

#define RFINDNODEA "d1:rd2:id"
#define RFINDNODEB "e"
#define RFINDNODEC "1:y1:re"

int EncodeResponseFindNode(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == RFindNode, "Not a find_node response");

    char *orig_dest = dest;
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
    HCpy(dest, message->id.value);
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

void ValuesCpy(char **dest, Peer *values, int count)
{
    assert(dest != NULL && "NULL pointer to char dest pointer");
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

int EncodeResponseGetPeers_values(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == RGetPeers, "Not a get_peers response");

    char *orig_dest = dest;
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
    HCpy(dest, message->id.value);
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

int EncodeResponseGetPeers_nodes(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == RGetPeers, "Not a get_peers response");

    char *orig_dest = dest;
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
    HCpy(dest, message->id.value);
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

int EncodeResponseGetPeers(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

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

int EncodeResponseAnnouncePeer(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == RAnnouncePeer, "Not a announce_peer response");

    char *orig_dest = dest;

    check(SLen(RANNOUNCEPEERA)
	  + HASHLEN
	  + SLen(RANNOUNCEPEERB)
	  + TLen(message)
	  + SLen(RANNOUNCEPEERC)
	  <= len,
	  "announce_peer response would overflow dest");

    SCpy(dest, RANNOUNCEPEERA);
    HCpy(dest, message->id.value);
    SCpy(dest, RANNOUNCEPEERB);
    TCpy(&dest, message);
    SCpy(dest, RANNOUNCEPEERC);

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

int EncodeResponseError(Message *message, char *dest, size_t len)
{
    assert(message != NULL && "NULL Message pointer");
    assert(dest != NULL && "NULL char dest pointer");

    check(message->type == RError, "Not an error response");

    char *orig_dest = dest;
    RErrorData *data = &message->data.rerror;

    check(SLen("d1:el")
	  + ILen(data->code)
	  + BStringLen(blength(data->message))
	  + SLen("e")
	  + TLen(message)
	  + SLen("1:y1:ee")
	  <= len,
	  "error response would overflow dest");

    SCpy(dest, "d1:el");
    ICpy(&dest, data->code);
    BStringCpy(&dest, bdata(data->message), blength(data->message));
    SCpy(dest, "e");
    TCpy(&dest, message);
    SCpy(dest, "1:y1:ee");

    assert(dest - orig_dest <= (ssize_t)len && "Overflow");

    return dest - orig_dest;
error:
    return -1;
}

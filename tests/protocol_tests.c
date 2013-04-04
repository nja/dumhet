#include <arpa/inet.h>
#include <string.h>

#include "minunit.h"
#include <dht/bucket.h>
#include <dht/message.h>
#include <dht/protocol.h>
#include <lcthw/bstrlib.h>

int same_bytes(char *expected, char *data)
{
    int len = strlen(expected);

    return memcmp(expected, data, len) == 0;
}

int same_bytes_len(char *expected, char *data, size_t len)
{
    return strlen(expected) == len && same_bytes(expected, data);
}

char *check_Message(Message *message, MessageType type)
{
    char *tid = "aa",
	*qid = "abcdefghij0123456789",
	*rid = "mnopqrstuvwxyz123456";
    
    mu_assert(message != NULL, "Decode failed");
    mu_assert(message->type == type, "Wrong message type");
    mu_assert(message->t != NULL, "No transaction id");
    mu_assert(same_bytes_len(tid, message->t, message->t_len), "Wrong transaction id");

    switch (type)
    {
    case QPing:
    case QFindNode:
    case QGetPeers:
    case QAnnouncePeer:
	mu_assert(same_bytes(qid, message->id.value), "Wrong query id");
	break;
    case RPing:
    case RFindNode:
    case RGetPeers:
    case RAnnouncePeer:
	mu_assert(same_bytes(rid, message->id.value), "Wrong reply id");
	break;
    case RError:
	mu_assert(0, "Bad type");
	break;
    }

    return NULL;
}    

char *test_Decode_QPing()
{
    char *data = "d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe";

    Message *message = Message_Decode(data, strlen(data), NULL);

    mu_assert(check_Message(message, QPing) == NULL, "Bad decoded message");

    Message_Destroy(message);

    return NULL;
}

struct MockResponses {
    GetPendingResponse_fp getPendingReponse;
    char *tid;
    MessageType type;
    Hash id;
    int check_tid;
};

PendingResponse GetMockResponseType(void *responses, char *tid, int *rc)
{
    struct MockResponses *mock = (struct MockResponses *)responses;

    if (mock->check_tid && !same_bytes_len(mock->tid, tid, sizeof(tid_t)))
    {
	log_err("Bad transaction id");
        *rc = -1;
	return (PendingResponse) { 0 };
    }

    rc = 0;
    return (PendingResponse) { mock->type, *(tid_t *)tid, mock->id, NULL };
}

struct MockResponses *GetMockResponses(char *tid, MessageType type, Hash id, int check_tid)
{
    struct MockResponses *responses = malloc(sizeof(struct MockResponses));
    check_mem(responses);

    responses->getPendingReponse = GetMockResponseType;
    responses->tid = tid;
    responses->type = type;
    responses->id = id;
    responses->check_tid = check_tid;

    return responses;
error:
    return NULL;
}

Hash ID(char *data)
{
    char *id20 = "id20:";
    char *ch = strstr(data, id20) + strlen(id20);
    char *end = ch + 20;

    Hash id;
    char *v = id.value;

    while (ch < end)
    {
        *v++ = *ch++;
    }

    return id;
}

char *test_Decode_RPing()
{
    char *data = "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    void *responses = GetMockResponses("aa", RPing, ID(data), 1);

    Message *message = Message_Decode(data,
				      strlen(data),
				      responses);

    mu_assert(check_Message(message, RPing) == NULL, "Bad decoded message");

    Message_Destroy(message);
    free(responses);

    return NULL;
}

char *test_Decode_QFindNode()
{
    char *data = "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe";

    Message *message = Message_Decode(data, strlen(data), NULL);

    mu_assert(check_Message(message, QFindNode) == NULL, "Bad decoded message");

    mu_assert(message->data.qfindnode.target != NULL, "No target");
    mu_assert(same_bytes("mnopqrstuvwxyz123456",
			 message->data.qfindnode.target->value),
	      "Wrong target");

    Message_Destroy(message);

    return NULL;
}

uint32_t chntohl(char *ch)
{
    return ch[0] << 24
	| ch[1] << 16
	| ch[2] << 8
	| ch[3];
}

uint16_t chntohs(char *ch)
{
    return ch[0] << 8
	| ch[1];
}

char *test_Decode_RFindNode()
{
    char *data = "d1:rd2:id20:mnopqrstuvwxyz1234565:nodes52:"
	"01234567890123456789ABCDEF"
	"????????????????????xxxxyy"
	"e1:t2:aa1:y1:re";
    void *responses = GetMockResponses("aa", RFindNode, ID(data), 1);

    Message *message = Message_Decode(data, strlen(data), responses);

    mu_assert(check_Message(message, RFindNode) == NULL, "Bad decoded message");

    mu_assert(message->data.rfindnode.nodes != NULL, "NULL nodes");
    mu_assert(message->data.rfindnode.count == 2, "Wrong node count");

    mu_assert(same_bytes("01234567890123456789",
			 message->data.rfindnode.nodes[0]->id.value),
	      "Wrong nodes[0] id");
    mu_assert(message->data.rfindnode.nodes[0]->addr.s_addr == chntohl("ABCD"),
	      "Wrong nodes[0] addr");
    mu_assert(message->data.rfindnode.nodes[0]->port == chntohs("EF"),
    	      "Wrong nodes[0] port");

    mu_assert(same_bytes("????????????????????",
			 message->data.rfindnode.nodes[1]->id.value),
	      "Wrong nodes[1] id");
    mu_assert(message->data.rfindnode.nodes[1]->addr.s_addr == chntohl("xxxx"),
	      "Wrong nodes[1] addr");
    mu_assert(message->data.rfindnode.nodes[1]->port == chntohs("yy"),
	      "Wrong nodes[1] port");

    Message_DestroyNodes(message);
    Message_Destroy(message);
    free(responses);

    return NULL;
}

char *test_Decode_QGetPeers()
{
    char *data = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
    char *info_hash = "mnopqrstuvwxyz123456";

    Message *message = Message_Decode(data, strlen(data), NULL);

    mu_assert(check_Message(message, QGetPeers) == NULL, "Bad decoded message");

    mu_assert(message->data.qgetpeers.info_hash != NULL, "Missing info_hash");
    mu_assert(same_bytes(info_hash, message->data.qgetpeers.info_hash->value),
	      "Wrong info_hash");

    Message_Destroy(message);

    return NULL;
}

char *test_Decode_RGetPeers_nodes()
{
    char *input = "d1:rd2:id20:mnopqrstuvwxyz1234565:nodes208:"
	"012345678901234567890xxxy0"
	"112345678901234567891xxxy1"
	"212345678901234567892xxxy2"
	"312345678901234567893xxxy3"
	"412345678901234567894xxxy4"
	"512345678901234567895xxxy5"
	"612345678901234567896xxxy6"
	"712345678901234567897xxxy7"
	"5:token8:aoeusnthe1:t2:aa1:y1:re";
    void *responses = GetMockResponses("aa", RGetPeers, ID(input), 1);

    Message *message = Message_Decode(input, strlen(input), responses);

    mu_assert(check_Message(message, RGetPeers) == NULL, "Bad decoded message");

    RGetPeersData *data = &message->data.rgetpeers;

    mu_assert(same_bytes_len("aoeusnth", data->token, data->token_len), "Bad token");
    mu_assert(data->values == NULL, "Bad values");
    mu_assert(data->count == BUCKET_K, "Wrong nodes count");

    char id[] = "_1234567890123456789",
    	addr[] = "_xxx",
    	port[] = "y_";

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	id[0] = '0' + i;
	addr[0] = '0' + i;
	port[1] = '0' + i;

	mu_assert(same_bytes(id, data->nodes[i]->id.value), "Bad id");
	mu_assert(data->nodes[i]->addr.s_addr == chntohl(addr), "Bad addr");
	mu_assert(data->nodes[i]->port == chntohs(port), "Bad port");
    }

    Message_DestroyNodes(message);
    Message_Destroy(message);
    free(responses);
    
    return NULL;
}

char *test_Decode_RGetPeers_values()
{
    char *input = "d1:rd2:id20:mnopqrstuvwxyz1234565:token8:aoeusnth6:valuesl"
	"6:" "0xxxy0"
	"6:" "1xxxy1"
	"6:" "2xxxy2"
	"ee1:t2:aa1:y1:re";
    void *responses = GetMockResponses("aa", RGetPeers, ID(input), 1);

    Message *message = Message_Decode(input, strlen(input), responses);

    mu_assert(check_Message(message, RGetPeers) == NULL, "Bad decoded message");

    RGetPeersData *data = &message->data.rgetpeers;

    mu_assert(same_bytes_len("aoeusnth", data->token, data->token_len), "Bad token");
    mu_assert(data->nodes == NULL, "Bad nodes");
    mu_assert(data->count == 3, "Wrong values count");

    char addr[] = "_xxx",
    	port[] = "y_";

    int i = 0;
    for (i = 0; i < 3; i++)
    {
	addr[0] = '0' + i;
	port[1] = '0' + i;
	mu_assert(data->values[i].addr == chntohl(addr), "Bad addr");
	mu_assert(data->values[i].port == chntohs(port), "Bad port");
    }

    Message_Destroy(message);
    free(responses);

    return NULL;
}
    
char *test_Decode_QAnnouncePeer()
{
    char *data = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe";
    char *info_hash = "mnopqrstuvwxyz123456", *token = "aoeusnth";
    const int port = 6881;

    Message *message = Message_Decode(data, strlen(data), NULL);

    mu_assert(check_Message(message, QAnnouncePeer) == NULL, "Bad decoded message");

    mu_assert(message->data.qannouncepeer.info_hash != NULL, "Missing info_hash");
    mu_assert(same_bytes(info_hash, message->data.qannouncepeer.info_hash->value),
	      "Wrong info_hash");
    mu_assert(message->data.qannouncepeer.token != NULL, "No token");
    mu_assert(message->data.qannouncepeer.token_len == strlen(token),
	      "Wrong token length");
    mu_assert(same_bytes(token, message->data.qannouncepeer.token),
	      "Wrong token");
    mu_assert(message->data.qannouncepeer.port == port, "Wrong port");

    Message_Destroy(message);

    return NULL;
}

char *test_Decode_RAnnouncePeer()
{
    char *data = "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    void *responses = GetMockResponses("aa", RAnnouncePeer, ID(data), 1);

    Message *message = Message_Decode(data,
                                      strlen(data),
                                      responses);

    mu_assert(check_Message(message, RAnnouncePeer) == NULL, "Bad decoded message");

    Message_Destroy(message);
    free(responses);

    return NULL;
}

char *test_Decode_RError()
{
    char *data[] = {
	"d1:eli201e7:Generice1:t2:aa1:y1:ee",
	"d1:eli202e6:Servere1:t2:aa1:y1:ee",
	"d1:eli203e8:Protocole1:t2:aa1:y1:ee",
	"d1:eli204e6:Methode1:t2:aa1:y1:ee",
	NULL
    };

    int expected_code[] = { 201, 202, 203, 204 };
    struct tagbstring expected_msg[] = {
	bsStatic("Generic"),
	bsStatic("Server"),
	bsStatic("Protocol"),
	bsStatic("Method")
    };

    int i = 0;
    while (data[i])
    {
	Message *message = Message_Decode(data[i], strlen(data[i]), NULL);

	mu_assert(message->type == RError, "Wrong message type");
	mu_assert(same_bytes_len("aa", message->t, message->t_len), "Wrong transaction id");

	RErrorData *data = &message->data.rerror;

	mu_assert(data->code == expected_code[i], "Wrong error code");
	mu_assert(bstrcmp(&expected_msg[i], data->message) == 0, "Wrong message");

	Message_Destroy(message);

	i++;
    }

    return NULL;
}

char *test_Decode_JunkQuery()
{
    char *ok[] = {
	"d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
    };
    ok[1]=ok[1];

    char *junk[] = {
	/* Wrong bencode */
	"foo",
	"i0e",
	"le",
	"1:x",
	/* No 'y' */
	"d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aae"
	/* No 'q' */
	"d1:ad2:id20:abcdefghij0123456789e1:t2:aa1:y1:qe",
	/* Unknown 'q' value */
	"d1:ad2:id20:abcdefghij0123456789e1:q3:foo:t2:aa1:y1:qe",
	/* No 't' */
	"d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:y1:qe",
	/* No arguments */
	"d1:q4:ping1:t2:aa1:y1:qe",
	"d1:q9:find_node1:t2:aa1:y1:qe",
	"d1:q9:get_peers1:t2:aa1:y1:qe",
	"d1:q13:announce_peer1:t2:aa1:y1:qe",
	/* Bad id */
	"d1:ad2:id19:bcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe",
	"d1:ad2:id21:+abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe",
	"d1:ad2:id0:9:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe",
	"d1:ad2:idi0e9:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	/* find_node target */
	"d1:ad2:id20:abcdefghij0123456789e1:q9:find_node1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567896:target19:nopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe",
	/* get_peers info_hash */
	"d1:ad2:id20:abcdefghij01234567899:info_h___20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash0:e1:q9:get_peers1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hashi0ee1:q9:get_peers1:t2:aa1:y1:qe",
	/* announce_peer info_hash */
	"d1:ad2:id20:abcdefghij01234567899:info_h___0:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash1:x4:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hashi0e4:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	/* announce_peer port */
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:po__i6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:port4:68815:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti65536e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti18446744073709551616e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti-1e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	/* announce_peer token */
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:to___8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:tokeni0ee1:q13:announce_peer1:t2:aa1:y1:qe",
	NULL
    };

    int i = 0;
    while (junk[i])
    {
	Message *result = Message_Decode(junk[i], strlen(junk[i]), NULL);
	mu_assert(result == NULL, "Decoded junk without error");
	i++;
    }

    return NULL;
}

char *test_junk_response(char **junk, void **gettype)
{
    int i = 0;
    while (junk[i])
    {
	int j = 0,
	    len = strlen(junk[i]);

	while (gettype[j])
	{
	    Message *result = Message_Decode(junk[i], len, gettype[j]);
	    mu_assert(result == NULL, "Decoded junk without error");

	    j++;
	}
	i++;
    }

    i = 0;
    while (gettype[i])
	free(gettype[i++]);

    return NULL;
}    

char *test_Decode_JunkResponse_find_node()
{
    char *junk[] = {
	/* find_node nodes */
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nodes53:+01234567890123456789ABCDEF????????????????????xxxxyye1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nod__52:01234567890123456789ABCDEF????????????????????xxxxyye1:t2:aa1:y1:re",
	NULL
    };
    void *gettype[] = {
	GetMockResponses(NULL, RFindNode, ID(junk[0]), 0),
	NULL
    };

    return test_junk_response(junk, gettype);
}

char *test_Decode_JunkResponse_get_peers()
{
    char *junk[] = {
	/* get_peers values */
	"d1:rd2:id20:mnopqrstuvwxyz1234565:token8:aoeusnth6:valu__l6:0xxxy06:1xxxy16:2xxxy2ee1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:token8:aoeusnth6:valuesl6:0xxxy07:+1xxxy16:2xxxy2ee1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:token8:aoeusnth6:values6:0xxxy0e1:t2:aa1:y1:re",
	/* get_peers nodes */
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nodes209:+012345678901234567890xxxy0112345678901234567891xxxy1212345678901234567892xxxy2312345678901234567893xxxy3412345678901234567894xxxy4512345678901234567895xxxy5612345678901234567896xxxy6712345678901234567897xxxy75:token8:aoeusnthe1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nod__208:012345678901234567890xxxy0112345678901234567891xxxy1212345678901234567892xxxy2312345678901234567893xxxy3412345678901234567894xxxy4512345678901234567895xxxy5612345678901234567896xxxy6712345678901234567897xxxy75:token8:aoeusnthe1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nodesi0e5:token8:aoeusnthe1:t2:aa1:y1:re",
	/* get_peers token */
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nodes208:012345678901234567890xxxy0112345678901234567891xxxy1212345678901234567892xxxy2312345678901234567893xxxy3412345678901234567894xxxy4512345678901234567895xxxy5612345678901234567896xxxy6712345678901234567897xxxy75:tok__8:aoeusnthe1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:tok__8:aoeusnth6:valuesl6:0xxxy06:1xxxy16:2xxxy2ee1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:nodes208:012345678901234567890xxxy0112345678901234567891xxxy1212345678901234567892xxxy2312345678901234567893xxxy3412345678901234567894xxxy4512345678901234567895xxxy5612345678901234567896xxxy6712345678901234567897xxxy75:tokeni0ee1:t2:aa1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz1234565:tokeni0e6:valuesl6:0xxxy06:1xxxy16:2xxxy2ee1:t2:aa1:y1:re",
	NULL
    };

    void *gettype[] = {
	GetMockResponses(NULL, RGetPeers, ID(junk[0]), 0),
	NULL
    };

    return test_junk_response(junk, gettype);
}

char *test_Decode_JunkResponse()
{
    char *junk[] = {
	/* y */
	"d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aae",
	"d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:_e",
	"d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:yi0ee",
	/* t */
	"d1:rd2:id20:mnopqrstuvwxyz123456e1:y1:re",
	"d1:rd2:id20:mnopqrstuvwxyz123456e1:ti0e1:y1:re",
	/* id */
	"d1:rd2:ix20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re",
	"d1:rd2:id0:5:nodes52:01234567890123456789ABCDEF????????????????????xxxxyye1:t2:aa1:y1:re",
	"d1:rd2:id21:+mnopqrstuvwxyz1234565:nodes208:012345678901234567890xxxy0112345678901234567891xxxy1212345678901234567892xxxy2312345678901234567893xxxy3412345678901234567894xxxy4512345678901234567895xxxy5612345678901234567896xxxy6712345678901234567897xxxy75:token8:aoeusnthe1:t2:aa1:y1:re",
	"d1:rd2:idi0e5:token8:aoeusnth6:valuesl6:0xxxy06:1xxxy16:2xxxy2ee1:t2:aa1:y1:re",
	"d1:rde1:t2:aa1:y1:re"
	/* r */
	"d1:t2:aa1:y1:re",
	"d1:rle1:t2:aa1:y1:re",
	NULL
    };

    Hash id = ID(junk[0]);

    void *gettype[] = {
	GetMockResponses(NULL, RPing, id, 0),
	GetMockResponses(NULL, RFindNode, id, 0),
	GetMockResponses(NULL, RGetPeers, id, 0),
	GetMockResponses(NULL, RAnnouncePeer, id, 0),
	NULL
    };

    return test_junk_response(junk, gettype);
}

PendingResponse GetRoundtripResponseMessageType(void *responses, char *t, int *rc)
{
    (void)(responses);

    Hash id = ID("id20:abcdefghij0123456789");

    if (same_bytes_len("pi", t, sizeof(tid_t)))
    {
        *rc = 0;
        return (PendingResponse) { RPing, *(tid_t *)t, id, NULL };
    }

    if (same_bytes_len("fn", t, sizeof(tid_t)))
    {
        *rc = 0;
        return (PendingResponse) { RFindNode, *(tid_t *)t, id, NULL };
    }

    if (same_bytes_len("gp", t, sizeof(tid_t)))
    {
        *rc = 0;
        return (PendingResponse) { RGetPeers, *(tid_t *)t, id, NULL };
    }

    if (same_bytes_len("ap", t, sizeof(tid_t)))
    {
        *rc = 0;
        return (PendingResponse) { RAnnouncePeer, *(tid_t *)t, id, NULL };
    }

    *rc = -1;
    return (PendingResponse) { 0 };
}

char *test_Roundtrip()
{
    char *input[] = {
	"d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe",
	"d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe",
	"d1:rd2:id20:abcdefghij0123456789e1:t2:pi1:y1:re",
	"d1:rd2:id20:abcdefghij01234567895:nodes52:01234567890123456789ABCDEF????????????????????xxxxyye1:t2:fn1:y1:re",
	"d1:rd2:id20:abcdefghij01234567895:nodes208:012345678901234567890xxxy0112345678901234567891xxxy1212345678901234567892xxxy2312345678901234567893xxxy3412345678901234567894xxxy4512345678901234567895xxxy5612345678901234567896xxxy6712345678901234567897xxxy75:token8:aoeusnthe1:t2:gp1:y1:re",
	"d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:0xxxy06:1xxxy16:2xxxy2ee1:t2:gp1:y1:re",
	"d1:rd2:id20:abcdefghij0123456789e1:t2:ap1:y1:re",
	"d1:eli201e23:A Generic Error Ocurrede1:t2:ee1:y1:ee",
	NULL
    };

    struct PendingResponses responses = { .getPendingResponse = GetRoundtripResponseMessageType };

    int i = 0;
    while (input[i])
    {
	int len = strlen(input[i]);

	Message *message = Message_Decode(input[i], len, &responses);

	mu_assert(message != NULL, "Decode failed");

	char *dest = calloc(1, len);
	mu_assert(dest != NULL, "malloc failed");

	int rc = Message_Encode(message, dest, len - 1);
	mu_assert(rc == -1, "Encoded to too small dest");

	rc = Message_Encode(message, dest, len);
	mu_assert(rc >= 0, "Encode failed");

	mu_assert(rc == len, "Encoded too little");
	mu_assert(same_bytes_len(input[i], dest, len), "Roundtrip failed");

        Message_DestroyNodes(message);
	Message_Destroy(message);
	free(dest);

	i++;
    }

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Decode_QPing);
    mu_run_test(test_Decode_RPing);
    mu_run_test(test_Decode_QFindNode);
    mu_run_test(test_Decode_RFindNode);
    mu_run_test(test_Decode_QGetPeers);
    mu_run_test(test_Decode_RGetPeers_nodes);
    mu_run_test(test_Decode_RGetPeers_values);
    mu_run_test(test_Decode_QAnnouncePeer);
    mu_run_test(test_Decode_RAnnouncePeer);
    mu_run_test(test_Decode_RError);

    mu_run_test(test_Decode_JunkQuery);
    mu_run_test(test_Decode_JunkResponse);
    mu_run_test(test_Decode_JunkResponse_find_node);
    mu_run_test(test_Decode_JunkResponse_get_peers);
    mu_run_test(test_Roundtrip);

    return NULL;
}

RUN_TESTS(all_tests);

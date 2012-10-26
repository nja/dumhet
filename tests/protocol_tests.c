#include "minunit.h"
#include <message.h>
#include <protocol.h>
#include <arpa/inet.h>

int same_bytes(char *expected, uint8_t *data)
{
    int len = strlen(expected);
    int i = 0;
    
    for (i = 0; i < len; i++)
    {
	if (expected[i] != data[i])
	    return 0;
    }

    return 1;
}

int same_bytes_len(char *expected, uint8_t *data, size_t len)
{
    if (strlen(expected) != len)
	return 0;

    return same_bytes(expected, data);
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
    mu_assert(message->id != NULL, "No id hash");

    switch (type)
    {
    case QPing:
    case QFindNode:
    case QGetPeers:
    case QAnnouncePeer:
	mu_assert(same_bytes(qid, message->id->value), "Wrong query id");
	break;
    case RPing:
    case RFindNode:
    case RGetPeers:
    case RAnnouncePeer:
	mu_assert(same_bytes(rid, message->id->value), "Wrong reply id");
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

    Message *message = Decode((uint8_t *)data, strlen(data), NULL);

    mu_assert(check_Message(message, QPing) == NULL, "Bad decoded message");

    Message_Destroy(message);

    return NULL;
}

int GetRPingResponseType(uint8_t *tid, size_t len, MessageType *type)
{
    if (!same_bytes_len("aa", tid, len))
    {
	log_err("Bad transaction id");
	return -1;
    }

    *type = RPing;
    return 0;
}

char *test_Decode_RPing()
{
    char *data = "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";

    Message *message = Decode((uint8_t *)data,
			      strlen(data),
			      GetRPingResponseType);

    mu_assert(check_Message(message, RPing) == NULL, "Bad decoded message");

    Message_Destroy(message);

    return NULL;
}

char *test_Decode_QFindNode()
{
    char *data = "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe";

    Message *message = Decode((uint8_t *)data, strlen(data), NULL);

    mu_assert(check_Message(message, QFindNode) == NULL, "Bad decoded message");

    mu_assert(message->data.qfindnode.target != NULL, "No target");
    mu_assert(same_bytes("mnopqrstuvwxyz123456",
			 (uint8_t *)&message->data.qfindnode.target->value),
	      "Wrong target");

    Message_Destroy(message);

    return NULL;
}

int GetRFindNodeResponseType(uint8_t *tid, size_t len, MessageType *type)
{
    if (!same_bytes_len("aa", tid, len))
    {
	log_err("Bad transaction id");
	return -1;
    }

    *type = RFindNode;
    return 0;
}

char *test_Decode_RFindNode()
{
    char *data = "d1:rd2:id20:mnopqrstuvwxyz1234565:nodes52:"
	"01234567890123456789ABCDEF"
	"????????????????????xxxxyy"
	"e1:t2:aa1:y1:re";

    Message *message = Decode((uint8_t *)data, strlen(data), GetRFindNodeResponseType);

    mu_assert(check_Message(message, RFindNode) == NULL, "Bad decoded message");

    mu_assert(message->data.rfindnode.nodes != NULL, "NULL nodes");
    mu_assert(message->data.rfindnode.count == 2, "Wrong node count");

    mu_assert(same_bytes("01234567890123456789",
			 message->data.rfindnode.nodes[0].id.value),
	      "Wrong nodes[0] id");
    mu_assert(message->data.rfindnode.nodes[0].addr == ntohl(*(uint32_t *)"ABCD"),
	      "Wrong nodes[0] addr");
    mu_assert(message->data.rfindnode.nodes[0].port == ntohs(*(uint16_t *)"EF"),
    	      "Wrong nodes[0] port");

    mu_assert(same_bytes("????????????????????",
			 message->data.rfindnode.nodes[1].id.value),
	      "Wrong nodes[1] id");
    mu_assert(message->data.rfindnode.nodes[1].addr == ntohl(*(uint32_t *)"xxxx"),
	      "Wrong nodes[1] addr");
    mu_assert(message->data.rfindnode.nodes[1].port == ntohs(*(uint16_t *)"yy"),
	      "Wrong nodes[1] port");

    Message_Destroy(message);

    return NULL;
}

char *test_Decode_QGetPeers()
{
    char *data = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
    char *info_hash = "mnopqrstuvwxyz123456";

    Message *message = Decode((uint8_t *)data, strlen(data), NULL);

    mu_assert(check_Message(message, QGetPeers) == NULL, "Bad decoded message");

    mu_assert(message->data.qgetpeers.info_hash != NULL, "Missing info_hash");
    mu_assert(same_bytes(info_hash, message->data.qgetpeers.info_hash->value),
	      "Wrong info_hash");

    Message_Destroy(message);

    return NULL;
}

int GetRGetPeersResponseType(uint8_t *tid, size_t len, MessageType *type)
{
    if (!same_bytes_len("aa", tid, len))
    {
	log_err("Bad transaction id");
	return -1;
    }

    *type = RGetPeers;
    return 0;
}
    
char *test_Decode_QAnnouncePeer()
{
    char *data = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe";
    char *info_hash = "mnopqrstuvwxyz123456", *token = "aoeusnth";
    const int port = 6881;

    Message *message = Decode((uint8_t *)data, strlen(data), NULL);

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

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Decode_QPing);
    mu_run_test(test_Decode_RPing);
    mu_run_test(test_Decode_QFindNode);
    mu_run_test(test_Decode_RFindNode);
    mu_run_test(test_Decode_QGetPeers);
    mu_run_test(test_Decode_QAnnouncePeer);

    return NULL;
}

RUN_TESTS(all_tests);

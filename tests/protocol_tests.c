#include "minunit.h"
#include <message.h>
#include <protocol.h>

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

char *check_Message(Message *message, MessageType type)
{
    char *token = "aa", *id = "abcdefghij0123456789";
    
    mu_assert(message != NULL, "Decode failed");
    mu_assert(message->type == type, "Wrong message type");
    mu_assert(message->t != NULL, "No transaction id");
    mu_assert(message->t_len == strlen(token), "Unexpected token length");
    mu_assert(same_bytes(token, message->t), "Wrong transaction id");
    mu_assert(message->id != NULL, "No id hash");
    mu_assert(same_bytes(id, message->id->value), "Wrong id");

    return NULL;
}    

char *test_Decode_QPing()
{
    char *data = "d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe";

    Message *message = Decode((uint8_t *)data, strlen(data));

    mu_assert(check_Message(message, QPing) == NULL, "Bad decoded message");

    Message_Destroy(message);

    return NULL;
}

char *test_Decode_QFindNode()
{
    char *data = "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe";

    Message *message = Decode((uint8_t *)data, strlen(data));

    mu_assert(check_Message(message, QFindNode) == NULL, "Bad decoded message");

    mu_assert(message->data.qfindnode.target != NULL, "No target");
    mu_assert(same_bytes("mnopqrstuvwxyz123456",
			 (uint8_t *)&message->data.qfindnode.target->value),
	      "Wrong target");

    Message_Destroy(message);

    return NULL;
}

char *test_Decode_QGetPeersData()
{
    char *data = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
    char *info_hash = "mnopqrstuvwxyz123456";

    Message *message = Decode((uint8_t *)data, strlen(data));

    mu_assert(check_Message(message, QGetPeers) == NULL, "Bad decoded message");

    mu_assert(message->data.qgetpeers.info_hash != NULL, "Missing info_hash");
    mu_assert(same_bytes(info_hash, message->data.qgetpeers.info_hash->value), "Wrong info_hash");

    Message_Destroy(message);

    return NULL;
}

    /* 
     * mu_assert(same_bytes("abcdefghij0123456789", message->id->value), "Wrong id");
     * mu_assert(message->data->token != NULL, "No token");
     * mu_assert(same_bytes("
     */

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Decode_QPing);
    mu_run_test(test_Decode_QFindNode);
    mu_run_test(test_Decode_QGetPeersData);

    return NULL;
}

RUN_TESTS(all_tests);

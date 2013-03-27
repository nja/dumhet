#include "minunit.h"
#include <dht/client.h>
#include <dht/handle.h>
#include <dht/message.h>
#include <dht/message_create.h>

int SameT(Message *a, Message *b)
{
    if (a->t_len != b->t_len)
        return 0;

    unsigned int i = 0;
    for (i = 0; i < a->t_len; i++)
    {
        if (a->t[i] != b->t[i])
            return 0;
    }

    return 1;
}

int HasRecentQuery(DhtClient *client, DhtHash id)
{
    DhtNode *node = DhtTable_FindNode(client->table, &id);
    check(node != NULL, "No node in client table");
    check(node->query_time > 0, "query_time not set");

    return 1;
error:
    return 0;
}

char *test_HandleQPing()
{
    DhtHash id = { "client id" }, from_id = { "from id" };
    DhtClient *client = DhtClient_Create(id, 0, 0, 0);
    DhtClient *from = DhtClient_Create(from_id, 1, 1, 1);

    DhtTable_InsertNode(client->table, &from->node);

    Message *qping = Message_CreateQPing(from);

    Message *reply = HandleQPing(client, qping);

    mu_assert(reply != NULL, "Handle_QPing failed");
    mu_assert(reply->type == RPing, "Wrong type");
    mu_assert(SameT(qping, reply), "Wrong t");
    mu_assert(HasRecentQuery(client, from_id), "Node query_time not set");

    DhtClient_Destroy(client);
    DhtClient_Destroy(from);
    Message_Destroy(qping);
    Message_Destroy(reply);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_HandleQPing);

    return NULL;
}

RUN_TESTS(all_tests);

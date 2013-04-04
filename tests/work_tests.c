#include "minunit.h"
#include <dht/client.h>
#include <dht/network.h>
#include <dht/work.h>
#include <dht/message_create.h>

#define TESTPORT 21715

char *test_Client_SendReceive()
{
    Hash sender_id = { "sender id" };
    Hash receiver_id = { "receiver id" };
    Client *sender = Client_Create(sender_id, htonl(INADDR_LOOPBACK), TESTPORT, 0);
    Client *receiver = Client_Create(receiver_id, htonl(INADDR_LOOPBACK), TESTPORT + 1, 0);

    NetworkUp(sender);
    NetworkUp(receiver);

    int rc = Client_Receive(receiver);
    mu_assert(rc == 0, "Client_Receive failed");
    rc = Client_Send(sender, sender->queries);
    mu_assert(rc == 0, "Client_Send failed");

    const int count = 8;

    int i = 0;
    for (i = 0; i < count; i++)
    {
        Message *ping = Message_CreateQPing(sender, &receiver->node);

        MessageQueue_Push(sender->queries, ping);
    }

    mu_assert(MessageQueue_Count(sender->queries) == count, "Wrong count");

    rc = Client_Send(sender, sender->queries);
    mu_assert(rc == 0, "Client_Send failed");
    mu_assert(MessageQueue_Count(sender->queries) == 0, "Wrong count");

    mu_assert(MessageQueue_Count(receiver->incoming) == 0, "Wrong count");

    rc = Client_Receive(receiver);
    mu_assert(rc == 0, "Client_Receive failed");
    mu_assert(MessageQueue_Count(receiver->incoming) == count, "Wrong count");

    while (MessageQueue_Count(receiver->incoming) > 0)
    {
        Message_Destroy(MessageQueue_Pop(receiver->incoming));
    }

    NetworkDown(sender);
    NetworkDown(receiver);

    Client_Destroy(sender);
    Client_Destroy(receiver);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Client_SendReceive);

    return NULL;
}

RUN_TESTS(all_tests);

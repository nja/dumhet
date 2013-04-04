#include <arpa/inet.h>

#include "minunit.h"
#include <dht/message_create.h>
#include <dht/table.h>
#include <dht/network.h>

#define TESTPORT 51271

char *test_NetworkUpDown()
{
    Hash id;

    Client *client = Client_Create(id, 0, TESTPORT, 0);

    int rc = NetworkUp(client);
    mu_assert(rc == 0, "NetworkUp failed");

    rc = NetworkDown(client);
    mu_assert(rc == 0, "NetworkDown failed");

    Client_Destroy(client);

    return NULL;
}

char *test_NetworkSendReceive()
{
    Hash id;
    Client *client = Client_Create(id, htonl(INADDR_LOOPBACK), TESTPORT, 0);
  
    int rc = NetworkUp(client);
    mu_assert(rc == 0, "NetworkUp failed");

    const size_t len = 3;
    char send[] = "foo";
    char recv[UDPBUFLEN];
    Node recvnode = {{{ 0 }}};

    rc = Send(client, &client->node, send, len);
    mu_assert(rc == 0, "Send failed");

    rc = Receive(client, &recvnode, recv, UDPBUFLEN);
    mu_assert(rc == (int)len, "Receive failed");

    mu_assert(strncmp(send, recv, len) == 0, "Received wrong data");
    mu_assert(client->node.addr.s_addr == recvnode.addr.s_addr, "Wrong recv addr");
    mu_assert(client->node.port == recvnode.port, "Wrong recv port");

    rc = NetworkDown(client);
    mu_assert(rc == 0, "NetworkDown failed");

    Client_Destroy(client);

    return NULL;
}

char *test_NetworkSendReceiveMessage()
{
    Hash ids = { "foo" }, idr = { "bar" };
    Client *sender = Client_Create(ids, htonl(INADDR_LOOPBACK), TESTPORT, 0);
    Client *recver = Client_Create(idr, htonl(INADDR_LOOPBACK), TESTPORT + 1, 0);

    int rc = NetworkUp(sender);
    mu_assert(rc == 0, "NetworkUp failed");
    rc = NetworkUp(recver);
    mu_assert(rc == 0, "NetworkUp failed");

    Message *send = Message_CreateQPing(sender, &recver->node);

    rc = SendMessage(sender, send);
    mu_assert(rc == 0, "SendMessage failed");

    Message *recv = ReceiveMessage(recver);
    mu_assert(recv != NULL, "ReceiveMessage failed");
    mu_assert(recv->type == QPing, "Received wrong type");
    mu_assert(recv->t_len == sizeof(tid_t), "Received wrong t_len");
    mu_assert(*(tid_t *)recv->t == *(tid_t *)send->t, "Received wrong t");
    mu_assert(Distance_Compare(&send->id, &recv->id) == 0, "Received wrong id");

    mu_assert(Node_Same(&sender->node, &recv->node), "Wrong node");

    Message_Destroy(send);
    Message_Destroy(recv);

    Client_Destroy(sender);
    Client_Destroy(recver);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_NetworkUpDown);
    mu_run_test(test_NetworkSendReceive);
    mu_run_test(test_NetworkSendReceiveMessage);

    return NULL;
}

RUN_TESTS(all_tests);

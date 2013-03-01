#include <arpa/inet.h>

#include "minunit.h"
#include <dht/dht.h>
#include <dht/network.h>

#define TESTPORT 51271

char *test_NetworkUpDown()
{
  DhtHash id;

  DhtClient *client = DhtClient_Create(id, 0, TESTPORT);

  int rc = NetworkUp(client);
  mu_assert(rc == 0, "NetworkUp failed");  

  rc = NetworkDown(client);
  mu_assert(rc == 0, "NetworkDown failed");

  DhtClient_Destroy(client);

  return NULL;
}

char *test_NetworkSendReceive()
{
  DhtHash id;
  DhtClient *client = DhtClient_Create(id, htonl(INADDR_LOOPBACK), TESTPORT);
  
  int rc = NetworkUp(client);
  mu_assert(rc == 0, "NetworkUp failed");  

  const size_t len = 3;
  char send[] = "foo";
  char recv[UDPBUFLEN];
  DhtNode recvnode = {{{ 0 }}};

  rc = Send(client, &client->node, send, len);
  mu_assert(rc == 0, "Send failed");

  rc = Receive(client, &recvnode, recv, UDPBUFLEN);
  mu_assert(rc == (int)len, "Receive failed");

  mu_assert(strncmp(send, recv, len) == 0, "Received wrong data");
  mu_assert(client->node.addr.s_addr == recvnode.addr.s_addr, "Wrong recv addr");
  mu_assert(client->node.port == recvnode.port, "Wrong recv port");

  rc = NetworkDown(client);
  mu_assert(rc == 0, "NetworkDown failed");

  DhtClient_Destroy(client);

  return NULL;
}

char *test_NetworkSendReceiveMessage()
{
    DhtHash ids = { "foo" }, idr = { "bar" };
    DhtClient *sender = DhtClient_Create(ids, htonl(INADDR_LOOPBACK), TESTPORT);
    DhtClient *recver = DhtClient_Create(idr, htonl(INADDR_LOOPBACK), TESTPORT + 1);

    int rc = NetworkUp(sender);
    mu_assert(rc == 0, "NetworkUp failed");
    rc = NetworkUp(recver);
    mu_assert(rc == 0, "NetworkUp failed");

    Message *send = Ping_Create(sender);

    rc = SendMessage(sender, send, &recver->node);
    mu_assert(rc == 0, "SendMessage failed");

    DhtNode from = {{{ 0 }}};

    Message *recv = ReceiveMessage(recver, &from);
    mu_assert(recv != NULL, "ReceiveMessage failed");
    mu_assert(recv->type == QPing, "Received wrong type");
    mu_assert(recv->t_len == sizeof(tid_t), "Received wrong t_len");
    mu_assert(*(tid_t *)recv->t == *(tid_t *)send->t, "Received wrong t");
    mu_assert(DhtDistance_Compare(&send->id, &recv->id) == 0, "Received wrong id");

    mu_assert(from.addr.s_addr == sender->node.addr.s_addr, "Wrong from addr");
    mu_assert(from.port == sender->node.port, "Wrong from port");

    Message_Destroy(send);
    Message_Destroy(recv);

    DhtClient_Destroy(sender);
    DhtClient_Destroy(recver);

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

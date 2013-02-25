#include "minunit.h"
#include <network.h>
#include <arpa/inet.h>

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

char *all_tests()
{
  mu_suite_start();

  mu_run_test(test_NetworkUpDown);
  mu_run_test(test_NetworkSendReceive);

  return NULL;
}

RUN_TESTS(all_tests);

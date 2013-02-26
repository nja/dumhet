#include "minunit.h"
#include <dht/dhtclient.h>

char *test_DhtClient_CreateDestroy()
{
  DhtHash id = { "abcdeABCDEabcdeABCD" };
  uint32_t addr = 0x10203040;
  uint16_t port = 0x5060;

  DhtClient *client = DhtClient_Create(id, addr, port);
  mu_assert(client != NULL, "DhtClient_Create failed");
  mu_assert(client->table != NULL, "No DhtTable created");
  mu_assert(client->pending != NULL, "No pending responses");
  mu_assert(client->buf != NULL, "No buffer allocated");
  mu_assert(client->node.addr.s_addr == addr, "Address not copied");
  mu_assert(client->node.port == port, "Port not copied");

  DhtClient_Destroy(client);
  
  return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtClient_CreateDestroy);

    return NULL;
}

RUN_TESTS(all_tests);

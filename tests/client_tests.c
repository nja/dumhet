#include "minunit.h"
#include <dht/client.h>

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

char *test_Token()
{
    DhtHash id = { "foobar" };
    uint32_t addr = 0x10203040;
    uint16_t port = 0x5060;

    DhtClient *client = DhtClient_Create(id, addr, port);

    DhtNode node_a = {{{ 0 }}};
    DhtNode node_b = {{{ 0 }}};
    node_a.addr.s_addr = 1;
    node_b.addr.s_addr = 2;

    Token token_a = DhtClient_MakeToken(client, &node_a);
    Token token_b = DhtClient_MakeToken(client, &node_b);

    int i = 0;
    while (i++ < SECRETS_LEN)
    {
        mu_assert(DhtClient_IsValidToken(client, &node_a,
                                         token_a.value, HASH_BYTES),
                  "Token should be valid");
        mu_assert(DhtClient_IsValidToken(client, &node_b,
                                         token_b.value, HASH_BYTES),
                  "Token should be valid");

        mu_assert(!DhtClient_IsValidToken(client, &node_b,
                                          token_a.value, HASH_BYTES),
                  "Token should be invalid");
        mu_assert(!DhtClient_IsValidToken(client, &node_a,
                                          token_b.value, HASH_BYTES),
                  "Token should be invalid");

        DhtClient_NewSecret(client);
    }

    mu_assert(!DhtClient_IsValidToken(client, &node_a,
                                      token_a.value, HASH_BYTES),
              "Token should now be invalid");
    mu_assert(!DhtClient_IsValidToken(client, &node_b,
                                     token_b.value, HASH_BYTES),
              "Token should now be invalid");

    DhtClient_Destroy(client);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtClient_CreateDestroy);
    mu_run_test(test_Token);

    return NULL;
}

RUN_TESTS(all_tests);

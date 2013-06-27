#include "minunit.h"
#include <dht/client.h>

char *test_Client_CreateDestroy()
{
    Hash id = { "abcdeABCDEabcdeABCD" };
    uint32_t addr = 0x10203040;
    uint16_t port = 0x5060;
    uint16_t peer_port = 0x0506;

    Client *client = Client_Create(id, addr, port, peer_port);
    mu_assert(client != NULL, "Client_Create failed");
    mu_assert(client->table != NULL, "No Table created");
    mu_assert(client->pending != NULL, "No pending responses");
    mu_assert(client->buf != NULL, "No buffer allocated");
    mu_assert(client->searches != NULL, "No searches array");
    mu_assert(client->node.addr.s_addr == addr, "Address not copied");
    mu_assert(client->node.port == port, "Port not copied");
    mu_assert(client->peer_port == peer_port, "Peer port not copied");

    Client_Destroy(client);
  
    return NULL;
}

char *test_Token()
{
    Hash id = { "foobar" };
    uint32_t addr = 0x10203040;
    uint16_t port = 0x5060;
    uint16_t peer_port = 0x0506;

    Client *client = Client_Create(id, addr, port, peer_port);

    Node node_a = {{{ 0 }}};
    Node node_b = {{{ 0 }}};
    node_a.addr.s_addr = 1;
    node_b.addr.s_addr = 2;

    Token token_a = Client_MakeToken(client, &node_a);
    Token token_b = Client_MakeToken(client, &node_b);

    int i = 0;
    while (i++ < SECRETS_LEN)
    {
        mu_assert(Client_IsValidToken(client, &node_a,
                                      token_a.value, HASH_BYTES),
                  "Token should be valid");
        mu_assert(Client_IsValidToken(client, &node_b,
                                      token_b.value, HASH_BYTES),
                  "Token should be valid");

        mu_assert(!Client_IsValidToken(client, &node_b,
                                       token_a.value, HASH_BYTES),
                  "Token should be invalid");
        mu_assert(!Client_IsValidToken(client, &node_a,
                                       token_b.value, HASH_BYTES),
                  "Token should be invalid");

        Client_NewSecret(client);
    }

    mu_assert(!Client_IsValidToken(client, &node_a,
                                   token_a.value, HASH_BYTES),
              "Token should now be invalid");
    mu_assert(!Client_IsValidToken(client, &node_b,
                                   token_b.value, HASH_BYTES),
              "Token should now be invalid");

    Client_Destroy(client);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Client_CreateDestroy);
    mu_run_test(test_Token);

    return NULL;
}

RUN_TESTS(all_tests);

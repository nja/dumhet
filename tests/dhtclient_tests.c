#include "minunit.h"
#include <dhtclient.h>

char *test_DhtClient_CreateDestroy()
{
  DhtHash id;

  DhtClient *client = DhtClient_Create(id, 0, 0);
  mu_assert(client != NULL, "DhtClient_Create failed");
  
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

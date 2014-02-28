#include "minunit.h"
#include <dht/client.h>
#include <dht/hooks.h>

Client *test_client = NULL;

void TestHookFun(void *hook_client, void *args)
{
    if (test_client == hook_client) {
        *(int *)args = 1;
    }
}

char *test_hooks()
{
    HookType hook_type = HookAddPeer;
    test_client = calloc(1, sizeof(Client));
    mu_assert(test_client != NULL, "calloc failed");

    test_client->hooks = Hooks_Create();
    mu_assert(test_client->hooks != NULL, "Hooks_Create failed");

    Hook *hook = Hook_Create(hook_type, TestHookFun);
    mu_assert(hook != NULL, "Hook_Create failed");

    int rc = Client_AddHook(test_client, hook);
    mu_assert(rc == 0, "AddHook failed");

    int x = 0;

    int count = Client_RunHook(test_client, hook_type, &x);
    mu_assert(count == 1, "Wrong count");
    mu_assert(x == 1, "Hook failed");

    Hooks_Destroy(test_client->hooks);
    Hook_Destroy(hook);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_hooks);

    return NULL;
}

RUN_TESTS(all_tests);

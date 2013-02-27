#include "minunit.h"
#include <dht/pendingresponses.h>

char *test_create_destroy()
{
    HashmapPendingResponses *responses = HashmapPendingResponses_Create();
    mu_assert(responses != NULL, "HashmapPendingResponses_Create failed");
    HashmapPendingResponses_Destroy(responses);

    return NULL;
}

char *test_destroy_with_entries()
{
    HashmapPendingResponses *responses = HashmapPendingResponses_Create();
    mu_assert(responses != NULL, "HashmapPendingResponses_Create failed");

    HashmapPendingResponses_Add(responses, QFindNode, 0);

    HashmapPendingResponses_Destroy(responses);

    return NULL;
}

char *test_compare()
{
    tid_t a = 0, b = 1;

    int cmp = PendingResponse_Compare(&a, &b);
    mu_assert(cmp < 0, "Compare fail");

    cmp = PendingResponse_Compare(&b, &a);
    mu_assert(cmp > 0, "Compare fail");

    cmp = PendingResponse_Compare(&a, &a);
    mu_assert(cmp == 0, "Compare fail");

    return NULL;
}

char *test_addremove()
{
    HashmapPendingResponses *responses = HashmapPendingResponses_Create();
    mu_assert(responses != NULL, "Hashmappendingresponses_Create failed");

    tid_t tid[] = { 1, 2, 3, 4, 0 };
    MessageType type[] = { QPing, QFindNode, QAnnouncePeer, QGetPeers };

    int i = 0;

    while (tid[i] != 0)
    {
	int rc = HashmapPendingResponses_Add(responses, type[i], tid[i]);
	mu_assert(rc == 0, "HashmapPendingResponses_Add failed");

	++i;
    }

    i = 0;
    
    while(tid[i] != 0)
    {
	MessageType rtype = RError;
	int rc = HashmapPendingResponses_Remove(responses, (char *)&tid[i], &rtype);
	mu_assert(rc == 0, "HashmapPendingResponses_Remove failed");
	mu_assert(rtype == type[i], "Wrong type");

	++i;
    }

    HashmapPendingResponses_Destroy(responses);

    return NULL;
}    

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_create_destroy);
    mu_run_test(test_destroy_with_entries);
    mu_run_test(test_compare);
    mu_run_test(test_addremove);

    return NULL;
}

RUN_TESTS(all_tests);

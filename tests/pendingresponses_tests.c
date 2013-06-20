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

    PendingResponse entry = { QFindNode, 0, {{ 0 }}, NULL };
    int rc = responses->addPendingResponse(responses, entry);
    mu_assert(rc == 0, "HashmapPendingResponses_Add failed");

    HashmapPendingResponses_Destroy(responses);

    return NULL;
}

char *test_compare()
{
    HashmapPendingResponses *responses = HashmapPendingResponses_Create();
    mu_assert(responses != NULL, "HashmapPendingResponses_Create failed");

    tid_t a = 0, b = 1;

    int cmp = responses->hashmap->compare(&a, &b);
    mu_assert(cmp < 0, "Compare fail");

    cmp = responses->hashmap->compare(&b, &a);
    mu_assert(cmp > 0, "Compare fail");

    cmp = responses->hashmap->compare(&a, &a);
    mu_assert(cmp == 0, "Compare fail");

    HashmapPendingResponses_Destroy(responses);

    return NULL;
}

char *test_addremove()
{
    HashmapPendingResponses *responses = HashmapPendingResponses_Create();
    mu_assert(responses != NULL, "Hashmappendingresponses_Create failed");

    tid_t tid[] = { 1, 2, 3, 4, 0 };
    MessageType type[] = { QPing, QFindNode, QAnnouncePeer, QGetPeers };

    int i = 0, dummy;

    while (tid[i] != 0)
    {
        PendingResponse entry = { type[i], tid[i], {{ 0 }}, &dummy };
	int rc = responses->addPendingResponse(responses, entry);
	mu_assert(rc == 0, "HashmapPendingResponses_Add failed");

	++i;
    }

    i = 0;
    
    while(tid[i] != 0)
    {
        int rc;
	PendingResponse entry
            = responses->getPendingResponse(responses, (char *)&tid[i], &rc);
        mu_assert(rc == 0, "HashmapPendingResponses_Remove failed");
	mu_assert(entry.type == type[i], "Wrong type");
        mu_assert(entry.tid == tid[i], "Wring tid");
        mu_assert(entry.context == &dummy, "Unexpected context");

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

#include <time.h>

#include "minunit.h"
#include <dht/hash.h>
#include <dht/node.h>

char *test_DhtNode_Status()
{
    time_t now = time(NULL);
    DhtHash id = {{ 0 }};

    DhtNode *node = DhtNode_Create(&id);

    for (node->pending_queries = 0; node->pending_queries < NODE_MAX_PENDING;
	 node->pending_queries++) {
	mu_assert(DhtNode_Status(node, now) == Unknown, "Wrong status");
    }

    mu_assert(DhtNode_Status(node, now) == Bad, "Wrong status");

    node->pending_queries = 0;
    node->reply_time = now;

    mu_assert(DhtNode_Status(node, now) == Good, "Wrong status");

    node->reply_time -= NODE_RESPITE;
    node->query_time = now;

    mu_assert(DhtNode_Status(node, now) == Good, "Wrong status");

    node->query_time -= NODE_RESPITE;

    for (node->pending_queries = 0; node->pending_queries < NODE_MAX_PENDING;
	 node->pending_queries++) {
	mu_assert(DhtNode_Status(node, now) == Questionable, "Wrong status");
    }

    mu_assert(DhtNode_Status(node, now) == Bad, "Wrong status");

    free(node);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtNode_Status);

    return NULL;
}

RUN_TESTS(all_tests);

#include "minunit.h"
#include <lcthw/list.h>
#include <assert.h>

static List *list = NULL;
char *test1 = "test1 data";
char *test2 = "test2 data";
char *test3 = "test3 data";


char *test_create()
{
    list = List_create();
    mu_assert(list != NULL, "Failed to create list.");

    return NULL;
}


char *test_destroy()
{
    void *a = malloc(1), *b = malloc(2);

    mu_assert(a != NULL, "malloc failed");
    mu_assert(b != NULL, "malloc failed");

    List_push(list, a);
    List_push(list, b);

    List_clear_destroy(list);

    return NULL;
}


char *test_push_pop()
{
    List_push(list, test1);
    mu_assert(List_last(list) == test1, "Wrong last value.");

    List_push(list, test2);
    mu_assert(List_last(list) == test2, "Wrong last value");

    List_push(list, test3);
    mu_assert(List_last(list) == test3, "Wrong last value.");
    mu_assert(List_count(list) == 3, "Wrong count on push.");

    char *val = List_pop(list);
    mu_assert(val == test3, "Wrong value on pop.");

    val = List_pop(list);
    mu_assert(val == test2, "Wrong value on pop.");

    val = List_pop(list);
    mu_assert(val == test1, "Wrong value on pop.");
    mu_assert(List_count(list) == 0, "Wrong count after pop.");

    return NULL;
}

char *test_shift()
{
    List_shift(list, test1);
    mu_assert(List_first(list) == test1, "Wrong last value.");

    List_shift(list, test2);
    mu_assert(List_first(list) == test2, "Wrong last value");

    List_shift(list, test3);
    mu_assert(List_first(list) == test3, "Wrong last value.");
    mu_assert(List_count(list) == 3, "Wrong count on shift.");

    return NULL;
}

char *test_remove()
{
    // we only need to test the middle remove case since push/shift 
    // already tests the other cases

    char *val = List_remove(list, list->first->next);
    mu_assert(val == test2, "Wrong removed element.");
    mu_assert(List_count(list) == 2, "Wrong count after remove.");
    mu_assert(List_first(list) == test3, "Wrong first after remove.");
    mu_assert(List_last(list) == test1, "Wrong last after remove.");

    return NULL;
}


char *test_unshift()
{
    char *val = List_unshift(list);
    mu_assert(val == test3, "Wrong value on unshift.");

    val = List_unshift(list);
    mu_assert(val == test1, "Wrong value on unshift.");
    mu_assert(List_count(list) == 0, "Wrong count after unshift.");

    return NULL;
}

char *test_remove_all()
{
    List_push(list, test1);
    List_push(list, test2);
    List_push(list, test3);
    List_push(list, test3);
    List_push(list, test1);
    List_push(list, test2);
    List_push(list, test3);

    mu_assert(List_count(list) == 7, "Wrong count");

    debug("aaaa");

    List_remove_all(list, test1);
    mu_assert(List_count(list) == 5, "Wrong count");
    mu_assert(List_first(list) == test2, "Wrong first");
    mu_assert(List_last(list) == test3, "Wrong last");

    debug("bbb");

    List_remove_all(list, test3);
    mu_assert(List_count(list) == 2, "Wrong count");
    mu_assert(List_first(list) == test2, "Wrong first");
    mu_assert(List_last(list) == test2, "Wrong last");

    debug("ccc");

    List_remove_all(list, test2);
    debug("ddd");

    mu_assert(List_count(list) == 0, "Wrong count");
    mu_assert(List_first(list) == NULL, "Wrong first");
    mu_assert(List_last(list) == NULL, "Wrong last");

    return NULL;
}

char *all_tests() {
    mu_suite_start();

    mu_run_test(test_create);
    mu_run_test(test_push_pop);
    mu_run_test(test_shift);
    mu_run_test(test_remove);
    mu_run_test(test_unshift);
    mu_run_test(test_remove_all);
    mu_run_test(test_destroy);

    return NULL;
}

RUN_TESTS(all_tests);


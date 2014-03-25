#include <assert.h>

#include <dht/client.h>
#include <dht/hooks.h>
#include <lcthw/darray.h>
#include <lcthw/dbg.h>
#include <lcthw/list.h>

DArray *Hooks_Create()
{
    DArray *array = DArray_create(sizeof(Hook *), HookTypeMax);
    check(array != NULL, "DArray_create failed");

    int i;
    for (i = 0; i < HookTypeMax; i++)
    {
        List *list = List_create();
        check_mem(list);
        DArray_push(array, list);
    }

    return array;
error:
    Hooks_Destroy(array);
    return NULL;
}

void Hooks_Destroy(DArray *array)
{
    if (array == NULL)
        return;

    while (DArray_count(array) > 0)
    {
        List_destroy(DArray_pop(array));
    }

    DArray_destroy(array);
}
    
int Client_AddHook(void *client_, Hook *hook)
{
    Client *client = (Client *)client_;

    assert(client != NULL && "NULL client pointer");
    assert(hook != NULL && "NULL Hook pointer");
    assert(client->hooks != NULL && "NULL Hooks pointer");

    List *list = DArray_get(client->hooks, hook->type);
    check(list != NULL, "NULL Hook List pointer");

    List_push(list, hook);
    return 0;
error:
    return -1;
}

int Client_RemoveHook(void *client_, Hook *hook)
{
    Client *client = (Client *)client_;

    assert(client != NULL && "NULL client pointer");
    assert(hook != NULL && "NULL Hook pointer");
    assert(client->hooks != NULL && "NULL DArray pointer");

    List *list = DArray_get(client->hooks, hook->type);
    check(list != NULL, "NULL Hook List pointer");

    List_remove_all(list, hook);

    return 0;
error:
    return -1;
}

Hook *Hook_Create(HookType type, HookOp fun)
{
    assert(fun != NULL && "NULL HookOp");

    Hook *hook = malloc(sizeof(Hook));
    check_mem(hook);

    hook->type = type;
    hook->fun = fun;

    return hook;
error:
    return NULL;
}

void Hook_Destroy(Hook *hook)
{
    free(hook);
}

int Client_RunHook(void *client_, HookType type, void *args)
{
    Client *client = (Client *)client_;

    assert(client != NULL && "NULL Client pointer");

    if (client->hooks == NULL)
        return 0;

    List *list = DArray_get(client->hooks, type);

    if (list == NULL)
        return 0;

    int count = 0;

    LIST_FOREACH(list, first, next, cur)
    {
        ((Hook *)cur->value)->fun(client, args);
        count++;
    }

    return count;
}

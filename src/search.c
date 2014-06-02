#include <assert.h>

#include <dht/search.h>
#include <dht/client.h>
#include <dht/close.h>
#include <dht/message_create.h>
#include <dht/table.h>
#include <lcthw/dbg.h>

Search *Search_Create(Hash *id)
{
    assert(id != NULL && "NULL Hash pointer");

    Search *search = calloc(1, sizeof(Search));
    check_mem(search);

    search->table = Table_Create(id);
    check(search->table != NULL, "Table_Create failed");

    search->peers = Peers_Create(id);
    check(search->peers != NULL, "Peers_Create failed");

    search->tokens = Hashmap_create(
        (Hashmap_compare)Distance_Compare,
        (Hashmap_hash)Hash_Hash);
    check(search->tokens != NULL, "Hashmap_create failed");

    return search;
error:
    Search_Destroy(search);
    return NULL;
}

struct FTokenEntry {
    struct FToken token;
    Hash id;
};

int FreeFTokenEntry_cb(void *, HashmapNode *node);

void Search_Destroy(Search *search)
{
    if (search == NULL)
    {
        return;
    }

    Table_DestroyNodes(search->table);
    Table_Destroy(search->table);
    Peers_Destroy(search->peers);

    Hashmap_traverse(search->tokens, NULL, FreeFTokenEntry_cb);
    Hashmap_destroy(search->tokens);

    free(search);
}

int Search_IsDone(Search *search, time_t now)
{
    const int search_respite = 3;

    if (search->send_time == 0)
        return 0;

    if (search->send_time + search_respite < now)
        return 1;

    return 0;
}

int Search_CopyTable(Search *search, Table *source)
{
    assert(search != NULL && "NULL Search pointer");
    assert(source != NULL && "NULL Table pointer");
    assert(search->table != NULL && "NULL Table pointer");

    return Table_ForEachNode(source, search->table, (NodeOp)Table_CopyAndAddNode);
}

int Search_AddPeers(Search *search, Peer *peers, int count)
{
    assert(search != NULL && "NULL Search pointer");
    assert(peers != NULL && "NULL Peer pointer");

    Peer *end = peers + count;

    while (peers < end)
    {
        int rc = Peers_AddPeer(search->peers, peers);
        check(rc == 0, "Peers_AddPeer failed");
        peers++;
    }

    return 0;
error:
    return -1;
}

void FTokenEntry_Delete(struct FTokenEntry *entry)
{
    if (entry != NULL)
    {
        free(entry->token.data);
        free(entry);
    }
}

int FreeFTokenEntry_cb(void *context, HashmapNode *node)
{
    (void)context;

    struct FTokenEntry *entry = node->data;
    FTokenEntry_Delete(entry);
    return 0;
}

struct FTokenEntry *FTokenEntry_Create(struct FToken token, Hash *id)
{
    assert(id != NULL && "NULL Hash pointer");

    struct FTokenEntry *entry = malloc(sizeof(struct FTokenEntry));
    check_mem(entry);

    entry->token.data = malloc(token.len);
    check_mem(entry->token.data);

    memcpy(entry->token.data, token.data, token.len);
    entry->token.len = token.len;
    entry->id = *id;

    return entry;
error:
    free(entry);
    return NULL;
}

int Search_SetToken(Search *search, Hash *id, struct FToken token)
{
    assert(search != NULL && "NULL Search pointer");
    assert(id != NULL && "NULL Hash pointer");

    struct FTokenEntry *old = Hashmap_delete(search->tokens, id);
    FTokenEntry_Delete(old);

    struct FTokenEntry *entry = FTokenEntry_Create(token, id);

    int rc = Hashmap_set(search->tokens, &entry->id, entry);
    check(rc == 0, "Hashmap_set failed");

    return 0;
error:
    FTokenEntry_Delete(entry);
    return -1;
}

struct FToken *Search_GetToken(Search *search, Hash *id)
{
    assert(search != NULL && "NULL Search pointer");
    assert(id != NULL && "NULL Hash pointer");

    struct FTokenEntry *entry = Hashmap_get(search->tokens, id);

    if (entry == NULL)
    {
        return NULL;
    }

    return &entry->token;
}

struct ClientSearch {
    Client *client;
    Search *search;
    int count;
};

int SendFindNodes(struct ClientSearch *context, Node *node)
{
    if (node->rfindnode_count > 0)
        return 0;

    Message *query = Message_CreateQFindNode(context->client,
                                             node,
                                             &context->search->table->id);
    check(query != NULL, "Message_CreateQFindNode failed");

    query->context = context->search;

    int rc = MessageQueue_Push(context->client->queries, query);
    check(rc == 0, "MessageQueue_Push failed");

    node->pending_queries++;

    return 0;
error:
    Message_Destroy(query);
    return -1;
}

int SendGetPeers(struct ClientSearch *context, Node *node)
{
    if (node->rgetpeers_count > 0)
        return 0;

    Message *query = Message_CreateQGetPeers(context->client,
                                             node,
                                             &context->search->table->id);
    check(query != NULL, "Message_CreateQGetPeers failed");

    query->context = context->search;

    int rc = MessageQueue_Push(context->client->queries, query);
    check(rc == 0, "MessageQueue_Push failed");

    node->pending_queries++;
    context->count++;

    return 0;
error:
    Message_Destroy(query);
    return -1;
}

int SendAnnouncePeer(struct ClientSearch *context, Node *node)
{
    if (node->rannounce_count > 0)
        return 0;

    struct FToken *token = Search_GetToken(context->search,
                                           &node->id);

    if (token == NULL)
    {
        return 0;
    }

    Message *query = Message_CreateQAnnouncePeer(context->client,
                                                 node,
                                                 &context->search->table->id,
                                                 token->data,
                                                 token->len);
    check(query != NULL, "Message_CreateQAnnouncePeer failed");

    query->context = context->search;

    int rc = MessageQueue_Push(context->client->queries, query);
    check(rc == 0, "MessageQueue_Push failed");
    query->context = context->search;


    node->pending_queries++;
    context->count++;

    return 0;
error:
    Message_Destroy(query);
    return -1;
}

int Search_DoWork(Client *client, Search *search)
{
    assert(client != NULL && "NULL Client pointer");
    assert(search != NULL && "NULL Search pointer");

    struct ClientSearch context = { .client = client, .search = search };

    int rc = Table_ForEachNode(search->table, &context, (NodeOp)SendFindNodes);
    check(rc == 0, "SendFindNodes failed");

    rc = Table_ForEachCloseNode(search->table,
                                &context,
                                (NodeOp)SendGetPeers);
    check(rc == 0, "SendGetPeers failed");

    rc = Table_ForEachCloseNode(search->table,
                                &context,
                                (NodeOp)SendAnnouncePeer);
    check(rc == 0, "SendAnnouncePeer failed");

    if (context.count > 0)
        search->send_time = time(NULL);

    return 0;
error:
    return -1;
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/sha.h>

#include <lcthw/dbg.h>
#include <dht/client.h>
#include <dht/network.h>
#include <dht/pendingresponses.h>
#include <dht/random.h>

int CreateSocket();

Client *Client_Create(DhtHash id,
                            uint32_t addr,
                            uint16_t port,
                            uint16_t peer_port)
{
    RandomState *rs = NULL;
    Client *client = calloc(1, sizeof(Client));
    check_mem(client);

    client->node.id = id;
    client->node.addr.s_addr = addr;
    client->node.port = port;

    client->peer_port = peer_port;

    client->table = DhtTable_Create(&client->node.id);
    check_mem(client->table);

    client->pending = (struct PendingResponses *)HashmapPendingResponses_Create();
    check(client->pending != NULL, "HashmapPendingResponses_Create failed");

    client->buf = calloc(1, UDPBUFLEN);
    check_mem(client->buf);

    client->peers = PeersHashmap_Create();
    check(client->peers != NULL, "PeersHashmap_Create failed");

    rs = RandomState_Create(time(NULL));
    check(rs != NULL, "RandomState_Create failed");

    int rc = Random_Fill(rs, (char *)client->secrets, SECRETS_LEN * sizeof(DhtHash));
    check(rc == 0, "Random_Fill failed");

    client->socket = CreateSocket();
    check(client->socket != -1, "CreateSocket failed");

    RandomState_Destroy(rs);

    return client;
error:
    RandomState_Destroy(rs);

    if (client != NULL)
    {
        free(client->buf);
        HashmapPendingResponses_Destroy((HashmapPendingResponses *)client->pending);
        DhtTable_Destroy(client->table);
        PeersHashmap_Destroy(client->peers);
    }

    free(client);

    return NULL;
}

void Client_Destroy(Client *client)
{
    if (client == NULL)
        return;

    DhtTable_Destroy(client->table);
    HashmapPendingResponses_Destroy((HashmapPendingResponses *)client->pending);
    free(client->buf);
    PeersHashmap_Destroy(client->peers);
  
    if (client->socket != -1)
        close(client->socket);

    free(client);
}

#define TOKEN_DATA_LEN (sizeof(DhtHash) + sizeof(in_addr_t))

Token MakeToken(Client *client, DhtNode *from, int secret)
{
    assert(client != NULL && "NULL Client pointer");
    assert(from != NULL && "NULL DhtNode pointer");
    assert(0 <= secret && secret < SECRETS_LEN && "Bad secret");

    unsigned char data[TOKEN_DATA_LEN];
    memcpy(data, &client->secrets[secret], sizeof(DhtHash));
    memcpy(data + sizeof(DhtHash), &from->addr.s_addr, sizeof(in_addr_t));

    Token token;

    SHA1(data, TOKEN_DATA_LEN, (unsigned char *)token.value);

    return token;
}

Token Client_MakeToken(Client *client, DhtNode *from)
{
    assert(client != NULL && "NULL Client pointer");
    assert(from != NULL && "NULL DhtNode pointer");

    return MakeToken(client, from, 0);
}

int Client_IsValidToken(Client *client, DhtNode *from,
                           char *token, size_t token_len)
{
    assert(client != NULL && "NULL Client pointer");
    assert(from != NULL && "NULL DhtNode pointer");
    assert(token != NULL && "NULL token char pointer");

    if (token_len != HASH_BYTES)
        return 0;

    int i = 0;
    for (i = 0; i < SECRETS_LEN; i++)
    {
        Token valid = MakeToken(client, from, i);

        if (DhtHash_Equals((DhtHash *)token, (DhtHash *)&valid))
            return 1;
    }

    return 0;
}

int Client_NewSecret(Client *client)
{
    assert(client != NULL && "NULL Client pointer");
    assert(client->buf != NULL && "NULL Client buf pointer");
    assert(HASH_BYTES == SHA_DIGEST_LENGTH && "Size confusion");

    SHA_CTX ctx;

    int rc = SHA1_Init(&ctx);
    check(rc == 1, "SHA1_Init failed");

    rc = SHA1_Update(&ctx, client->table->id.value, HASH_BYTES);
    check(rc == 1, "SHA1_Update failed");

    rc = SHA1_Update(&ctx, client->buf, UDPBUFLEN);
    check(rc == 1, "SHA1_Update failed");

    rc = SHA1_Update(&ctx, client->secrets, sizeof(DhtHash) * SECRETS_LEN);
    check(rc == 1, "SHA1_Update failed");

    int i;
    for (i = SECRETS_LEN - 1; i > 0; i--)
        client->secrets[i] = client->secrets[i - 1];

    rc = SHA1_Final((unsigned char *)&client->secrets[0].value, &ctx);
    check(rc == 1, "SHA1_Final failed");

    return 0;
error:
    return -1;
}

int CreateSocket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    check(sock != -1, "Create socket failed");

    return sock;
error:
    return -1;
}

int Client_AddPeer(Client *client, DhtHash *info_hash, Peer *peer)
{
    assert(client != NULL && "NULL Client pointer");
    assert(info_hash != NULL && "NULL DhtHash pointer");
    assert(peer != NULL && "NULL Peer pointer");

    int rc = PeersHashmap_AddPeer(client->peers, info_hash, peer);
    check(rc == 0, "PeersHashmap_AddPeer failed");

    return 0;
error:
    return -1;
}

int Client_GetPeers(Client *client, DhtHash *info_hash, DArray **peers)
{
    assert(client != NULL && "NULL Client pointer");
    assert(info_hash != NULL && "NULL DhtHash pointer");
    assert(peers != NULL && "NULL pointer to DArray pointer");

    *peers = DArray_create(sizeof(Peer *), 128);
    check(*peers != NULL, "DArray_create failed");

    int rc = PeersHashmap_GetPeers(client->peers, info_hash, *peers);
    check(rc == 0, "PeersHashmap_GetPeers failed");

    return 0;
error:
    DArray_destroy(*peers);
    return -1;
}

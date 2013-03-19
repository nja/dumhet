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

DhtClient *DhtClient_Create(DhtHash id, uint32_t addr, uint16_t port)
{
  RandomState *rs = NULL;
  DhtClient *client = calloc(1, sizeof(DhtClient));
  check_mem(client);

  client->node.id = id;
  client->node.addr.s_addr = addr;
  client->node.port = port;

  client->table = DhtTable_Create(&client->node.id);
  check_mem(client->table);

  client->pending = (struct PendingResponses *)HashmapPendingResponses_Create();
  check(client->pending != NULL, "HashmapPendingResponses_Create failed");

  client->buf = calloc(1, UDPBUFLEN);
  check_mem(client->buf);

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
  }

  free(client);

  return NULL;
}

void DhtClient_Destroy(DhtClient *client)
{
  if (client == NULL)
    return;

  DhtTable_Destroy(client->table);
  HashmapPendingResponses_Destroy((HashmapPendingResponses *)client->pending);
  free(client->buf);
  
  if (client->socket != -1)
    close(client->socket);

  free(client);
}

#define TOKEN_DATA_LEN (sizeof(DhtHash) + sizeof(in_addr_t))

Token MakeToken(DhtClient *client, DhtNode *from, int secret)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(from != NULL && "NULL DhtNode pointer");
    assert(0 <= secret && secret < SECRETS_LEN && "Bad secret");

    unsigned char data[TOKEN_DATA_LEN];
    memcpy(data, &client->secrets[secret], sizeof(DhtHash));
    memcpy(data + sizeof(DhtHash), &from->addr.s_addr, sizeof(in_addr_t));

    Token token;

    SHA1(data, TOKEN_DATA_LEN, (unsigned char *)token.value);

    return token;
}

Token DhtClient_MakeToken(DhtClient *client, DhtNode *from)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(from != NULL && "NULL DhtNode pointer");

    return MakeToken(client, from, 0);
}

int DhtClient_IsValidToken(DhtClient *client, DhtNode *from,
                           char *token, size_t token_len)
{
    assert(client != NULL && "NULL DhtClient pointer");
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

int DhtClient_NewSecret(DhtClient *client)
{
    assert(client != NULL && "NULL DhtClient pointer");
    assert(client->buf != NULL && "NULL DhtClient buf pointer");
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

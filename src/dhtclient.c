#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <lcthw/dbg.h>
#include <dht/dhtclient.h>
#include <dht/pendingresponses.h>
#include <dht/network.h>

int CreateSocket();

DhtClient *DhtClient_Create(DhtHash id, uint32_t addr, uint16_t port)
{
  DhtClient *client = calloc(1, sizeof(DhtClient));
  check_mem(client);

  client->node.id = id;
  client->node.addr.s_addr = addr;
  client->node.port = port;

  client->table = DhtTable_Create(&client->node.id);
  check_mem(client->table);

  client->pending = HashmapPendingResponses_Create();
  check_mem(client->pending);

  client->buf = malloc(UDPBUFLEN);
  check_mem(client->buf);

  client->socket = CreateSocket();
  check(client->socket != -1, "CreateSocket failed");

  return client;
 error:
  return NULL;
}

void DhtClient_Destroy(DhtClient *client)
{
  if (client == NULL)
    return;

  DhtTable_Destroy(client->table);
  HashmapPendingResponses_Destroy(client->pending);
  free(client->buf);
  
  if (client->socket != -1)
    close(client->socket);

  free(client);
}

int CreateSocket()
{
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  check(sock != -1, "Create socket failed");

  return sock;
 error:
  return -1;
}

Message *Ping_Create(DhtClient *client)
{
    assert(client != NULL && "NULL DhtClient pointer");

    Message *ping = calloc(1, sizeof(Message));
    check_mem(ping);

    ping->type = QPing;

    ping->t = malloc(sizeof(tid_t));
    check_mem(ping->t);
    *(tid_t *)ping->t = client->next_t++;
    ping->t_len = sizeof(tid_t);

    ping->id = client->node.id;

    return ping;
error:
    return NULL;
}

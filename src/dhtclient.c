#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dhtclient.h>
#include <lcthw/dbg.h>

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

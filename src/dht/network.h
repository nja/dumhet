#ifndef _network_h
#define _network_h

#include <dht/client.h>

#define UDPBUFLEN (0xFFFF -8 -20)

int NetworkUp(Client *client);
int NetworkDown(Client *client);

int Send(Client *client, DhtNode *node, char *buf, size_t len);
int Receive(Client *client, DhtNode *node, char *buf, size_t len);

int SendMessage(Client *client, Message *msg, DhtNode *node);
Message *ReceiveMessage(Client *client, DhtNode *node);

#endif

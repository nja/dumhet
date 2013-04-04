#ifndef _dht_network_h
#define _dht_network_h

#include <dht/client.h>

#define UDPBUFLEN (0xFFFF -8 -20)

int NetworkUp(Client *client);
int NetworkDown(Client *client);

int Send(Client *client, Node *node, char *buf, size_t len);
int Receive(Client *client, Node *node, char *buf, size_t len);

int SendMessage(Client *client, Message *msg);
int ReceiveMessage(Client *client, Message **message);

#endif

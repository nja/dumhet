#ifndef _network_h
#define _network_h

#include <dht/dhtclient.h>

#define UDPBUFLEN (0xFFFF -8 -20)

int NetworkUp(DhtClient *client);
int NetworkDown(DhtClient *client);

int Send(DhtClient *client, DhtNode *node, char *buf, size_t len);
int Receive(DhtClient *client, DhtNode *node, char *buf, size_t len);

int SendMessage(DhtClient *client, Message *msg, DhtNode *node);
Message *ReceiveMessage(DhtClient *client, DhtNode *node);

#endif

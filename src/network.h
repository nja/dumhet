#ifndef _network_h
#define _network_h

#include <dhtclient.h>

#define UDPBUFLEN (0xFFFF -8 -20)

int NetworkUp(DhtClient *client);
int NetworkDown(DhtClient *client);

int Send(DhtClient *client, DhtNode *node, char *buf, size_t len);
int Receive(DhtClient *client, DhtNode *node, char *buf, size_t len);

#endif


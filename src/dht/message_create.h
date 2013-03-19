#ifndef _message_create_h
#define _message_create_h

#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQPing(DhtClient *client);

Message *Message_CreateRFindNode(DhtClient *client, Message *query, DArray *found);
Message *Message_CreateRPing(DhtClient *client);

#endif

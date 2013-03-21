#ifndef _message_create_h
#define _message_create_h

#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQPing(DhtClient *client);
Message *Message_CreateQFindNode(DhtClient *client, DhtHash *id);
Message *Message_CreateQGetPeers(DhtClient *client, DhtHash *id);

Message *Message_CreateRFindNode(DhtClient *client, DArray *found);
Message *Message_CreateRPing(DhtClient *client);
Message *Message_CreateRAnnouncePeer(DhtClient *client);
Message *Message_CreateRGetPeers(DhtClient *client,
                                 DArray *peers,
                                 DArray *nodes,
                                 Token *token);

#endif

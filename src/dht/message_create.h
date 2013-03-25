#ifndef _message_create_h
#define _message_create_h

#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQPing(DhtClient *client);
Message *Message_CreateQFindNode(DhtClient *client, DhtHash *id);
Message *Message_CreateQGetPeers(DhtClient *client, DhtHash *id);
Message *Message_CreateQAnnouncePeer(DhtClient *client,
                                     DhtHash *info_hash,
                                     Token *token);

Message *Message_CreateRFindNode(DhtClient *client, Message *query, DArray *found);
Message *Message_CreateRPing(DhtClient *client, Message *query);
Message *Message_CreateRAnnouncePeer(DhtClient *client, Message *query);
Message *Message_CreateRGetPeers(DhtClient *client,
                                 Message *query,
                                 DArray *peers,
                                 DArray *nodes,
                                 Token *token);

#endif

#ifndef _dht_handle_h
#define _dht_handle_h

#include <dht/client.h>
#include <dht/message.h>

typedef int (*ReplyHandler)(Client *client, Message *reply);
typedef Message *(*QueryHandler)(Client *client, Message *query);

ReplyHandler GetReplyHandler(MessageType type);
QueryHandler GetQueryHandler(MessageType type);

int HandleUnknown(Client *client, Message *message);

int HandleRError(Client *client, Message *reply);
int HandleReply(Client *client, Message *reply);
int HandleRFindNode(Client *client, Message *reply);
int HandleRGetPeers(Client *client, Message *reply);

Message *HandleQFindNode(Client *client, Message *query);
Message *HandleQPing(Client *client, Message *query);
Message *HandleQAnnouncePeer(Client *client, Message *query);
Message *HandleQGetPeers(Client *client, Message *query);

#endif

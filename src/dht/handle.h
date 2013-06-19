#ifndef _dht_handle_h
#define _dht_handle_h

#include <dht/client.h>
#include <dht/message.h>

/* A ReplyHandler notes that the node replied and performs any
 * additional actions prompted by the reply.
 * returns 0 on success, -1 on failure. */
typedef int (*ReplyHandler)(Client *client, Message *reply);
/* A QueryHandler notes that the node has sent a query and
 * returns the reply Message on success or NULL on failure. */
typedef Message *(*QueryHandler)(Client *client, Message *query);

/* Gets the appropriate ReplyHandler or QueryHandler */
ReplyHandler GetReplyHandler(MessageType type);
QueryHandler GetQueryHandler(MessageType type);

/* Logs the unknown message type. */
int HandleUnknown(Client *client, Message *message);

/* Logs the error and notes that the node replied. */
int HandleRError(Client *client, Message *reply);
/* Notes that the node replied. */
int HandleReply(Client *client, Message *reply);
/* Adds the replied nodes to the Search and notes that the node replied. */
int HandleRFindNode(Client *client, Message *reply);
/* Adds the replied Peers or nodes to the Search and notes that the node replied. */
int HandleRGetPeers(Client *client, Message *reply);

/* Gathers the closest nodes. */
Message *HandleQFindNode(Client *client, Message *query);
/* Just notes the query. */
Message *HandleQPing(Client *client, Message *query);
/* Adds the announced peer after verifying the query token. */
Message *HandleQAnnouncePeer(Client *client, Message *query);
/* Finds announced peers or closer nodes. */
Message *HandleQGetPeers(Client *client, Message *query);

#endif

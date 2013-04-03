#ifndef _handle_h
#define _handle_h

int HandleReply(DhtClient *client, Message *reply);
int HandleRFindNode(DhtClient *client, Message *reply);
int HandleRGetPeers(DhtClient *client, Message *reply);

Message *HandleQFindNode(DhtClient *client, Message *query);
Message *HandleQPing(DhtClient *client, Message *query);
Message *HandleQAnnouncePeer(DhtClient *client, Message *query, DhtNode *from);
Message *HandleQGetPeers(DhtClient *client, Message *query, DhtNode *from);

#endif

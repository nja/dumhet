#ifndef _handle_h
#define _handle_h

int HandleRFindNode(DhtClient *client, Message *message);
int HandleRGetPeers(DhtClient *client, Message *message);

Message *HandleQFindNode(DhtClient *client, Message *query);
Message *HandleQPing(DhtClient *client, Message *query);
Message *HandleQAnnouncePeer(DhtClient *client, Message *query, DhtNode *from);
Message *HandleQGetPeers(DhtClient *client, Message *query, DhtNode *from);

#endif

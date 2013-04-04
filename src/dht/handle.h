#ifndef _handle_h
#define _handle_h

int HandleReply(Client *client, Message *reply);
int HandleRFindNode(Client *client, Message *reply);
int HandleRGetPeers(Client *client, Message *reply);

Message *HandleQFindNode(Client *client, Message *query);
Message *HandleQPing(Client *client, Message *query);
Message *HandleQAnnouncePeer(Client *client, Message *query, DhtNode *from);
Message *HandleQGetPeers(Client *client, Message *query, DhtNode *from);

#endif

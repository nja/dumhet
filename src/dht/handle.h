#ifndef _dht_handle_h
#define _dht_handle_h

int HandleReply(Client *client, Message *reply);
int HandleRFindNode(Client *client, Message *reply);
int HandleRGetPeers(Client *client, Message *reply);

Message *HandleQFindNode(Client *client, Message *query);
Message *HandleQPing(Client *client, Message *query);
Message *HandleQAnnouncePeer(Client *client, Message *query);
Message *HandleQGetPeers(Client *client, Message *query);

#endif

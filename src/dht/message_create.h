#ifndef _dht_message_create_h
#define _dht_message_create_h

#include <dht/client.h>
#include <dht/message.h>

Message *Message_CreateQPing(Client *client, Node *to);
Message *Message_CreateQFindNode(Client *client, Node *to, Hash *id);
Message *Message_CreateQGetPeers(Client *client, Node *to, Hash *info_hash);
Message *Message_CreateQAnnouncePeer(Client *client,
                                     Node *to,
                                     Hash *info_hash,
                                     char *token,
                                     size_t token_len);

Message *Message_CreateRFindNode(Client *client, Message *query, DArray *found);
Message *Message_CreateRPing(Client *client, Message *query);
Message *Message_CreateRAnnouncePeer(Client *client, Message *query);
Message *Message_CreateRGetPeers(Client *client,
                                 Message *query,
                                 DArray *peers,
                                 DArray *nodes,
                                 Token *token);

Message *Message_CreateRErrorBadToken(Client *client, Message *query);
Message *Message_CreateRError(Client *client, Message *query);

#endif

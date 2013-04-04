#ifndef _dht_work_h
#define _dht_work_h

#include <dht/client.h>

int Client_Send(Client *client, MessageQueue *queue);
int Client_Receive(Client *client);
int Client_HandleMessages(Client *client);
int Client_RunHooks(Client *client);
int Client_CleanSearches(Client *client);
int Client_CleanPeers(Client *client);

#endif

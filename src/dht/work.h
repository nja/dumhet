#ifndef _dht_work_h
#define _dht_work_h

#include <dht/client.h>

/* The following functions returns 0 on success, -1 on failure. */

/* Sends all the messages on queue from client. */
int Client_Send(Client *client, MessageQueue *queue);
/* Decodes available messages and queues them as incoming. */
int Client_Receive(Client *client);
/* Handles the incoming queue, queuing replies to queries. */
int Client_HandleMessages(Client *client);
int Client_RunHooks(Client *client);
int Client_HandleSearches(Client *client);
int Client_CleanSearches(Client *client);
int Client_CleanPeers(Client *client);

#endif

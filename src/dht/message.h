#ifndef _dht_message_h
#define _dht_message_h

#include <dht/dht.h>

void Message_Destroy(Message *message);
void Message_DestroyNodes(Message *message);

#endif

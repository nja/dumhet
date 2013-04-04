#ifndef _dht_messagequeue_h
#define _dht_messagequeue_h

#include <dht/message.h>
#include <lcthw/darray.h>

/* LIFO but whatever */
typedef DArray MessageQueue;

#define MessageQueue_Create() DArray_create(sizeof(Message *), 0x100)
#define MessageQueue_Destroy DArray_destroy

#define MessageQueue_Push DArray_push
#define MessageQueue_Pop DArray_pop
#define MessageQueue_Count DArray_count

#define MessageQueue_Clear(MQ) while (DArray_count((MQ)) > 0) Message_Destroy(DArray_pop((MQ)));

#endif

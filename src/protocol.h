#ifndef _protocol_h
#define _protocol_h

#include <bencode.h>
#include <message.h>

typedef int (*GetResponseType_fp)(char *transaction_id, size_t tid_len, MessageType *type);
Message *Message_Decode(char *data, size_t len, GetResponseType_fp getResponseType);

int Message_Encode(Message *message, char *buf, size_t len);

#endif

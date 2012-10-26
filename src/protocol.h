#ifndef _protocol_h
#define _protocol_h

#include <bencode.h>
#include <message.h>

typedef int (*GetResponseType_fp)(uint8_t *transaction_id, size_t tid_len, MessageType *type);
Message *Decode(uint8_t *data, size_t len, GetResponseType_fp getResponseType);

int Message_Encode(Message *message, uint8_t *buf, size_t len);

#endif

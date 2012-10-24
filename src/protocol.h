#ifndef _protocol_h
#define _protocol_h

#include <bencode.h>
#include <message.h>

Message *Decode(uint8_t *data, size_t len);

int Message_Encode(Message *message, uint8_t *buf, size_t len);

#endif

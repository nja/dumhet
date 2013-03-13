#ifndef _protocol_h
#define _protocol_h

#include <dht/bencode.h>
#include <dht/message.h>

typedef uint16_t tid_t;

typedef struct PendingResponse {
    MessageType type;
    tid_t tid;
    DhtHash id;
    void *context;
} PendingResponse;

typedef PendingResponse (*GetPendingResponse_fp)(void *responses,
                                                      char *transaction_id,
                                                      int *rc);

struct PendingResponses {
    GetPendingResponse_fp getPendingResponse;
};

Message *Message_Decode(char *data, size_t len, struct PendingResponses *pending); 

int Message_Encode(Message *message, char *buf, size_t len);

#endif

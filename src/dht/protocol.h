#ifndef _dht_protocol_h
#define _dht_protocol_h

#include <dht/bencode.h>
#include <dht/message.h>

/* Type of the transaction ids we generate.
 * Incoming tids may be of arbitrary length. */
typedef uint16_t tid_t;

/* One of these is stored for every query we send. When a reply
 * arrives, we know if we're expecting it, what type it should be, and
 * have a pointer to the context when needed. */
typedef struct PendingResponse {
    MessageType type;
    tid_t tid;
    Hash id;
    void *context;
    int is_new;                 /* Don't know their id yet */
} PendingResponse;

/* Try to get the entry for the given transaction_id. (By now we
 * should know that its length is sizeof(tid_t). Any other length
 * error would cause an error before we get here.)
 * On success, *rc is 0 and a valid PendingResponse is returned.
 * Otherwise, *rc is -1 and the return value is empty. */
typedef PendingResponse (*GetPendingResponse_fp)(void *responses,
                                                 char *transaction_id,
                                                 int *rc);

typedef int (*AddPendingResponse_fp)(void *responses, PendingResponse entry);

struct PendingResponses {
    GetPendingResponse_fp getPendingResponse;
    AddPendingResponse_fp addPendingResponse;
};

/* Decodes data to a Message. Returns NULL on failure. */
Message *Message_Decode(char *data, size_t len, struct PendingResponses *pending); 

/* Encodes the Message to the dest buffer. Returns the size of the encoded
 * message on success, -1 on error. */
int Message_Encode(Message *message, char *buf, size_t len);

#endif

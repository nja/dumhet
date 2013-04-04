#ifndef _dht_message_h
#define _dht_message_h

#include <dht/node.h>
#include <lcthw/bstrlib.h>

typedef struct Peer {
    uint32_t addr;
    uint16_t port;
} Peer;

typedef enum MessageType {
    MUnknown = 0,
    QPing = 0100, QFindNode = 0101, QGetPeers = 0102, QAnnouncePeer = 0104,
    RPing = 0200, RFindNode = 0201, RGetPeers = 0202, RAnnouncePeer = 0204,
    RError = 0210,
} MessageType;

#define MessageType_IsQuery(T) ((T) & 0100)
#define MessageType_IsReply(T) ((T) & 0200)

typedef struct QPingData {
} QPingData;

typedef struct QFindNodeData {
    Hash *target;
} QFindNodeData;

typedef struct QGetPeersData {
    Hash *info_hash;
} QGetPeersData;

typedef struct QAnnouncePeerData {
    Hash *info_hash;
    uint16_t port;
    char *token;
    size_t token_len;
} QAnnouncePeerData;

typedef struct RPingData {
} RPingData;

typedef struct RFindNodeData {
    Node **nodes;
    size_t count;
} RFindNodeData;

/* May be cast as RFindNodeData */
typedef struct RGetPeersData {
    Node **nodes;
    size_t count;
    char *token;
    size_t token_len;
    Peer *values;
} RGetPeersData;

typedef struct RAnnouncePeerData {
} RAnnouncePeerData;

typedef struct RErrorData {
    int code;
    bstring message;
} RErrorData;

typedef int32_t merror_t;

typedef struct Message {
    MessageType type;
    merror_t errors;
    Node node;
    char *t;
    size_t t_len;
    Hash id;
    void *context;
    union {
	QPingData qping;
	QFindNodeData qfindnode;
	QGetPeersData qgetpeers;
	QAnnouncePeerData qannouncepeer;
	RPingData rping;
	RFindNodeData rfindnode;
	RGetPeersData rgetpeers;
	RAnnouncePeerData rannouncepeer;
	RErrorData rerror;
    } data;
} Message;

void Message_Destroy(Message *message);
void Message_DestroyNodes(Message *message);

#define MERROR_UNKNOWN_TYPE       0x0001
#define MERROR_INVALID_QUERY_TYPE 0x0002
#define MERROR_INVALID_TID        0x0004
#define MERROR_INVALID_NODE_ID    0x0008
#define MERROR_INVALID_DATA       0x0010
#define MERROR_PROGRAM            0x0020

#define RERROR_GENERIC       201
#define RERROR_SERVER        202
#define RERROR_PROTOCOL      203
#define RERROR_METHODUNKNOWN 204

#endif

#ifndef _message_h
#define _message_h

#include <dht.h>
#include <bstrlib.h>

typedef struct Peer {
    uint32_t addr;
    uint16_t port;
} Peer;

typedef enum MessageType {
    QPing, QFindNode, QGetPeers, QAnnouncePeer,
    RPing, RFindNode, RGetPeers, RAnnouncePeer,
    RError    
} MessageType;

typedef struct QPingData {
} QPingData;

typedef struct QFindNodeData {
    DhtHash *target;
} QFindNodeData;

typedef struct QGetPeersData {
    DhtHash *info_hash;
} QGetPeersData;

typedef struct QAnnouncePeerData {
    DhtHash *info_hash;
    int port;
    uint8_t *token;
    size_t token_len;
} QAnnouncePeerData;

typedef struct RPingData {
} RPingData;

typedef struct RFindNodeData {
    DhtNode *nodes;
    size_t count;
} RFindNodeData;

typedef struct RGetPeersData {
    uint8_t *token;
    size_t token_len;
    Peer *values;
    DhtNode *nodes;
    size_t count;
} RGetPeersData;

typedef struct RAnnouncePeerData {
} RAnnouncePeerData;

typedef struct RErrorData {
    int code;
    bstring message;
} RErrorData;

typedef struct Message {
    MessageType type;
    uint8_t *t;
    size_t t_len;
    DhtHash *id;
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

#endif

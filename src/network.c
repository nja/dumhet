#include <arpa/inet.h>
#include <assert.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <dht/client.h>
#include <dht/hooks.h>
#include <dht/network.h>
#include <lcthw/dbg.h>

int NetworkUp(Client *client)
{
    assert(client != NULL && "NULL Client pointer");
    assert(client->socket != -1 && "Invalid Client socket");

    struct sockaddr_in sockaddr = { 0 };

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = client->node.port;
    sockaddr.sin_addr = client->node.addr;

    int rc = bind(client->socket,
                  (struct sockaddr *) &sockaddr,
                  sizeof(struct sockaddr_in));
    check(rc == 0, "bind failed");

    return 0;
error:
    return -1;
}

int NetworkDown(Client *client)
{
    assert(client != NULL && "NULL Client pointer");
    assert(client->socket != -1 && "Invalid Client socket");

    int rc;

retry:
  
    rc = close(client->socket);

    if (rc == -1 && errno == EINTR)
        goto retry;

    check(rc == 0, "Close on socket fd failed");

    return 0;
error:
    return -1;
}

int Send(Client *client, Node *node, char *buf, size_t len)
{
    assert(client != NULL && "NULL Client pointer");
    assert(node != NULL && "NULL Node pointer");
    assert(buf != NULL && "NULL buf pointer");
    assert(len <= UDPBUFLEN && "buf too large for UDP");

    struct sockaddr_in addr = { 0 };

    addr.sin_family = AF_INET;
    addr.sin_addr = node->addr;
    addr.sin_port = node->port;

    int rc = sendto(client->socket,
                    buf,
                    len,
                    0,
                    (struct sockaddr *) &addr,
                    sizeof(addr));
    check(rc == (int)len, "sendto failed");

    return 0;
error:
    return -1;
}

int Receive(Client *client, Node *node, char *buf, size_t len)
{
    assert(client != NULL && "NULL Client pointer");
    assert(node != NULL && "NULL Node pointer");
    assert(buf != NULL && "NULL buf pointer");
    assert(len >= UDPBUFLEN && "buf too small for UDP");

    struct sockaddr_in srcaddr = { 0 };
    socklen_t addrlen = sizeof(srcaddr);

    errno = 0;
    int rc = recvfrom(client->socket,
                      buf,
                      len,
                      MSG_DONTWAIT,
                      (struct sockaddr *) &srcaddr,
                      &addrlen);

    assert(addrlen == sizeof(srcaddr) && "Unexpected addrlen from recvfrom");

    if (rc == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        *node = (Node){{{ 0 }}};
        return 0;
    }

    check(rc >= 0, "Receive failed");

    node->addr = srcaddr.sin_addr;
    node->port = srcaddr.sin_port;

    return rc;
error:
    return -1;
}

int SendMessage(Client *client, Message *msg)
{
    assert(client != NULL && "NULL Client pointer");
    assert(msg != NULL && "NULL Message pointer");
    assert(msg->t_len == sizeof(tid_t) && "Wrong outgoing t");

    int len = Message_Encode(msg, client->buf, UDPBUFLEN);
    check(len > 0, "Message_Encode failed");

    int rc = Send(client, &msg->node, client->buf, len);
    check(rc == 0, "Send failed");

    if (MessageType_IsQuery(msg->type))
    {
        PendingResponse entry = { .type = MessageType_AsReply(msg->type),
                                  .tid = *(tid_t *)msg->t,
                                  .id = msg->node.id,
                                  .context = msg->context };

        rc = client->pending->addPendingResponse(client->pending, entry);
        check(rc == 0, "addPendingResponses failed");
    }

    Client_RunHook(client, HookSendMessage, msg);

    return 0;
error:
    return -1;
}

int ReceiveMessage(Client *client, Message **message)
{
    assert(client != NULL && "NULL Client pointer");

    Node node = {{{ 0 }}};

    int len = Receive(client, &node, client->buf, UDPBUFLEN);
    check(len >= 0, "Receive failed");

    if (len == 0)
    {
        *message = NULL;
        return 0;
    }

    Message *decoded = Message_Decode(client->buf,
                                      len,
                                      (struct PendingResponses *)client->pending);
    check(decoded != NULL, "Message_Decode failed");

    decoded->node = node;
    decoded->node.id = decoded->id;

    *message = decoded;

    return 1;
error:
    return -1;
}

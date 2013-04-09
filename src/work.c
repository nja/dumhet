#include <dht/client.h>
#include <dht/handle.h>
#include <dht/network.h>
#include <dht/work.h>

int Client_Handle(Client *client)
{
    assert(client != NULL && "NULL Client pointer");

    Message *message = NULL, *reply = NULL;

    while (MessageQueue_Count(client->incoming))
    {
        message = MessageQueue_Pop(client->incoming);
        check(message != NULL, "MessageQueue_Pop failed");

        if (MessageType_IsQuery(message->type))
        {
            QueryHandler handler = GetQueryHandler(message->type);
            check(handler != NULL, "GetQueryHandler failed");

            Message *reply = handler(client, message);
            check(reply != NULL, "QueryHandler failed");

            int rc = MessageQueue_Push(client->replies, reply);
            check(rc == 0, "MessageQueue_Push failed");
        }
        else if (MessageType_IsReply(message->type))
        {
            ReplyHandler handler = GetReplyHandler(message->type);
            check(handler != NULL, "GetReplyHandler failed");

            int rc = handler(client, message);
            check(rc == 0, "ReplyHandler failed");
        }
        else
        {
            int rc = HandleUnknown(client, message);
            check(rc == 0, "HandleUnknown failed");
        }

        Message_Destroy(message);
    }

    return 0;
error:
    Message_Destroy(message);
    Message_Destroy(reply);

    return -1;
}

int Client_Send(Client *client, MessageQueue *queue)
{
    assert(client != NULL && "NULL Client pointer");
    assert(queue != NULL && "NULL MessageQueue pointer");

    Message *message = NULL;

    while (MessageQueue_Count(queue) > 0)
    {
        message = MessageQueue_Pop(queue);
        check(message != NULL, "MessageQueue_Pop failed");

        int rc = SendMessage(client, message);
        check(rc == 0, "SendMessage failed");

        Message_Destroy(message);
    }

    return 0;
error:
    Message_Destroy(message);
    return -1;
}

int Client_Receive(Client *client)
{
    assert(client != NULL && "NULL Client pointer");

    Message *message = NULL;

    while (1)
    {
        message = NULL;
        int rc = ReceiveMessage(client, &message);

        if (rc == 0)
        {
            break;
        }

        check(rc >= 0, "ReceiveMessage failed");
        check(message != NULL, "NULL Message");

        rc = MessageQueue_Push(client->incoming, message);
        check(rc == 0, "MessageQueue_Push failed");
    }

    return 0;
error:
    Message_Destroy(message);
    return -1;
}

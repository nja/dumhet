#include <dht/message.h>
#include <lcthw/dbg.h>

void Message_Destroy(Message *message)
{
    if (message == NULL)
	return;

    free(message->t);

    switch (message->type)
    {
    case QPing:
	break;
    case QFindNode:
	Hash_Destroy(message->data.qfindnode.target);
	break;
    case QGetPeers:
	Hash_Destroy(message->data.qgetpeers.info_hash);
	break;
    case QAnnouncePeer:
	Hash_Destroy(message->data.qannouncepeer.info_hash);
	free(message->data.qannouncepeer.token);
	break;
    case RPing:
	break;
    case RFindNode:
	free(message->data.rfindnode.nodes);
	break;
    case RGetPeers:
	free(message->data.rgetpeers.token);
	free(message->data.rgetpeers.values); /* TODO: destroy? */
        free(message->data.rgetpeers.nodes);
	break;
    case RAnnouncePeer:
	break;
    case RError:
	bdestroy(message->data.rerror.message);
	break;
    case MUnknown:
        break;
    }

    free(message);
}

void Message_DestroyNodes(Message *message)
{
    if (message == NULL)
        return;

    switch (message->type)
    {
    case RFindNode:
    case RGetPeers:
        if (message->data.rfindnode.nodes != NULL)
        {
            Node_DestroyBlock(message->data.rfindnode.nodes,
                              message->data.rfindnode.count);
        }
        break;
    default: break;
    }
}

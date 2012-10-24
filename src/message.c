#include <message.h>
#include <dbg.h>

void Message_Destroy(Message *message)
{
    if (message == NULL)
	return;

    free(message->t);

    DhtHash_Destroy(message->id);

    size_t i = 0;

    switch (message->type)
    {
    case QPing:
	break;
    case QFindNode:
	DhtHash_Destroy(message->data.qfindnode.target);
	break;
    case QGetPeers:
	DhtHash_Destroy(message->data.qgetpeers.info_hash);
	break;
    case QAnnouncePeer:
	DhtHash_Destroy(message->data.qannouncepeer.info_hash);
	free(message->data.qannouncepeer.token);
	break;
    case RPing:
	break;
    case RFindNode:
	DhtNode_Destroy(message->data.rfindnode.nodes);
	break;
    case RGetPeers:
	free(message->data.rgetpeers.token);
	free(message->data.rgetpeers.values); /* TODO: destroy? */

	for (i = 0; i < message->data.rgetpeers.count; i++)
	    DhtNode_Destroy(message->data.rgetpeers.nodes[i]);

	free(message->data.rgetpeers.nodes);
	break;
    case RAnnouncePeer:
	break;
    case RError:
	bdestroy(message->data.rerror.message);
	break;
    default:
	log_err("Destroying message of unknown type");
    }

    free(message);
}

//  Test case for issue 81

#include <malamute.h>

static const char *
    broker_endpoint = "ipc://@/malamute";
static bool
    verbose = false;
static const char *
    stream_name = "stream";


static void
s_consumer (zsock_t *pipe, void *args)
{
    mlm_client_verbose = verbose;
    mlm_client_t *consumer = mlm_client_new ();
    assert (consumer);
    int rc = mlm_client_connect (consumer, broker_endpoint, 1000, "consumer");
    assert (rc == 0);

    mlm_client_set_consumer (consumer, stream_name, ".*");

    //  Tell parent we're ready to go
    zsock_signal (pipe, 0);

    int count = 0;
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (consumer), NULL);
    while (!zsys_interrupted) {
        zsock_t *which = zpoller_wait (poller, -1);
        if (which == pipe)
            break;              //  Caller sent us $TERM
        else
        if (which == mlm_client_msgpipe (consumer)) {
            zmsg_t *content = mlm_client_recv (consumer);
            assert (content);
            char *string = zmsg_popstr (content);
            zsys_info ("Received %s", string);
            free (string);
            zmsg_destroy (&content);
        }
    }
    mlm_client_destroy (&consumer);
}

static void
s_producer (zsock_t *pipe, void *args)
{
    mlm_client_verbose = verbose;
    mlm_client_t *producer = mlm_client_new ();
    assert (producer);
    int rc = mlm_client_connect (producer, broker_endpoint, 1000, "producer");
    assert (rc == 0);

    //  Tell parent we're ready to go
    zsock_signal (pipe, 0);

    int count = 0;
    zpoller_t *poller = zpoller_new (pipe, NULL);
    while (!zsys_interrupted) {
        zsock_t *which = zpoller_wait (poller, 500);
        if (which == pipe)
            break;              //  Caller sent us $TERM
        zmsg_t *content = zmsg_new ();
        zmsg_addstrf (content, "message %d", ++count);
        mlm_client_send (producer, stream_name, "subject", &content);
    }
    mlm_client_destroy (&producer);
}


int main (int argc, char *argv [])
{
    //  Start a broker to test against
    zactor_t *broker = zactor_new (mlm_server, NULL);
    zsock_send (broker, "ssi", "SET", "server/verbose", verbose);
    zsock_send (broker, "ssi", "SET", "server/timeout", 1000);
    zsock_send (broker, "ss", "BIND", broker_endpoint);

    zactor_t *consumer = zactor_new (s_consumer, NULL);
    zactor_t *producer = zactor_new (s_producer, NULL);

    //  Wait for interrupt
    zsock_wait (consumer);

    zactor_destroy (&consumer);
    zactor_destroy (&producer);
    zactor_destroy (&broker);
    return 0;
}

//  Test case for issue 81

#include <malamute.h>

static const char *
    broker_endpoint = "ipc://@/malamute";
static bool
    verbose = true;


static void
s_consumer (zsock_t *pipe, void *args)
{
    mlm_client_verbose = verbose;
    mlm_client_t *consumer = mlm_client_new ();
    assert (consumer);
    int rc = mlm_client_connect (consumer, broker_endpoint, 1000, "consumer");
    assert (rc == 0);

    mlm_client_set_consumer (consumer, "weather", "temp.*");

    //  Tell parent we're ready to go
    zsock_signal (pipe, 0);

    //  Receive 10 messages
    int count;
    for (count = 0; count < 10; count++) {
        zsys_info ("receiving message %d", count);
        mlm_client_recv (consumer);
        if (zsys_interrupted)
            break;
    }
    //  Wait for parent to terminate us
    free (zstr_recv (pipe));
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

    mlm_client_set_producer (producer, "weather");

    //  Tell parent we're ready to go
    zsock_signal (pipe, 0);

    //  Send messages for 10 seconds
    int count;
    for (count = 0; count < 10; count++) {
        zsys_info ("sending message %d", count);
        mlm_client_sendx (producer, "test", "test", NULL);
        zclock_sleep (1000);
        if (zsys_interrupted)
            break;
    }
    //  Wait for parent to terminate us
    free (zstr_recv (pipe));
    mlm_client_destroy (&producer);
}


int main (int argc, char *argv [])
{
    //  Start a broker to test against
    zactor_t *broker = zactor_new (mlm_server, NULL);
    zsock_send (broker, "ssi", "SET", "server/verbose", verbose);
    zsock_send (broker, "ss", "BIND", broker_endpoint);

    zactor_t *consumer = zactor_new (s_consumer, NULL);
    zactor_t *producer = zactor_new (s_producer, NULL);

    printf (" OK\n");
    zactor_destroy (&consumer);
    zactor_destroy (&producer);
    zactor_destroy (&broker);
    return 0;
}

//  Test case for issue 81

#include <malamute.h>

static const char *
    broker_endpoint = "ipc://@/malamute";
static bool
    verbose = false;


static void
s_consumer (zsock_t *pipe, void *args)
{
    mlm_client_verbose = verbose;
    mlm_client_t *consumer = mlm_client_new ();
    assert (consumer);
    int rc = mlm_client_connect (consumer, broker_endpoint, 1000, "");
    assert (rc == 0);

    mlm_client_set_consumer (consumer, "weather", ".*");

    //  Tell parent we're ready to go
    zsock_signal (pipe, 0);

    //  Receive 20 messages
    int count;
    for (count = 0; count < 500; count++) {
        mlm_client_recv (consumer);
        if (zsys_interrupted)
            break;
        zsys_info ("received message %d", count);
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
    int rc = mlm_client_connect (producer, broker_endpoint, 1000, "");
    assert (rc == 0);

    mlm_client_set_producer (producer, "weather");

    //  Tell parent we're ready to go
    zsock_signal (pipe, 0);

    //  Send messages for 10 seconds
    int count;
    for (count = 0; count < 100; count++) {
        zsys_info ("sending message %d", count);
        mlm_client_sendx (producer, "test", "test", NULL);
        zclock_sleep (100);
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
    zsock_send (broker, "ssi", "SET", "server/timeout", 60000);
    zsock_send (broker, "ss", "BIND", broker_endpoint);

    zactor_t *consumer = zactor_new (s_consumer, NULL);
    zactor_t *producer1 = zactor_new (s_producer, NULL);
    zactor_t *producer2 = zactor_new (s_producer, NULL);
    zactor_t *producer3 = zactor_new (s_producer, NULL);
    zactor_t *producer4 = zactor_new (s_producer, NULL);
    zactor_t *producer5 = zactor_new (s_producer, NULL);

    printf (" OK\n");
    zactor_destroy (&consumer);
    zactor_destroy (&producer1);
    zactor_destroy (&producer2);
    zactor_destroy (&producer3);
    zactor_destroy (&producer4);
    zactor_destroy (&producer5);
    zactor_destroy (&broker);
    return 0;
}

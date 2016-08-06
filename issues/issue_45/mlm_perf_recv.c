/*  =========================================================================
    mlm_perf_recv.c, the consumer of messages : the receiver.

    Runs various tests on Malamute to test performance of the core engines.
    =========================================================================
*/

#include <malamute.h>

void recv_actor (zsock_t *pipe, void *args)
{
    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    int rc = mlm_client_connect (writer, MLM_DEFAULT_ENDPOINT, 1000, "writer1");
    assert (rc == 0);

    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_connect (reader, MLM_DEFAULT_ENDPOINT, 1000, "reader1");
    assert (rc == 0);

    mlm_client_set_consumer (reader, "weather", "temp.");
    zsock_t *incoming = mlm_client_msgpipe (reader);
    
    zpoller_t *poller = zpoller_new (incoming, pipe, NULL);
    assert (poller);
    zsock_signal (pipe, 0);
    int count = 0;

    while (!zpoller_terminated (poller)) {
        zsock_t *which = (zsock_t *) zpoller_wait (poller, -1);
        if (which == pipe)
            break;
        else
        if (which == incoming) {
            zmsg_t *msg = mlm_client_recv (reader);
            assert (msg);
            if (count == 0)
                printf (".");
            count++;
            if ((count % 10000) == 0)
                printf (".");
            
            if (streq (mlm_client_subject (reader), "temp.signal")) {
                printf ("Signaling after %d messages\n", count);
                mlm_client_sendx (writer, "weather", "signal.ack", "ACK", NULL);
                count = 0;
            }
            zmsg_destroy (&msg);
         }
    }
    zpoller_destroy (&poller);
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
    
}

int main (int argc, char *argv [])
{
    setbuf (stdout, NULL);

    zactor_t *zrecv = zactor_new (recv_actor, NULL);
    assert (zrecv);
    
    zpoller_t *poller = zpoller_new (zrecv,  NULL);
    assert (poller);

    while (!zpoller_terminated (poller)) {
        zsock_t *which = (zsock_t *)zpoller_wait (poller, -1);
    }
    printf ("destroy");
    zactor_destroy (&zrecv);
    return 0;
}

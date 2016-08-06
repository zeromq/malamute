#include <malamute.h>

static void
s_producer (zsock_t *pipe, void *args)
{
    mlm_client_t *client = mlm_client_new ();
    assert (client);
    mlm_client_connect (client, MLM_DEFAULT_ENDPOINT, 1000, NULL);
    zsock_signal (pipe, 0);

    //  Send 1M messages of 1K then wait for 5 seconds
    int remaining = 1000000;
    while (remaining) {
        zmsg_t *content = zmsg_new ();
        zframe_t *frame = zframe_new (NULL, 1024);
        memset (zframe_data (frame), 0, 1024);
        zmsg_append (content, &frame);
        if (mlm_client_send (client, "stream", "subject", &content)) {
            zsys_debug ("mlm_client_send failed");
            assert (false);
        }
    }
    mlm_client_destroy (&client);
    zsock_destroy (&pipe);
}

static void
s_consumer (zsock_t *pipe, void *args)
{
    mlm_client_t *client = mlm_client_new ();
    assert (client);
    mlm_client_connect (client, MLM_DEFAULT_ENDPOINT, 1000, NULL);
    int rc = mlm_client_set_consumer (client, "stream", ".*" );
    assert (rc == 0);
    zsock_signal (pipe, 0);

    while (!zsys_interrupted) {
        zmsg_t *content = mlm_client_recv (client);
        if (!content)
            break;

        zmsg_destroy (&content);
    }
    mlm_client_destroy (&client);
    zsock_destroy (&pipe);
}


int main (void)
{
    zactor_t *consumer = zactor_new (s_consumer, NULL);
    assert (consumer);

    zactor_t *producer = zactor_new (s_producer, NULL);
    assert (producer);

    zpoller_t *poller = zpoller_new (producer, consumer, NULL);
    assert (poller);
    while (!zpoller_terminated (poller)) {
        zpoller_wait (poller, -1);
        break;
    }
    zactor_destroy (&producer);
    zactor_destroy (&consumer);
    zpoller_destroy (&poller);
    return 0;
}

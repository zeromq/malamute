#include <malamute.h>

#define ENDPOINT "ipc://@/malamute"
#define STREAM "stream"

static void
s_producer (zsock_t *pipe, void *args)
{
    int id = * (int *) args;

    char name [20];
    sprintf (name, "producer-%d", id);

    mlm_client_t *agent = mlm_client_new ();
    assert (agent);
    mlm_client_connect (agent, ENDPOINT, 1000, name);

    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {
        int a;
        for (a = 0; a < 1000; a++) {
            zmsg_t *zmsg = zmsg_new ();
            zframe_t *f = zframe_new ("ahoy", 5);
            zmsg_append (zmsg, &f);
            int r = mlm_client_send (agent, STREAM, "SUBJECT", &zmsg);
            if (r)
                zsys_debug ("mlm_client_send result = %i",r);
        }
        zclock_sleep (77); // give a break
    }
    mlm_client_destroy (&agent);
    zsock_destroy (&pipe);
}

static void
s_consumer (zsock_t *pipe, void *args)
{
    int id = *(int *) args;

    char name [20];
    sprintf (name, "consumer-%d", id);

    mlm_client_t *agent = mlm_client_new ();
    assert (agent);
    mlm_client_connect (agent, ENDPOINT, 1000, name);
    int ret = mlm_client_set_consumer (agent, STREAM, ".*" );
    assert (ret == 0);

    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {
        zmsg_t *msg = mlm_client_recv (agent);
        if (msg) {
            zframe_t *f = zmsg_pop (msg);
            char *data = (char *) zframe_data (f);
            int s = zframe_size (f);
            if (s != 5 || strcmp (data, "ahoy") != 0 ) {
                zsys_debug ("consumer %i: data lost frame size: %i content: %s\n",
                           id,
                           s,
                           s ? data : "n/a" );
                assert (false);
            }
            zframe_destroy (&f);
            zmsg_destroy (&msg);
        }
        else
            zsys_debug ("ZMSG is NULL");
    }
    mlm_client_destroy (&agent);
    zsock_destroy (&pipe);
}


int main (void)
{
    int id = 1;
    zactor_t *producer1 = zactor_new (s_producer, &id);
    assert (producer1);

    id++;
    zactor_t *consumer1 = zactor_new (s_consumer, &id);
    assert (consumer1);

    id++;
    zactor_t *consumer2 = zactor_new (s_consumer, &id);
    assert (consumer2);

    //id++;
    //zactor_t *consumer3 = zactor_new (s_consumer, &id);
    //assert (consumer3);

    zpoller_t *poller = zpoller_new (producer1, consumer1, consumer2 /*, consumer3 */, NULL);
    assert (poller);

    while (!zpoller_terminated (poller)) {
        zpoller_wait (poller, -1);
        break;
    }

    zactor_destroy (&producer1);
    zactor_destroy (&consumer1);
    zactor_destroy (&consumer2);
    //zactor_destroy (&consumer3);
    zpoller_destroy (&poller);

    return 0;
}

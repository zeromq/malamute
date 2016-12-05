#include <czmq.h>
#include <malamute.h>


//  Upon subscription to stream publish rand number each half a second
//
//  Actor commands:
//      CONNECT/endpoint/name
//      PRODUCER/stream
static void
producer_actor (zsock_t *pipe, void *args)
{
    mlm_client_t *client = mlm_client_new ();
    assert (client);

    zpoller_t *poller = zpoller_new (pipe, NULL);
    assert (poller);

    zsock_signal (pipe, 0); 

    char *agent_name = NULL;

    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, 500);

        if (which == NULL && (zpoller_terminated (poller) || zsys_interrupted)) {
                zsys_warning ("zpoller_terminated () or zsys_interrupted. Shutting down.");
                break;
        }
        
        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            char *command = zmsg_popstr (message);

            if (streq (command, "$TERM")) {
                zstr_free (&command);
                zmsg_destroy (&message);
                break;
            }
            else
            if (streq (command, "CONNECT")) {
                char *endpoint = zmsg_popstr (message);
                char *name = zmsg_popstr (message);
                if (endpoint && name) {
                    agent_name = strdup (name);
                    int rv = mlm_client_connect (client, endpoint, 1000, name);
                    if (rv != 0)
                        zsys_error ("mlm_client_connect (endpoint = '%s', name = '%s') failed.", endpoint, name);
                    else
                        zpoller_add (poller, mlm_client_msgpipe (client));
                }
                else {
                    zsys_warning ("Bad CONNECT command. Expected CONNECT/endpoint/name.");
                }
                zstr_free (&endpoint);
                zstr_free (&name);
            }
            else
            if (streq (command, "PRODUCER")) {
                char *stream = zmsg_popstr (message);
                if (stream) {
                    int rv = mlm_client_set_producer (client, stream);
                    if (rv != 0)
                        zsys_error ("mlm_client_set_producer (stream = '%s') failed.", stream);
                }
                else {
                    zsys_warning ("Bad PRODUCER command. Expected PRODUCER/stream");
                }
                zstr_free (&stream);
            }
            zstr_free (&command);
            zmsg_destroy (&message);
            continue;
        }

        if (!mlm_client_connected (client))
            continue;

        zmsg_t *message = zmsg_new ();
        assert (message);
        char *format = NULL;
        asprintf (&format, "%d", randof (10000));
        zmsg_addstr (message, format);
        zstr_free (&format);
        int rv = mlm_client_send (client, "Random number", &message);
        if (rv != 0)
            zsys_error ("mlm_client_send () failed.");
    }
    zstr_free (&agent_name);
    zpoller_destroy (&poller);
    mlm_client_destroy (&client);
}

static void
consumer_actor (zsock_t *pipe, void *args)
{
    mlm_client_t *client = mlm_client_new ();
    assert (client);

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);
    assert (poller);

    zsock_signal (pipe, 0);

    char *agent_name = NULL; 

    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, 500);
        if (which == NULL && (zpoller_terminated (poller) || zsys_interrupted)) {
                zsys_warning ("zpoller_terminated () or zsys_interrupted. Shutting down.");
                break;
        }

        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            char *command = zmsg_popstr (message);

            if (streq (command, "$TERM")) {
                zstr_free (&command);
                zmsg_destroy (&message);
                break;
            }
            else
            if (streq (command, "CONNECT")) {
                char *endpoint = zmsg_popstr (message);
                char *name = zmsg_popstr (message);
                if (endpoint && name) {
                    agent_name = strdup (name);
                    int rv = mlm_client_connect (client, endpoint, 1000, name);
                    if (rv != 0)
                        zsys_error ("mlm_client_connect (endpoint = '%s', name = '%s') failed.", endpoint, name);
                    else
                        zpoller_add (poller, mlm_client_msgpipe (client));
                }
                else {
                    zsys_warning ("Bad CONNECT command. Expected CONNECT/endpoint/name.");
                }
                zstr_free (&endpoint);
                zstr_free (&name);
            }
            else
            if (streq (command, "CONSUMER")) {
                char *stream = zmsg_popstr (message);
                char *pattern = zmsg_popstr (message);
                if (stream && pattern) {
                    int rv = mlm_client_set_consumer (client, stream, pattern);
                    if (rv != 0)
                        zsys_error ("mlm_client_set_consumer (stream = '%s', patter = '%s') failed.", stream, pattern);
                }
                else {
                    zsys_warning ("Bad CONSUMER command. Expected CONSUMER/stream/pattern");
                }
                zstr_free (&stream);
                zstr_free (&pattern);
            }
            zstr_free (&command);
            zmsg_destroy (&message);
            continue;
        }

        if (!mlm_client_connected (client))
            continue;

        zmsg_t *message = mlm_client_recv (client);
        if (!message)
           continue;
        char *pop = zmsg_popstr (message);
        assert (pop);
        zsys_info ("%s: %s", agent_name, pop);
        zstr_free (&pop);
        zmsg_destroy (&message);
    }
    zstr_free (&agent_name);
    zpoller_destroy (&poller);
    mlm_client_destroy (&client);
}

int
main () {

    const char *endpoint = "inproc://mlm_issue_stream_crash";

    zsys_init ();
    
    zsys_debug ("--- Malamute issue reproduction example ---");
    zsys_debug ("Issue description:");
    zsys_debug ("\tAssigning character '*' as regex pattern to mlm_client_set_consumer () breaks the stream.");
    zsys_debug ("\tNot even the already subscribed clients receive nothing. This does happen with character ");
    zsys_debug ("\t'*' only (this is the only one we know about SO FAR); bad regular expression, i.e. '[' ");
    zsys_debug ("\tdoes not break it at all.");
    zsys_debug ("-----------------------------------------------------------------------------------------");
    zsys_debug ("");

    //  Malamute
    zactor_t *server = zactor_new (mlm_server, "Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
//    zstr_sendx (server, "VERBOSE", NULL);
    zclock_sleep (500);

    //  producer FIRST
    zactor_t *producer_1 = zactor_new (producer_actor, (void *) NULL);
    assert (producer_1);
    zstr_sendx (producer_1, "CONNECT", endpoint, "producer_1", NULL);
    zstr_sendx (producer_1, "PRODUCER", "FIRST", NULL);
    zclock_sleep (500);

    //  consumer FIRST
    zactor_t *consumer_0 = zactor_new (consumer_actor, (void *) NULL);
    assert (consumer_0);
    zstr_sendx (consumer_0, "CONNECT", endpoint, "consumer_FIRST_1", NULL);
    zstr_sendx (consumer_0, "CONSUMER", "FIRST", ".*", NULL);
    zclock_sleep (500);

    //  producer WORLD
    zactor_t *producer_2 = zactor_new (producer_actor, (void *) NULL);
    assert (producer_2);
    zstr_sendx (producer_2, "CONNECT", endpoint, "producer_2", NULL);
    zstr_sendx (producer_2, "PRODUCER", "WORLD", NULL);
    zclock_sleep (500);

    //  consumer WORLD
    zactor_t *consumer_1 = zactor_new (consumer_actor, (void *) NULL);
    assert (consumer_1);
    zstr_sendx (consumer_1, "CONNECT", endpoint, "consumer_WORLD_1", NULL);
    zstr_sendx (consumer_1, "CONSUMER", "WORLD", ".*", NULL);
    zclock_sleep (500);

    //  consumer WORLD
    zactor_t *consumer_2 = zactor_new (consumer_actor, (void *) NULL);
    assert (consumer_2);
    zstr_sendx (consumer_2, "CONNECT", endpoint, "consumer_WORLD_2", NULL);
    zstr_sendx (consumer_2, "CONSUMER", "WORLD", ".*", NULL);
    zclock_sleep (500);

    zsys_debug (" -----\tEXPLANATION MESSAGE: Let this run for 5 seconds.");
    zclock_sleep (5000);

    zsys_debug (" -----\tEXPLANATION MESSAGE: Creating consumer_WORLD_3 with pattern '*'. This will break stream WORLD."
                " Only consumer_FIRST_1 will continue.");
    //  consumer WORLD
    zactor_t *consumer_3 = zactor_new (consumer_actor, (void *) NULL);
    assert (consumer_3);
    zstr_sendx (consumer_3, "CONNECT", endpoint, "consumer_WORLD_3", NULL);
    zstr_sendx (consumer_3, "CONSUMER", "WORLD", "*", NULL);
    zclock_sleep (500);

    //  Accept and print any message back from server
    while (true) {
        char *message = zstr_recv (server);
        if (message) {
            puts (message);
            free (message);
        }
        else {
            puts ("interrupted");
            break;
        }
    }

    zactor_destroy (&server); 
    zactor_destroy (&producer_1); 
    zactor_destroy (&producer_2); 
    zactor_destroy (&consumer_0); 
    zactor_destroy (&consumer_1); 
    zactor_destroy (&consumer_2); 
    zactor_destroy (&consumer_3); 

    return EXIT_SUCCESS;
}

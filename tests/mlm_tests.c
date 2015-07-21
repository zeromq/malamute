#include <malamute.h>

// we use char* text in message
// extract text from zmsg

static zpoller_t *poller = NULL;

// receive message with timeout
static zmsg_t *
my_mlm_client_recv (mlm_client_t *client, int timeout)
{
    if (zsys_interrupted)
        return NULL;

    if (!poller)
        poller = zpoller_new (mlm_client_msgpipe (client), NULL);
    
    zsock_t *which = (zsock_t *) zpoller_wait (poller, timeout);
    if (which == mlm_client_msgpipe (client)) {
        zmsg_t *reply = mlm_client_recv (client);
        return reply;
    }
    return NULL;
}


//  mlm component producing messages

static
void client (void)
{
    mlm_client_t *client = mlm_client_new ("ipc://@/malamute", 1000, "client");
    if (!client) {
        zsys_error ("could not connect to Malamute server");
        exit (0);
    }
    int request_nbr = 0;
    while (!zsys_interrupted) {
        zmsg_t *msg = zmsg_new ();
        zmsg_addstrf (msg, "ahoj %d", ++request_nbr);
        zsys_info ("Sending request number=%d", request_nbr);
        mlm_client_sendto (client, "server", "something", NULL, 1000, &msg);
        
        msg = my_mlm_client_recv (client, 1000);
        if (msg) {
            char *reply = zmsg_popstr (msg);
            zsys_info ("received: %s", reply);
            free (reply);
            zmsg_destroy (&msg);
            zclock_sleep (100);
        }
        else
            zsys_error ("MISSED reply number=%d", request_nbr);
    }
    mlm_client_destroy (&client);
}

//  mlm component consuming messages

static void
server (void)
{
    mlm_client_t *client = mlm_client_new ("ipc://@/malamute", 1000, "server");
    if (!client) {
        zsys_error ("could not connect to Malamute server");
        exit (0);
    }
    int reply_nbr = 0;
    while (!zsys_interrupted) {
        zmsg_t *msg = mlm_client_recv (client);
        if (msg) {
            char *request = zmsg_popstr (msg);
            zsys_info ("received: %s sender=%s", request, mlm_client_sender (client));
            zmsg_addstrf (msg, "%s OK (%d)", request, ++reply_nbr);
            mlm_client_sendto (client,
                mlm_client_sender (client), "anything", NULL, 0, &msg);
        }
    }
    mlm_client_destroy (&client);
}


int
main (int argc, char *argv [])
{
    mlm_client_verbose = true;
    if (argc == 1)
        printf ("usage: mlm_tests  (client|server)\n");
    else
    if (streq (argv [1], "client"))
        client ();
    else
    if (streq (argv [1], "server"))
        server ();
    
    zpoller_destroy (&poller);
    return 0;
}

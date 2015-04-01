/*  =========================================================================
    mlm_client - Malamute client stack

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    Provides an async client API to the Malamute Protocol.
@discuss
@end
*/

#include "mlm_classes.h"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

//  This structure defines the context for a client connection
typedef struct {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine. The cmdpipe gets
    //  messages sent to the actor; the msgpipe may be used for
    //  faster asynchronous message flows.
    zsock_t *cmdpipe;           //  Command pipe to/from caller API
    zsock_t *msgpipe;           //  Message pipe to/from caller API
    zsock_t *dealer;            //  Socket to talk to server
    mlm_proto_t *message;       //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  Own properties
    int heartbeat_timer;        //  Timeout for heartbeats to server
    int retries;                //  How many heartbeats we've tried
} client_t;

//  Include the generated client engine
#include "mlm_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    //  We'll ping the server once per second
    self->heartbeat_timer = 1000;
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  use_plain_security_mechanism
//

static void
use_plain_security_mechanism (client_t *self)
{
    zsock_set_plain_username (self->dealer, self->args->username);
    zsock_set_plain_password (self->dealer, self->args->password);
}


//  ---------------------------------------------------------------------------
//  connect_to_server_endpoint
//

static void
connect_to_server_endpoint (client_t *self)
{
    if (zsock_connect (self->dealer, "%s", self->args->endpoint)) {
        engine_set_exception (self, bad_endpoint_event);
        zsys_warning ("could not connect to %s", self->args->endpoint);
    }
}


//  ---------------------------------------------------------------------------
//  set_client_address
//

static void
set_client_address (client_t *self)
{
    mlm_proto_set_address (self->message, self->args->address);
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
    engine_set_timeout (self, self->args->timeout);
}


//  ---------------------------------------------------------------------------
//  client_is_connected
//

static void
client_is_connected (client_t *self)
{
    self->retries = 0;
    engine_set_connected (self, true);
    engine_set_timeout (self, self->heartbeat_timer);
}


//  ---------------------------------------------------------------------------
//  check_if_connection_is_dead
//

static void
check_if_connection_is_dead (client_t *self)
{
    //  We send at most 3 heartbeats before expiring the server
    if (++self->retries >= 3) {
        engine_set_timeout (self, 0);
        engine_set_connected (self, false);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  prepare_stream_write_command
//

static void
prepare_stream_write_command (client_t *self)
{
    mlm_proto_set_stream (self->message, self->args->stream);
}


//  ---------------------------------------------------------------------------
//  prepare_stream_read_command
//

static void
prepare_stream_read_command (client_t *self)
{
    mlm_proto_set_stream (self->message, self->args->stream);
    mlm_proto_set_pattern (self->message, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  prepare_service_offer_command
//

static void
prepare_service_offer_command (client_t *self)
{
    mlm_proto_set_address (self->message, self->args->address);
    mlm_proto_set_pattern (self->message, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  pass_stream_message_to_app
//  TODO: these methods could be generated automatically from the protocol
//

static void
pass_stream_message_to_app (client_t *self)
{
    zstr_sendm (self->msgpipe, "STREAM DELIVER");
    zsock_bsend (self->msgpipe, "sssp",
                 mlm_proto_address (self->message),
                 mlm_proto_sender (self->message),
                 mlm_proto_subject (self->message),
                 mlm_proto_get_content (self->message));
}


//  ---------------------------------------------------------------------------
//  pass_mailbox_message_to_app
//

static void
pass_mailbox_message_to_app (client_t *self)
{
    zstr_sendm (self->msgpipe, "MAILBOX DELIVER");
    zsock_bsend (self->msgpipe, "ssssp",
                 mlm_proto_sender (self->message),
                 mlm_proto_address (self->message),
                 mlm_proto_subject (self->message),
                 mlm_proto_tracker (self->message),
                 mlm_proto_get_content (self->message));
}


//  ---------------------------------------------------------------------------
//  pass_service_message_to_app
//

static void
pass_service_message_to_app (client_t *self)
{
    zstr_sendm (self->msgpipe, "SERVICE DELIVER");
    zsock_bsend (self->msgpipe, "ssssp",
                 mlm_proto_sender (self->message),
                 mlm_proto_address (self->message),
                 mlm_proto_subject (self->message),
                 mlm_proto_tracker (self->message),
                 mlm_proto_get_content (self->message));
}


//  ---------------------------------------------------------------------------
//  signal_success
//

static void
signal_success (client_t *self)
{
    zsock_send (self->cmdpipe, "si", "SUCCESS", 0);
}


//  ---------------------------------------------------------------------------
//  signal_bad_endpoint
//

static void
signal_bad_endpoint (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Bad server endpoint");
}


//  ---------------------------------------------------------------------------
//  signal_failure
//

static void
signal_failure (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, mlm_proto_status_reason (self->message));
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
    if (mlm_proto_status_code (self->message) == MLM_PROTO_COMMAND_INVALID)
        engine_set_next_event (self, command_invalid_event);
    else
        engine_set_next_event (self, other_event);
}


//  ---------------------------------------------------------------------------
//  signal_unhandled_error
//

static void
signal_unhandled_error (client_t *self)
{
    zsys_error ("unhandled error code from Malamute server");
}


//  ---------------------------------------------------------------------------
//  signal_server_not_present
//

static void
signal_server_not_present (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Server is not reachable");
}


//  ---------------------------------------------------------------------------
//  Selftest
void
mlm_client_test (bool verbose)
{
    printf (" * mlm_client: \n");

    //  @selftest
    mlm_client_verbose = verbose;

    //  Start a server to test against, and bind to endpoint
    zactor_t *server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "src/mlm_client.cfg", NULL);

    //  Install authenticator to test PLAIN access
    zactor_t *auth = zactor_new (zauth, NULL);
    assert (auth);
    if (verbose) {
        zstr_sendx (auth, "VERBOSE", NULL);
        zsock_wait (auth);
    }
    zstr_sendx (auth, "PLAIN", "src/passwords.cfg", NULL);
    zsock_wait (auth);

    /////
    // Test mutual address exchange
    /**
       -----------                                                                                             ----------

       | Frontend | -> 1) provide addr, req opp addr   ->  |\         / |  <-  1) Provide addr, Req opp addr  | Backend |
                                                              >Broker<                                              
       -----------     2) Receive opp addr             <-                  -> 2) Receive opposite address      ----------
       
       3) Direct connection establishment
       -----------              ----------- 
                                            
       | Frontend | <- Send -> | Backend |
                                            
       -----------              ----------- 

     */
    mlm_client_t *frontend = mlm_client_new ();
    assert(frontend);
    mlm_client_t *backend = mlm_client_new();
    assert(backend);

    // connect front side to broker
    printf("connecting frontend to broker\n");
    int rc = mlm_client_set_plain_auth (frontend, "writer", "secret");
    assert (rc == 0);
    rc=mlm_client_connect (frontend, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    // connect back side to broker    
    printf("connecting backend to broker\n");
    rc = mlm_client_set_plain_auth (backend, "reader", "secret");
    assert (rc == 0);
    rc=mlm_client_connect (backend, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);
    
    //before you ever set a service, you should've already called bind in order to facilitate a 
    // connection from the other side. In this way, it's causally correct.
    // the frontend tells the broker that it is interested in addresses that are on the other side
    printf("setting frontend service\n");
    mlm_client_set_worker(frontend, "backendEndpoints", "SET*");
    // the backend tells the broker that it is interested in addresses that are on the other side
    printf("setting backend service\n");
    mlm_client_set_worker(backend, "frontendEndpoints", "SET*");
    
    // the frontend tells the broker what it's address is, fulfilling a need that the backend set;
    //  consequently, the broker sends this to the backend, which has set a service that it is
    //  subscribed to. SET* matches SET
    printf("sending frontend address SET message\n");
    mlm_client_sendforx(frontend, "frontendEndpoints", "SET", "inproc://frontend", NULL);
    // the backend tells the broker what it's address is, fullfilling a need that the frontend set;
    //  consequently, the broker sends this to the frontend, which has reported a service to the broker
    //  the broker matches SET to SET*.
    printf("sending backend address SET message\n");
    mlm_client_sendforx(backend, "backendEndpoints", "SET", "inproc://backend", NULL);
    
    char *set=NULL, *opp_addr=NULL;
    printf("receiving on backend");
    mlm_client_recvx(backend, &set, &opp_addr, NULL);
    assert(set); 
    assert(opp_addr); 
    // connect backend to frontend here, then clean up
    zstr_free(&opp_addr);
    zstr_free(&set);

    printf("receiving on backend");
    mlm_client_recvx(frontend, &set, &opp_addr, NULL);
    assert(set);
    assert(opp_addr);
    // connect frontend to backend here, then clean up
    zstr_free(&opp_addr);
    zstr_free(&set);

    // cleanup
    mlm_client_destroy(&backend);
    mlm_client_destroy(&frontend);
    
    // End of mutual address exchange tests
    /////
    
    //  Test stream pattern
    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    rc = mlm_client_set_plain_auth (writer, "writer", "secret");
    assert (rc == 0);
    assert (mlm_client_connected (writer) == false);
    rc = mlm_client_connect (writer, "tcp://127.0.0.1:9999", 1000, "writer");
    assert (rc == 0);
    assert (mlm_client_connected (writer) == true);

    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_set_plain_auth (reader, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_set_producer (writer, "weather");
    mlm_client_set_consumer (reader, "weather", "temp.*");

    mlm_client_sendx (writer, "temp.moscow", "1", NULL);
    mlm_client_sendx (writer, "rain.moscow", "2", NULL);
    mlm_client_sendx (writer, "temp.madrid", "3", NULL);
    mlm_client_sendx (writer, "rain.madrid", "4", NULL);
    mlm_client_sendx (writer, "temp.london", "5", NULL);
    mlm_client_sendx (writer, "rain.london", "6", NULL);

    char *subject, *content;
    mlm_client_recvx (reader, &subject, &content, NULL);
    assert (streq (subject, "temp.moscow"));
    assert (streq (content, "1"));
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_sender (reader), "writer"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader, &subject, &content, NULL);
    assert (streq (subject, "temp.madrid"));
    assert (streq (content, "3"));
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_subject (reader), "temp.madrid"));
    assert (streq (mlm_client_sender (reader), "writer"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader, &subject, &content, NULL);
    assert (streq (subject, "temp.london"));
    assert (streq (content, "5"));
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_sender (reader), "writer"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_destroy (&reader);

    //  Test mailbox pattern
    reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_set_plain_auth (reader, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader, "tcp://127.0.0.1:9999", 1000, "mailbox");
    assert (rc == 0);

    mlm_client_sendtox (writer, "mailbox", "subject 1", "Message 1", "attachment", NULL);

    char *attach;
    mlm_client_recvx (reader, &subject, &content, &attach, NULL);
    assert (streq (subject, "subject 1"));
    assert (streq (content, "Message 1"));
    assert (streq (attach, "attachment"));
    assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
    assert (streq (mlm_client_subject (reader), "subject 1"));
    assert (streq (mlm_client_sender (reader), "writer"));
    zstr_free (&subject);
    zstr_free (&content);
    zstr_free (&attach);

    //  Now test that mailbox survives reader disconnect
    mlm_client_destroy (&reader);
    mlm_client_sendtox (writer, "mailbox", "subject 2", "Message 2", NULL);
    mlm_client_sendtox (writer, "mailbox", "subject 3", "Message 3", NULL);

    reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_set_plain_auth (reader, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader, "tcp://127.0.0.1:9999", 500, "mailbox");
    assert (rc == 0);

    mlm_client_recvx (reader, &subject, &content, &attach, NULL);
    assert (streq (subject, "subject 2"));
    assert (streq (content, "Message 2"));
    assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader, &subject, &content, &attach, NULL);
    assert (streq (subject, "subject 3"));
    assert (streq (content, "Message 3"));
    assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
    zstr_free (&subject);
    zstr_free (&content);

    //  Test service pattern
    mlm_client_set_worker (reader, "printer", "bw.*");
    mlm_client_set_worker (reader, "printer", "color.*");

    mlm_client_sendforx (writer, "printer", "bw.A4", "Important contract", NULL);
    mlm_client_sendforx (writer, "printer", "bw.A5", "Special conditions", NULL);

    mlm_client_recvx (reader, &subject, &content, NULL);
    assert (streq (subject, "bw.A4"));
    assert (streq (content, "Important contract"));
    assert (streq (mlm_client_command (reader), "SERVICE DELIVER"));
    assert (streq (mlm_client_sender (reader), "writer"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader, &subject, &content, NULL);
    assert (streq (subject, "bw.A5"));
    assert (streq (content, "Special conditions"));
    assert (streq (mlm_client_command (reader), "SERVICE DELIVER"));
    assert (streq (mlm_client_sender (reader), "writer"));
    zstr_free (&subject);
    zstr_free (&content);

    //  Test that writer shutdown does not cause message loss
    mlm_client_set_consumer (reader, "weather", "temp.*");
    mlm_client_sendx (writer, "temp.brussels", "7", NULL);
    mlm_client_destroy (&writer);

    mlm_client_recvx (reader, &subject, &content, NULL);
    assert (streq (subject, "temp.brussels"));
    assert (streq (content, "7"));
    zstr_free (&subject);
    zstr_free (&content);
    mlm_client_destroy (&reader);

    //  Test multiple readers and multiple writers
    mlm_client_t *writer1 = mlm_client_new ();
    assert (writer1);
    rc = mlm_client_set_plain_auth (writer1, "writer", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (writer1, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_t *writer2 = mlm_client_new ();
    assert (writer2);
    rc = mlm_client_set_plain_auth (writer2, "writer", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (writer2, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_t *reader1 = mlm_client_new ();
    assert (reader1);
    rc = mlm_client_set_plain_auth (reader1, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader1, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_t *reader2 = mlm_client_new ();
    assert (reader2);
    rc = mlm_client_set_plain_auth (reader2, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader2, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_set_producer (writer1, "weather");
    mlm_client_set_producer (writer2, "traffic");
    mlm_client_set_consumer (reader1, "weather", "newyork");
    mlm_client_set_consumer (reader1, "traffic", "newyork");
    mlm_client_set_consumer (reader2, "weather", "newyork");
    mlm_client_set_consumer (reader2, "traffic", "newyork");

    mlm_client_sendx (writer1, "newyork", "8", NULL);

    mlm_client_recvx (reader1, &subject, &content, NULL);
    assert (streq (mlm_client_address (reader1), "weather"));
    assert (streq (subject, "newyork"));
    assert (streq (content, "8"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader2, &subject, &content, NULL);
    assert (streq (mlm_client_address (reader2), "weather"));
    assert (streq (subject, "newyork"));
    assert (streq (content, "8"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_sendx (writer2, "newyork", "85", NULL);

    mlm_client_recvx (reader1, &subject, &content, NULL);
    assert (streq (mlm_client_address (reader1), "traffic"));
    assert (streq (subject, "newyork"));
    assert (streq (content, "85"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader2, &subject, &content, NULL);
    assert (streq (mlm_client_address (reader2), "traffic"));
    assert (streq (subject, "newyork"));
    assert (streq (content, "85"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_destroy (&writer1);
    mlm_client_destroy (&writer2);
    mlm_client_destroy (&reader1);
    mlm_client_destroy (&reader2);

    //  Done, shut down
    zactor_destroy (&auth);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}

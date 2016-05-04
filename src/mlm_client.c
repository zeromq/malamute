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
    zlistx_t *replays;          //  Replay server-side state set-up
} client_t;

//  Include the generated client engine
#include "mlm_client_engine.inc"

//  Work with server-side state replay
typedef struct {
    char *name;                 //  Replay command
    char *stream;               //  Stream name
    char *pattern;              //  Stream pattern if any
} replay_t;

static replay_t *
s_replay_new (const char *name, const char *stream, const char *pattern)
{
    replay_t *self = (replay_t *) zmalloc (sizeof (replay_t));
    if (self) {
        self->name = strdup (name);
        self->stream = strdup (stream);
        self->pattern = pattern? strdup (pattern): NULL;
    }
    return self;
}

static void
s_replay_destroy (replay_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        replay_t *self = *self_p;
        free (self->name);
        free (self->stream);
        free (self->pattern);
        free (self);
        *self_p = NULL;
    }
}

static void
s_replay_execute (client_t *self, replay_t *replay)
{
    if (replay) {
        if (streq (replay->name, "STREAM WRITE")) {
            engine_set_next_event (self, set_producer_event);
            mlm_proto_set_stream (self->message, replay->stream);
        }
        else
        if (streq (replay->name, "STREAM READ")) {
            engine_set_next_event (self, set_consumer_event);
            mlm_proto_set_stream (self->message, replay->stream);
            mlm_proto_set_pattern (self->message, replay->pattern);
        }
        else
        if (streq (replay->name, "SERVICE OFFER")) {
            engine_set_next_event (self, set_worker_event);
            mlm_proto_set_address (self->message, replay->stream);
            mlm_proto_set_pattern (self->message, replay->pattern);
        }
    }
    else
        engine_set_next_event (self, replay_ready_event);
}

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    //  We'll ping the server once per second
    self->heartbeat_timer = 1000;
    self->replays = zlistx_new ();
    zlistx_set_destructor (self->replays, (czmq_destructor *) s_replay_destroy);
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    zlistx_destroy (&self->replays);
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
    engine_set_expiry (self, self->args->timeout);
}


//  ---------------------------------------------------------------------------
//  client_is_connected
//

static void
client_is_connected (client_t *self)
{
    engine_set_connected (self, true);
    //  We send a PING to the server on every heartbeat
    engine_set_heartbeat (self, self->heartbeat_timer);
    //  We get an expired event if server sends nothing within 3 heartbeats
    engine_set_expiry (self, self->heartbeat_timer * 4);
}


//  ---------------------------------------------------------------------------
//  server_has_gone_offline
//

static void
server_has_gone_offline (client_t *self)
{
    //  We stop the heartbeats and thereby stop sending a PING to the server
    //  periodically
    engine_set_heartbeat (self, 0);
    engine_set_connected (self, false);
}


//  ---------------------------------------------------------------------------
//  prepare_stream_write_command
//

static void
prepare_stream_write_command (client_t *self)
{
    zlistx_add_end (self->replays,
        s_replay_new ("STREAM WRITE", self->args->stream, NULL));
    mlm_proto_set_stream (self->message, self->args->stream);
}


//  ---------------------------------------------------------------------------
//  prepare_stream_read_command
//

static void
prepare_stream_read_command (client_t *self)
{
    zlistx_add_end (self->replays,
        s_replay_new ("STREAM READ", self->args->stream, self->args->pattern));
    mlm_proto_set_stream (self->message, self->args->stream);
    mlm_proto_set_pattern (self->message, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  prepare_service_offer_command
//

static void
prepare_service_offer_command (client_t *self)
{
    zlistx_add_end (self->replays,
        s_replay_new ("SERVICE OFFER", self->args->address, self->args->pattern));
    mlm_proto_set_address (self->message, self->args->address);
    mlm_proto_set_pattern (self->message, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  get_first_replay_command
//

static void
get_first_replay_command (client_t *self)
{
    replay_t *replay = (replay_t *) zlistx_first (self->replays);
    s_replay_execute (self, replay);
}


//  ---------------------------------------------------------------------------
//  get_next_replay_command
//

static void
get_next_replay_command (client_t *self)
{
    replay_t *replay = (replay_t *) zlistx_next (self->replays);
    s_replay_execute (self, replay);
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
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Syntax error in server endpoint");
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
    if (mlm_proto_status_code (self->message) == MLM_PROTO_FAILED)
        engine_set_next_event (self, failed_event);
    else
        engine_set_next_event (self, other_event);
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

    //  Test api, when client is not connected at all
    mlm_client_t *client = mlm_client_new ();
    assert (client);
    mlm_client_set_verbose (client, verbose);
    assert (mlm_client_connected (client) == false);
    int rc = mlm_client_set_producer (client, "weather");
    assert (mlm_client_connected (client) == false);
    assert ( rc == -1 );
    rc = mlm_client_set_consumer (client, "weather", ".*");
    assert (mlm_client_connected (client) == false);
    assert ( rc == -1 );
    rc = mlm_client_set_worker (client, "weather", ".*");
    assert (mlm_client_connected (client) == false);
    assert ( rc == -1 );
    mlm_client_destroy (&client);

    //  Start a server to test against, and bind to endpoint
    //  this instance of the server is going to be be killed
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

    // Test the robustness of the client, againt server failure
    client = mlm_client_new ();
    assert (client);
    mlm_client_set_verbose (client, verbose);
    rc = mlm_client_set_plain_auth (client, "writer", "secret");
    assert ( rc == 0 );
    rc = mlm_client_connect (client, "tcp://127.0.0.1:9999", 1000, "client_robust");
    assert ( rc == 0 );
    //      stop the server
    zactor_destroy (&server);

    rc = mlm_client_set_producer (client, "new_stream");
    assert ( rc == -1 );
    rc = mlm_client_set_consumer (client, "new_stream", ".*");
    assert ( rc == -1 );
    rc = mlm_client_set_worker (client, "new_stream", ".*");
    assert ( rc == -1 );
    assert ( mlm_client_connected (client) == false);
    mlm_client_set_verbose (client, verbose);
    mlm_client_destroy (&client);

    // Test the ability to reconnect to the server, if the server returns soon
    server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "src/mlm_client.cfg", NULL);

    client = mlm_client_new ();
    assert (client);
    mlm_client_set_verbose (client, verbose);
    rc = mlm_client_set_plain_auth (client, "writer", "secret");
    assert ( rc == 0 );
    rc = mlm_client_connect (client, "tcp://127.0.0.1:9999", 1000, "client_reconnect");
    assert ( rc == 0 );
    //      stop the server
    zactor_destroy (&server);
    //      and return it
    server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "src/mlm_client.cfg", NULL);
    rc = mlm_client_set_producer (client, "new_stream");
    assert ( rc == -1 ); // the  method set producer is called too fast,
    // so, the client didn't manage to establish a new connection with
    // the newly appeared server
    zclock_sleep (5000); // wait a bit
    // after a while we are connected again
    assert (mlm_client_connected (client) == true );

    rc = mlm_client_set_consumer (client, "new_stream", ".*");
    assert ( rc == 0 );
    rc = mlm_client_set_worker (client, "new_stream", ".*");
    assert ( rc == 0 );
    zactor_destroy (&server);
    mlm_client_destroy (&client);

    // Test the ability to reconnect to the server, if the server returns when
    // the client is already in disconnected state
    server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "src/mlm_client.cfg", NULL);

    client = mlm_client_new ();
    assert (client);
    mlm_client_set_verbose (client, verbose);
    rc = mlm_client_set_plain_auth (client, "writer", "secret");
    assert ( rc == 0 );
    rc = mlm_client_connect (client, "tcp://127.0.0.1:9999", 1000, "client_reconnect");
    assert ( rc == 0 );
    //      stop the server
    zactor_destroy (&server);
    zclock_sleep (10000); // wait a bit
    assert (mlm_client_connected (client) == false);
    //      and return it
    server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "src/mlm_client.cfg", NULL);
    zclock_sleep (5000); // wait a bit
    // after a while we are connected again
    assert (mlm_client_connected (client) == true);

    rc = mlm_client_set_consumer (client, "new_stream", ".*");
    assert ( rc == 0 );
    rc = mlm_client_set_worker (client, "new_stream", ".*");
    assert ( rc == 0 );
    zactor_destroy (&server);
    mlm_client_destroy (&client);

    //  Start a server to test against, and bind to endpoint
    server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "src/mlm_client.cfg", NULL);

    //  Test stream pattern
    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    mlm_client_set_verbose (writer, verbose);
    rc = mlm_client_set_plain_auth (writer, "writer", "secret");
    assert (rc == 0);
    assert (mlm_client_connected (writer) == false);
    // try to connect to server that doesn't exist
    rc = mlm_client_connect (writer, "nonsence",1000, "writes");
    assert (rc == -1);
    assert (mlm_client_connected (writer) == false);
    // try to connect to other server, that should exist.
    rc = mlm_client_connect (writer, "tcp://127.0.0.1:9999", 1000, "writer");
    assert (rc == 0);
    assert (mlm_client_connected (writer) == true);

    mlm_client_t *reader = mlm_client_new ();
    mlm_client_set_verbose (reader, verbose);
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
    mlm_client_set_verbose (reader, verbose);
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
    mlm_client_set_verbose (reader, verbose);
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
    mlm_client_set_verbose (writer1, verbose);
    rc = mlm_client_set_plain_auth (writer1, "writer", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (writer1, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_t *writer2 = mlm_client_new ();
    assert (writer2);
    mlm_client_set_verbose (writer2, verbose);
    rc = mlm_client_set_plain_auth (writer2, "writer", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (writer2, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_t *reader1 = mlm_client_new ();
    assert (reader1);
    mlm_client_set_verbose (reader1, verbose);
    rc = mlm_client_set_plain_auth (reader1, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader1, "tcp://127.0.0.1:9999", 1000, "");
    assert (rc == 0);

    mlm_client_t *reader2 = mlm_client_new ();
    assert (reader2);
    mlm_client_set_verbose (reader2, verbose);
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


//  ---------------------------------------------------------------------------
//  announce_unhandled_error
//

static void
announce_unhandled_error (client_t *self)
{
    zsys_error ("unhandled error code from Malamute server");
}

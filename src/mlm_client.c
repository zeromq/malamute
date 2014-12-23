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
//  connect_to_server_endpoint
//

static void
connect_to_server_endpoint (client_t *self)
{
    //  If address is username/password, use these plain authentication
    if (strchr (self->args->address, '/')) {
        char *password = strchr (self->args->address, '/');
        *password++ = 0;
        zsock_set_plain_username (self->dealer, self->args->address);
        zsock_set_plain_password (self->dealer, password);
    }
    if (zsock_connect (self->dealer, "%s", self->args->endpoint)) {
        engine_set_exception (self, error_event);
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
//  use_heartbeat_timer
//

static void
use_heartbeat_timer (client_t *self)
{
    engine_set_timeout (self, self->heartbeat_timer);
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
                 mlm_proto_stream (self->message),
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
    printf (" * mlm_client: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    //  Start a server to test against, and bind to endpoint
    zactor_t *server = zactor_new (mlm_server, "mlm_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "CONFIGURE", "src/mlm_client.cfg", NULL);

    //  Install authenticator to test PLAIN access
    zactor_t *auth = zactor_new (zauth, NULL);
    assert (auth);
    if (verbose) {
        zstr_sendx (auth, "VERBOSE", NULL);
        zsock_wait (auth);
    }
    zstr_sendx (auth, "PLAIN", "src/passwords.cfg", NULL);
    zsock_wait (auth);

//     //  Test stream pattern
//     mlm_client_t *writer = mlm_client_new ("ipc://@/malamute", 1000, "writer/secret");
//     assert (writer);
//     if (verbose)
//         mlm_client_verbose (writer);
// 
//     mlm_client_t *reader = mlm_client_new ("ipc://@/malamute", 1000, "reader/secret");
//     assert (reader);
//     if (verbose)
//         mlm_client_verbose (reader);
// 
//     mlm_client_set_producer (writer, "weather");
//     mlm_client_set_consumer (reader, "weather", "temp.*");
// 
//     mlm_client_sendx (writer, "temp.moscow", "1", NULL);
//     mlm_client_sendx (writer, "rain.moscow", "2", NULL);
//     mlm_client_sendx (writer, "temp.madrid", "3", NULL);
//     mlm_client_sendx (writer, "rain.madrid", "4", NULL);
//     mlm_client_sendx (writer, "temp.london", "5", NULL);
//     mlm_client_sendx (writer, "rain.london", "6", NULL);
// 
    char *subject, *content;
//     mlm_client_recvx (reader, &subject, &content, NULL);
//     assert (streq (subject, "temp.moscow"));
//     assert (streq (content, "1"));
//     assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
//     assert (streq (mlm_client_sender (reader), "writer"));
//     zstr_free (&subject);
//     zstr_free (&content);
// 
//     mlm_client_recvx (reader, &subject, &content, NULL);
//     assert (streq (subject, "temp.madrid"));
//     assert (streq (content, "3"));
//     assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
//     assert (streq (mlm_client_subject (reader), "temp.madrid"));
//     assert (streq (mlm_client_sender (reader), "writer"));
//     zstr_free (&subject);
//     zstr_free (&content);
// 
//     mlm_client_recvx (reader, &subject, &content, NULL);
//     assert (streq (subject, "temp.london"));
//     assert (streq (content, "5"));
//     assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
//     assert (streq (mlm_client_sender (reader), "writer"));
//     zstr_free (&subject);
//     zstr_free (&content);
// 
//     //  Test mailbox pattern
//     mlm_client_sendtox (writer, "reader", "subject 1", "Message 1", "attachment", NULL);
// 
//     char *attach;
//     mlm_client_recvx (reader, &subject, &content, &attach, NULL);
//     assert (streq (subject, "subject 1"));
//     assert (streq (content, "Message 1"));
//     assert (streq (attach, "attachment"));
//     assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
//     assert (streq (mlm_client_subject (reader), "subject 1"));
//     assert (streq (mlm_client_sender (reader), "writer"));
//     zstr_free (&subject);
//     zstr_free (&content);
//     zstr_free (&attach);
// 
//     //  Now test that mailbox survives reader disconnect
//     mlm_client_destroy (&reader);
//     mlm_client_sendtox (writer, "reader", "subject 2", "Message 2", NULL);
//     mlm_client_sendtox (writer, "reader", "subject 3", "Message 3", NULL);
// 
//     reader = mlm_client_new ("ipc://@/malamute", 500, "reader/secret");
//     assert (reader);
//     if (verbose)
//         mlm_client_verbose (reader);
// 
//     mlm_client_recvx (reader, &subject, &content, &attach, NULL);
//     assert (streq (subject, "subject 2"));
//     assert (streq (content, "Message 2"));
//     assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
//     zstr_free (&subject);
//     zstr_free (&content);
// 
//     mlm_client_recvx (reader, &subject, &content, &attach, NULL);
//     assert (streq (subject, "subject 3"));
//     assert (streq (content, "Message 3"));
//     assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
//     zstr_free (&subject);
//     zstr_free (&content);
// 
//     //  Test service pattern
//     mlm_client_set_worker (reader, "printer", "bw.*");
//     mlm_client_set_worker (reader, "printer", "color.*");
// 
//     mlm_client_sendforx (writer, "printer", "bw.A4", "Important contract", NULL);
//     mlm_client_sendforx (writer, "printer", "bw.A5", "Special conditions", NULL);
// 
//     mlm_client_recvx (reader, &subject, &content, NULL);
//     assert (streq (subject, "bw.A4"));
//     assert (streq (content, "Important contract"));
//     assert (streq (mlm_client_command (reader), "SERVICE DELIVER"));
//     assert (streq (mlm_client_sender (reader), "writer"));
//     zstr_free (&subject);
//     zstr_free (&content);
// 
//     mlm_client_recvx (reader, &subject, &content, NULL);
//     assert (streq (subject, "bw.A5"));
//     assert (streq (content, "Special conditions"));
//     assert (streq (mlm_client_command (reader), "SERVICE DELIVER"));
//     assert (streq (mlm_client_sender (reader), "writer"));
//     zstr_free (&subject);
//     zstr_free (&content);
//         
//     //  Test that writer shutdown does not cause message loss
//     mlm_client_set_consumer (reader, "weather", "temp.*");
//     mlm_client_sendx (writer, "temp.brussels", "7", NULL);
//     mlm_client_destroy (&writer);
//     
//     mlm_client_recvx (reader, &subject, &content, NULL);
//     assert (streq (subject, "temp.brussels"));
//     assert (streq (content, "7"));
//     zstr_free (&subject);
//     zstr_free (&content);
//     mlm_client_destroy (&reader);
//     
    //  Test multiple readers for same message
    mlm_client_t *writer = mlm_client_new ("ipc://@/malamute", 1000, "writer/secret");
    assert (writer);
    if (verbose)
        mlm_client_verbose (writer);

    mlm_client_t *reader1 = mlm_client_new ("ipc://@/malamute", 1000, "reader/secret");
    assert (reader1);
    if (verbose)
        mlm_client_verbose (reader1);

    mlm_client_t *reader2 = mlm_client_new ("ipc://@/malamute", 1000, "reader/secret");
    assert (reader2);
    if (verbose)
        mlm_client_verbose (reader2);

    mlm_client_set_producer (writer, "weather");
    mlm_client_set_consumer (reader1, "weather", "temp.*");
    mlm_client_set_consumer (reader2, "weather", "temp.*");

    mlm_client_sendx (writer, "temp.newyork", "8", NULL);

    mlm_client_recvx (reader1, &subject, &content, NULL);
    assert (streq (subject, "temp.newyork"));
    assert (streq (content, "8"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader2, &subject, &content, NULL);
    assert (streq (subject, "temp.newyork"));
    assert (streq (content, "8"));
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_destroy (&writer);
    mlm_client_destroy (&reader1);
    mlm_client_destroy (&reader2);
    
    //  Done, shut down
    zactor_destroy (&auth);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}

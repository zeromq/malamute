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
    mlm_msg_t *message;         //  Message to/from server
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
    mlm_msg_set_address (self->message, self->args->address);
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
    mlm_msg_set_stream (self->message, self->args->stream);
}


//  ---------------------------------------------------------------------------
//  prepare_stream_read_command
//

static void
prepare_stream_read_command (client_t *self)
{
    mlm_msg_set_stream (self->message, self->args->stream);
    mlm_msg_set_pattern (self->message, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  prepare_service_offer_command
//

static void
prepare_service_offer_command (client_t *self)
{
    mlm_msg_set_service (self->message, self->args->service);
    mlm_msg_set_pattern (self->message, self->args->pattern);
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
                 mlm_msg_stream (self->message),
                 mlm_msg_sender (self->message),
                 mlm_msg_subject (self->message),
                 mlm_msg_get_content (self->message));
}


//  ---------------------------------------------------------------------------
//  pass_mailbox_message_to_app
//

static void
pass_mailbox_message_to_app (client_t *self)
{
    zstr_sendm (self->msgpipe, "MAILBOX DELIVER");
    zsock_bsend (self->msgpipe, "ssssp",
                 mlm_msg_sender (self->message),
                 mlm_msg_address (self->message),
                 mlm_msg_subject (self->message),
                 mlm_msg_tracker (self->message),
                 mlm_msg_get_content (self->message));
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
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, mlm_msg_status_reason (self->message));
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
    if (mlm_msg_status_code (self->message) == MLM_MSG_COMMAND_INVALID)
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
    zstr_sendx (server, "BIND", "ipc://@/malamute", NULL);

    //  Test stream pattern
    mlm_client_t *writer = mlm_client_new ("ipc://@/malamute", 500, "writer");
    assert (writer);
    if (verbose)
        mlm_client_verbose (writer);

    mlm_client_t *reader = mlm_client_new ("ipc://@/malamute", 500, "reader");
    assert (reader);
    if (verbose)
        mlm_client_verbose (reader);

    mlm_client_produce (writer, "weather");
    mlm_client_consume (reader, "weather", "temp.*");

    zmsg_t *msg;
    msg = zmsg_new ();
    zmsg_addstr (msg, "1");
    mlm_client_stream_send (writer, "temp.moscow", &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "2");
    mlm_client_stream_send (writer, "rain.moscow", &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "3");
    mlm_client_stream_send (writer, "temp.madrid", &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "4");
    mlm_client_stream_send (writer, "rain.madrid", &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "5");
    mlm_client_stream_send (writer, "temp.london", &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "6");
    mlm_client_stream_send (writer, "rain.london", &msg);

    char *content;
    msg = mlm_client_recv (reader);
    assert (msg);
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_subject (reader), "temp.moscow"));
    assert (streq (mlm_client_sender (reader), "writer"));
    content = zmsg_popstr (msg);
    assert (streq (content, "1"));
    zstr_free (&content);
    zmsg_destroy (&msg);
    
    msg = mlm_client_recv (reader);
    assert (msg);
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_subject (reader), "temp.madrid"));
    assert (streq (mlm_client_sender (reader), "writer"));
    content = zmsg_popstr (msg);
    assert (streq (content, "3"));
    zstr_free (&content);
    zmsg_destroy (&msg);
    
    msg = mlm_client_recv (reader);
    assert (msg);
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_subject (reader), "temp.london"));
    assert (streq (mlm_client_sender (reader), "writer"));
    content = zmsg_popstr (msg);
    assert (streq (content, "5"));
    zstr_free (&content);
    zmsg_destroy (&msg);

    //  Test mailbox pattern
    msg = zmsg_new ();
    zmsg_addstr (msg, "Message 1");
    zmsg_addmem (msg, "attachment", sizeof ("attachment"));
    mlm_client_mailbox_send (writer, "reader", "subject 1", "", 0, &msg);
    
    msg = mlm_client_recv (reader);
    assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
    assert (streq (mlm_client_subject (reader), "subject 1"));
    assert (streq (mlm_client_sender (reader), "writer"));
    content = zmsg_popstr (msg);
    assert (streq (content, "Message 1"));
    zstr_free (&content);
    content = zmsg_popstr (msg);
    assert (streq (content, "attachment"));
    zstr_free (&content);
    zmsg_destroy (&msg);

    //  Now test that mailbox survives reader disconnect
    mlm_client_destroy (&reader);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "Message 2");
    mlm_client_mailbox_send (writer, "reader", "subject 2", "", 0, &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "Message 3");
    mlm_client_mailbox_send (writer, "reader", "subject 3", "", 0, &msg);

    reader = mlm_client_new ("ipc://@/malamute", 500, "reader");
    assert (reader);
    if (verbose)
        mlm_client_verbose (reader);

    msg = mlm_client_recv (reader);
    assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
    assert (streq (mlm_client_subject (reader), "subject 2"));
    
    msg = mlm_client_recv (reader);
    assert (streq (mlm_client_command (reader), "MAILBOX DELIVER"));
    assert (streq (mlm_client_subject (reader), "subject 3"));
    
    //  Test service pattern
    mlm_client_provide (reader, "printer", "bw.*");
    mlm_client_provide (reader, "printer", "color.*");

    msg = zmsg_new ();
    zmsg_addstr (msg, "Important contract");
    mlm_client_service_send (writer, "printer", "bw.A4", "", 0, &msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "Special conditions");
    mlm_client_service_send (writer, "printer", "bw.A4", "", 0, &msg);
            
    //  Done, shut down
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);

    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}

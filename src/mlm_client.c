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
    //  messages sent to the actor; the msg_pipe may be used for
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


//  Handle the message pipe to/from calling API

static int
publish_message (zloop_t *loop, zsock_t *reader, void *argument)
{
    client_t *self = (client_t *) argument;
    char *subject;
    zmsg_t *msg;
    zsock_brecv (self->msgpipe, "sp", &subject, &msg);
    assert (msg);
    mlm_msg_set_id (self->message, MLM_MSG_STREAM_PUBLISH);
    mlm_msg_set_subject (self->message, subject);
    mlm_msg_set_content (self->message, &msg);
    mlm_msg_send (self->message, self->dealer);
    return 0;
}


//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    //  We'll ping the server once per second
    self->heartbeat_timer = 1000;
    engine_handle_socket (self, self->msgpipe, publish_message);
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
//  prepare_for_stream_write
//

static void
prepare_for_stream_write (client_t *self)
{
    mlm_msg_set_stream (self->message, self->args->stream);
}


//  ---------------------------------------------------------------------------
//  prepare_for_stream_read
//

static void
prepare_for_stream_read (client_t *self)
{
    mlm_msg_set_stream (self->message, self->args->stream);
    mlm_msg_set_pattern (self->message, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  deliver_message_to_application
//

static void
deliver_message_to_application (client_t *self)
{
    zsock_bsend (self->msgpipe, "ssp",
                 mlm_msg_sender (self->message),
                 mlm_msg_subject (self->message),
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

    //  Do a simple client-writer test, using the high level API rather
    //  than the actor message interface.
    //  TODO: it would be simpler to pass endpoint & timeout in constructor,
    //  needs changes to zproto_client to make this work.
    mlm_client_t *writer = mlm_client_new ("ipc://@/malamute", 500);
    assert (writer);
    if (verbose)
        mlm_client_verbose (writer);

    mlm_client_t *reader = mlm_client_new ("ipc://@/malamute", 500);
    assert (reader);
    if (verbose)
        mlm_client_verbose (reader);

    mlm_client_produce (writer, "weather");
    mlm_client_consume (reader, "weather", "temp.*");

    //  TODO: wrap this into the client API, as send/recv methods
    zmsg_t *msg;
    msg = zmsg_new ();
    zmsg_addstr (msg, "1");
    zsock_bsend (mlm_client_msgpipe (writer), "sp", "temp.moscow", msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "2");
    zsock_bsend (mlm_client_msgpipe (writer), "sp", "rain.moscow", msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "3");
    zsock_bsend (mlm_client_msgpipe (writer), "sp", "temp.madrid", msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "4");
    zsock_bsend (mlm_client_msgpipe (writer), "sp", "rain.madrid", msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "5");
    zsock_bsend (mlm_client_msgpipe (writer), "sp", "temp.london", msg);
    
    msg = zmsg_new ();
    zmsg_addstr (msg, "6");
    zsock_bsend (mlm_client_msgpipe (writer), "sp", "rain.london", msg);

    char *sender, *subject, *content;
    
    zsock_brecv (mlm_client_msgpipe (reader), "ssp", &sender, &subject, &msg);
    content = zmsg_popstr (msg);
    assert (streq (content, "1"));
    assert (streq (subject, "temp.moscow"));
    zstr_free (&content);
    zmsg_destroy (&msg);
    
    zsock_brecv (mlm_client_msgpipe (reader), "ssp", &sender, &subject, &msg);
    content = zmsg_popstr (msg);
    assert (streq (content, "3"));
    assert (streq (subject, "temp.madrid"));
    zstr_free (&content);
    zmsg_destroy (&msg);

    zsock_brecv (mlm_client_msgpipe (reader), "ssp", &sender, &subject, &msg);
    content = zmsg_popstr (msg);
    assert (streq (content, "5"));
    assert (streq (subject, "temp.london"));
    zstr_free (&content);
    zmsg_destroy (&msg);

    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);

    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  prepare_for_stream_publish
//

static void
prepare_for_stream_publish (client_t *self)
{

}

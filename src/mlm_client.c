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
    //  and are set by the generated engine.
    zsock_t *pipe;              //  Actor pipe back to caller
    zsock_t *dealer;            //  Socket to talk to server
    mlm_msg_t *msgout;          //  Message to send to server
    mlm_msg_t *msgin;           //  Message received from server
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
    mlm_msg_set_stream (self->msgout, self->args->stream);
}


//  ---------------------------------------------------------------------------
//  prepare_for_stream_read
//

static void
prepare_for_stream_read (client_t *self)
{
    mlm_msg_set_stream (self->msgout, self->args->stream);
    mlm_msg_set_pattern (self->msgout, self->args->pattern);
}


//  ---------------------------------------------------------------------------
//  prepare_for_stream_publish
//

static void
prepare_for_stream_publish (client_t *self)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, self->args->content);
    mlm_msg_set_subject (self->msgout, self->args->subject);
    mlm_msg_set_content (self->msgout, &msg);
}


//  ---------------------------------------------------------------------------
//  deliver_message_to_application
//

static void
deliver_message_to_application (client_t *self)
{
    char *content = zmsg_popstr (mlm_msg_content (self->msgin));
    zsock_send (self->pipe, "ssss", "MESSAGE",
                mlm_msg_sender (self->msgin),
                mlm_msg_subject (self->msgin),
                content);
    zstr_free (&content);
}


//  ---------------------------------------------------------------------------
//  signal_success
//

static void
signal_success (client_t *self)
{
    zsock_send (self->pipe, "si", "SUCCESS", 0);
}


//  ---------------------------------------------------------------------------
//  signal_failure
//

static void
signal_failure (client_t *self)
{
    zsock_send (self->pipe, "sis", "FAILURE", -1,
                mlm_msg_status_reason (self->msgin));
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
    if (mlm_msg_status_code (self->msgin) == MLM_MSG_COMMAND_INVALID)
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
    zsock_send (self->pipe, "sis", "FAILURE", -1,
                "Server is not reachable");
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

    mlm_client_attach (writer, "weather");
    mlm_client_subscribe (reader, "weather", "temp.*");
    
    mlm_client_send (writer, "temp.moscow", "1");
    mlm_client_send (writer, "rain.moscow", "2");
    mlm_client_send (writer, "temp.chicago", "3");
    mlm_client_send (writer, "rain.chicago", "4");
    mlm_client_send (writer, "temp.london", "5");
    mlm_client_send (writer, "rain.london", "6");

    char *message = mlm_client_recv (reader);
    assert (streq (message, "1"));
    assert (streq (mlm_client_subject (reader), "temp.moscow"));
    
    message = mlm_client_recv (reader);
    assert (streq (message, "3"));
    assert (streq (mlm_client_subject (reader), "temp.chicago"));

    message = mlm_client_recv (reader);
    assert (streq (message, "5"));
    assert (streq (mlm_client_subject (reader), "temp.london"));

    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);

    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}

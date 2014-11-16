/*  =========================================================================
    mlm_server - Malamute Server

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*
@header
    This actor implements the Malamute service. The actor uses the CZMQ socket
    command interface, rather than a classic C API. You can start as many
    instances of the Malamute service as you like. Each will run in its own
    namespace, as virtual hosts. This class is wrapped as a main program via
    the malamute.c application, and can be wrapped in other languages in the
    same way as any C API.
@discuss
    This is a minimal, incomplete implementation of Malamute. It does however
    not have any known bugs.
@end
*/

#include "mlm_classes.h"

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This is a simple stream class

typedef struct {
    zactor_t *actor;            //  Stream engine, zactor
    zsock_t *msgpipe;           //  Socket to send messages to for stream
} stream_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    
    zhash_t *streams;           //  Holds stream instances by name

    //  Hold currently dispatching message here
    char *sender;               //  Originating client
    char *subject;              //  Message subject
    zmsg_t *content;            //  Message content
};


//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    mlm_msg_t *message;         //  Message in and out

    //  These properties are specific for this application
    char *address;              //  Address of this client
    stream_t *writer;           //  Stream we're writing to, if any
    zlist_t *readers;           //  All streams we're reading from
};

//  Include the generated server engine
#include "mlm_server_engine.inc"

//  Forward traffic to clients

static int
s_forward_traffic (zloop_t *loop, zsock_t *reader, void *argument)
{
    server_t *self = (server_t *) argument;
    zmsg_destroy (&self->content);
    void *client;
    zsock_brecv (reader, "pssm", &client, &self->sender, &self->subject, &self->content);
    engine_send_event ((client_t *) client, forward_event);
    return 0;
}


static void
s_stream_destroy (stream_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        stream_t *self = *self_p;
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        free (self);
        *self_p = NULL;
    }
}

static stream_t *
s_stream_new (client_t *client, const char *name)
{
    stream_t *self = (stream_t *) zmalloc (sizeof (stream_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        if (self->msgpipe) {
            engine_handle_socket (client->server, self->msgpipe, s_forward_traffic);
            self->actor = zactor_new (mlm_stream_simple, backend);
        }
        if (!self->actor)
            s_stream_destroy (&self);
    }
    return self;
}


//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    self->streams = zhash_new ();
    zhash_set_destructor (self->streams, (czmq_destructor *) s_stream_destroy);
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zmsg_destroy (&self->content);
    zhash_destroy (&self->streams);
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    return NULL;
}


//  Allocate properties and structures for a new client connection and
//  optionally engine_set_next_event (). Return 0 if OK, or -1 on error.

static int
client_initialize (client_t *self)
{
    self->readers = zlist_new ();
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    zlist_destroy (&self->readers);
    free (self->address);
}


//  ---------------------------------------------------------------------------
//  register_new_client
//

static void
register_new_client (client_t *self)
{
    self->address = strdup (mlm_msg_address (self->message));
    mlm_msg_set_status_code (self->message, MLM_MSG_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  open_stream_writer
//

stream_t *
s_require_stream (client_t *self, const char *stream_name)
{
    stream_t *stream = (stream_t *) zhash_lookup (self->server->streams, stream_name);
    if (!stream)
        stream = s_stream_new (self, stream_name);
    if (stream)
        zhash_insert (self->server->streams, stream_name, stream);
    return (stream);
}


static void
open_stream_writer (client_t *self)
{
    //  A writer talks to a single stream
    self->writer = s_require_stream (self, mlm_msg_stream (self->message));
    if (self->writer)
        mlm_msg_set_status_code (self->message, MLM_MSG_SUCCESS);
    else {
        mlm_msg_set_status_code (self->message, MLM_MSG_INTERNAL_ERROR);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  open_stream_reader
//

static void
open_stream_reader (client_t *self)
{
    stream_t *stream = s_require_stream (self, mlm_msg_stream (self->message));
    if (stream) {
        zlist_append (self->readers, stream);
        zsock_send (stream->actor, "sps", "COMPILE", self, mlm_msg_pattern (self->message));
        mlm_msg_set_status_code (self->message, MLM_MSG_SUCCESS);
    }
    else {
        mlm_msg_set_status_code (self->message, MLM_MSG_INTERNAL_ERROR);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  write_message_to_stream
//

static void
write_message_to_stream (client_t *self)
{
    if (self->writer)
        zsock_bsend (self->writer->msgpipe, "pssp",
                    self,
                    self->address,
                    mlm_msg_subject (self->message),
                    mlm_msg_get_content (self->message));
    else {
        //  In fact we can't really reply to a STREAM_PUBLISH
        mlm_msg_set_status_code (self->message, MLM_MSG_COMMAND_INVALID);
        engine_set_exception (self, exception_event);
    }
}


//  --------------------------------------------------------------------------
//  get_content_to_forward
//

static void
get_content_to_forward (client_t *self)
{
    mlm_msg_set_sender  (self->message, self->server->sender);
    mlm_msg_set_subject (self->message, self->server->subject);
    mlm_msg_set_content (self->message, &self->server->content);
}


//  ---------------------------------------------------------------------------
//  write_message_to_mailbox
//

static void
write_message_to_mailbox (client_t *self)
{
    mlm_msg_set_status_code (self->message, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  write_message_to_service
//

static void
write_message_to_service (client_t *self)
{
    mlm_msg_set_status_code (self->message, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  open_service_worker
//

static void
open_service_worker (client_t *self)
{
    mlm_msg_set_status_code (self->message, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  have_message_confirmation
//

static void
have_message_confirmation (client_t *self)
{
    mlm_msg_set_status_code (self->message, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  credit_the_client
//

static void
credit_the_client (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  deregister_the_client
//

static void
deregister_the_client (client_t *self)
{
    stream_t *stream = (stream_t *) zlist_pop (self->readers);
    while (stream) {
        zsock_send (stream->actor, "sp", "CANCEL", self);
        stream = (stream_t *) zlist_pop (self->readers);
    }
    mlm_msg_set_status_code (self->message, MLM_MSG_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  allow_time_to_settle
//

static void
allow_time_to_settle (client_t *self)
{
    //  We are still using hard pointers rather than cycled client IDs, so
    //  there may be messages pending from a stream which refer to our client.
    //  Stupid strategy for now is to give the client thread a while to process
    //  these, before killing it.
    engine_set_wakeup_event (self, 1000, settled_event);
}


//  ---------------------------------------------------------------------------
//  message_not_valid_in_this_state
//

static void
message_not_valid_in_this_state (client_t *self)
{
    mlm_msg_set_status_code (self->message, MLM_MSG_COMMAND_INVALID);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  Selftest

void
mlm_server_test (bool verbose)
{
    printf (" * mlm_server: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    zactor_t *server = zactor_new (mlm_server, "mlm_server_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/malamute", NULL);

    zsock_t *reader = zsock_new (ZMQ_DEALER);
    assert (reader);
    zsock_connect (reader, "ipc://@/malamute");
    zsock_set_rcvtimeo (reader, 500);

    mlm_msg_t *message = mlm_msg_new ();

    //  Server insists that connection starts properly
    mlm_msg_set_id (message, MLM_MSG_STREAM_WRITE);
    mlm_msg_send (message, reader);
    mlm_msg_recv (message, reader);
    assert (mlm_msg_id (message) == MLM_MSG_ERROR);
    assert (mlm_msg_status_code (message) == MLM_MSG_COMMAND_INVALID);

    //  Now do a stream publish-subscribe test
    zsock_t *writer = zsock_new (ZMQ_DEALER);
    assert (writer);
    zsock_connect (writer, "ipc://@/malamute");
    zsock_set_rcvtimeo (reader, 500);

    //  Open connections from both reader and writer
    mlm_msg_set_id (message, MLM_MSG_CONNECTION_OPEN);
    mlm_msg_send (message, reader);
    mlm_msg_recv (message, reader);
    assert (mlm_msg_id (message) == MLM_MSG_OK);

    mlm_msg_set_id (message, MLM_MSG_CONNECTION_OPEN);
    mlm_msg_send (message, writer);
    mlm_msg_recv (message, writer);
    assert (mlm_msg_id (message) == MLM_MSG_OK);

    //  Prepare to write and read a "weather" stream
    mlm_msg_set_id (message, MLM_MSG_STREAM_WRITE);
    mlm_msg_set_stream (message, "weather");
    mlm_msg_send (message, writer);
    mlm_msg_recv (message, writer);
    assert (mlm_msg_id (message) == MLM_MSG_OK);

    mlm_msg_set_id (message, MLM_MSG_STREAM_READ);
    mlm_msg_set_pattern (message, "temp.*");
    mlm_msg_send (message, reader);
    mlm_msg_recv (message, reader);
    assert (mlm_msg_id (message) == MLM_MSG_OK);

    //  Now send some weather data, with null contents
    mlm_msg_set_id (message, MLM_MSG_STREAM_PUBLISH);
    mlm_msg_set_subject (message, "temp.moscow");
    mlm_msg_send (message, writer);
    mlm_msg_set_subject (message, "rain.moscow");
    mlm_msg_send (message, writer);
    mlm_msg_set_subject (message, "temp.chicago");
    mlm_msg_send (message, writer);
    mlm_msg_set_subject (message, "rain.chicago");
    mlm_msg_send (message, writer);
    mlm_msg_set_subject (message, "temp.london");
    mlm_msg_send (message, writer);
    mlm_msg_set_subject (message, "rain.london");
    mlm_msg_send (message, writer);

    //  We should receive exactly three deliveries, in order
    mlm_msg_recv (message, reader);
    assert (mlm_msg_id (message) == MLM_MSG_STREAM_DELIVER);
    assert (streq (mlm_msg_subject (message), "temp.moscow"));

    mlm_msg_recv (message, reader);
    assert (mlm_msg_id (message) == MLM_MSG_STREAM_DELIVER);
    assert (streq (mlm_msg_subject (message), "temp.chicago"));

    mlm_msg_recv (message, reader);
    assert (mlm_msg_id (message) == MLM_MSG_STREAM_DELIVER);
    assert (streq (mlm_msg_subject (message), "temp.london"));

    mlm_msg_destroy (&message);
        
    //  Finished, we can clean up
    zsock_destroy (&writer);
    zsock_destroy (&reader);
    zactor_destroy (&server);
    
    //  @end
    printf ("OK\n");
}

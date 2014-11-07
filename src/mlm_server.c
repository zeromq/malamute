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
    zsock_t *publish;           //  Socket to send messages to for stream
} stream_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    
    zhash_t *streams;           //  Holds stream instances by name
    zsock_t *traffic;           //  Traffic from stream engines comes here

    //  Hold currently dispatching message here
    char *sender;
    char *subject;
    zmsg_t *content;
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

static void
s_stream_destroy (stream_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        stream_t *self = *self_p;
        zactor_destroy (&self->actor);
        zsock_destroy (&self->publish);
        free (self);
        *self_p = NULL;
    }
}

static stream_t *
s_stream_new (const char *name)
{
    stream_t *self = (stream_t *) zmalloc (sizeof (stream_t));
    if (self) {
        self->publish = zsock_new (ZMQ_PUSH);
        if (self->publish) {
            zsock_bind (self->publish, "inproc://mlm-server/%s", name);
            self->actor = zactor_new (mlm_stream_simple, (void *) name);
        }
        if (!self->actor)
            s_stream_destroy (&self);
    }
    return self;
}


//  Forward traffic to clients

static int
s_forward_traffic (zloop_t *loop, zsock_t *reader, void *argument)
{
    server_t *self = (server_t *) argument;
    zmsg_destroy (&self->content);
    zmq_msg_t msg;
    zmq_msg_init (&msg);
    zmq_msg_recv (&msg, zsock_resolve (self->traffic), 0);
    void *client;
    memcpy (&client, zmq_msg_data (&msg), sizeof (void *));
    self->sender = (char *) zmq_msg_data (&msg) + sizeof (void *);
    self->subject = (char *) zmq_msg_data (&msg) + sizeof (void *) + strlen (self->sender) + 1;
    self->content = zmsg_recv (self->traffic);
    
    engine_send_event ((client_t *) client, forward_event);
    zmq_msg_close (&msg);
    return 0;
}


//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    self->streams = zhash_new ();
    self->traffic = zsock_new_pull ("inproc://mlm-server");
    engine_handle_socket (self, self->traffic, s_forward_traffic);
    zhash_set_destructor (self->streams, (czmq_destructor *) s_stream_destroy);
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zmsg_destroy (&self->content);
    zhash_destroy (&self->streams);
    zsock_destroy (&self->traffic);
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
    stream_t *stream = (stream_t *) zlist_pop (self->readers);
    while (stream) {
        zsock_send (stream->actor, "sp", "CANCEL", self);
        stream = (stream_t *) zlist_pop (self->readers);
    }
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
//  deregister_the_client
//

static void
deregister_the_client (client_t *self)
{
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
        stream = s_stream_new (stream_name);
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
    //  store sender, address, subject in one block
    //  get message content and send
    zmq_msg_t msg;
    zmq_msg_init_size (&msg,
        sizeof (void *) + strlen (self->address) + strlen (mlm_msg_subject (self->message)) + 2);
    memcpy (zmq_msg_data (&msg), &self, sizeof (void *));
    strcpy ((char *) zmq_msg_data (&msg) + sizeof (void *), self->address);
    strcpy ((char *) zmq_msg_data (&msg) + sizeof (void *) + strlen (self->address) + 1,
            mlm_msg_subject (self->message));

    if (self->writer) {
        zmq_msg_send (&msg, zsock_resolve (self->writer->publish), ZMQ_MORE);
        zmsg_t *message = mlm_msg_get_content (self->message);
        zmsg_send (&message, self->writer->publish);
    }
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
    zmsg_t *content = zmsg_dup (self->server->content);
    mlm_msg_set_sender  (self->message, self->server->sender);
    mlm_msg_set_subject (self->message, self->server->subject);
    mlm_msg_set_content (self->message, &content);
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
        
    //  Finished, we can clean up
    zsock_destroy (&writer);
    zsock_destroy (&reader);
    zactor_destroy (&server);
    
    //  @end
    printf ("OK\n");
}

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

#include "../include/malamute.h"

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    
    //  These properties are specific for this application
    zlist_t *patterns;          //  List of patterns subscribed to

    //  When we're forwarding a message, we need these
    const char *sender;         //  Client sender address
    const char *subject;        //  Message subject
    zmsg_t *content;            //  Message content
};


//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    mlm_msg_t *request;         //  Last received request
    mlm_msg_t *reply;           //  Reply to send out, if any

    //  These properties are specific for this application
    char *address;              //  Address of client
};

//  Include the generated server engine
#include "mlm_server_engine.inc"

//  This is a simple pattern class

typedef struct {
    char *pattern;           //  Regular pattern to match on
    zrex_t *rex;                //  Expression, compiled as a zrex object
    zlist_t *clients;           //  All clients that asked for this pattern
} pattern_t;

static void
s_pattern_destroy (pattern_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pattern_t *self = *self_p;
        zrex_destroy (&self->rex);
        zlist_destroy (&self->clients);
        free (self->pattern);
        free (self);
        *self_p = NULL;
    }
}

static pattern_t *
s_pattern_new (const char *pattern, client_t *client)
{
    pattern_t *self = (pattern_t *) zmalloc (sizeof (pattern_t));
    if (self) {
        self->rex = zrex_new (pattern);
        if (self->rex)
            self->pattern = strdup (pattern);
        if (self->pattern)
            self->clients = zlist_new ();
        if (self->clients)
            zlist_append (self->clients, client);
        else
            s_pattern_destroy (&self);
    }
    return self;
}


//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    self->patterns = zlist_new ();
    zlist_set_destructor (self->patterns, (czmq_destructor *) s_pattern_destroy);
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zlist_destroy (&self->patterns);
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
    //  Construct properties here
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    pattern_t *pattern = (pattern_t *) zlist_first (self->server->patterns);
    while (pattern) {
        zlist_remove (pattern->clients, self);
        pattern = (pattern_t *) zlist_next (self->server->patterns);
    }
    free (self->address);
}


//  ---------------------------------------------------------------------------
//  register_new_client
//

static void
register_new_client (client_t *self)
{
    self->address = strdup (mlm_msg_address (self->request));
    mlm_msg_set_status_code (self->reply, MLM_MSG_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  deregister_the_client
//

static void
deregister_the_client (client_t *self)
{
    mlm_msg_set_status_code (self->reply, MLM_MSG_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  open_stream_writer
//

static void
open_stream_writer (client_t *self)
{
    mlm_msg_set_status_code (self->reply, MLM_MSG_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  open_stream_reader
//

static void
open_stream_reader (client_t *self)
{
    pattern_t *pattern = (pattern_t *) zlist_first (self->server->patterns);
    while (pattern) {
        if (streq (pattern->pattern, mlm_msg_pattern (self->request))) {
            client_t *client = (client_t *) zlist_first (pattern->clients);
            while (client) {
                if (client == self)
                    break;      //  This client is already on the list
                client = (client_t *) zlist_next (pattern->clients);
            }
            //  Add client, if it's new
            if (!client)
                zlist_append (pattern->clients, self);
            break;
        }
        pattern = (pattern_t *) zlist_next (self->server->patterns);
    }
    //  Add pattern, if it's new
    if (!pattern)
        zlist_append (self->server->patterns,
                      s_pattern_new (mlm_msg_pattern (self->request), self));
}


//  ---------------------------------------------------------------------------
//  write_message_to_stream
//

static void
write_message_to_stream (client_t *self)
{
    //  Keep track of the message we're sending out to subscribers
    self->server->sender = self->address;
    self->server->subject = mlm_msg_subject (self->request);
    self->server->content = mlm_msg_content (self->request);

    //  Now find all matching subscribers
    pattern_t *pattern = (pattern_t *) zlist_first (self->server->patterns);
    while (pattern) {
        if (zrex_matches (pattern->rex, mlm_msg_subject (self->request))) {
            client_t *client = (client_t *) zlist_first (pattern->clients);
            while (client) {
                if (client != self)
                    engine_send_event (client, forward_event);
                client = (client_t *) zlist_next (pattern->clients);
            }
        }
        pattern = (pattern_t *) zlist_next (self->server->patterns);
    }
}


//  --------------------------------------------------------------------------
//  get_content_to_forward
//

static void
get_content_to_forward (client_t *self)
{
    zmsg_t *content = zmsg_dup (self->server->content);
    mlm_msg_set_sender  (self->reply, self->server->sender);
    mlm_msg_set_subject (self->reply, self->server->subject);
    mlm_msg_set_content (self->reply, &content);
}


//  ---------------------------------------------------------------------------
//  write_message_to_mailbox
//

static void
write_message_to_mailbox (client_t *self)
{
    mlm_msg_set_status_code (self->reply, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  write_message_to_service
//

static void
write_message_to_service (client_t *self)
{
    mlm_msg_set_status_code (self->reply, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  open_service_worker
//

static void
open_service_worker (client_t *self)
{
    mlm_msg_set_status_code (self->reply, MLM_MSG_NOT_IMPLEMENTED);
    engine_set_exception (self, exception_event);
}


//  ---------------------------------------------------------------------------
//  have_message_confirmation
//

static void
have_message_confirmation (client_t *self)
{
    mlm_msg_set_status_code (self->reply, MLM_MSG_NOT_IMPLEMENTED);
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
    mlm_msg_set_status_code (self->reply, MLM_MSG_COMMAND_INVALID);
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

    mlm_msg_t *message;

    //  Server insists that connection starts properly
    mlm_msg_send_stream_write (reader, "weather");
    message = mlm_msg_recv (reader);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_ERROR);
    assert (mlm_msg_status_code (message) == MLM_MSG_COMMAND_INVALID);
    mlm_msg_destroy (&message);

    //  Now do a stream publish-subscribe test
    zsock_t *writer = zsock_new (ZMQ_DEALER);
    assert (writer);
    zsock_connect (writer, "ipc://@/malamute");
    zsock_set_rcvtimeo (reader, 500);

    //  Open connections from both reader and writer
    mlm_msg_send_connection_open (reader, "reader");
    message = mlm_msg_recv (reader);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_OK);
    mlm_msg_destroy (&message);

    mlm_msg_send_connection_open (writer, "writer");
    message = mlm_msg_recv (writer);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_OK);
    mlm_msg_destroy (&message);

    //  Prepare to write and read a "weather" stream
    mlm_msg_send_stream_write (writer, "weather");
    message = mlm_msg_recv (writer);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_OK);
    mlm_msg_destroy (&message);

    mlm_msg_send_stream_read (reader, "weather", "temp.*");
    message = mlm_msg_recv (reader);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_OK);
    mlm_msg_destroy (&message);

    //  Now send some weather data, with null contents
    mlm_msg_send_stream_publish (writer, "temp.moscow", NULL);
    mlm_msg_send_stream_publish (writer, "rain.moscow", NULL);
    mlm_msg_send_stream_publish (writer, "temp.chicago", NULL);
    mlm_msg_send_stream_publish (writer, "rain.chicago", NULL);
    mlm_msg_send_stream_publish (writer, "temp.london", NULL);
    mlm_msg_send_stream_publish (writer, "rain.london", NULL);

    //  We should receive exactly three deliveries, in order
    message = mlm_msg_recv (reader);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_STREAM_DELIVER);
    assert (streq (mlm_msg_subject (message), "temp.moscow"));
    mlm_msg_destroy (&message);

    message = mlm_msg_recv (reader);
    assert (message);
    assert (mlm_msg_id (message) == MLM_MSG_STREAM_DELIVER);
    assert (streq (mlm_msg_subject (message), "temp.chicago"));
    mlm_msg_destroy (&message);

    message = mlm_msg_recv (reader);
    assert (message);
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

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


//  ---------------------------------------------------------------------------
//  This is a simple stream class

typedef struct {
    zactor_t *actor;            //  Stream engine, zactor
    zsock_t *msgpipe;           //  Socket to send messages to for stream
} stream_t;


//  ---------------------------------------------------------------------------
//  Mailboxes are lists of mailbox messages, hashed by address

typedef struct {
    char *name;                 //  Mailbox name
    client_t *client;           //  Destination client, if any
    zlistx_t *messages;         //  Pending messages
} mailbox_t;


//  ---------------------------------------------------------------------------
//  Services are lists of messages and workers

typedef struct {
    char *name;                 //  Service name
    zlistx_t *offers;           //  Service offers
    zlistx_t *messages;         //  Pending messages
} service_t;


//  ---------------------------------------------------------------------------
//  This holds a single service offer

typedef struct {
    char *pattern;              //  Regular pattern to match on
    zrex_t *rex;                //  Expression, compiled as a zrex object
    client_t *client;           //  Offering client
} offer_t;


//  ---------------------------------------------------------------------------
//  This holds a single mailbox or service message

typedef struct {
    char *sender;               //  Originating client
    char *address;              //  Message address or service
    char *subject;              //  Message subject
    char *tracker;              //  Message tracker
    int64_t expiry;             //  Message expiry time
    zmsg_t *content;            //  Message content
} message_t;


//  ---------------------------------------------------------------------------
//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    
    zhashx_t *streams;          //  Holds stream instances by name
    zhashx_t *mailboxes;        //  Holds mailboxes by address
    zhashx_t *services;         //  Holds services by name

    //  Hold currently dispatching stream message here
    char *sender;               //  Originating client
    char *subject;              //  Message subject
    zmsg_t *content;            //  Message content
    
    //  Hold currently dispatching service message here
    message_t *message;         //  Message structure
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
    mailbox_t *mailbox;         //  Our mailbox, if any
    zlistx_t *readers;          //  All streams we're reading from
};

//  Include the generated server engine
#include "mlm_server_engine.inc"

//  Forward stream traffic to clients

static int
s_forward_stream_traffic (zloop_t *loop, zsock_t *reader, void *argument)
{
    server_t *self = (server_t *) argument;
    zmsg_destroy (&self->content);
    void *client;
    zsock_brecv (reader, "pssm", &client, &self->sender, &self->subject, &self->content);
    engine_send_event ((client_t *) client, stream_message_event);
    return 0;
}


//  Work with stream instance

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
s_stream_new (client_t *client)
{
    stream_t *self = (stream_t *) zmalloc (sizeof (stream_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        if (self->msgpipe) {
            engine_handle_socket (client->server, self->msgpipe, s_forward_stream_traffic);
            self->actor = zactor_new (mlm_stream_simple, backend);
        }
        if (!self->actor)
            s_stream_destroy (&self);
    }
    return self;
}

static stream_t *
s_stream_require (client_t *self, const char *name)
{
    stream_t *stream = (stream_t *) zhashx_lookup (self->server->streams, name);
    if (!stream)
        stream = s_stream_new (self);
    if (stream)
        zhashx_insert (self->server->streams, name, stream);
    return (stream);
}


//  Work with a mailbox/service message instance

static message_t *
s_message_new (const char *sender, const char *address, mlm_msg_t *msg)
{
    message_t *self = (message_t *) zmalloc (sizeof (message_t));
    if (self) {
        self->sender = strdup (sender);
        self->address = strdup (address);
        self->subject = strdup (mlm_msg_subject (msg));
        self->tracker = strdup (mlm_msg_tracker (msg));
        self->content = mlm_msg_get_content (msg);
    }
    return self;
}

static void
s_message_destroy (message_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        message_t *self = *self_p;
        free (self->sender);
        free (self->address);
        free (self->subject);
        free (self->tracker);
        zmsg_destroy (&self->content);
        free (self);
        *self_p = NULL;
    }
}


//  Work with mailbox instance

static void
s_mailbox_destroy (mailbox_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mailbox_t *self = *self_p;
        zlistx_destroy (&self->messages);
        free (self->name);
        free (self);
        *self_p = NULL;
    }
}

static mailbox_t *
s_mailbox_new (const char *name)
{
    mailbox_t *self = (mailbox_t *) zmalloc (sizeof (mailbox_t));
    if (self) {
        self->name = strdup (name);
        if (self->name)
            self->messages = zlistx_new ();
        if (self->messages)
            zlistx_set_destructor (self->messages, (czmq_destructor *) s_message_destroy);
        else
            s_mailbox_destroy (&self);
    }
    return self;
}

static mailbox_t *
s_mailbox_require (client_t *self, const char *name)
{
    mailbox_t *mailbox = (mailbox_t *) zhashx_lookup (self->server->mailboxes, name);
    if (!mailbox)
        mailbox = s_mailbox_new (name);
    if (mailbox)
        zhashx_insert (self->server->mailboxes, name, mailbox);
    return (mailbox);
}

//  Alert mailbox client, if any

static void
s_mailbox_dispatch (mailbox_t *self)
{
    if (self->client && zlistx_size (self->messages))
        engine_send_event (self->client, mailbox_message_event);
}


//  Work with service offer instance

static void
s_offer_destroy (offer_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        offer_t *self = *self_p;
        zrex_destroy (&self->rex);
        free (self->pattern);
        free (self);
        *self_p = NULL;
    }
}

static offer_t *
s_offer_new (client_t *client, const char *pattern)
{
    offer_t *self = (offer_t *) zmalloc (sizeof (offer_t));
    if (self) {
        self->rex = zrex_new (pattern);
        if (self->rex)
            self->pattern = strdup (pattern);
        if (self->pattern)
            self->client = client;
        else
            s_offer_destroy (&self);
    }
    return self;
}


//  Work with service instance

static void
s_service_destroy (service_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        service_t *self = *self_p;
        zlistx_destroy (&self->messages);
        zlistx_destroy (&self->offers);
        free (self->name);
        free (self);
        *self_p = NULL;
    }
}

static service_t *
s_service_new (const char *name)
{
    service_t *self = (service_t *) zmalloc (sizeof (service_t));
    if (self) {
        self->name = strdup (name);
        if (self->name)
            self->messages = zlistx_new ();
        if (self->messages)
            self->offers = zlistx_new ();
        if (self->offers) {
            zlistx_set_destructor (self->messages, (czmq_destructor *) s_message_destroy);
            zlistx_set_destructor (self->offers, (czmq_destructor *) s_offer_destroy);
        }
        else
            s_service_destroy (&self);
    }
    return self;
}

static service_t *
s_service_require (client_t *self, const char *name)
{
    service_t *service = (service_t *) zhashx_lookup (self->server->services, name);
    if (!service)
        service = s_service_new (name);
    if (service)
        zhashx_insert (self->server->services, name, service);
    return (service);
}

void
s_service_dispatch (service_t *self, server_t *server)
{
    //  for each message, check regexp and dispatch if possible
    if (zlistx_size (self->offers)) {
        message_t *message = (message_t *) zlistx_first (self->messages);
        while (message) {
            offer_t *offer = (offer_t *) zlistx_first (self->offers);
            while (offer) {
                if (zrex_matches (offer->rex, message->subject)) {
                    assert (!server->message);
                    server->message = (message_t *) zlistx_detach (
                        self->messages, zlistx_cursor (self->messages));
                    engine_send_event (offer->client, service_message_event);
                    zlistx_move_end (self->offers, zlistx_cursor (self->offers));
                    break;
                }
                offer = (offer_t *) zlistx_next (self->offers);
            }
            message = (message_t *) zlistx_next (self->messages);
        }
    }
}


//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    self->streams = zhashx_new ();
    self->mailboxes = zhashx_new ();
    self->services = zhashx_new ();
    zhashx_set_destructor (self->streams, (czmq_destructor *) s_stream_destroy);
    zhashx_set_destructor (self->mailboxes, (czmq_destructor *) s_mailbox_destroy);
    zhashx_set_destructor (self->services, (czmq_destructor *) s_service_destroy);
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zmsg_destroy (&self->content);
    zhashx_destroy (&self->streams);
    zhashx_destroy (&self->mailboxes);
    zhashx_destroy (&self->services);
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
    self->readers = zlistx_new ();
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    zlistx_destroy (&self->readers);
    free (self->address);
}


//  ---------------------------------------------------------------------------
//  register_new_client
//

static void
register_new_client (client_t *self)
{
    self->address = strdup (mlm_msg_address (self->message));
    //  If client specified an address, lookup or create mailbox
    //  We don't do any access control yet
    if (*self->address) {
        self->mailbox = s_mailbox_require (self, self->address);
        //  No matter, mailbox now belongs to this client
        self->mailbox->client = self;
    }
    mlm_msg_set_status_code (self->message, MLM_MSG_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  store_stream_writer
//

static void
store_stream_writer (client_t *self)
{
    //  A writer talks to a single stream
    self->writer = s_stream_require (self, mlm_msg_stream (self->message));
    if (self->writer)
        mlm_msg_set_status_code (self->message, MLM_MSG_SUCCESS);
    else {
        mlm_msg_set_status_code (self->message, MLM_MSG_INTERNAL_ERROR);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  store_stream_reader
//

static void
store_stream_reader (client_t *self)
{
    stream_t *stream = s_stream_require (self, mlm_msg_stream (self->message));
    if (stream) {
        zlistx_add_end (self->readers, stream);
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
        //  In fact we can't really reply to a STREAM_SEND
        mlm_msg_set_status_code (self->message, MLM_MSG_COMMAND_INVALID);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  write_message_to_mailbox
//

static void
write_message_to_mailbox (client_t *self)
{
    mailbox_t *mailbox = s_mailbox_require (self, mlm_msg_address (self->message));
    assert (mailbox);
    zlistx_add_end (mailbox->messages,
        s_message_new (self->address, mailbox->name, self->message));
    s_mailbox_dispatch (mailbox);
}


//  ---------------------------------------------------------------------------
//  write_message_to_service
//

static void
write_message_to_service (client_t *self)
{
    service_t *service = s_service_require (self, mlm_msg_address (self->message));
    assert (service);
    zlistx_add_end (service->messages,
        s_message_new (self->address, service->name, self->message));
    s_service_dispatch (service, self->server);
}


//  ---------------------------------------------------------------------------
//  store_service_offer
//

static void
store_service_offer (client_t *self)
{
    service_t *service = s_service_require (self, mlm_msg_address (self->message));
    assert (service);
    offer_t *offer = s_offer_new (self, mlm_msg_pattern (self->message));
    assert (offer);
    zlistx_add_end (service->offers, offer);
}


//  ---------------------------------------------------------------------------
//  get_stream_message_to_deliver
//

static void
get_stream_message_to_deliver (client_t *self)
{
    mlm_msg_set_sender  (self->message, self->server->sender);
    mlm_msg_set_subject (self->message, self->server->subject);
    mlm_msg_set_content (self->message, &self->server->content);
}


//  ---------------------------------------------------------------------------
//  get_mailbox_message_to_deliver
//

static void
get_mailbox_message_to_deliver (client_t *self)
{
    assert (self->mailbox);
    assert (zlistx_size (self->mailbox->messages));

    //  Get next message in mailbox queue
    message_t *message = (message_t *) zlistx_detach (self->mailbox->messages, NULL);
    assert (message);
    mlm_msg_set_sender  (self->message, message->sender);
    mlm_msg_set_address (self->message, message->address);
    mlm_msg_set_subject (self->message, message->subject);
    mlm_msg_set_content (self->message, &message->content);
    s_message_destroy (&message);
}


//  ---------------------------------------------------------------------------
//  check_for_mailbox_messages
//

static void
check_for_mailbox_messages (client_t *self)
{
    if (self->mailbox)
        s_mailbox_dispatch (self->mailbox);
}


//  ---------------------------------------------------------------------------
//  get_service_message_to_deliver
//

static void
get_service_message_to_deliver (client_t *self)
{
    //  We pass the message via the server
    message_t *message = self->server->message;
    assert (message);
    mlm_msg_set_sender  (self->message, message->sender);
    mlm_msg_set_address (self->message, message->address);
    mlm_msg_set_subject (self->message, message->subject);
    mlm_msg_set_content (self->message, &message->content);
    s_message_destroy (&self->server->message);
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
    //  Cancel all stream subscriptions
    stream_t *stream = (stream_t *) zlistx_detach (self->readers, NULL);
    while (stream) {
        zsock_send (stream->actor, "sp", "CANCEL", self);
        stream = (stream_t *) zlistx_detach (self->readers, NULL);
    }
    //  Detach from mailbox, if any
    if (self->mailbox)
        self->mailbox->client = NULL;

    //  Cancel all service offerings
    service_t *service = (service_t *) zhashx_first (self->server->services);
    while (service) {
        offer_t *offer = (offer_t *) zlistx_first (service->offers);
        while (offer) {
            if (offer->client == self)
                zlistx_delete (service->offers, zlistx_cursor (service->offers));
            offer = (offer_t *) zlistx_next (service->offers);
        }
        service = (service_t *) zhashx_next (self->server->services);
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
    mlm_msg_set_id (message, MLM_MSG_STREAM_SEND);
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

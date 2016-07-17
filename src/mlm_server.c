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
    char *name;                 //  Stream name
    zactor_t *actor;            //  Stream engine, zactor
    zsock_t *msgpipe;           //  Socket to send messages to for stream
} stream_t;


//  ---------------------------------------------------------------------------
//  Services are lists of messages and workers

typedef struct {
    char *name;                 //  Service name
    zlistx_t *offers;           //  Service offers
    zlistx_t *queue;            //  Pending messages
} service_t;


//  ---------------------------------------------------------------------------
//  This holds a single service offer

typedef struct {
    char *pattern;              //  Regular pattern to match on
    zrex_t *rex;                //  Expression, compiled as a zrex object
    client_t *client;           //  Offering client
} offer_t;


//  ---------------------------------------------------------------------------
//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    int mailbox_size_limit;     //  Maximum size of mailbox
    zactor_t *mailbox;          //  Mailbox engine
    zhashx_t *streams;          //  Holds stream instances by name
    zhashx_t *services;         //  Holds services by name
    zhashx_t *clients;          //  Holds clients by address
    int service_queue_size_limit;   //  Limit on queue size (per service)
};


//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    mlm_proto_t *message;       //  Message in and out

    //  These properties are specific for this application
    char *address;              //  Address of this client
    stream_t *writer;           //  Stream we're writing to, if any
    zlistx_t *readers;          //  All streams we're reading from

    //  Hold currently dispatching service message here
    mlm_msg_t *msg;             //  Message structure
};

//  Include the generated server engine
#include "mlm_server_engine.inc"

//  Forward stream traffic to clients

static int
s_forward_stream_traffic (zloop_t *loop, zsock_t *reader, void *argument)
{
    client_t *client;
    mlm_msg_t *msg;
    zsock_brecv (reader, "pp", &client, &msg);
    assert (client);
    assert (!client->msg);
    client->msg = msg;
    engine_send_event (client, stream_message_event);
    assert (!client->msg);
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
        free (self->name);
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
        self->name = strdup (name);
        if (self->name)
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
        stream = s_stream_new (self, name);
    if (stream)
        zhashx_insert (self->server->streams, name, stream);
    return stream;
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
        zlistx_destroy (&self->queue);
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
            self->queue = zlistx_new ();
        if (self->queue)
            self->offers = zlistx_new ();
        if (self->offers) {
            zlistx_set_destructor (self->queue, (czmq_destructor *) mlm_msg_destroy);
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
    return service;
}

static bool
s_service_dispatch_message (service_t *self, mlm_msg_t *message)
{
    offer_t *offer = (offer_t *) zlistx_first (self->offers);
    while (offer) {
        if (zrex_matches (offer->rex, mlm_msg_subject (message))) {
            client_t *target = offer->client;
            assert (target);
            assert (!target->msg);
            target->msg = message;
            engine_send_event (target, service_message_event);
            zlistx_move_end (self->offers, zlistx_cursor (self->offers));
            return true;
        }
        offer = (offer_t *) zlistx_next (self->offers);
    }

    return false;
}

static void
s_service_dispatch (service_t *self)
{
    //  for each message, check regexp and dispatch if possible
    if (zlistx_size (self->offers)) {
        mlm_msg_t *message = (mlm_msg_t *) zlistx_first (self->queue);
        while (message) {
            if (s_service_dispatch_message (self, message))
                zlistx_detach (self->queue, zlistx_cursor (self->queue));
            message = (mlm_msg_t *) zlistx_next (self->queue);
        }
    }
}


//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    self->mailbox_size_limit = -1;
    self->mailbox = zactor_new (mlm_mailbox_bounded, &self->mailbox_size_limit);
    assert (self->mailbox);
    self->streams = zhashx_new ();
    assert (self->streams);
    self->services = zhashx_new ();
    assert (self->services);
    self->clients = zhashx_new ();
    assert (self->clients);
    zhashx_set_destructor (self->streams, (czmq_destructor *) s_stream_destroy);
    zhashx_set_destructor (self->services, (czmq_destructor *) s_service_destroy);
    self->service_queue_size_limit = -1;
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    zactor_destroy (&self->mailbox);
    zhashx_destroy (&self->streams);
    zhashx_destroy (&self->services);
    zhashx_destroy (&self->clients);
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    return NULL;
}

//  Apply new configuration.

static void
server_configuration (server_t *self, zconfig_t *config)
{
    self->service_queue_size_limit = atoi (
        zconfig_get (config, "mlm_server/service/queue/size-limit", "-1"));
    self->mailbox_size_limit = atoi (
        zconfig_get (config, "mlm_server/mailbox/size-limit", "-1"));

    // Inform mailbox about new size limit
    zsock_send (self->mailbox, "si",
        "MAILBOX-SIZE-LIMIT", self->mailbox_size_limit);
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
    self->address = strdup (mlm_proto_address (self->message));
    if (*self->address) {
        //  If there's an existing client with this address, expire it
        //  The alternative would be to reject new clients with the same address
        zstr_free (&self->address);
        self->address = strdup (mlm_proto_address (self->message));
        client_t *existing = (client_t *) zhashx_lookup (
            self->server->clients, self->address);
        if (existing)
            engine_send_event (existing, expired_event);

        //  In any case, we now own this address
        zhashx_update (self->server->clients, self->address, self);
    }
    if (*self->address)
        zsys_info ("client address='%s' - registering", self->address);
    mlm_proto_set_status_code (self->message, MLM_PROTO_SUCCESS);
}


//  ---------------------------------------------------------------------------
//  store_stream_writer
//

static void
store_stream_writer (client_t *self)
{
    //  A writer talks to a single stream
    self->writer = s_stream_require (self, mlm_proto_stream (self->message));
    if (self->writer)
        mlm_proto_set_status_code (self->message, MLM_PROTO_SUCCESS);
    else {
        engine_set_exception (self, exception_event);
        zsys_warning ("writer trying to talk to multiple streams");
    }
}


//  ---------------------------------------------------------------------------
//  store_stream_reader
//

static void
store_stream_reader (client_t *self)
{
    stream_t *stream = s_stream_require (self, mlm_proto_stream (self->message));
    if (stream) {
        zlistx_add_end (self->readers, stream);
        zsock_send (stream->actor, "sps", "COMPILE", self, mlm_proto_pattern (self->message));
        mlm_proto_set_status_code (self->message, MLM_PROTO_SUCCESS);
    }
    else {
        engine_set_exception (self, exception_event);
        zsys_warning ("reader trying to talk to multiple streams");
    }
}


//  ---------------------------------------------------------------------------
//  write_message_to_stream
//

static void
write_message_to_stream (client_t *self)
{
    if (self->writer) {
        mlm_msg_t *msg = mlm_msg_new (
            self->address,
            self->writer->name,
            mlm_proto_subject (self->message),
            NULL,
            mlm_proto_timeout (self->message),
            mlm_proto_get_content (self->message));
        zsock_bsend (self->writer->msgpipe, "pp", self, msg);
    }
    else {
        engine_set_exception (self, exception_event);
        zsys_warning ("client attempted to send without writer");
    }
}


//  ---------------------------------------------------------------------------
//  write_message_to_mailbox
//

static void
write_message_to_mailbox (client_t *self)
{
    mlm_msg_t *msg = mlm_msg_new (
        self->address,
        mlm_proto_address (self->message),
        mlm_proto_subject (self->message),
        mlm_proto_tracker (self->message),
        mlm_proto_timeout (self->message),
        mlm_proto_get_content (self->message));

    //  Try to dispatch to client immediately, if it's connected
    client_t *target = (client_t *) zhashx_lookup (
        self->server->clients, mlm_proto_address (self->message));

    if (target) {
        assert (!target->msg);
        target->msg = msg;
        engine_send_event (target, mailbox_message_event);
    }
    else
        //  Else store in the eponymous mailbox
        zsock_send (self->server->mailbox, "ssp", "STORE",
                    mlm_proto_address (self->message), msg);
}


//  ---------------------------------------------------------------------------
//  write_message_to_service
//

static void
write_message_to_service (client_t *self)
{
    mlm_msg_t *msg = mlm_msg_new (
        self->address,
        mlm_proto_address (self->message),
        mlm_proto_subject (self->message),
        mlm_proto_tracker (self->message),
        mlm_proto_timeout (self->message),
        mlm_proto_get_content (self->message));

    service_t *service = s_service_require (self, mlm_proto_address (self->message));
    assert (service);
    if (!s_service_dispatch_message (service, msg)) {
        const int queue_size_limit = self->server->service_queue_size_limit;
        if (queue_size_limit == -1
        ||  zlistx_size (service->queue) < queue_size_limit)
            zlistx_add_end (service->queue, msg);
        else
            mlm_msg_destroy (&msg);
    }
}


//  ---------------------------------------------------------------------------
//  store_service_offer
//

static void
store_service_offer (client_t *self)
{
    service_t *service = s_service_require (self, mlm_proto_address (self->message));
    assert (service);
    offer_t *offer = s_offer_new (self, mlm_proto_pattern (self->message));
    assert (offer);
    zlistx_add_end (service->offers, offer);
}


//  ---------------------------------------------------------------------------
//  dispatch_the_service
//

static void
dispatch_the_service (client_t *self)
{
    service_t *service = s_service_require (self, mlm_proto_address (self->message));
    assert (service);
    s_service_dispatch (service);
}


//  ---------------------------------------------------------------------------
//  check_for_mailbox_messages
//

static void
check_for_mailbox_messages (client_t *self)
{
    if (*self->address) {
        zsock_send (self->server->mailbox, "ss", "QUERY", self->address);
        //  TODO: break into async reply back to server with client
        //  ID number that server can route to correct client, if it
        //  exists... we need some kind of internal async messaging
        //  to cover all different cases
        //  - engine_send_event (client, event, args)
        //  - stream return
        //  - mailbox to client
        //  - service request to client
        //  -> can each client have a DEALER socket?
        //  -> lookup/route on client unique name?
        //  -> server/client path? centralized for process?
        //  -> do we need lookup, or can we use ROUTER sockets?
        //  -> perhaps using ID?
        //  <<general model for internal messaging>>
        //  requirements, route by name, detect lost route?
        //  credit based flow control
        //  can send mlm_msg_t's all over the place
        zsock_recv (self->server->mailbox, "p", &self->msg);
        if (self->msg)
            engine_set_next_event (self, mailbox_message_event);
    }
}


//  ---------------------------------------------------------------------------
//  get_message_to_deliver
//

static void
get_message_to_deliver (client_t *self)
{
    mlm_msg_set_proto (self->msg, self->message);
    mlm_msg_unlink (&self->msg);
}


//  ---------------------------------------------------------------------------
//  have_message_confirmation
//

static void
have_message_confirmation (client_t *self)
{
    engine_set_exception (self, exception_event);
    zsys_warning ("message confirmations are not implemented");
}


//  ---------------------------------------------------------------------------
//  credit_the_client
//

static void
credit_the_client (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  client_expired
//

static void
client_expired (client_t *self)
{
    if (*self->address)
        zsys_info ("client address='%s' - expired", self->address);
}


//  ---------------------------------------------------------------------------
//  client_closed_connection
//

static void
client_closed_connection (client_t *self)
{
    if (*self->address)
        zsys_info ("client address='%s' - closed connection", self->address);
}


//  ---------------------------------------------------------------------------
//  client_had_exception
//

static void
client_had_exception (client_t *self)
{
    zsys_info ("client address='%s' - unimplemented command", self->address);
}


//  ---------------------------------------------------------------------------
//  deregister_the_client
//

static void
deregister_the_client (client_t *self)
{
    if (*self->address)
        zsys_info ("client address='%s' - de-registering", self->address);

    //  Cancel all stream subscriptions
    stream_t *stream = (stream_t *) zlistx_detach (self->readers, NULL);
    while (stream) {
        zsock_send (stream->actor, "sp", "CANCEL", self);
        stream = (stream_t *) zlistx_detach (self->readers, NULL);
    }
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
    if (*self->address)
        zhashx_delete (self->server->clients, self->address);
    mlm_proto_set_status_code (self->message, MLM_PROTO_SUCCESS);
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
//  signal_operation_failed
//

static void
signal_operation_failed (client_t *self)
{
    mlm_proto_set_status_code (self->message, MLM_PROTO_FAILED);
}


//  ---------------------------------------------------------------------------
//  signal_command_invalid
//

static void
signal_command_invalid (client_t *self)
{
    zsys_info ("client address='%s' - invalid command", self->address);
    mlm_proto_set_status_code (self->message, MLM_PROTO_COMMAND_INVALID);
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
    zstr_sendx (server, "BIND", "tcp://127.0.0.1:9999", NULL);

    zsock_t *reader = zsock_new (ZMQ_DEALER);
    assert (reader);
    zsock_connect (reader, "tcp://127.0.0.1:9999");
    zsock_set_rcvtimeo (reader, 500);

    mlm_proto_t *proto = mlm_proto_new ();

    //  Server insists that connection starts properly
    mlm_proto_set_id (proto, MLM_PROTO_STREAM_WRITE);
    mlm_proto_send (proto, reader);
    mlm_proto_recv (proto, reader);
    assert (mlm_proto_id (proto) == MLM_PROTO_ERROR);
    assert (mlm_proto_status_code (proto) == MLM_PROTO_COMMAND_INVALID);

    //  Now do a stream publish-subscribe test
    zsock_t *writer = zsock_new (ZMQ_DEALER);
    assert (writer);
    zsock_connect (writer, "tcp://127.0.0.1:9999");
    zsock_set_rcvtimeo (reader, 500);

    //  Open connections from both reader and writer
    mlm_proto_set_id (proto, MLM_PROTO_CONNECTION_OPEN);
    mlm_proto_send (proto, reader);
    mlm_proto_recv (proto, reader);
    assert (mlm_proto_id (proto) == MLM_PROTO_OK);

    mlm_proto_set_id (proto, MLM_PROTO_CONNECTION_OPEN);
    mlm_proto_send (proto, writer);
    mlm_proto_recv (proto, writer);
    assert (mlm_proto_id (proto) == MLM_PROTO_OK);

    //  Prepare to write and read a "weather" stream
    mlm_proto_set_id (proto, MLM_PROTO_STREAM_WRITE);
    mlm_proto_set_stream (proto, "weather");
    mlm_proto_send (proto, writer);
    mlm_proto_recv (proto, writer);
    assert (mlm_proto_id (proto) == MLM_PROTO_OK);

    mlm_proto_set_id (proto, MLM_PROTO_STREAM_READ);
    mlm_proto_set_pattern (proto, "temp.*");
    mlm_proto_send (proto, reader);
    mlm_proto_recv (proto, reader);
    assert (mlm_proto_id (proto) == MLM_PROTO_OK);

    //  Now send some weather data, with null contents
    mlm_proto_set_id (proto, MLM_PROTO_STREAM_SEND);
    mlm_proto_set_subject (proto, "temp.moscow");
    mlm_proto_send (proto, writer);
    mlm_proto_set_subject (proto, "rain.moscow");
    mlm_proto_send (proto, writer);
    mlm_proto_set_subject (proto, "temp.chicago");
    mlm_proto_send (proto, writer);
    mlm_proto_set_subject (proto, "rain.chicago");
    mlm_proto_send (proto, writer);
    mlm_proto_set_subject (proto, "temp.london");
    mlm_proto_send (proto, writer);
    mlm_proto_set_subject (proto, "rain.london");
    mlm_proto_send (proto, writer);

    //  We should receive exactly three deliveries, in order
    mlm_proto_recv (proto, reader);
    assert (mlm_proto_id (proto) == MLM_PROTO_STREAM_DELIVER);
    assert (streq (mlm_proto_subject (proto), "temp.moscow"));

    mlm_proto_recv (proto, reader);
    assert (mlm_proto_id (proto) == MLM_PROTO_STREAM_DELIVER);
    assert (streq (mlm_proto_subject (proto), "temp.chicago"));

    mlm_proto_recv (proto, reader);
    assert (mlm_proto_id (proto) == MLM_PROTO_STREAM_DELIVER);
    assert (streq (mlm_proto_subject (proto), "temp.london"));

    mlm_proto_destroy (&proto);

    //  Finished, we can clean up
    zsock_destroy (&writer);
    zsock_destroy (&reader);
    zactor_destroy (&server);

    //  @end
    printf ("OK\n");
}

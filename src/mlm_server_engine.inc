/*  =========================================================================
    mlm_server_engine - mlm_server engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mlm_server.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#define ZPROTO_UNUSED(object) (void)object

//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    connected_state = 2,
    defaults_state = 3,
    //  Internal state to mark zombie clients
    zombie_state_ = 4
} state_t;

typedef enum {
    NULL_event = 0,
    terminate_event = 1,
    connection_open_event = 2,
    stream_write_event = 3,
    stream_read_event = 4,
    stream_cancel_event = 5,
    stream_send_event = 6,
    mailbox_send_event = 7,
    service_send_event = 8,
    service_offer_event = 9,
    confirm_event = 10,
    credit_event = 11,
    connection_ping_event = 12,
    connection_close_event = 13,
    stream_message_event = 14,
    mailbox_message_event = 15,
    service_message_event = 16,
    expired_event = 17,
    exception_event = 18
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connected",
    "defaults"
};

static char *
s_event_name [] = {
    "(NONE)",
    "terminate",
    "CONNECTION_OPEN",
    "STREAM_WRITE",
    "STREAM_READ",
    "STREAM_CANCEL",
    "STREAM_SEND",
    "MAILBOX_SEND",
    "SERVICE_SEND",
    "SERVICE_OFFER",
    "CONFIRM",
    "CREDIT",
    "CONNECTION_PING",
    "CONNECTION_CLOSE",
    "stream_message",
    "mailbox_message",
    "service_message",
    "expired",
    "exception"
};

//  ---------------------------------------------------------------------------
//  Context for the whole server task. This embeds the application-level
//  server context at its start (the entire structure, not a reference),
//  so we can cast a pointer between server_t and s_server_t arbitrarily.

typedef struct {
    server_t server;            //  Application-level server context
    zsock_t *pipe;              //  Socket to back to caller API
    zsock_t *router;            //  Socket to talk to clients
    int port;                   //  Server port bound to
    zloop_t *loop;              //  Reactor for server sockets
    mlm_proto_t *message;       //  Message received or sent
    zhash_t *clients;           //  Clients we're connected to
    zhash_t *zombies;           //  Zombie clients waiting to be free()d
    zconfig_t *config;          //  Configuration tree
    uint client_id;             //  Client identifier counter
    size_t timeout;             //  Default client expiry timeout
    bool verbose;               //  Verbose logging enabled?
    char *log_prefix;           //  Default log prefix
} s_server_t;


//  ---------------------------------------------------------------------------
//  Context for each connected client. This embeds the application-level
//  client context at its start (the entire structure, not a reference),
//  so we can cast a pointer between client_t and s_client_t arbitrarily.

typedef struct {
    client_t client;            //  Application-level client context
    s_server_t *server;         //  Parent server context
    size_t refcount;            //  Optional reference counter
    char *hashkey;              //  Key into server->clients hash
    zframe_t *routing_id;       //  Routing_id back to client
    uint unique_id;             //  Client identifier in server
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int wakeup;                 //  zloop timer for client alarms
    void *ticket;               //  zloop ticket for client timeouts
    event_t wakeup_event;       //  Wake up with this event
    char log_prefix [41];       //  Log prefix string
} s_client_t;

static int
    server_initialize (server_t *self);
static void
    server_terminate (server_t *self);
static zmsg_t *
    server_method (server_t *self, const char *method, zmsg_t *msg);
static void
    server_configuration (server_t *self, zconfig_t *config);
static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_server_config_global (s_server_t *server);
static void
    s_client_execute (s_client_t *client, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static int
    s_client_handle_ticket (zloop_t *loop, int timer_id, void *argument);
static void
    s_client_finalize (s_client_t **self_p);
static void
    register_new_client (client_t *self);
static void
    check_for_mailbox_messages (client_t *self);
static void
    signal_command_invalid (client_t *self);
static void
    store_stream_writer (client_t *self);
static void
    store_stream_reader (client_t *self);
static void
    cancel_stream_reader (client_t *self);
static void
    write_message_to_stream (client_t *self);
static void
    write_message_to_mailbox (client_t *self);
static void
    write_message_to_service (client_t *self);
static void
    store_service_offer (client_t *self);
static void
    dispatch_the_service (client_t *self);
static void
    have_message_confirmation (client_t *self);
static void
    credit_the_client (client_t *self);
static void
    client_closed_connection (client_t *self);
static void
    deregister_the_client (client_t *self);
static void
    get_message_to_deliver (client_t *self);
static void
    client_expired (client_t *self);
static void
    signal_operation_failed (client_t *self);
static void
    client_had_exception (client_t *self);

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Optional reference counting. The engine itself does not need this, but if
//  the application wishes to delay the final destruction of a client, it
//  can do so by using these functions
static void engine_client_get (client_t *client)
{
    if (!client)
        return;
    s_client_t *self = (s_client_t *) client;
    ++self->refcount;
}

//  Relese the application reference to client
static void engine_client_put (client_t *client)
{
    if (!client)
        return;
    s_client_t *self = (s_client_t *) client;
    if (--self->refcount == 0 && self->state == zombie_state_) {
        zhash_delete (self->server->zombies, self->hashkey);
        s_client_finalize (&self);
    }
}

#define engine_client_is_valid(c) ((c) && ((s_client_t *)(c))->state != zombie_state_)

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  router socket and treat that as the event.

static void
engine_set_next_event (client_t *client, event_t event)
{
    if (engine_client_is_valid (client)) {
        s_client_t *self = (s_client_t *) client;
        self->next_event = event;
    }
}

//  Raise an exception with 'event', halting any actions in progress.
//  Continues execution of actions defined for the exception event.

static void
engine_set_exception (client_t *client, event_t event)
{
    if (engine_client_is_valid (client)) {
        s_client_t *self = (s_client_t *) client;
        self->exception = event;
    }
}

//  Set wakeup alarm after 'delay' msecs. The next state should
//  handle the wakeup event. The alarm is cancelled on any other
//  event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (engine_client_is_valid (client)) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup) {
            zloop_timer_end (self->server->loop, self->wakeup);
            self->wakeup = 0;
        }
        self->wakeup = zloop_timer (
            self->server->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Execute 'event' on specified client. Use this to send events to
//  other clients. Cancels any wakeup alarm on that client.

static void
engine_send_event (client_t *client, event_t event)
{
    if (engine_client_is_valid (client)) {
        s_client_t *self = (s_client_t *) client;
        s_client_execute (self, event);
    }
}

//  Execute 'event' on all clients known to the server. If you pass a
//  client argument, that client will not receive the broadcast. If you
//  want to pass any arguments, store them in the server context.

static void
engine_broadcast_event (server_t *server, client_t *client, event_t event)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        zlist_t *keys = zhash_keys (self->clients);
        char *key = (char *) zlist_first (keys);
        while (key) {
            s_client_t *target = (s_client_t *) zhash_lookup (self->clients, key);
            if (target != (s_client_t *) client)
                s_client_execute (target, event);
            key = (char *) zlist_next (keys);
        }
        zlist_destroy (&keys);
    }
}

//  Poll actor or zsock for activity, invoke handler on any received
//  message. Handler must be a CZMQ zloop_fn function; receives server
//  as arg.

static void
engine_handle_socket (server_t *server, void *sock, zloop_reader_fn handler)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        //  Resolve zactor_t -> zsock_t
        if (zactor_is (sock))
            sock = zactor_sock ((zactor_t *) sock);
        else
            assert (zsock_is (sock));
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, (zsock_t *) sock, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, (zsock_t *) sock);
        }
        else
            zloop_reader_end (self->loop, (zsock_t *) sock);
    }
}

//  Register monitor function that will be called at regular intervals
//  by the server engine. Returns an identifier that can be used to cancel it.

static int
engine_set_monitor (server_t *server, size_t interval, zloop_timer_fn monitor)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        int rc = zloop_timer (self->loop, interval, 0, monitor, self);
        assert (rc >= 0);
        return rc;
    }

    return -1;
}

//  Cancel the monitor function with the given identifier.
static void
engine_cancel_monitor (server_t *server, int identifier)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        int rc = zloop_timer_end (self->loop, identifier);
        assert (rc >= 0);
    }
}

//  Set log file prefix; this string will be added to log data, to make
//  log data more searchable. The string is truncated to ~20 chars.

static void
engine_set_log_prefix (client_t *client, const char *string)
{
    if (engine_client_is_valid (client)) {
        s_client_t *self = (s_client_t *) client;
        snprintf (self->log_prefix, sizeof (self->log_prefix),
            "%6d:%-33s", self->unique_id, string);
    }
}

//  Set a configuration value in the server's configuration tree. The
//  properties this engine uses are: server/verbose, server/timeout, and
//  server/background. You can also configure other abitrary properties.

static void
engine_configure (server_t *server, const char *path, const char *value)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        zconfig_put (self->config, path, value);
        s_server_config_global (self);
    }
}

//  Return true if server is running in verbose mode, else return false.

static bool
engine_verbose (server_t *server)
{
    if (server) {
        s_server_t *self = (s_server_t *) server;
        return self->verbose;
    }
    return false;
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_client_get (NULL);
    engine_client_put (NULL);
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_send_event (NULL, NULL_event);
    engine_broadcast_event (NULL, NULL, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
    engine_set_monitor (NULL, 0, NULL);
    engine_cancel_monitor (NULL, 0);
    engine_set_log_prefix (NULL, NULL);
    engine_configure (NULL, NULL, NULL);
    engine_verbose (NULL);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (mlm_proto_t *message)
{
    assert (message);
    switch (mlm_proto_id (message)) {
        case MLM_PROTO_CONNECTION_OPEN:
            return connection_open_event;
            break;
        case MLM_PROTO_CONNECTION_PING:
            return connection_ping_event;
            break;
        case MLM_PROTO_CONNECTION_CLOSE:
            return connection_close_event;
            break;
        case MLM_PROTO_STREAM_WRITE:
            return stream_write_event;
            break;
        case MLM_PROTO_STREAM_READ:
            return stream_read_event;
            break;
        case MLM_PROTO_STREAM_SEND:
            return stream_send_event;
            break;
        case MLM_PROTO_MAILBOX_SEND:
            return mailbox_send_event;
            break;
        case MLM_PROTO_SERVICE_SEND:
            return service_send_event;
            break;
        case MLM_PROTO_SERVICE_OFFER:
            return service_offer_event;
            break;
        case MLM_PROTO_CREDIT:
            return credit_event;
            break;
        case MLM_PROTO_CONFIRM:
            return confirm_event;
            break;
        case MLM_PROTO_STREAM_CANCEL:
            return stream_cancel_event;
            break;
        default:
            //  Invalid mlm_proto_t
            return terminate_event;
    }
}


//  ---------------------------------------------------------------------------
//  Client methods

static s_client_t *
s_client_new (s_server_t *server, zframe_t *routing_id)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    assert (self);
    assert ((s_client_t *) &self->client == self);

    self->server = server;
    self->hashkey = zframe_strhex (routing_id);
    self->routing_id = zframe_dup (routing_id);
    self->unique_id = server->client_id++;
    engine_set_log_prefix (&self->client, server->log_prefix);

    self->client.server = (server_t *) server;
    self->client.message = server->message;
    //  propagate client identifier
    self->client.unique_id = self->unique_id;

    //  If expiry timers are being used, create client ticket
    if (server->timeout)
        self->ticket = zloop_ticket (server->loop, s_client_handle_ticket, self);
    //  Give application chance to initialize and set next event
    self->state = start_state;
    self->event = NULL_event;
    client_initialize (&self->client);
    return self;
}

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        if (self->wakeup)
            zloop_timer_end (self->server->loop, self->wakeup);
        if (self->ticket)
            zloop_ticket_delete (self->server->loop, self->ticket);
        zframe_destroy (&self->routing_id);
        //  Provide visual clue if application misuses client reference
        engine_set_log_prefix (&self->client, "*** TERMINATED ***");
        if (!self->refcount) {
            s_client_finalize (self_p);
        } else {
            zhash_insert (self->server->zombies, self->hashkey, self);
            self->state = zombie_state_;
        }
    }
}

static void
s_client_finalize (s_client_t **self_p)
{
    s_client_t *self = *self_p;
    free (self->hashkey);
    client_terminate (&self->client);
    free (self);
    *self_p = NULL;
}

//  Callback when we remove client from 'clients' hash table
static void
s_client_free (void *argument)
{
    s_client_t *client = (s_client_t *) argument;
    s_client_destroy (&client);
}


//  Execute state machine as long as we have events

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup) {
        zloop_timer_end (self->server->loop, self->wakeup);
        self->wakeup = 0;
    }
    while (self->next_event > 0) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->server->verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == connection_open_event) {
                    if (!self->exception) {
                        //  register new client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ register new client", self->log_prefix);
                        register_new_client (&self->client);
                    }
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  check for mailbox messages
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ check for mailbox messages", self->log_prefix);
                        check_for_mailbox_messages (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == connection_close_event) {
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  client closed connection
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client closed connection", self->log_prefix);
                        client_closed_connection (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == stream_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send STREAM_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_STREAM_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == mailbox_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send MAILBOX_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send MAILBOX_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_MAILBOX_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  check for mailbox messages
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ check for mailbox messages", self->log_prefix);
                        check_for_mailbox_messages (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == service_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send SERVICE_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send SERVICE_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_SERVICE_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  client expired
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client expired", self->log_prefix);
                        client_expired (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == exception_event) {
                    if (!self->exception) {
                        //  signal operation failed
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ signal operation failed", self->log_prefix);
                        signal_operation_failed (&self->client);
                    }
                    if (!self->exception) {
                        //  send ERROR
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ERROR",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_ERROR);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  client had exception
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client had exception", self->log_prefix);
                        client_had_exception (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  signal command invalid
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ signal command invalid", self->log_prefix);
                        signal_command_invalid (&self->client);
                    }
                    if (!self->exception) {
                        //  send ERROR
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ERROR",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_ERROR);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                }
                break;

            case connected_state:
                if (self->event == stream_write_event) {
                    if (!self->exception) {
                        //  store stream writer
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ store stream writer", self->log_prefix);
                        store_stream_writer (&self->client);
                    }
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == stream_read_event) {
                    if (!self->exception) {
                        //  store stream reader
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ store stream reader", self->log_prefix);
                        store_stream_reader (&self->client);
                    }
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == stream_cancel_event) {
                    if (!self->exception) {
                        //  cancel stream reader
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ cancel stream reader", self->log_prefix);
                        cancel_stream_reader (&self->client);
                    }
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == stream_send_event) {
                    if (!self->exception) {
                        //  write message to stream
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ write message to stream", self->log_prefix);
                        write_message_to_stream (&self->client);
                    }
                }
                else
                if (self->event == mailbox_send_event) {
                    if (!self->exception) {
                        //  write message to mailbox
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ write message to mailbox", self->log_prefix);
                        write_message_to_mailbox (&self->client);
                    }
                }
                else
                if (self->event == service_send_event) {
                    if (!self->exception) {
                        //  write message to service
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ write message to service", self->log_prefix);
                        write_message_to_service (&self->client);
                    }
                }
                else
                if (self->event == service_offer_event) {
                    if (!self->exception) {
                        //  store service offer
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ store service offer", self->log_prefix);
                        store_service_offer (&self->client);
                    }
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  dispatch the service
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ dispatch the service", self->log_prefix);
                        dispatch_the_service (&self->client);
                    }
                }
                else
                if (self->event == confirm_event) {
                    if (!self->exception) {
                        //  have message confirmation
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ have message confirmation", self->log_prefix);
                        have_message_confirmation (&self->client);
                    }
                }
                else
                if (self->event == credit_event) {
                    if (!self->exception) {
                        //  credit the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ credit the client", self->log_prefix);
                        credit_the_client (&self->client);
                    }
                }
                else
                if (self->event == connection_ping_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PONG
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PONG",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_CONNECTION_PONG);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                }
                else
                if (self->event == connection_close_event) {
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  client closed connection
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client closed connection", self->log_prefix);
                        client_closed_connection (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == stream_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send STREAM_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_STREAM_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == mailbox_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send MAILBOX_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send MAILBOX_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_MAILBOX_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  check for mailbox messages
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ check for mailbox messages", self->log_prefix);
                        check_for_mailbox_messages (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == service_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send SERVICE_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send SERVICE_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_SERVICE_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  client expired
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client expired", self->log_prefix);
                        client_expired (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == exception_event) {
                    if (!self->exception) {
                        //  signal operation failed
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ signal operation failed", self->log_prefix);
                        signal_operation_failed (&self->client);
                    }
                    if (!self->exception) {
                        //  send ERROR
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ERROR",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_ERROR);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  client had exception
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client had exception", self->log_prefix);
                        client_had_exception (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  signal command invalid
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ signal command invalid", self->log_prefix);
                        signal_command_invalid (&self->client);
                    }
                    if (!self->exception) {
                        //  send ERROR
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ERROR",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_ERROR);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                break;

            case defaults_state:
                if (self->event == connection_close_event) {
                    if (!self->exception) {
                        //  send OK
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send OK",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_OK);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  client closed connection
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client closed connection", self->log_prefix);
                        client_closed_connection (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == stream_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send STREAM_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_STREAM_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == mailbox_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send MAILBOX_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send MAILBOX_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_MAILBOX_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  check for mailbox messages
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ check for mailbox messages", self->log_prefix);
                        check_for_mailbox_messages (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == service_message_event) {
                    if (!self->exception) {
                        //  get message to deliver
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ get message to deliver", self->log_prefix);
                        get_message_to_deliver (&self->client);
                    }
                    if (!self->exception) {
                        //  send SERVICE_DELIVER
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send SERVICE_DELIVER",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_SERVICE_DELIVER);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  client expired
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client expired", self->log_prefix);
                        client_expired (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else
                if (self->event == exception_event) {
                    if (!self->exception) {
                        //  signal operation failed
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ signal operation failed", self->log_prefix);
                        signal_operation_failed (&self->client);
                    }
                    if (!self->exception) {
                        //  send ERROR
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ERROR",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_ERROR);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  client had exception
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ client had exception", self->log_prefix);
                        client_had_exception (&self->client);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  signal command invalid
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ signal command invalid", self->log_prefix);
                        signal_command_invalid (&self->client);
                    }
                    if (!self->exception) {
                        //  send ERROR
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ send ERROR",
                                self->log_prefix);
                        mlm_proto_set_id (self->server->message, MLM_PROTO_ERROR);
                        mlm_proto_set_routing_id (self->server->message, self->routing_id);
                        mlm_proto_send (self->server->message, self->server->router);
                    }
                    if (!self->exception) {
                        //  deregister the client
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ deregister the client", self->log_prefix);
                        deregister_the_client (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->server->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->next_event = terminate_event;
                    }
                }
                break;
            case zombie_state_:
                zsys_error ("%p: zombie client context used", self);
                return;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->server->verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);

            self->next_event = self->exception;
        }
        if (self->next_event == terminate_event) {
            //  Automatically calls s_client_destroy
            zhash_delete (self->server->clients, self->hashkey);
            break;
        }
        else
        if (self->server->verbose)
            zsys_debug ("%s:         > %s",
                self->log_prefix, s_state_name [self->state]);
    }
}

//  zloop callback when client ticket expires

static int
s_client_handle_ticket (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    self->ticket = NULL;        //  Ticket is now dead
    s_client_execute (self, expired_event);
    return 0;
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(timer_id);
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Server methods

static void
s_server_config_global (s_server_t *self)
{
    //  Built-in server configuration options
    //
    //  If we didn't already set verbose, check if the config tree wants it
    if (!self->verbose
    && atoi (zconfig_get (self->config, "server/verbose", "0")))
        self->verbose = true;

    //  Default client timeout is 60 seconds
    self->timeout = atoi (
        zconfig_get (self->config, "server/timeout", "60000"));
    zloop_set_ticket_delay (self->loop, self->timeout);

    //  Do we want to run server in the background?
    int background = atoi (
        zconfig_get (self->config, "server/background", "0"));
    if (!background)
        zsys_set_logstream (stdout);
}

static s_server_t *
s_server_new (zsock_t *pipe)
{
    s_server_t *self = (s_server_t *) zmalloc (sizeof (s_server_t));
    assert (self);
    assert ((s_server_t *) &self->server == self);

    self->pipe = pipe;
    self->router = zsock_new (ZMQ_ROUTER);
    assert (self->router);
    //  By default the socket will discard outgoing messages above the
    //  HWM of 1,000. This isn't helpful for high-volume streaming. We
    //  will use a unbounded queue here. If applications need to guard
    //  against queue overflow, they should use a credit-based flow
    //  control scheme.
    zsock_set_unbounded (self->router);
    self->message = mlm_proto_new ();
    self->clients = zhash_new ();
    self->zombies = zhash_new ();
    self->config = zconfig_new ("root", NULL);
    self->loop = zloop_new ();
    srandom ((unsigned int) zclock_time ());
    self->client_id = randof (1000);
    s_server_config_global (self);

    //  Initialize application server context
    self->server.pipe = self->pipe;
    self->server.config = self->config;
    server_initialize (&self->server);

    s_satisfy_pedantic_compilers ();
    return self;
}

static void
s_server_destroy (s_server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_server_t *self = *self_p;
        mlm_proto_destroy (&self->message);
        //  Destroy clients before destroying the server
        zhash_destroy (&self->clients);
        //  Forcibly destroy any zombie clients as we are shutting down
        s_client_t *zombie = (s_client_t *)zhash_first (self->zombies);
        while (zombie) {
            s_client_finalize (&zombie);
            zombie = (s_client_t *)zhash_next (self->zombies);
        }
        zhash_destroy (&self->zombies);
        server_terminate (&self->server);
        zsock_destroy (&self->router);
        zconfig_destroy (&self->config);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  Apply service-specific configuration tree:
//   * apply server configuration
//   * print any echo items in top-level sections
//   * apply sections that match methods

static void
s_server_config_service (s_server_t *self)
{
    //  Apply echo commands and class methods
    zconfig_t *section = zconfig_locate (self->config, "mlm_server");
    if (section)
        section = zconfig_child (section);

    while (section) {
        if (streq (zconfig_name (section), "echo"))
            zsys_notice ("%s", zconfig_value (section));
        else
        if (streq (zconfig_name (section), "bind")) {
            char *endpoint = zconfig_get (section, "endpoint", "?");
            self->port = zsock_bind (self->router, "%s", endpoint);
            if (self->port == -1)
                zsys_warning ("could not bind to %s (%s)", endpoint, zmq_strerror (zmq_errno ()));
        }
#if (ZMQ_VERSION_MAJOR >= 4)
        else
        if (streq (zconfig_name (section), "security")) {
            char *mechanism = zconfig_get (section, "mechanism", "null");
            char *domain = zconfig_get (section, "domain", NULL);
            if (domain)
                zsock_set_zap_domain (self->router, domain);
            if (streq (mechanism, "null")) {
                zsys_notice ("server is using NULL security");
            }
            else
            if (streq (mechanism, "plain")) {
                zsys_notice ("server is using PLAIN security");
                zsock_set_plain_server (self->router, 1);
            }
            else
                zsys_warning ("mechanism=%s is not supported", mechanism);
        }
#endif
        section = zconfig_next (section);
    }
    s_server_config_global (self);
}

//  Process message from pipe

static int
s_server_handle_pipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);

    s_server_t *self = (s_server_t *) argument;
    zmsg_t *msg = zmsg_recv (self->pipe);
    if (!msg)
        return -1;              //  Interrupted; exit zloop
    char *method = zmsg_popstr (msg);
    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (method, "$TERM")) {
        //  Shutdown the engine
        zstr_free (&method);
        zmsg_destroy (&msg);
        return -1;
    }
    else
    if (streq (method, "BIND")) {
        //  Bind to a specified endpoint, which may use an ephemeral port
        char *endpoint = zmsg_popstr (msg);
        self->port = zsock_bind (self->router, "%s", endpoint);
        if (self->port == -1)
            zsys_warning ("could not bind to %s", endpoint);
        zstr_free (&endpoint);
    }
    else
    if (streq (method, "PORT")) {
        //  Return PORT + port number from the last bind, if any
        zstr_sendm (self->pipe, "PORT");
        zstr_sendf (self->pipe, "%d", self->port);
    }
    else                       //  Deprecated method name
    if (streq (method, "LOAD") || streq (method, "CONFIGURE")) {
        char *filename = zmsg_popstr (msg);
        zconfig_destroy (&self->config);
        self->config = zconfig_load (filename);
        if (self->config) {
            s_server_config_service (self);
            self->server.config = self->config;
            server_configuration (&self->server, self->config);
        }
        else {
            zsys_warning ("cannot load config file '%s'", filename);
            self->config = zconfig_new ("root", NULL);
        }
        zstr_free (&filename);
    }
    else
    if (streq (method, "SET")) {
        char *path = zmsg_popstr (msg);
        char *value = zmsg_popstr (msg);
        zconfig_put (self->config, path, value);
        if (streq (path, "server/animate")) {
            zsys_warning ("'%s' is deprecated, use VERBOSE command instead", path);
            self->verbose = (atoi (value) == 1);
        }
        s_server_config_global (self);
        zstr_free (&value);
        zstr_free (&path);
    }
    else
    if (streq (method, "SAVE")) {
        char *filename = zmsg_popstr (msg);
        if (zconfig_save (self->config, filename))
            zsys_warning ("cannot save config file '%s'", filename);
        zstr_free (&filename);
    }
    else {
        //  Execute custom method
        zmsg_t *reply = server_method (&self->server, method, msg);
        //  If reply isn't null, send it to caller
        zmsg_send (&reply, self->pipe);
    }
    zstr_free (&method);
    zmsg_destroy (&msg);
    return 0;
}

//  Handle a protocol message from the client

static int
s_server_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(reader);

    s_server_t *self = (s_server_t *) argument;
    //  We process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->router) & ZMQ_POLLIN) {
        int rc = mlm_proto_recv (self->message, self->router);
        if (rc == -1)
            return -1;      //  Interrupted; exit zloop


        //  TODO: use binary hashing on routing_id
        char *hashkey = zframe_strhex (mlm_proto_routing_id (self->message));
        s_client_t *client = (s_client_t *) zhash_lookup (self->clients, hashkey);
        if (client == NULL) {
            client = s_client_new (self, mlm_proto_routing_id (self->message));
            zhash_insert (self->clients, hashkey, client);
            zhash_freefn (self->clients, hashkey, s_client_free);
        }
        free (hashkey);
        //  Any input from client counts as activity
        if (client->ticket)
            zloop_ticket_reset (self->loop, client->ticket);

        if (rc == -2) {
            continue;       //  Malformed, but malformed_event doesn't exist
                            //  -> discard the message
        }
        //  Pass to client state machine
        s_client_execute (client, s_protocol_event (self->message));
    }
    return 0;
}

//  Watch server config file and reload if changed

static int
s_watch_server_config (zloop_t *loop, int timer_id, void *argument)
{
    ZPROTO_UNUSED(loop);
    ZPROTO_UNUSED(timer_id);

    s_server_t *self = (s_server_t *) argument;
    if (zconfig_has_changed (self->config)
    &&  zconfig_reload (&self->config) == 0) {
        s_server_config_service (self);
        self->server.config = self->config;
        server_configuration (&self->server, self->config);
        zsys_notice ("reloaded configuration from %s",
            zconfig_filename (self->config));
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the server actor, which polls its two sockets and processes
//  incoming messages

void
mlm_server (zsock_t *pipe, void *args)
{
    //  Initialize
    s_server_t *self = s_server_new (pipe);
    assert (self);
    zsock_signal (pipe, 0);
    //  Actor argument may be a string used for logging
    self->log_prefix = args? (char *) args: "";

    //  Set-up server monitor to watch for config file changes
    engine_set_monitor ((server_t *) self, 1000, s_watch_server_config);
    //  Set up handler for the two main sockets the server uses
    engine_handle_socket ((server_t *) self, self->pipe, s_server_handle_pipe);
    engine_handle_socket ((server_t *) self, self->router, s_server_handle_protocol);

    //  Run reactor until there's a termination signal
    zloop_start (self->loop);

    //  Reactor has ended
    s_server_destroy (&self);
}

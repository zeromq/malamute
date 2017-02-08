/*  =========================================================================
    mlm_client_engine - Malamute Client engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mlm_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    connecting_state = 2,
    connected_state = 3,
    confirming_state = 4,
    terminating_state = 5,
    reconnecting_state = 6,
    disconnected_state = 7,
    defaults_state = 8,
    have_error_state = 9
} state_t;

typedef enum {
    NULL_event = 0,
    set_plain_auth_event = 1,
    connect_event = 2,
    bad_endpoint_event = 3,
    destructor_event = 4,
    set_producer_event = 5,
    set_consumer_event = 6,
    set_worker_event = 7,
    ok_event = 8,
    expired_event = 9,
    bad_pattern_event = 10,
    stream_deliver_event = 11,
    mailbox_deliver_event = 12,
    service_deliver_event = 13,
    error_event = 14,
    connection_pong_event = 15,
    replay_ready_event = 16,
    heartbeat_event = 17,
    command_invalid_event = 18,
    failed_event = 19,
    other_event = 20
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connecting",
    "connected",
    "confirming",
    "terminating",
    "reconnecting",
    "disconnected",
    "defaults",
    "have error"
};

static char *
s_event_name [] = {
    "(NONE)",
    "set_plain_auth",
    "connect",
    "bad_endpoint",
    "destructor",
    "set_producer",
    "set_consumer",
    "set_worker",
    "OK",
    "expired",
    "bad_pattern",
    "STREAM_DELIVER",
    "MAILBOX_DELIVER",
    "SERVICE_DELIVER",
    "ERROR",
    "CONNECTION_PONG",
    "replay_ready",
    "heartbeat",
    "command_invalid",
    "failed",
    "other"
};


//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *username;
    char *password;
    char *endpoint;
    uint32_t timeout;
    char *address;
    char *stream;
    char *pattern;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    mlm_proto_t *message;       //  Message received or sent
    client_args_t args;         //  Method arguments structure
    bool verbose;               //  True if we're tracing state machine
    bool connected;             //  True if client is connected
    bool terminated;            //  True if client is shutdown
    bool fsm_stopped;           //  "terminate" action called
    size_t expiry;              //  Expiry timer, msecs
    size_t heartbeat;           //  Heartbeat timer, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for expiry
    int wakeup_timer;           //  zloop timer for alarms
    int heartbeat_timer;        //  zloop timer for heartbeat
    event_t wakeup_event;       //  Wake up with this event
    char log_prefix [41];       //  Log prefix string
} s_client_t;

static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_client_destroy (s_client_t **self_p);
static void
    s_client_execute (s_client_t *self, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static int
    s_client_handle_heartbeat (zloop_t *loop, int timer_id, void *argument);
static int
    s_client_handle_expiry (zloop_t *loop, int timer_id, void *argument);
static void
    s_satisfy_pedantic_compilers (void);
static void
    use_plain_security_mechanism (client_t *self);
static void
    signal_success (client_t *self);
static void
    remember_client_address (client_t *self);
static void
    connect_to_server_endpoint (client_t *self);
static void
    set_client_address (client_t *self);
static void
    use_connect_timeout (client_t *self);
static void
    signal_bad_endpoint (client_t *self);
static void
    signal_failure (client_t *self);
static void
    client_is_connected (client_t *self);
static void
    signal_server_not_present (client_t *self);
static void
    prepare_stream_write_command (client_t *self);
static void
    prepare_stream_read_command (client_t *self);
static void
    prepare_service_offer_command (client_t *self);
static void
    signal_bad_pattern (client_t *self);
static void
    pass_stream_message_to_app (client_t *self);
static void
    pass_mailbox_message_to_app (client_t *self);
static void
    pass_service_message_to_app (client_t *self);
static void
    server_has_gone_offline (client_t *self);
static void
    check_status_code (client_t *self);
static void
    get_first_replay_command (client_t *self);
static void
    get_next_replay_command (client_t *self);
static void
    announce_unhandled_error (client_t *self);

//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *cmdpipe, zsock_t *msgpipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->cmdpipe = cmdpipe;
        self->msgpipe = msgpipe;
        self->state = start_state;
        self->event = NULL_event;
        snprintf (self->log_prefix, sizeof (self->log_prefix),
            "%6d:%-33s", randof (1000000), "mlm_client");
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = mlm_proto_new ();
        if (self->message)
            self->loop = zloop_new ();
        if (self->loop) {
            //  Give application chance to initialize and set next event
            self->client.cmdpipe = self->cmdpipe;
            self->client.msgpipe = self->msgpipe;
            self->client.dealer = self->dealer;
            self->client.message = self->message;
            self->client.args = &self->args;
            if (client_initialize (&self->client))
                s_client_destroy (&self);
        }
        else
            s_client_destroy (&self);
    }
    s_satisfy_pedantic_compilers ();
    return self;
}

//  Destroy the client connection

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        zstr_free (&self->args.username);
        zstr_free (&self->args.password);
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.address);
        zstr_free (&self->args.stream);
        zstr_free (&self->args.pattern);
        client_terminate (&self->client);
        mlm_proto_destroy (&self->message);
        zsock_destroy (&self->msgpipe);
        zsock_destroy (&self->dealer);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  dealer socket and treat that as the event.

static void
engine_set_next_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->next_event = event;
    }
}

//  Raise an exception with 'event', halting any actions in progress.
//  Continues execution of actions defined for the exception event.

static void
engine_set_exception (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->exception = event;
    }
}

//  Set wakeup alarm after 'delay' msecs. The next state should handle the
//  wakeup event. The alarm is cancelled on any other event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup_timer) {
            zloop_timer_end (self->loop, self->wakeup_timer);
            self->wakeup_timer = 0;
        }
        self->wakeup_timer = zloop_timer (
            self->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Set a heartbeat timer. The interval is in msecs and must be
//  non-zero. The state machine must handle the "heartbeat" event.
//  The heartbeat happens every interval no matter what traffic the
//  client is sending or receiving.

static void
engine_set_heartbeat (client_t *client, size_t heartbeat)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->heartbeat = heartbeat;
        if (self->heartbeat_timer) {
            zloop_timer_end (self->loop, self->heartbeat_timer);
            self->heartbeat_timer = 0;
        }
        if (self->heartbeat)
            self->heartbeat_timer = zloop_timer (
                self->loop, self->heartbeat, 1, s_client_handle_heartbeat, self);
    }
}


//  Set expiry timer. Setting a non-zero expiry causes the state machine
//  to receive an "expired" event if there is no incoming traffic for that many
//  milliseconds. This cycles over and over until/unless the code sets a
//  zero expiry. The state machine must handle the "expired" event.

//  Macro to support deprecated name: remove after 2016-07-31
#define engine_set_timeout engine_set_expiry

static void
engine_set_expiry (client_t *client, size_t expiry)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->expiry = expiry;
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        if (self->expiry)
            self->expiry_timer = zloop_timer (
                self->loop, self->expiry, 1, s_client_handle_expiry, self);
    }
}

//  Poll socket for activity, invoke handler on any received message.
//  Handler must be a CZMQ zloop_fn function; receives client as arg.

static void
engine_handle_socket (client_t *client, zsock_t *sock, zloop_reader_fn handler)
{
    if (client && sock) {
        s_client_t *self = (s_client_t *) client;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, sock, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, sock);
        }
        else
            zloop_reader_end (self->loop, sock);
    }
}

//  Set connected to true/false. The client must call this if it wants
//  to provide the API with the connected status.

static void
engine_set_connected (client_t *client, bool connected)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->connected = connected;
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_heartbeat (NULL, 0);
    engine_set_expiry (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
    engine_set_connected (NULL, 0);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (s_client_t *self, mlm_proto_t *message)
{
    assert (message);
    switch (mlm_proto_id (message)) {
        case MLM_PROTO_CONNECTION_PONG:
            return connection_pong_event;
            break;
        case MLM_PROTO_STREAM_DELIVER:
            return stream_deliver_event;
            break;
        case MLM_PROTO_MAILBOX_DELIVER:
            return mailbox_deliver_event;
            break;
        case MLM_PROTO_SERVICE_DELIVER:
            return service_deliver_event;
            break;
        case MLM_PROTO_OK:
            return ok_event;
            break;
        case MLM_PROTO_ERROR:
            return error_event;
            break;
        default:
            zsys_error ("%s: unknown command %s, halting",
                        self->log_prefix, mlm_proto_command (message));
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is stopped, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated                    //  Actor is dying
        && !self->fsm_stopped                   //  FSM has finished
        && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == set_plain_auth_event) {
                    if (!self->exception) {
                        //  use plain security mechanism
                        if (self->verbose)
                            zsys_debug ("%s:         $ use plain security mechanism", self->log_prefix);
                        use_plain_security_mechanism (&self->client);
                    }
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                }
                else
                if (self->event == connect_event) {
                    if (!self->exception) {
                        //  remember client address
                        if (self->verbose)
                            zsys_debug ("%s:         $ remember client address", self->log_prefix);
                        remember_client_address (&self->client);
                    }
                    if (!self->exception) {
                        //  connect to server endpoint
                        if (self->verbose)
                            zsys_debug ("%s:         $ connect to server endpoint", self->log_prefix);
                        connect_to_server_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  set client address
                        if (self->verbose)
                            zsys_debug ("%s:         $ set client address", self->log_prefix);
                        set_client_address (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("%s:         $ use connect timeout", self->log_prefix);
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send CONNECTION_OPEN
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_OPEN",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_OPEN);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = connecting_state;
                }
                else
                if (self->event == bad_endpoint_event) {
                    if (!self->exception) {
                        //  signal bad endpoint
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal bad endpoint", self->log_prefix);
                        signal_bad_endpoint (&self->client);
                    }
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == set_producer_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                }
                else
                if (self->event == set_consumer_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                }
                else
                if (self->event == set_worker_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case connecting_state:
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal server not present
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal server not present", self->log_prefix);
                        signal_server_not_present (&self->client);
                    }
                    if (!self->exception)
                        self->state = start_state;
                }
                else
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == connection_pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case connected_state:
                if (self->event == set_producer_event) {
                    if (!self->exception) {
                        //  prepare stream write command
                        if (self->verbose)
                            zsys_debug ("%s:         $ prepare stream write command", self->log_prefix);
                        prepare_stream_write_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_WRITE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send STREAM_WRITE",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_STREAM_WRITE);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = confirming_state;
                }
                else
                if (self->event == set_consumer_event) {
                    if (!self->exception) {
                        //  prepare stream read command
                        if (self->verbose)
                            zsys_debug ("%s:         $ prepare stream read command", self->log_prefix);
                        prepare_stream_read_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_READ
                        if (self->verbose)
                            zsys_debug ("%s:         $ send STREAM_READ",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_STREAM_READ);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = confirming_state;
                }
                else
                if (self->event == set_worker_event) {
                    if (!self->exception) {
                        //  prepare service offer command
                        if (self->verbose)
                            zsys_debug ("%s:         $ prepare service offer command", self->log_prefix);
                        prepare_service_offer_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send SERVICE_OFFER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send SERVICE_OFFER",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_SERVICE_OFFER);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = confirming_state;
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send CONNECTION_CLOSE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_CLOSE",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_CLOSE);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = terminating_state;
                }
                else
                if (self->event == bad_pattern_event) {
                    if (!self->exception) {
                        //  signal bad pattern
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal bad pattern", self->log_prefix);
                        signal_bad_pattern (&self->client);
                    }
                }
                else
                if (self->event == stream_deliver_event) {
                    if (!self->exception) {
                        //  pass stream message to app
                        if (self->verbose)
                            zsys_debug ("%s:         $ pass stream message to app", self->log_prefix);
                        pass_stream_message_to_app (&self->client);
                    }
                }
                else
                if (self->event == mailbox_deliver_event) {
                    if (!self->exception) {
                        //  pass mailbox message to app
                        if (self->verbose)
                            zsys_debug ("%s:         $ pass mailbox message to app", self->log_prefix);
                        pass_mailbox_message_to_app (&self->client);
                    }
                }
                else
                if (self->event == service_deliver_event) {
                    if (!self->exception) {
                        //  pass service message to app
                        if (self->verbose)
                            zsys_debug ("%s:         $ pass service message to app", self->log_prefix);
                        pass_service_message_to_app (&self->client);
                    }
                }
                else
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == connection_pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  server has gone offline
                        if (self->verbose)
                            zsys_debug ("%s:         $ server has gone offline", self->log_prefix);
                        server_has_gone_offline (&self->client);
                    }
                    if (!self->exception)
                        self->state = disconnected_state;
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case confirming_state:
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  server has gone offline
                        if (self->verbose)
                            zsys_debug ("%s:         $ server has gone offline", self->log_prefix);
                        server_has_gone_offline (&self->client);
                    }
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                    if (!self->exception)
                        self->state = disconnected_state;
                }
                else
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case terminating_state:
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case reconnecting_state:
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception) {
                        //  get first replay command
                        if (self->verbose)
                            zsys_debug ("%s:         $ get first replay command", self->log_prefix);
                        get_first_replay_command (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                }
                else
                if (self->event == set_producer_event) {
                    if (!self->exception) {
                        //  send STREAM_WRITE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send STREAM_WRITE",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_STREAM_WRITE);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  get next replay command
                        if (self->verbose)
                            zsys_debug ("%s:         $ get next replay command", self->log_prefix);
                        get_next_replay_command (&self->client);
                    }
                }
                else
                if (self->event == set_consumer_event) {
                    if (!self->exception) {
                        //  send STREAM_READ
                        if (self->verbose)
                            zsys_debug ("%s:         $ send STREAM_READ",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_STREAM_READ);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  get next replay command
                        if (self->verbose)
                            zsys_debug ("%s:         $ get next replay command", self->log_prefix);
                        get_next_replay_command (&self->client);
                    }
                }
                else
                if (self->event == set_worker_event) {
                    if (!self->exception) {
                        //  send SERVICE_OFFER
                        if (self->verbose)
                            zsys_debug ("%s:         $ send SERVICE_OFFER",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_SERVICE_OFFER);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception) {
                        //  get next replay command
                        if (self->verbose)
                            zsys_debug ("%s:         $ get next replay command", self->log_prefix);
                        get_next_replay_command (&self->client);
                    }
                }
                else
                if (self->event == replay_ready_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  server has gone offline
                        if (self->verbose)
                            zsys_debug ("%s:         $ server has gone offline", self->log_prefix);
                        server_has_gone_offline (&self->client);
                    }
                    if (!self->exception)
                        self->state = disconnected_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case disconnected_state:
                if (self->event == set_producer_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                }
                else
                if (self->event == set_consumer_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                }
                else
                if (self->event == set_worker_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == connection_pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  server has gone offline
                        if (self->verbose)
                            zsys_debug ("%s:         $ server has gone offline", self->log_prefix);
                        server_has_gone_offline (&self->client);
                    }
                    if (!self->exception)
                        self->state = disconnected_state;
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case defaults_state:
                if (self->event == heartbeat_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_PING",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == connection_pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  server has gone offline
                        if (self->verbose)
                            zsys_debug ("%s:         $ server has gone offline", self->log_prefix);
                        server_has_gone_offline (&self->client);
                    }
                    if (!self->exception)
                        self->state = disconnected_state;
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case have_error_state:
                if (self->event == command_invalid_event) {
                    if (!self->exception) {
                        //  set client address
                        if (self->verbose)
                            zsys_debug ("%s:         $ set client address", self->log_prefix);
                        set_client_address (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("%s:         $ use connect timeout", self->log_prefix);
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send CONNECTION_OPEN
                        if (self->verbose)
                            zsys_debug ("%s:         $ send CONNECTION_OPEN",
                                self->log_prefix);
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_OPEN);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else
                if (self->event == failed_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == other_event) {
                    if (!self->exception) {
                        //  announce unhandled error
                        if (self->verbose)
                            zsys_debug ("%s:         $ announce unhandled error", self->log_prefix);
                        announce_unhandled_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("%s: unhandled event %s in %s",
                        self->log_prefix,
                        s_event_name [self->event],
                        s_state_name [self->state]);
                    assert (false);
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (self->verbose)
            zsys_debug ("%s:         > %s",
                    self->log_prefix, s_state_name [self->state]);
    }
}

//  zloop callback when client heartbeat timer expires

static int
s_client_handle_heartbeat (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, heartbeat_event);
    if (self->terminated)
        return -1;

    if (self->heartbeat > 0)
        self->heartbeat_timer = zloop_timer (
            loop, self->heartbeat, 1, s_client_handle_heartbeat, self);
    return 0;
}

//  zloop callback when client expiry timeout expires

static int
s_client_handle_expiry (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    if (self->terminated)
        return -1;

    if (self->expiry > 0)
        self->expiry_timer = zloop_timer (
            loop, self->expiry, 1, s_client_handle_expiry, self);
    return 0;
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Handle command pipe to/from calling API

static int
s_client_handle_cmdpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    char *method = zstr_recv (self->cmdpipe);
    if (!method)
        return -1;                  //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);

    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "$CONNECTED"))
        zsock_send (self->cmdpipe, "i", self->connected);
    else
    if (streq (method, "SET VERBOSE"))
        zsock_recv (self->cmdpipe, "i", &self->verbose);
    else
    if (streq (method, "SET PLAIN AUTH")) {
        zstr_free (&self->args.username);
        zstr_free (&self->args.password);
        zsock_recv (self->cmdpipe, "ss", &self->args.username, &self->args.password);
        s_client_execute (self, set_plain_auth_event);
    }
    else
    if (streq (method, "CONNECT")) {
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.address);
        zsock_recv (self->cmdpipe, "s4s", &self->args.endpoint, &self->args.timeout, &self->args.address);
        s_client_execute (self, connect_event);
    }
    else
    if (streq (method, "DESTRUCTOR")) {
        s_client_execute (self, destructor_event);
    }
    else
    if (streq (method, "SET PRODUCER")) {
        zstr_free (&self->args.stream);
        zsock_recv (self->cmdpipe, "s", &self->args.stream);
        s_client_execute (self, set_producer_event);
    }
    else
    if (streq (method, "SET CONSUMER")) {
        zstr_free (&self->args.stream);
        zstr_free (&self->args.pattern);
        zsock_recv (self->cmdpipe, "ss", &self->args.stream, &self->args.pattern);
        s_client_execute (self, set_consumer_event);
    }
    else
    if (streq (method, "SET WORKER")) {
        zstr_free (&self->args.address);
        zstr_free (&self->args.pattern);
        zsock_recv (self->cmdpipe, "ss", &self->args.address, &self->args.pattern);
        s_client_execute (self, set_worker_event);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->cmdpipe)) {
        zsys_error ("%s: trailing API command frames (%s)",
            self->log_prefix, method);
        zmsg_t *more = zmsg_recv (self->cmdpipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  Handle message pipe to/from calling API

static int
s_client_handle_msgpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->msgpipe) & ZMQ_POLLIN) {
        char *method = zstr_recv (self->msgpipe);
        if (!method)
            return -1;              //  Interrupted; exit zloop
        if (self->verbose)
            zsys_debug ("%s:     API message=%s", self->log_prefix, method);

        //  Front-end shuts down msgpipe before cmdpipe, this little
        //  handshake just ensures all traffic on the msgpipe has been
        //  flushed before the calling thread continues with destroying
        //  the actor.
        if (streq (method, "$FLUSH"))
            zsock_signal (self->cmdpipe, 0);
        else
        if (streq (method, "STREAM SEND")) {
            char *subject;
            zmsg_t *content;
            zsock_brecv (self->msgpipe, "sp", &subject, &content);
            mlm_proto_set_id (self->message, MLM_PROTO_STREAM_SEND);
            mlm_proto_set_subject (self->message, subject);
            mlm_proto_set_content (self->message, &content);
            mlm_proto_send (self->message, self->dealer);
        }
        else
        if (streq (method, "MAILBOX SEND")) {
            char *address;
            char *subject;
            char *tracker;
            uint32_t timeout;
            zmsg_t *content;
            zsock_brecv (self->msgpipe, "sss4p", &address, &subject, &tracker, &timeout, &content);
            mlm_proto_set_id (self->message, MLM_PROTO_MAILBOX_SEND);
            mlm_proto_set_address (self->message, address);
            mlm_proto_set_subject (self->message, subject);
            mlm_proto_set_tracker (self->message, tracker);
            mlm_proto_set_timeout (self->message, timeout);
            mlm_proto_set_content (self->message, &content);
            mlm_proto_send (self->message, self->dealer);
        }
        else
        if (streq (method, "SERVICE SEND")) {
            char *address;
            char *subject;
            char *tracker;
            uint32_t timeout;
            zmsg_t *content;
            zsock_brecv (self->msgpipe, "sss4p", &address, &subject, &tracker, &timeout, &content);
            mlm_proto_set_id (self->message, MLM_PROTO_SERVICE_SEND);
            mlm_proto_set_address (self->message, address);
            mlm_proto_set_subject (self->message, subject);
            mlm_proto_set_tracker (self->message, tracker);
            mlm_proto_set_timeout (self->message, timeout);
            mlm_proto_set_content (self->message, &content);
            mlm_proto_send (self->message, self->dealer);
        }
        //  Cleanup pipe if any argument frames are still waiting to be eaten
        if (zsock_rcvmore (self->msgpipe)) {
            zsys_error ("%s: trailing API message frames (%s)", self->log_prefix, method);
            zmsg_t *more = zmsg_recv (self->msgpipe);
            zmsg_print (more);
            zmsg_destroy (&more);
        }
        zstr_free (&method);
    }
    return 0;
}


//  Handle a message (a protocol reply) from the server

static int
s_client_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->dealer) & ZMQ_POLLIN) {
        if (mlm_proto_recv (self->message, self->dealer))
            return -1;              //  Interrupted; exit zloop

        //  Any input from server counts as activity
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        //  Reset expiry timer if expiry timeout not zero
        if (self->expiry)
            self->expiry_timer = zloop_timer (
                self->loop, self->expiry, 1, s_client_handle_expiry, self);
        s_client_execute (self, s_protocol_event (self, self->message));
        if (self->terminated)
            return -1;
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the client actor, which polls its two sockets and processes
//  incoming messages

void
mlm_client (zsock_t *cmdpipe, void *msgpipe)
{
    //  Initialize
    s_client_t *self = s_client_new (cmdpipe, (zsock_t *) msgpipe);
    if (self) {
        zsock_signal (cmdpipe, 0);

        //  Set up handler for the sockets the client uses
        engine_handle_socket ((client_t *) self, self->cmdpipe, s_client_handle_cmdpipe);
        engine_handle_socket ((client_t *) self, self->msgpipe, s_client_handle_msgpipe);
        engine_handle_socket ((client_t *) self, self->dealer, s_client_handle_protocol);

        //  Run reactor until there's a termination signal
        zloop_start (self->loop);

        //  Reactor has ended
        s_client_destroy (&self);
    }
    else
        zsock_signal (cmdpipe, -1);
}


//  ---------------------------------------------------------------------------
//  Class interface

struct _mlm_client_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    bool connected;             //  Client currently connected or not
    char *command;              //  Last received protocol command
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
    char *address;              //  Returned by actor reply
    char *sender;               //  Returned by actor reply
    char *subject;              //  Returned by actor reply
    zmsg_t *content;            //  Returned by actor reply
    char *tracker;              //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new mlm_client

MLM_EXPORT mlm_client_t *
mlm_client_new (void)
{
    mlm_client_t *self = (mlm_client_t *) zmalloc (sizeof (mlm_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        if (self->msgpipe)
            self->actor = zactor_new (mlm_client, backend);
        if (!self->actor)
            mlm_client_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the mlm_client
//  Disconnect from server. Waits for a short timeout for confirmation from the     
//  server, then disconnects anyhow.                                                

static int 
mlm_client_destructor (mlm_client_t *self);

void
mlm_client_destroy (mlm_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mlm_client_t *self = *self_p;
        if (self->actor && !zsys_interrupted) {
            //  Before destroying the actor we have to flush any pending
            //  traffic on the msgpipe, otherwise it gets lost in a fire and
            //  forget scenario. We do this by sending $FLUSH to the msgpipe
            //  and waiting for a signal back on the cmdpipe.
            if (zstr_send (self->msgpipe, "$FLUSH") == 0)
                zsock_wait (self->actor);
            mlm_client_destructor (self);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        zstr_free (&self->command);
        zstr_free (&self->reason);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.

zactor_t *
mlm_client_actor (mlm_client_t *self)
{
    assert (self);
    return self->actor;
}


//  ---------------------------------------------------------------------------
//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.

zsock_t *
mlm_client_msgpipe (mlm_client_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.

bool
mlm_client_connected (mlm_client_t *self)
{
    assert (self);
    int connected;
    zsock_send (self->actor, "s", "$CONNECTED");
    zsock_recv (self->actor, "i", &connected);
    return connected == 1;
}


//  ---------------------------------------------------------------------------
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted.

static int
s_accept_reply (mlm_client_t *self, ...)
{
    assert (self);
    while (!zsys_interrupted) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            break;              //  Interrupted

        va_list args;
        va_start (args, self);
        char *filter = va_arg (args, char *);
        while (filter) {
            if (streq (reply, filter)) {
                if (streq (reply, "SUCCESS")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "FAILURE")) {
                    zstr_free (&self->reason);
                    zsock_recv (self->actor, "is", &self->status, &self->reason);
                }
                break;
            }
            filter = va_arg (args, char *);
        }
        va_end (args);
        //  If anything was remaining on pipe, flush it
        zsock_flush (self->actor);
        zstr_free (&reply);
        if (filter) {
            return 0;           //  We matched one of the filters
        }
    }
    return -1;          //  Interrupted
}


//  ---------------------------------------------------------------------------
//  Set PLAIN authentication username and password. If you do not call this, the    
//  client will use NULL authentication. TODO: add "set curve auth".                
//  Returns >= 0 if successful, -1 if interrupted.

int 
mlm_client_set_plain_auth (mlm_client_t *self, const char *username, const char *password)
{
    assert (self);

    zsock_send (self->actor, "sss", "SET PLAIN AUTH", username, password);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful. The caller may      
//  specify its address.                                                            
//  Returns >= 0 if successful, -1 if interrupted.

int 
mlm_client_connect (mlm_client_t *self, const char *endpoint, uint32_t timeout, const char *address)
{
    assert (self);

    zsock_send (self->actor, "ss4s", "CONNECT", endpoint, timeout, address);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Disconnect from server. Waits for a short timeout for confirmation from the     
//  server, then disconnects anyhow.                                                
//  Returns >= 0 if successful, -1 if interrupted.

int 
mlm_client_destructor (mlm_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "DESTRUCTOR");
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Prepare to publish to a specified stream. After this, all messages are sent to  
//  this stream exclusively.                                                        
//  Returns >= 0 if successful, -1 if interrupted.

int 
mlm_client_set_producer (mlm_client_t *self, const char *stream)
{
    assert (self);

    zsock_send (self->actor, "ss", "SET PRODUCER", stream);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Consume messages with matching subjects. The pattern is a regular expression    
//  using the CZMQ zrex syntax. The most useful elements are: ^ and $ to match the  
//  start and end, . to match any character, \s and \S to match whitespace and      
//  non-whitespace, \d and \D to match a digit and non-digit, \a and \A to match    
//  alphabetic and non-alphabetic, \w and \W to match alphanumeric and              
//  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
//  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
//  Returns >= 0 if successful, -1 if interrupted.

int 
mlm_client_set_consumer (mlm_client_t *self, const char *stream, const char *pattern)
{
    assert (self);

    zsock_send (self->actor, "sss", "SET CONSUMER", stream, pattern);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Offer a particular named service, where the pattern matches request subjects    
//  using the CZMQ zrex syntax.                                                     
//  Returns >= 0 if successful, -1 if interrupted.

int 
mlm_client_set_worker (mlm_client_t *self, const char *address, const char *pattern)
{
    assert (self);

    zsock_send (self->actor, "sss", "SET WORKER", address, pattern);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send STREAM SEND message to server, takes ownership of message and
//  destroys when done sending it.

int
mlm_client_send (mlm_client_t *self, const char *subject, zmsg_t **content_p)
{
    assert (self);
    //  Send message name as first, separate frame
    zstr_sendm (self->msgpipe, "STREAM SEND");
    int rc = zsock_bsend (self->msgpipe, "sp", subject, *content_p);
    *content_p = NULL;          //  Take ownership of content
    return rc;
}


//  ---------------------------------------------------------------------------
//  Send MAILBOX SEND message to server, takes ownership of message and
//  destroys when done sending it.

int
mlm_client_sendto (mlm_client_t *self, const char *address, const char *subject, const char *tracker, uint32_t timeout, zmsg_t **content_p)
{
    assert (self);
    //  Send message name as first, separate frame
    zstr_sendm (self->msgpipe, "MAILBOX SEND");
    int rc = zsock_bsend (self->msgpipe, "sss4p", address, subject, tracker, timeout, *content_p);
    *content_p = NULL;          //  Take ownership of content
    return rc;
}


//  ---------------------------------------------------------------------------
//  Send SERVICE SEND message to server, takes ownership of message and
//  destroys when done sending it.

int
mlm_client_sendfor (mlm_client_t *self, const char *address, const char *subject, const char *tracker, uint32_t timeout, zmsg_t **content_p)
{
    assert (self);
    //  Send message name as first, separate frame
    zstr_sendm (self->msgpipe, "SERVICE SEND");
    int rc = zsock_bsend (self->msgpipe, "sss4p", address, subject, tracker, timeout, *content_p);
    *content_p = NULL;          //  Take ownership of content
    return rc;
}


//  ---------------------------------------------------------------------------
//  Receive message from server; caller destroys message when done

zmsg_t *
mlm_client_recv (mlm_client_t *self)
{
    zstr_free (&self->command);
    self->command = zstr_recv (self->msgpipe);
    if (!self->command)
        return NULL;            //  Interrupted

    if (streq (self->command, "STREAM DELIVER")) {
        zsock_brecv (self->msgpipe, "sssp", &self->address, &self->sender, &self->subject, &self->content);
        return self->content;
    }
    else
    if (streq (self->command, "MAILBOX DELIVER")) {
        zsock_brecv (self->msgpipe, "ssssp", &self->sender, &self->address, &self->subject, &self->tracker, &self->content);
        return self->content;
    }
    else
    if (streq (self->command, "SERVICE DELIVER")) {
        zsock_brecv (self->msgpipe, "ssssp", &self->sender, &self->address, &self->subject, &self->tracker, &self->content);
        return self->content;
    }
    return NULL;
}

//  ---------------------------------------------------------------------------
//  Return last received command. Can be one of these values:
//      "STREAM DELIVER"
//      "MAILBOX DELIVER"
//      "SERVICE DELIVER"
const char *
mlm_client_command (mlm_client_t *self)
{
    assert (self);
    return self->command;
}


//  ---------------------------------------------------------------------------
//  Return last received status

int 
mlm_client_status (mlm_client_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received reason

const char *
mlm_client_reason (mlm_client_t *self)
{
    assert (self);
    return self->reason;
}


//  ---------------------------------------------------------------------------
//  Return last received address

const char *
mlm_client_address (mlm_client_t *self)
{
    assert (self);
    return self->address;
}


//  ---------------------------------------------------------------------------
//  Return last received sender

const char *
mlm_client_sender (mlm_client_t *self)
{
    assert (self);
    return self->sender;
}


//  ---------------------------------------------------------------------------
//  Return last received subject

const char *
mlm_client_subject (mlm_client_t *self)
{
    assert (self);
    return self->subject;
}


//  ---------------------------------------------------------------------------
//  Return last received content

zmsg_t *
mlm_client_content (mlm_client_t *self)
{
    assert (self);
    return self->content;
}


//  ---------------------------------------------------------------------------
//  Return last received tracker

const char *
mlm_client_tracker (mlm_client_t *self)
{
    assert (self);
    return self->tracker;
}


//  ---------------------------------------------------------------------------
//  Enable verbose tracing (animation) of state machine activity.

void
mlm_client_set_verbose (mlm_client_t *self, bool verbose)
{
    assert (self);
    zsock_send (self->actor, "si", "SET VERBOSE", verbose);
}


//  ---------------------------------------------------------------------------
//  Send multipart string message to stream, end list with NULL
//  Returns 0 if OK, -1 if failed due to lack of memory or other error.

int
mlm_client_sendx (
    mlm_client_t *self,
    const char *subject,
    const char *content, ...)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();
    if (!msg)
        return -1;              //  Can't allocate memory
    va_list args;
    va_start (args, content);
    while (content) {
        zmsg_addstr (msg, content);
        content = va_arg (args, char *);
    }
    va_end (args);
    return mlm_client_send (self, subject, &msg);
}


//  ---------------------------------------------------------------------------
//  Send multipart string to mailbox, end list with NULL
//  Returns 0 if OK, -1 if failed due to lack of memory or other error.

int
mlm_client_sendtox (
    mlm_client_t *self,
    const char *address,
    const char *subject,
    const char *content, ...)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();
    if (!msg)
        return -1;              //  Can't allocate memory
    va_list args;
    va_start (args, content);
    while (content) {
        zmsg_addstr (msg, content);
        content = va_arg (args, char *);
    }
    va_end (args);
    return mlm_client_sendto (self, address, subject, NULL, 0, &msg);
}


//  ---------------------------------------------------------------------------
//  Send multipart string to service, end list with NULL
//  Returns 0 if OK, -1 if failed due to lack of memory or other error.

int
mlm_client_sendforx (
    mlm_client_t *self,
    const char *address,
    const char *subject,
    const char *content, ...)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();
    if (!msg)
        return -1;              //  Can't allocate memory
    va_list args;
    va_start (args, content);
    while (content) {
        zmsg_addstr (msg, content);
        content = va_arg (args, char *);
    }
    va_end (args);
    return mlm_client_sendfor (self, address, subject, NULL, 0, &msg);
}


//  ---------------------------------------------------------------------------
//  Receive a subject and string content from the server. The content may be
//  1 or more string frames. This method is orthogonal to the sendx methods.
//  End the string arguments with NULL. If there are not enough frames in
//  the received message, remaining strings are set to NULL. Returns number
//  of string contents received, or -1 in case of error. Free the returned
//  subject and content strings when finished with them. To get the type of
//  the command, use mlm_client_command ().

int
mlm_client_recvx (
    mlm_client_t *self,
    char **subject_p,
    char **string_p, ...)
{
    assert (self);
    zmsg_t *msg = mlm_client_recv (self);
    if (!msg)
        return -1;

    *subject_p = strdup (mlm_client_subject (self));
    int count = 0;
    va_list args;
    va_start (args, string_p);
    while (string_p) {
        *string_p = zmsg_popstr (msg);
        string_p = va_arg (args, char **);
        count++;
    }
    va_end (args);
    zmsg_destroy (&msg);
    return count;
}


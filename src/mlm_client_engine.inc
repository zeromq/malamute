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
    disconnecting_state = 5,
    defaults_state = 6,
    have_error_state = 7,
    reconnecting_state = 8
} state_t;

typedef enum {
    NULL_event = 0,
    constructor_event = 1,
    bad_endpoint_event = 2,
    ok_event = 3,
    expired_event = 4,
    set_producer_event = 5,
    set_consumer_event = 6,
    set_worker_event = 7,
    destructor_event = 8,
    stream_deliver_event = 9,
    mailbox_deliver_event = 10,
    service_deliver_event = 11,
    error_event = 12,
    connection_pong_event = 13,
    command_invalid_event = 14,
    other_event = 15
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connecting",
    "connected",
    "confirming",
    "disconnecting",
    "defaults",
    "have error",
    "reconnecting"
};

static char *
s_event_name [] = {
    "(NONE)",
    "constructor",
    "bad_endpoint",
    "OK",
    "expired",
    "set_producer",
    "set_consumer",
    "set_worker",
    "destructor",
    "STREAM_DELIVER",
    "MAILBOX_DELIVER",
    "SERVICE_DELIVER",
    "ERROR",
    "CONNECTION_PONG",
    "command_invalid",
    "other"
};
 

//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    int timeout;
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
    bool terminated;            //  True if client is shutdown
    size_t timeout;             //  inactivity timeout, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for timeouts
    int wakeup_timer;           //  zloop timer for alarms
    event_t wakeup_event;       //  Wake up with this event
    bool verbose;               //  Verbose logging enabled?
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
    s_client_handle_timeout (zloop_t *loop, int timer_id, void *argument);
static void
    s_satisfy_pedantic_compilers (void);
static void
    connect_to_server_endpoint (client_t *self);
static void
    set_client_address (client_t *self);
static void
    use_connect_timeout (client_t *self);
static void
    signal_failure (client_t *self);
static void
    signal_success (client_t *self);
static void
    use_heartbeat_timer (client_t *self);
static void
    signal_server_not_present (client_t *self);
static void
    prepare_stream_write_command (client_t *self);
static void
    prepare_stream_read_command (client_t *self);
static void
    prepare_service_offer_command (client_t *self);
static void
    pass_stream_message_to_app (client_t *self);
static void
    pass_mailbox_message_to_app (client_t *self);
static void
    pass_service_message_to_app (client_t *self);
static void
    check_status_code (client_t *self);
static void
    signal_unhandled_error (client_t *self);

//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *cmdpipe, zsock_t *msgpipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->cmdpipe = cmdpipe;
        self->msgpipe = msgpipe;
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = mlm_proto_new ();
        if (self->message)
            self->loop = zloop_new ();
        if (self->loop) {
            //  Give application chance to initialize and set next event
            self->state = start_state;
            self->event = NULL_event;
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

//  Set timeout for next protocol read. By default, will wait forever
//  or until the process is interrupted. The timeout is in milliseconds.
//  The state machine must handle the "expired" event.

static void
engine_set_timeout (client_t *client, size_t timeout)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->timeout = timeout;
        if (self->timeout)
            self->expiry_timer = zloop_timer (
                self->loop, self->timeout, 1, s_client_handle_timeout, self);
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

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_timeout (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
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
            zsys_error ("mlm_client: unknown command %s, halting", mlm_proto_command (message));
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is terminated, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (self->verbose) {
            zsys_debug ("mlm_client: %s:", s_state_name [self->state]);
            zsys_debug ("mlm_client:        %s", s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == constructor_event) {
                    if (!self->exception) {
                        //  connect to server endpoint
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ connect to server endpoint");
                        connect_to_server_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  set client address
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ set client address");
                        set_client_address (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send CONNECTION_OPEN
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send CONNECTION_OPEN");
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_OPEN);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = connecting_state;
                }
                else
                if (self->event == bad_endpoint_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal failure");
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("mlm_client: unhandled event %s in %s",
                        s_event_name [self->event], s_state_name [self->state]);
                    assert (false);
                }
                break;

            case connecting_state:
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal success");
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  use heartbeat timer
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ use heartbeat timer");
                        use_heartbeat_timer (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal server not present
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal server not present");
                        signal_server_not_present (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ check status code");
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
                            zsys_debug ("mlm_client:            $ prepare stream write command");
                        prepare_stream_write_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_WRITE
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send STREAM_WRITE");
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
                            zsys_debug ("mlm_client:            $ prepare stream read command");
                        prepare_stream_read_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_READ
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send STREAM_READ");
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
                            zsys_debug ("mlm_client:            $ prepare service offer command");
                        prepare_service_offer_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send SERVICE_OFFER
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send SERVICE_OFFER");
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
                            zsys_debug ("mlm_client:            $ send CONNECTION_CLOSE");
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_CLOSE);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = disconnecting_state;
                }
                else
                if (self->event == stream_deliver_event) {
                    if (!self->exception) {
                        //  pass stream message to app
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ pass stream message to app");
                        pass_stream_message_to_app (&self->client);
                    }
                }
                else
                if (self->event == mailbox_deliver_event) {
                    if (!self->exception) {
                        //  pass mailbox message to app
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ pass mailbox message to app");
                        pass_mailbox_message_to_app (&self->client);
                    }
                }
                else
                if (self->event == service_deliver_event) {
                    if (!self->exception) {
                        //  pass service message to app
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ pass service message to app");
                        pass_service_message_to_app (&self->client);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send CONNECTION_PING");
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_PING);
                        mlm_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ check status code");
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
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal success");
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
                            zsys_debug ("mlm_client:            $ signal failure");
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == connection_pong_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case disconnecting_state:
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal success");
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal failure");
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ check status code");
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
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ check status code");
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
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send CONNECTION_OPEN
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send CONNECTION_OPEN");
                        mlm_proto_set_id (self->message, MLM_PROTO_CONNECTION_OPEN);
                        mlm_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = reconnecting_state;
                }
                else
                if (self->event == other_event) {
                    if (!self->exception) {
                        //  signal unhandled error
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ signal unhandled error");
                        signal_unhandled_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("mlm_client: unhandled event %s in %s",
                        s_event_name [self->event], s_state_name [self->state]);
                    assert (false);
                }
                break;

            case reconnecting_state:
                if (self->event == ok_event) {
                    if (!self->exception) {
                        //  use heartbeat timer
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ use heartbeat timer");
                        use_heartbeat_timer (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == connection_pong_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->verbose)
                zsys_debug ("mlm_client:            ! %s", s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (self->verbose)
            zsys_debug ("mlm_client:            > %s", s_state_name [self->state]);
    }
}

//  zloop callback when client inactivity timer expires

static int
s_client_handle_timeout (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    return self->terminated? -1: 0;
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
        zsys_debug ("mlm_client:        API command=%s", method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;       //  Start verbose logging
    else
    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "CONSTRUCTOR")) {
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.address);
        zsock_recv (self->cmdpipe, "sis", &self->args.endpoint, &self->args.timeout, &self->args.address);
        s_client_execute (self, constructor_event);
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
        zsys_error ("mlm_client: trailing API command frames (%s)", method);
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
            zsys_debug ("mlm_client:        API message=%s", method);

        //  Front-end shuts down msgpipe before cmdpipe
        if (streq (method, "$TERM"))
            zsock_signal (self->cmdpipe, 0);
        else
        if (streq (method, "STREAM SEND")) {
            const char *subject;
            zmsg_t *content;
            zsock_brecv (self->msgpipe, "sp", &subject, &content);
            mlm_proto_set_id (self->message, MLM_PROTO_STREAM_SEND);
            mlm_proto_set_subject (self->message, subject);
            mlm_proto_set_content (self->message, &content);
            mlm_proto_send (self->message, self->dealer);
        }
        else
        if (streq (method, "MAILBOX SEND")) {
            const char *address;
            const char *subject;
            const char *tracker;
            int timeout;
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
            const char *address;
            const char *subject;
            const char *tracker;
            int timeout;
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
            zsys_error ("mlm_client: trailing API message frames (%s)", method);
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
        //  Reset expiry timer if timeout is not zero
        if (self->timeout)
            self->expiry_timer = zloop_timer (
                self->loop, self->timeout, 1, s_client_handle_timeout, self);
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
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
    char *command;              //  Returned by actor reply
    char *address;              //  Returned by actor reply
    char *sender;               //  Returned by actor reply
    char *subject;              //  Returned by actor reply
    zmsg_t *content;            //  Returned by actor reply
    char *tracker;              //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new mlm_client
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful. The caller may      
//  specify its address.                                                            

static int
mlm_client_constructor (mlm_client_t *self, const char *endpoint, int timeout, const char *address);

MLM_EXPORT mlm_client_t *
mlm_client_new (const char *endpoint, int timeout, const char *address)
{
    mlm_client_t *self = (mlm_client_t *) zmalloc (sizeof (mlm_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        self->actor = zactor_new (mlm_client, backend);
        if (self->actor)
            self->status = mlm_client_constructor (self, endpoint, timeout, address);
        if (self->status == -1)
            zactor_destroy (&self->actor);
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
            //  Shut down msgpipe first so that client can do clean shutdown,
            //  sending any pending messages and handshaking goodbye to server
            zstr_send (self->msgpipe, "$TERM");
            zsock_wait (self->actor);
            mlm_client_destructor (self);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        zstr_free (&self->reason);
        zstr_free (&self->command);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Enable verbose logging of client activity

void
mlm_client_verbose (mlm_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "VERBOSE");
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
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted or timed-out.

static int
s_accept_reply (mlm_client_t *self, ...)
{
    assert (self);
    while (!zsys_interrupted) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            break;              //  Interrupted or timed-out
        
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
        if (filter) {
            zstr_free (&reply);
            return 0;           //  We matched one of the filters
        }
    }
    return -1;          //  Interrupted or timed-out
}


//  ---------------------------------------------------------------------------
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful. The caller may      
//  specify its address.                                                            
//  Returns >= 0 if successful, -1 if interrupted.

static int
mlm_client_constructor (mlm_client_t *self, const char *endpoint, int timeout, const char *address)
{
    assert (self);
    zsock_send (self->actor, "ssis", "CONSTRUCTOR", endpoint, timeout, address);
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
//  Consume messages with matching addresses. The pattern is a regular expression   
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
//  Send STREAM SEND message to server, takes ownership of message
//  and destroys when done sending it.

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
//  Send MAILBOX SEND message to server, takes ownership of message
//  and destroys when done sending it.

int
mlm_client_sendto (mlm_client_t *self, const char *address, const char *subject, const char *tracker, int timeout, zmsg_t **content_p)
{
    assert (self);
    //  Send message name as first, separate frame
    zstr_sendm (self->msgpipe, "MAILBOX SEND");
    int rc = zsock_bsend (self->msgpipe, "sss4p", address, subject, tracker, timeout, *content_p);
    *content_p = NULL;          //  Take ownership of content
    return rc;
}


//  ---------------------------------------------------------------------------
//  Send SERVICE SEND message to server, takes ownership of message
//  and destroys when done sending it.

int
mlm_client_sendfor (mlm_client_t *self, const char *address, const char *subject, const char *tracker, int timeout, zmsg_t **content_p)
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
//  Return last received command

const char *
mlm_client_command (mlm_client_t *self)
{
    assert (self);
    return self->command;
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


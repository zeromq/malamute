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
    ok_event = 2,
    expired_event = 3,
    produce_event = 4,
    consume_event = 5,
    send_event = 6,
    destructor_event = 7,
    stream_deliver_event = 8,
    error_event = 9,
    connection_pong_event = 10,
    command_invalid_event = 11,
    other_event = 12
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
    "OK",
    "expired",
    "produce",
    "consume",
    "send",
    "destructor",
    "STREAM_DELIVER",
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
    char *stream;
    char *pattern;
    char *subject;
    char *content;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *pipe;              //  Socket to back to caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
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
    use_connect_timeout (client_t *self);
static void
    signal_success (client_t *self);
static void
    use_heartbeat_timer (client_t *self);
static void
    signal_server_not_present (client_t *self);
static void
    prepare_for_stream_write (client_t *self);
static void
    prepare_for_stream_read (client_t *self);
static void
    prepare_for_stream_publish (client_t *self);
static void
    deliver_message_to_application (client_t *self);
static void
    signal_failure (client_t *self);
static void
    check_status_code (client_t *self);
static void
    signal_unhandled_error (client_t *self);

//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *pipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->pipe = pipe;
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->loop = zloop_new ();
        if (self->loop)
            self->client.msgout = mlm_msg_new (0);
        if (self->client.msgout) {
            //  Give application chance to initialize and set next event
            self->state = start_state;
            self->event = NULL_event;
            self->client.pipe = self->pipe;
            self->client.dealer = self->dealer;
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
        zstr_free (&self->args.stream);
        zstr_free (&self->args.pattern);
        zstr_free (&self->args.subject);
        zstr_free (&self->args.content);
        client_terminate (&self->client);
        mlm_msg_destroy (&self->client.msgout);
        mlm_msg_destroy (&self->client.msgin);
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
engine_handle_socket (client_t *client, zsock_t *socket, zloop_reader_fn handler)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, socket, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, socket);
        }
        else
            zloop_reader_end (self->loop, socket);
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
s_protocol_event (s_client_t *self, mlm_msg_t *msgin)
{
    assert (msgin);
    switch (mlm_msg_id (msgin)) {
        case MLM_MSG_CONNECTION_PONG:
            return connection_pong_event;
            break;
        case MLM_MSG_STREAM_DELIVER:
            return stream_deliver_event;
            break;
        case MLM_MSG_OK:
            return ok_event;
            break;
        case MLM_MSG_ERROR:
            return error_event;
            break;
        default:
            zsys_error ("mlm_client: invalid protocol event, halting");
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
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send CONNECTION_OPEN
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send CONNECTION_OPEN");
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_CONNECTION_OPEN);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = connecting_state;
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
                if (self->event == produce_event) {
                    if (!self->exception) {
                        //  prepare for stream write
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ prepare for stream write");
                        prepare_for_stream_write (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_WRITE
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send STREAM_WRITE");
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_STREAM_WRITE);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = confirming_state;
                }
                else
                if (self->event == consume_event) {
                    if (!self->exception) {
                        //  prepare for stream read
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ prepare for stream read");
                        prepare_for_stream_read (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_READ
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send STREAM_READ");
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_STREAM_READ);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = confirming_state;
                }
                else
                if (self->event == send_event) {
                    if (!self->exception) {
                        //  prepare for stream publish
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ prepare for stream publish");
                        prepare_for_stream_publish (&self->client);
                    }
                    if (!self->exception) {
                        //  send STREAM_PUBLISH
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send STREAM_PUBLISH");
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_STREAM_PUBLISH);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
                    }
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send CONNECTION_CLOSE
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send CONNECTION_CLOSE");
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_CONNECTION_CLOSE);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
                    }
                    if (!self->exception)
                        self->state = disconnecting_state;
                }
                else
                if (self->event == stream_deliver_event) {
                    if (!self->exception) {
                        //  deliver message to application
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ deliver message to application");
                        deliver_message_to_application (&self->client);
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send CONNECTION_PING
                        if (self->verbose)
                            zsys_debug ("mlm_client:            $ send CONNECTION_PING");
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_CONNECTION_PING);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
                    }
                }
                else
                if (self->event == error_event) {
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
                        mlm_msg_set_id (self->client.msgout, MLM_MSG_CONNECTION_OPEN);
                        mlm_msg_send (&self->client.msgout, self->dealer);
                        self->client.msgout = mlm_msg_new (0);
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


//  Process message from pipe

static int
s_client_handle_pipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    while (zsock_events (self->pipe) & ZMQ_POLLIN) {
        char *method = zstr_recv (self->pipe);
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
            zsock_recv (self->pipe, "si", &self->args.endpoint, &self->args.timeout);
            s_client_execute (self, constructor_event);
            if (self->terminated)
                return -1;
        }
        else
        if (streq (method, "DESTRUCTOR")) {
            s_client_execute (self, destructor_event);
            if (self->terminated)
                return -1;
        }
        else
        if (streq (method, "PRODUCE")) {
            zstr_free (&self->args.stream);
            zsock_recv (self->pipe, "s", &self->args.stream);
            s_client_execute (self, produce_event);
            if (self->terminated)
                return -1;
        }
        else
        if (streq (method, "CONSUME")) {
            zstr_free (&self->args.stream);
            zstr_free (&self->args.pattern);
            zsock_recv (self->pipe, "ss", &self->args.stream, &self->args.pattern);
            s_client_execute (self, consume_event);
            if (self->terminated)
                return -1;
        }
        else
        if (streq (method, "SEND")) {
            zstr_free (&self->args.subject);
            zstr_free (&self->args.content);
            zsock_recv (self->pipe, "ss", &self->args.subject, &self->args.content);
            s_client_execute (self, send_event);
            if (self->terminated)
                return -1;
        }
        //  Cleanup pipe if any argument frames are still waiting to be eaten
        if (zsock_rcvmore (self->pipe)) {
            zsys_error ("mlm_client: trailing API command frames (%s)", method);
            zmsg_t *more = zmsg_recv (self->pipe);
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
        mlm_msg_t *msgin = mlm_msg_recv (self->dealer);
        if (!msgin)
            return -1;              //  Interrupted; exit zloop

        mlm_msg_destroy (&self->client.msgin);
        self->client.msgin = msgin;

        //  Any input from client counts as activity
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        //  Reset expiry timer if timeout is not zero
        if (self->timeout)
            self->expiry_timer = zloop_timer (
                self->loop, self->timeout, 1, s_client_handle_timeout, self);
        s_client_execute (self, s_protocol_event (self, msgin));
        if (self->terminated)
            return -1;
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the client actor, which polls its two sockets and processes
//  incoming messages

void
mlm_client (zsock_t *pipe, void *args)
{
    //  Initialize
    s_client_t *self = s_client_new (pipe);
    if (self) {
        zsock_signal (pipe, 0);
        
        //  Set up handler for the two main sockets the client uses
        engine_handle_socket ((client_t *) self, self->pipe, s_client_handle_pipe);
        engine_handle_socket ((client_t *) self, self->dealer, s_client_handle_protocol);

        //  Run reactor until there's a termination signal
        zloop_start (self->loop);

        //  Reactor has ended
        s_client_destroy (&self);
    }
}


//  ---------------------------------------------------------------------------
//  Class interface

struct _mlm_client_t {
    zactor_t *actor;            //  Client actor
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
    char *sender;               //  Returned by actor reply
    char *subject;              //  Returned by actor reply
    char *content;              //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new mlm_client
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful.                     

static int
mlm_client_constructor (mlm_client_t *self, const char *endpoint, int timeout);

mlm_client_t *
mlm_client_new (const char *endpoint, int timeout)
{
    mlm_client_t *self = (mlm_client_t *) zmalloc (sizeof (mlm_client_t));
    if (self) {
        self->actor = zactor_new (mlm_client, NULL);
        if (self->actor)
            self->status = mlm_client_constructor (self, endpoint, timeout);
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
        if (self->actor)
            mlm_client_destructor (self);
        zactor_destroy (&self->actor);
        zstr_free (&self->reason);
        zstr_free (&self->sender);
        zstr_free (&self->subject);
        zstr_free (&self->content);
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
//  Return actor for low-level command control and polling

zactor_t *
mlm_client_actor (mlm_client_t *self)
{
    assert (self);
    return self->actor;
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
    while (true) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            return -1;          //  Interrupted or timed-out
        
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
                else
                if (streq (reply, "MESSAGE")) {
                    zstr_free (&self->sender);
                    zstr_free (&self->subject);
                    zstr_free (&self->content);
                    zsock_recv (self->actor, "sss", &self->sender, &self->subject, &self->content);
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
}


//  ---------------------------------------------------------------------------
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful.                     
//  Returns >= 0 if successful, -1 if interrupted.

static int
mlm_client_constructor (mlm_client_t *self, const char *endpoint, int timeout)
{
    assert (self);
    zsock_send (self->actor, "ssi", "CONSTRUCTOR", endpoint, timeout);
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
//  Caller will send messages to this stream exclusively.                           
//  Returns >= 0 if successful, -1 if interrupted.

int
mlm_client_produce (mlm_client_t *self, const char *stream)
{
    assert (self);
    zsock_send (self->actor, "ss", "PRODUCE", stream);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Consume messages with a matching addresses. The pattern is a regular expression 
//  using the CZMQ zrex syntax. The most useful elements are: ^ and $ to match the  
//  start and end, . to match any character, \s and \S to match whitespace and      
//  non-whitespace, \d and \D to match a digit and non-digit, \a and \A to match    
//  alphabetic and non-alphabetic, \w and \W to match alphanumeric and              
//  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
//  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
//  Returns >= 0 if successful, -1 if interrupted.

int
mlm_client_consume (mlm_client_t *self, const char *stream, const char *pattern)
{
    assert (self);
    zsock_send (self->actor, "sss", "CONSUME", stream, pattern);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return 0;
}


//  ---------------------------------------------------------------------------
//  Send a message to the current stream. The server does not store messages. If a  
//  message is published before consumers arrive, they will miss it. Currently only 
//  supports string contents. Does not return a status value; send commands are     
//  asynchronous and unconfirmed.                                                   

int
mlm_client_send (mlm_client_t *self, const char *subject, const char *content)
{
    assert (self);
    zsock_send (self->actor, "sss", "SEND", subject, content);
    return 0;
}


//  ---------------------------------------------------------------------------
//  Receive next message from server. Returns the message content, as a string, if  
//  any. The caller should not modify or free this string.                          
//  Returns NULL on an interrupt.

char *
mlm_client_recv (mlm_client_t *self)
{
    assert (self);
    if (s_accept_reply (self, "MESSAGE", NULL))
        return NULL;            //  Interrupted or timed-out
    return self->content;
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

char *
mlm_client_reason (mlm_client_t *self)
{
    assert (self);
    return self->reason;
}


//  ---------------------------------------------------------------------------
//  Return last received sender

char *
mlm_client_sender (mlm_client_t *self)
{
    assert (self);
    return self->sender;
}


//  ---------------------------------------------------------------------------
//  Return last received subject

char *
mlm_client_subject (mlm_client_t *self)
{
    assert (self);
    return self->subject;
}


//  ---------------------------------------------------------------------------
//  Return last received content

char *
mlm_client_content (mlm_client_t *self)
{
    assert (self);
    return self->content;
}

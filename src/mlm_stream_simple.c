/*  =========================================================================
    mlm_stream_simple - simple stream engine

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    This class implements a simple stream engine and serves as a basis for
    your own, more sophisticated stream engines. Stream engines are built
    as CZMQ actors.
@discuss

@end
*/

#include "../include/czmq.h"

//  This is a simple selector class

typedef struct {
    char *pattern;              //  Regular pattern to match on
    zrex_t *rex;                //  Expression, compiled as a zrex object
    zlist_t *clients;           //  All clients that asked for this pattern
} selector_t;

static void
s_selector_destroy (selector_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        selector_t *self = *self_p;
        zrex_destroy (&self->rex);
        zlist_destroy (&self->clients);
        free (self->pattern);
        free (self);
        *self_p = NULL;
    }
}

static selector_t *
s_selector_new (void *client, const char *pattern)
{
    selector_t *self = (selector_t *) zmalloc (sizeof (selector_t));
    if (self) {
        self->rex = zrex_new (pattern);
        if (self->rex)
            self->pattern = strdup (pattern);
        if (self->pattern)
            self->clients = zlist_new ();
        if (self->clients)
            zlist_append (self->clients, client);
        else
            s_selector_destroy (&self);
    }
    return self;
}


//  --------------------------------------------------------------------------
//  The self_t structure holds the state for one actor instance

typedef struct {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    zlist_t *selectors;         //  List of selectors we hold
    zsock_t *traffic;           //  Traffic sending socket
} self_t;

static self_t *
s_self_new (zsock_t *pipe)
{
    self_t *self = (self_t *) zmalloc (sizeof (self_t));
    self->pipe = pipe;
    self->poller = zpoller_new (self->pipe, NULL);
    self->selectors = zlist_new ();
    zlist_set_destructor (self->selectors, (czmq_destructor *) s_selector_destroy);
    return self;
}

static void
s_self_destroy (self_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        self_t *self = *self_p;
        zpoller_destroy (&self->poller);
        zlist_destroy (&self->selectors);
        zsock_destroy (&self->traffic);
        free (self);
        *self_p = NULL;
    }
}

static void
s_stream_compile (self_t *self, void *client, const char *pattern)
{
    selector_t *selector = (selector_t *) zlist_first (self->selectors);
    while (selector) {
        if (streq (selector->pattern, pattern)) {
            void *client = zlist_first (selector->clients);
            while (client) {
                if (client == self)
                    break;      //  Duplicate client, ignore
                client = zlist_next (selector->clients);
            }
            //  Add client, if it's new
            if (!client)
                zlist_append (selector->clients, self);
            break;
        }
        selector = (selector_t *) zlist_next (self->selectors);
    }
    //  Add selector, if it's new
    if (!selector)
        zlist_append (self->selectors, s_selector_new (client, pattern));
}


static void
s_stream_accept (self_t *self, void *client, char *sender, char *subject, zmsg_t *content)
{
    selector_t *selector = (selector_t *) zlist_first (self->selectors);
    while (selector) {
        if (zrex_matches (selector->rex, subject)) {
            void *recipient = zlist_first (selector->clients);
            while (recipient) {
                if (recipient != client)
                    zsock_send (self->traffic, "pssm", recipient, sender, subject, content);
                recipient = zlist_next (selector->clients);
            }
        }
        selector = (selector_t *) zlist_next (self->selectors);
    }
}


static void
s_stream_cancel (self_t *self, void *client)
{
    selector_t *selector = (selector_t *) zlist_first (self->selectors);
    while (selector) {
        zlist_remove (selector->clients, client);
        selector = (selector_t *) zlist_next (self->selectors);
    }
}


//  --------------------------------------------------------------------------
//  Here we implement the stream API

static int
s_self_handle_pipe (self_t *self)
{
    char *method = zstr_recv (self->pipe);
    if (!method)
        return -1;              //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("mlm_stream_simple: API command=%s", method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;       //  Start verbose logging
    else
    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "TRAFFIC")) {
        char *endpoint;
        zsock_recv (self->pipe, "s", &endpoint);
        if (!self->traffic)
            self->traffic = zsock_new_push (endpoint);
        zstr_free (&endpoint);
    }
    if (streq (method, "COMPILE")) {
        void *client;
        char *pattern;
        zsock_recv (self->pipe, "ps", &client, &pattern);
        s_stream_compile (self, client, pattern);
        zstr_free (&pattern);
    }
    else
    if (streq (method, "ACCEPT")) {
        void *client;
        char *sender, *subject;
        zmsg_t *content;
        zsock_recv (self->pipe, "pssm", &client, &sender, &subject, &content);
        s_stream_accept (self, client, sender, subject, content);
        zstr_free (&sender);
        zstr_free (&subject);
        zmsg_destroy (&content);
    }
    else
    if (streq (method, "CANCEL")) {
        void *client;
        zsock_recv (self->pipe, "p", &client);
        s_stream_cancel (self, client);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->pipe)) {
        zsys_error ("mlm_stream_simple: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->pipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  --------------------------------------------------------------------------
//  This method implements the mlm_stream_simple actor interface

void
mlm_stream_simple (zsock_t *pipe, void *args)
{
    self_t *self = s_self_new (pipe);
    //  Signal successful initialization
    zsock_signal (pipe, 0);

    while (!self->terminated) {
        zsock_t *which = zpoller_wait (self->poller, -1);
        if (which == self->pipe)
            s_self_handle_pipe (self);
        else
        if (zpoller_terminated (self->poller))
            break;          //  Interrupted
    }
    s_self_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Selftest

void
mlm_stream_simple_test (bool verbose)
{
    printf (" * mlm_stream_simple: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *stream = zactor_new (mlm_stream_simple, NULL);
    assert (stream);
    if (verbose)
        zstr_sendx (stream, "VERBOSE", NULL);

    zactor_destroy (&stream);
    //  @end
    printf ("OK\n");
}

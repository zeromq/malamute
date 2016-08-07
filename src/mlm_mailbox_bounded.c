/*  =========================================================================
    mlm_mailbox_bounded - Simple bounded mailbox engine

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    This class implements a simple bounded mailbox engine. It is built
    as CZMQ actor.
@discuss
@end
*/

#include "mlm_classes.h"

typedef struct {
    zlistx_t *queue;
    size_t mailbox_size;    // size of queue in bytes
} mailbox_t;

static void
s_mailbox_destroy (mailbox_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mailbox_t *self = *self_p;
        zlistx_destroy (&self->queue);
        free (self);
        *self_p = NULL;
    }
}

static mailbox_t *
s_mailbox_new ()
{
    mailbox_t *self = (mailbox_t *) zmalloc (sizeof (mailbox_t));
    if (self) {
        self->queue = zlistx_new ();
        if (self->queue)
            zlistx_set_destructor (self->queue, (czmq_destructor *) mlm_msg_destroy);
        else
            s_mailbox_destroy (&self);
    }
    return self;
}

static void
s_mailbox_enqueue (mailbox_t *self, mlm_msg_t *msg)
{
    zlistx_add_end (self->queue, msg);
    const size_t msg_content_size =
        zmsg_content_size (mlm_msg_content (msg));
    self->mailbox_size += msg_content_size;
}

static mlm_msg_t *
s_mailbox_dequeue (mailbox_t *self)
{
    mlm_msg_t *msg = (mlm_msg_t *) zlistx_detach (self->queue, NULL);
    if (msg) {
        const size_t msg_content_size =
             zmsg_content_size (mlm_msg_content (msg));
        self->mailbox_size -= msg_content_size;
    }
    return msg;
}


//  --------------------------------------------------------------------------
//  The self_t structure holds the state for one actor instance

typedef struct {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    size_t mailbox_size_limit;  //  Limit on memory the mailbox can use
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    zhashx_t *mailboxes;        //  Mailboxes as queues of messages
} self_t;

static void
s_self_destroy (self_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        self_t *self = *self_p;
        zpoller_destroy (&self->poller);
        zhashx_destroy (&self->mailboxes);
        free (self);
        *self_p = NULL;
    }
}

static self_t *
s_self_new (zsock_t *pipe, size_t mailbox_size_limit)
{
    self_t *self = (self_t *) zmalloc (sizeof (self_t));
    if (self) {
        self->pipe = pipe;
        self->mailbox_size_limit = mailbox_size_limit;
        self->poller = zpoller_new (self->pipe, NULL);
        if (self->poller)
            self->mailboxes = zhashx_new ();
        if (self->mailboxes)
            zhashx_set_destructor (self->mailboxes, (czmq_destructor *) s_mailbox_destroy);
        else
            s_self_destroy (&self);
    }
    return self;
}


//  --------------------------------------------------------------------------
//  The function converts mailbox size limit from string into size_t value.
//  The string "max" translates into maximum size_t value.

static bool
s_scan_mailbox_size_limit (self_t *self, const char *input)
{
    if (streq (input, "max"))
        self->mailbox_size_limit = (size_t) -1;
    else {
        int bytes_scanned = 0;
        const int n =
            sscanf (input, "%zu%n", &self->mailbox_size_limit, &bytes_scanned);
        if (n == 0 || bytes_scanned < strlen (input))
            return false;
    }
    return true;
}


//  --------------------------------------------------------------------------
//  Here we implement the mailbox API

static int
s_self_handle_command (self_t *self)
{
    char *method = zstr_recv (self->pipe);
    if (!method)
        return -1;              //  Interrupted; exit zloop
    if (self->verbose)
        zsys_debug ("mlm_mailbox_bounded: API command=%s", method);

    if (streq (method, "VERBOSE"))
        self->verbose = true;       //  Start verbose logging
    else
    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "MAILBOX-SIZE-LIMIT")) {
        char *str;
        zsock_recv (self->pipe, "s", &str);
        if (s_scan_mailbox_size_limit (self, str)) {
            if (self->verbose)
                zsys_debug ("mlm_mailbox_bounded: mailbox size limit set to %zu bytes",
                        self->mailbox_size_limit);
        }
        else {
            if (self->verbose)
                zsys_debug ("mlm_mailbox_bounded: invalid mailbox size limit (%s)", str);
        }
        zstr_free (&str);
    }
    if (streq (method, "STORE")) {
        char *address;
        mlm_msg_t *msg;
        zsock_recv (self->pipe, "sp", &address, &msg);
        mailbox_t *mailbox = (mailbox_t *) zhashx_lookup (self->mailboxes, address);
        if (!mailbox) {
            mailbox = s_mailbox_new ();
            zhashx_insert (self->mailboxes, address, mailbox);
        }
        const size_t msg_size =
            zmsg_content_size (mlm_msg_content (msg));
        if (mailbox->mailbox_size + msg_size <= self->mailbox_size_limit)
            s_mailbox_enqueue (mailbox, msg);
        else
            mlm_msg_unlink (&msg);
        zstr_free (&address);
    }
    else
    if (streq (method, "QUERY")) {
        char *address;
        mlm_msg_t *msg = NULL;
        zsock_recv (self->pipe, "s", &address);
        mailbox_t *mailbox = (mailbox_t *) zhashx_lookup (self->mailboxes, address);
        if (mailbox && mailbox->mailbox_size > 0)
            msg = s_mailbox_dequeue (mailbox);
        zsock_send (self->pipe, "p", msg);
        zstr_free (&address);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->pipe)) {
        zsys_error ("mlm_mailbox_bounded: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->pipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  --------------------------------------------------------------------------
//  This method implements the mlm_mailbox actor interface

void
mlm_mailbox_bounded (zsock_t *pipe, void *args)
{
    self_t *self = s_self_new (pipe, (size_t) -1);
    //  Signal successful initialization
    zsock_signal (pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, -1);
        if (which == self->pipe)
            s_self_handle_command (self);
        else
        if (zpoller_terminated (self->poller))
            break;          //  Interrupted
    }
    s_self_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Selftest

void
mlm_mailbox_bounded_test (bool verbose)
{
    printf (" * mlm_mailbox_bounded: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *mailbox = zactor_new (mlm_mailbox_bounded, NULL);
    assert (mailbox);
    if (verbose)
        zstr_sendx (mailbox, "VERBOSE", NULL);
    zsock_send (mailbox, "si", "MAILBOX-SIZE-LIMIT", 1024);

    zactor_destroy (&mailbox);
    //  @end
    printf ("OK\n");
}

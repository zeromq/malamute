/*  =========================================================================
    mlm_msg - Message held by server

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*
@header
    mlm_msg - Message held by server
@discuss
@end
*/

#include "mlm_classes.h"

//  Structure of our class

struct _mlm_msg_t {
    char *sender;               //  Originating client
    char *address;              //  Message address or service
    char *subject;              //  Message subject
    char *tracker;              //  Message tracker
    int64_t expiry;             //  Message expiry time
    zmsg_t *content;            //  Message content
    void *refcount;             //  Number of instances
};

//  --------------------------------------------------------------------------
//  Create a new mlm_msg; takes ownership of content, which the caller should
//  not use after this call.

mlm_msg_t *
mlm_msg_new (
    const char *sender, const char *address, const char *subject,
    const char *tracker, uint timeout, zmsg_t *content)
{
    assert (sender);
    mlm_msg_t *self = (mlm_msg_t *) zmalloc (sizeof (mlm_msg_t));
    if (self) {
        self->sender = sender? strdup (sender): NULL;
        self->address = address? strdup (address): NULL;
        self->subject = subject? strdup (subject): NULL;
        self->tracker = tracker? strdup (tracker): NULL;
        self->expiry = zclock_time () + timeout;
        self->content = content;
        self->refcount = zmq_atomic_counter_new ();
        zmq_atomic_counter_set (self->refcount, 1);
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the mlm_msg

void
mlm_msg_destroy (mlm_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mlm_msg_t *self = *self_p;
        free (self->sender);
        free (self->address);
        free (self->subject);
        free (self->tracker);
        zmsg_destroy (&self->content);
        zmq_atomic_counter_destroy (&self->refcount);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Return message subject

char *
mlm_msg_subject (mlm_msg_t *self)
{
    return self->subject;
}


//  --------------------------------------------------------------------------
//  Return message address

char *
mlm_msg_address (mlm_msg_t *self)
{
    return self->address;
}


//  --------------------------------------------------------------------------
//  Return message content

zmsg_t *
mlm_msg_content (mlm_msg_t *self)
{
    return self->content;
}


//  --------------------------------------------------------------------------
//  Store message into mlm_proto object

void
mlm_msg_set_proto (mlm_msg_t *self, mlm_proto_t *proto)
{
    if (self->sender)
        mlm_proto_set_sender  (proto, self->sender);
    if (self->address)
        mlm_proto_set_address (proto, self->address);
    if (self->subject)
        mlm_proto_set_subject (proto, self->subject);
    if (self->content) {
        //  Duplicate content since we may send to multiple clients
        zmsg_t *content = zmsg_dup (self->content);
        mlm_proto_set_content (proto, &content);
    }
}


//  --------------------------------------------------------------------------
//  Get reference-counted copy of message

mlm_msg_t *
mlm_msg_link (mlm_msg_t *self)
{
    assert (self);
    zmq_atomic_counter_inc (self->refcount);
    return self;
}


//  --------------------------------------------------------------------------
//  Drop reference to message

void
mlm_msg_unlink (mlm_msg_t **self_p)
{
    assert (self_p);
    mlm_msg_t *self = *self_p;
    assert (self);
    
    if (!zmq_atomic_counter_dec (self->refcount))
        mlm_msg_destroy (&self);
    *self_p = NULL;
}


//  --------------------------------------------------------------------------
//  Selftest

int
mlm_msg_test (bool verbose)
{
    printf (" * mlm_msg: ");

    //  @selftest
    //  Simple create/destroy test
    mlm_msg_t *self = mlm_msg_new (
        "sender", "address", "subject", "tracker", 0, zmsg_new ());
    assert (self);
    mlm_msg_destroy (&self);

    //  Test reference counting
    self = mlm_msg_new (
        "sender", "address", "subject", "tracker", 0, zmsg_new ());
    mlm_msg_t *copy = mlm_msg_link (self);
    mlm_msg_unlink (&copy);
    mlm_msg_unlink (&self);
    //  @end
    printf ("OK\n");
    return 0;
}

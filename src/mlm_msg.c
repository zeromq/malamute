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
};

//  --------------------------------------------------------------------------
//  Create a new mlm_msg

mlm_msg_t *
mlm_msg_new (
    const char *sender, const char *address, const char *subject,
    const char *tracker, uint timeout, zmsg_t **content_p)
{
    mlm_msg_t *self = (mlm_msg_t *) zmalloc (sizeof (mlm_msg_t));
    if (self) {
        self->sender = strdup (sender);
        self->address = strdup (address);
        self->subject = strdup (subject);
        self->tracker = strdup (tracker);
        self->expiry = zclock_time () + timeout;
        self->content = *content_p;
        *content_p = NULL;
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
//  Store message into mlm_proto object

void
mlm_msg_set_proto (mlm_msg_t *self, mlm_proto_t *proto)
{
    mlm_proto_set_sender  (proto, self->sender);
    mlm_proto_set_address (proto, self->address);
    mlm_proto_set_subject (proto, self->subject);
    mlm_proto_set_content (proto, &self->content);
}


//  --------------------------------------------------------------------------
//  Selftest

int
mlm_msg_test (bool verbose)
{
    printf (" * mlm_msg: ");

    //  @selftest
    //  Simple create/destroy test
    zmsg_t *content = zmsg_new ();
    mlm_msg_t *self = mlm_msg_new ("sender", "address", "subject", "tracker", 0, &content);
    assert (self);
    assert (content == NULL);
    mlm_msg_destroy (&self);
    //  @end
    printf ("OK\n");
    return 0;
}

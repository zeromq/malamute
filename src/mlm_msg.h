/*  =========================================================================
    mlm_msg - Message held by server

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MLM_MSG_H_INCLUDED__
#define __MLM_MSG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new mlm_msg; takes ownership of content, which the caller should
//  not use after this call.
mlm_msg_t *
    mlm_msg_new (const char *sender, const char *address, const char *subject,
                 const char *tracker, uint timeout, zmsg_t *content);

//  Destroy the mlm_msg
void
    mlm_msg_destroy (mlm_msg_t **self_p);

//  Return message subject
char *
    mlm_msg_subject (mlm_msg_t *self);

//  Return message address
char *
    mlm_msg_address (mlm_msg_t *self);

//  Return message content
zmsg_t *
    mlm_msg_content (mlm_msg_t *self);

//  Store message into mlm_proto object
void
    mlm_msg_set_proto (mlm_msg_t *self, mlm_proto_t *proto);

//  Get reference-counted copy of message
mlm_msg_t *
    mlm_msg_link (mlm_msg_t *self);

//  Drop reference to message
void
    mlm_msg_unlink (mlm_msg_t **self_p);

//  Self test of this class
MLM_EXPORT void
    mlm_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

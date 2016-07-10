/*  =========================================================================
    mlm_mailbox_bounded - Simple bounded mailbox engine

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef MLM_MAILBOX_BOUNDED_H_INCLUDED
#define MLM_MAILBOX_BOUNDED_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  To work with mlm_mailbox_bounded, use the mailbox command API:
//
//  Create new mlm_mailbox_bounded actor instance, passing mailbox size limit:
//
//      zactor_t *mlm_mailbox_bounded_actor = zactor_new (mlm_mailbox_bounded, "myname");
//
//  Destroy mlm_mailbox_bounded actor instance:
//
//      zactor_destroy (&mlm_mailbox_bounded_actor);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (mlm_mailbox_bounded_actor, "VERBOSE");
//
//  Set the mailbox size limit to 65536 messages:
//
//      zstr_send (mlm_mailbox_bounded_actor, "MAILBOX-SIZE-LIMIT", 65536);
//
MLM_EXPORT void
    mlm_mailbox_bounded (zsock_t *pipe, void *args);

//  Self test of this class
MLM_EXPORT void
    mlm_mailbox_bounded_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

/*  =========================================================================
    mlm_msgq - Message queue implementation

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef MLM_MSGQ_H_INCLUDED
#define MLM_MSGQ_H_INCLUDED

typedef struct _mlm_msgq_cfg_t mlm_msgq_cfg_t;

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new mlm_msgq
MLM_PRIVATE mlm_msgq_t *
    mlm_msgq_new (void);

//  Destroy the mlm_msgq
MLM_PRIVATE void
    mlm_msgq_destroy (mlm_msgq_t **self_p);


// Pass a pointer to a mlm_msgq_cfg_t instance holding the message limits.
// Caller must keep the instance until the queue is destroyed
MLM_PRIVATE void
    mlm_msgq_set_cfg (mlm_msgq_t *self, const mlm_msgq_cfg_t *cfg);

// Enqueue a message. The message is dropped if it does not fit into the
// size limit.
MLM_PRIVATE void
    mlm_msgq_enqueue (mlm_msgq_t *self, mlm_msg_t *msg);

// Remove the first message from the queue and return it.
MLM_PRIVATE mlm_msg_t *
    mlm_msgq_dequeue (mlm_msgq_t *self);

// Remove the message pointed at by the cursor and return it.
MLM_PRIVATE mlm_msg_t *
    mlm_msgq_dequeue_cursor (mlm_msgq_t *self);

// zlistx_first on the underlying message list
MLM_PRIVATE mlm_msg_t *
    mlm_msgq_first (mlm_msgq_t *self);

// zlistx_next on the underlying message list
MLM_PRIVATE mlm_msg_t *
    mlm_msgq_next (mlm_msgq_t *self);

// Create a configuration object
MLM_PRIVATE mlm_msgq_cfg_t *
    mlm_msgq_cfg_new (const char *config_path);

// Destroy a configuration object
MLM_PRIVATE void
    mlm_msgq_cfg_destroy (mlm_msgq_cfg_t **self_p);

// Load configuration from passed zconfig_t object
MLM_PRIVATE void
    mlm_msgq_cfg_configure (mlm_msgq_cfg_t *self, zconfig_t *config);

//  @end

#ifdef __cplusplus
}
#endif

#endif

/*  =========================================================================
    zbits - work with bitmaps

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of CZMQ, the high-level C binding for 0MQ:
    http://czmq.zeromq.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef __ZBITS_H_INCLUDED__
#define __ZBITS_H_INCLUDED__

typedef struct _zbits_t zbits_t;

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

//  Create a new bitmap of all zeros
zbits_t *
    zbits_new (size_t bits);

//  Destroy a bitmap
void
    zbits_destroy (zbits_t **self_p);

//  Return 1 if specified bit is set, else return 0.
int
    zbits_get (zbits_t *self, uint index);

//  Sets the specified bit in the bitmap.
void
    zbits_set (zbits_t *self, uint index);

//  Clears the specified bit in the bitmap.
void
    zbits_clear (zbits_t *self, uint index);
    
//  Resets all bits to zero.
void
    zbits_reset (zbits_t *self);

//  Count the number of bits set to 1.
int
    zbits_ones (zbits_t *self);

//  AND a bit string into the current bit string.
void
    zbits_and (zbits_t *self, zbits_t *source);

//  AND a bit string into the current bit string.
void
    zbits_or (zbits_t *self, zbits_t *source);

//  XOR a bit string into the current bit string.
void
    zbits_xor (zbits_t *self, zbits_t *source);

//  NOT the current bit string.
void
    zbits_not (zbits_t *self);

//  Look for the first bit that is set to 1. Returns -1 if no bit was
//  set in the entire bit string.
int
    zbits_first (zbits_t *self);

//  Look for the last bit that is set to 1. Returns -1 if no bit was
//  set in the entire bit string.
int
    zbits_last (zbits_t *self);

//  Look for the next bit that is set to 1. Returns -1 if no matching
//  bit was found.
int
    zbits_next (zbits_t *self);

//  Look for the previous bit that is set to 1. Returns -1 if no
//  matching bit was found.
int
    zbits_prev (zbits_t *self);

//  Look for the first bit that is set to 0. Returns -1 if no bit was
//  clear in the entire bit string.
int
    zbits_first_zero (zbits_t *self);

//  Look for the next bit that is set to 0. Returns -1 if no matching
//  bit was found.
int
    zbits_next_zero (zbits_t *self);

//  Look for the previous bit that is set to 0. Returns -1 if no
//  matching bit was found.
int
    zbits_prev_zero (zbits_t *self);

//  Sets the first zero bit and return that index. If there were no zero
//  bits available, returns -1.
int
    zbits_insert (zbits_t *self);

//  Writes the bitmap to the specified file stream. To read the bitmap,
//  use zbits_fget(). Returns 0 if OK, -1 on failure.
int
    zbits_fput (zbits_t *self, FILE *file);

//  Reads a bitmap from the specified stream. You must have previously
//  written the bitmap using zbits_fput(). Overwrites the current bitmap
//  with it.
int
    zbits_fget (zbits_t *self, FILE *file);

//  Self test of this class
MLM_EXPORT void
    zbits_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif


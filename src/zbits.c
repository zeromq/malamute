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

/*
@header
    A zbits object provides a large bitmap and operations on it. We do not
    compress bitmaps. This model is aimed at speed, not memory efficiency.
    To give control over memory usage, bitmaps have a configurable size, set
    at construction time.
@discuss
    This model comes from the topic matching engine in OpenAMQ (2007), and
    was proven in large-scale deployments. The code is derived from the iMatix
    Portable Runtime (IPR):
    https://github.com/imatix/openamq/tree/master/tooling/base2/ipr.
@end
*/

// #include <czmq.h>
// #include "zbits.h"
#include "mlm_classes.h"

struct _zbits_t {
    byte *data;                             //  Pointer into data block
    size_t bits;                            //  Bitmap size in bits
    size_t size;                            //  Bitmap size in bytes
    size_t used;                            //  Number of bytes actually used
    uint cursor;                            //  Cursor for first/next
};

//  How many bits are set in a given number
static byte
s_bit_count [256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

//  Index of first set bit in byte, the first bit off in byte is 
//  at s_first_bit_on [255 - byte]
static char
s_first_bit_on [256] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

//  Index of last set bit in byte, the last bit off in byte is
//  at s_last_bit_on [255 - byte]
static char
s_last_bit_on [256] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

//  Lookup for value of byte when the nth bit is set
static byte
s_bit_set [8] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

//  Lookup for value of byte when the nth bit is cleared
static byte
s_bit_clear [8] = {
    0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F
};


//  --------------------------------------------------------------------------
//  Create a new bitmap of all zeros. The bitmap size is a number of bits and
//  must be divisible by 64.

zbits_t *
zbits_new (size_t bits)
{
    assert (bits % 64 == 0);

    zbits_t *self = (zbits_t *) zmalloc (sizeof (zbits_t) + bits / 8);
    if (self) {
        self->data = (byte *) self + sizeof (zbits_t);
        self->bits = bits;
        self->size = bits / 8;
        self->used = 0;
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy a bitmap

void
zbits_destroy (zbits_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zbits_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Return 1 if specified bit is set, else return 0.

int
zbits_get (zbits_t *self, uint index)
{
    assert (index < self->bits);
    uint byte_nbr = index / 8;
    uint bit_nbr = index % 8;
    return (self->data [byte_nbr] & s_bit_set [bit_nbr]) != 0;
}

//  --------------------------------------------------------------------------
//  Sets the specified bit in the bitmap.

void
zbits_set (zbits_t *self, uint index)
{
    assert (index < self->bits);
    uint byte_nbr = index / 8;
    uint bit_nbr = index % 8;
    self->data [byte_nbr] |= s_bit_set [bit_nbr];
    if (self->used < byte_nbr + 1)
        self->used = byte_nbr + 1;
}


//  --------------------------------------------------------------------------
//  Clears the specified bit in the bitmap.

void
zbits_clear (zbits_t *self, uint index)
{
    assert (index < self->bits);
    uint byte_nbr = index / 8;
    uint bit_nbr = index % 8;
    self->data [byte_nbr] &= s_bit_clear [bit_nbr];
}


//  --------------------------------------------------------------------------
//  Resets all bits to zero.

void
zbits_erase (zbits_t *self)
{
    self->used = 0;
    memset ((void *) self->data, 0, self->size);
}


//  --------------------------------------------------------------------------
//  Count the number of bits set to 1.

int
zbits_ones (zbits_t *self)
{
    assert (self->used <= self->size);
    uint cur_size;
    int bits_set = 0;
    for (cur_size = 0; cur_size < self->used; cur_size++)
        bits_set += s_bit_count [self->data [cur_size]];
    
    return bits_set;
}


//  --------------------------------------------------------------------------
//  AND a bit string into the current bit string.

void
zbits_and (zbits_t *self, zbits_t *source)
{
    assert (source);
    assert (source->used <= self->size);
    
    uint data_size = min (self->used, source->used);
    uint cur_size;
    for (cur_size = 0; cur_size < data_size; cur_size += sizeof (int64_t))
        *(int64_t *) (self->data + cur_size) &= *(int64_t *) (source->data + cur_size);

    memset (self->data + cur_size, 0, self->size - cur_size);
}


//  --------------------------------------------------------------------------
//  AND a bit string into the current bit string.

void
zbits_or (zbits_t *self, zbits_t *source)
{
    assert (source);
    assert (source->used <= self->size);

    uint data_size = max (self->used, source->used);
    uint cur_size;
    for (cur_size = 0; cur_size < data_size; cur_size += sizeof (int64_t))
        *(int64_t *) (self->data + cur_size) |= *(int64_t *) (source->data + cur_size);

    self->used = data_size;
}


//  --------------------------------------------------------------------------
//  XOR a bit string into the current bit string.

void
zbits_xor (zbits_t *self, zbits_t *source)
{
    assert (source);
    assert (source->used <= self->size);

    uint data_size = max (self->used, source->used);
    uint cur_size;
    for (cur_size = 0; cur_size < data_size; cur_size += sizeof (int64_t))
        *(int64_t *) (self->data + cur_size) ^= *(int64_t *) (source->data + cur_size);

    self->used = data_size;
}


//  --------------------------------------------------------------------------
//  NOT the current bit string.

void
zbits_not (zbits_t *self)
{
    assert (self);
    assert (self->used <= self->size);
    
    uint cur_size;
    for (cur_size = 0; cur_size < self->used; cur_size += sizeof (int64_t))
        *(int64_t *) (self->data + cur_size) = ~(*(int64_t *) (self->data + cur_size));

    //  Set the rest of the bitmap to 1
    memset (self->data + cur_size, 0xFF, self->size - cur_size);
    self->used = self->size;
}


//  --------------------------------------------------------------------------
//  Look for the first bit that is set to 1. Returns -1 if no bit was
//  set in the entire bit string.

int
zbits_first (zbits_t *self)
{
    assert (self);
    self->cursor = 0;
    if (zbits_get (self, self->cursor))
        return self->cursor;
    else
        return zbits_next (self);
}


//  --------------------------------------------------------------------------
//  Look for the last bit that is set to 1. Returns -1 if no bit was
//  set in the entire bit string.

int
zbits_last (zbits_t *self)
{
    assert (self);
    self->cursor = self->bits - 1;
    if (zbits_get (self, self->cursor))
        return 0;
    else
        return zbits_prev (self);
}


//  --------------------------------------------------------------------------
//  Look for the next bit that is set to 1. Returns -1 if no matching
//  bit was found.

int
zbits_next (zbits_t *self)
{
    assert (self);
    //  Mask clears all bits lower than current bit index 0-7
    static byte clear_trailing_bits [8] = {
        254, 252, 248, 240, 224, 192, 128, 0
    };
    uint byte_nbr = self->cursor / 8;
    uint bit_nbr = self->cursor % 8;

    byte cur_byte = self->data [byte_nbr] & clear_trailing_bits [bit_nbr];
    while (cur_byte == 0 && byte_nbr < self->used)
        cur_byte = self->data [++byte_nbr];

    if (cur_byte > 0) {
        self->cursor = byte_nbr * 8 + s_first_bit_on [cur_byte];
        return self->cursor;
    }
    else
        return -1;
}


//  --------------------------------------------------------------------------
//  Look for the previous bit that is set to 1. Returns -1 if no
//  matching bit was found.

int
zbits_prev (zbits_t *self)
{
    assert (self);
    //  Mask clears all bits higher than current bit index 0-7
    static byte clear_leading_bits [8] = {
        0, 1, 3, 7, 15, 31, 63, 127
    };
    uint byte_nbr = self->cursor / 8;
    uint bit_nbr = self->cursor % 8;

    byte cur_byte = self->data [byte_nbr] & clear_leading_bits [bit_nbr];
    while (cur_byte == 0 && byte_nbr > 0)
        cur_byte = self->data [--byte_nbr];

    if (cur_byte > 0) {
        self->cursor = byte_nbr * 8 + s_last_bit_on [cur_byte];
        return self->cursor;
    }
    else
        return -1;
}


//  --------------------------------------------------------------------------
//  Look for the first bit that is set to 0. Returns -1 if no bit was
//  clear in the entire bit string.

int
zbits_first_zero (zbits_t *self)
{
    assert (self);
    self->cursor = 0;
    if (zbits_get (self, self->cursor) == 0)
        return self->cursor;
    else
        return zbits_next_zero (self);
}


//  --------------------------------------------------------------------------
//  Look for the next bit that is set to 0. Returns -1 if no matching
//  bit was found.

int
zbits_next_zero (zbits_t *self)
{
    assert (self);
    //  Mask sets all bits lower than current bit index 0-7
    static byte set_trailing_bits [8] = {
        0, 1, 3, 7, 15, 31, 63, 127
    };
    uint byte_nbr = self->cursor / 8;
    uint bit_nbr = self->cursor % 8;

    byte cur_byte = self->data [byte_nbr] | set_trailing_bits [bit_nbr];
    while (cur_byte == 255 && byte_nbr < self->used)
        cur_byte = self->data [++byte_nbr];

    if (cur_byte < 255) {
        self->cursor = byte_nbr * 8 + s_first_bit_on [255 - cur_byte];
        return self->cursor;
    }
    else
        return -1;
}


//  --------------------------------------------------------------------------
//  Look for the previous bit that is set to 0. Returns -1 if no
//  matching bit was found.

int
zbits_prev_zero (zbits_t *self)
{
    assert (self);
    //  Mask sets all bits higher than current bit index 0-7
    static byte set_leading_bits [8] = {
        254, 252, 248, 240, 224, 192, 128, 0
    };
    uint byte_nbr = self->cursor / 8;
    uint bit_nbr = self->cursor % 8;

    byte cur_byte = self->data [byte_nbr] | set_leading_bits [bit_nbr];
    while (cur_byte == 255 && byte_nbr < self->used)
        cur_byte = self->data [--byte_nbr];

    if (cur_byte < 255) {
        self->cursor = byte_nbr * 8 + s_last_bit_on [255 - cur_byte];
        return self->cursor;
    }
    else
        return -1;
}


//  --------------------------------------------------------------------------
//  Sets the first zero bit and return that index. If there were no zero
//  bits available, returns -1.

int
zbits_insert (zbits_t *self)
{
    int bit = zbits_first_zero (self);
    if (bit >= 0)
        zbits_set (self, bit);
    return bit;
}


//  --------------------------------------------------------------------------
//  Writes the bitmap to the specified file stream. To read the bitmap,
//  use zbits_fget(). Returns 0 if OK, -1 on failure.

int
zbits_fput (zbits_t *self, FILE *file)
{
    qbyte net_size = htonl (self->used);
    if (fwrite (&net_size, sizeof (net_size), 1, file)
    &&  fwrite (self->data, sizeof (self->data), 1, file))
        return 0;
    else
        return -1;
}


//  --------------------------------------------------------------------------
//  Reads a bitmap from the specified stream. You must have previously
//  written the bitmap using zbits_fput(). Overwrites the current bitmap
//  with it.

int
zbits_fget (zbits_t *self, FILE *file)
{
    qbyte net_size;
    if (fread (&net_size, sizeof (net_size), 1, file) == 1
    &&  fread (self->data, sizeof (self->data), 1, file) == 1) {
        self->used = ntohl (net_size);
        return 0;
    }
    else
        return -1;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zbits_test (bool verbose)
{
    printf (" * zbits: ");

    //  @selftest
    zbits_t *bits = zbits_new (1024);
    zbits_set (bits, 9);
    zbits_set (bits, 10);
    int bit = zbits_first (bits);
    assert (bit == 9);
    bit = zbits_next (bits);
    assert (bit == 10);
    bit = zbits_next (bits);
    assert (bit == -1);
    zbits_destroy (&bits);

    //  Fuller test
    bits = zbits_new (1024);
    int count;
    for (count = 0; count < 300; count++)
        zbits_set (bits, count);

    bit = zbits_first (bits);
    for (count = 0; count < 300; count++) {
        assert (bit == count);
        bit = zbits_next (bits);
    }
    zbits_destroy (&bits);

    bits = zbits_new (1024);
    zbits_set (bits, 0);
    zbits_set (bits, 1);
    zbits_set (bits, 13);
    zbits_set (bits, 26);
    zbits_set (bits, 39);
    zbits_set (bits, 52);
    assert (zbits_get (bits, 0));
    assert (zbits_get (bits, 1));
    assert (zbits_get (bits, 13));
    assert (zbits_get (bits, 26));
    assert (zbits_get (bits, 39));
    assert (zbits_get (bits, 52));
    assert (zbits_get (bits, 51) == 0);
    assert (zbits_get (bits, 53) == 0);
    bit = zbits_first (bits);
    assert (bit == 0);
    bit = zbits_next (bits);
    assert (bit == 1);
    bit = zbits_next (bits);
    assert (bit == 13);
    bit = zbits_next (bits);
    assert (bit == 26);
    bit = zbits_next (bits);
    assert (bit == 39);
    bit = zbits_next (bits);
    assert (bit == 52);
    bit = zbits_prev (bits);
    assert (bit == 39);
    bit = zbits_prev (bits);
    assert (bit == 26);
    bit = zbits_prev (bits);
    assert (bit == 13);
    bit = zbits_prev (bits);
    assert (bit == 1);
    bit = zbits_prev (bits);
    assert (bit == 0);
    
    bit = zbits_first_zero (bits);
    assert (bit == 2);

    zbits_t *bits2 = zbits_new (1024);
    zbits_set (bits2, 26);
    zbits_set (bits2, 52);
    zbits_set (bits2, 99);
    zbits_clear (bits2, 99);

    zbits_and (bits2, bits);
    bit = zbits_first (bits2);
    assert (bit == 26);
    bit = zbits_next (bits2);
    assert (bit == 52);

    zbits_or (bits2, bits);
    bit = zbits_first (bits2);
    assert (bit == 0);
    bit = zbits_next (bits2);
    assert (bit == 1);
    bit = zbits_next (bits2);
    assert (bit == 13);
    bit = zbits_next (bits2);
    assert (bit == 26);
    bit = zbits_next (bits2);
    assert (bit == 39);
    bit = zbits_next (bits2);
    assert (bit == 52);
    
    bit = zbits_prev (bits2);
    assert (bit == 39);
    bit = zbits_prev (bits2);
    assert (bit == 26);
    bit = zbits_prev (bits2);
    assert (bit == 13);
    bit = zbits_prev (bits2);
    assert (bit == 1);
    bit = zbits_prev (bits2);
    assert (bit == 0);
    bit = zbits_prev (bits2);
    assert (bit == -1);

    zbits_clear (bits2, 26);
    zbits_clear (bits2, 52);
    zbits_xor (bits2, bits);
    bit = zbits_first (bits2);
    assert (bit == 26);
    bit = zbits_next (bits2);
    assert (bit == 52);

    bit = zbits_insert (bits);
    assert (bit == 2);
    
    zbits_erase (bits);
    zbits_destroy (&bits);
    zbits_destroy (&bits2);
    //  @end

    printf ("OK\n");
}

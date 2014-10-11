/*  =========================================================================
    mlm_selftest - run self tests

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "mlm_classes.h"

int main (int argc, char *argv [])
{
    bool verbose;
    if (argc == 2 && streq (argv [1], "-v"))
        verbose = true;
    else
        verbose = false;

    printf ("Running self tests...\n");
    mlm_msg_test (verbose);
    printf ("Tests passed OK\n");
    return 0;
}

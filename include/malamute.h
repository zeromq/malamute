/*  =========================================================================
    malamute - Malamute public API

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute project.
    
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================*/

#ifndef __MALAMUTE_H_INCLUDED__
#define __MALAMUTE_H_INCLUDED__

//  Version macros for compile-time API detection

#define MALAMUTE_VERSION_MAJOR 0
#define MALAMUTE_VERSION_MINOR 0
#define MALAMUTE_VERSION_PATCH 1

#define MALAMUTE_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define MALAMUTE_VERSION \
    MALAMUTE_MAKE_VERSION(MALAMUTE_VERSION_MAJOR, MALAMUTE_VERSION_MINOR, MALAMUTE_VERSION_PATCH)

#include <czmq.h>
#if CZMQ_VERSION < 30000
#   error "Malamute needs CZMQ/3.0.0 or later"
#endif

#include <zyre.h>
#if ZYRE_VERSION < 10100
#   error "Malamute needs Zyre/1.1.0 or later"
#endif

//  Public API
#include "mlm_msg.h"
#include "mlm_server.h"

#endif

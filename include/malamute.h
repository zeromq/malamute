/*  =========================================================================
    malamute - All the enterprise messaging patterns in one box

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MALAMUTE_H_INCLUDED__
#define __MALAMUTE_H_INCLUDED__

//  Include the project library file
#include "mlm_library.h"

//  Exported definitions

#if defined (__UTYPE_LINUX)
#   define MLM_DEFAULT_ENDPOINT "ipc://@/malamute"
#else
#   define MLM_DEFAULT_ENDPOINT "tcp://127.0.0.1:9999"
#endif

#endif

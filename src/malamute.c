/*  =========================================================================
    malamute - command-line service

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "mlm_classes.h"

#define PRODUCT         "Malamute service/0.0.1"
#define COPYRIGHT       "Copyright (c) 2014 the Contributors"
#define NOWARRANTY \
"This Software is provided under the MPLv2 License on an \"as is\" basis,\n" \
"without warranty of any kind, either expressed, implied, or statutory.\n"

int main (int argc, char *argv [])
{
    puts (PRODUCT);
    puts (COPYRIGHT);
    puts (NOWARRANTY);

    if (argc == 2 && streq (argv [1], "-h")) {
        puts ("Usage: malamute [-h | config-file]");
        puts ("  Default config-file is 'malamute.cfg'");
        return 0;
    }
    //  Collect configuration file name
    const char *config_file = "malamute.cfg";
    if (argc > 1)
        config_file = argv [1];

    //  Send logging to system facility as well as stdout
    zsys_init ();
    zsys_set_logsystem (true);
    
    //  Load config file for our own use here
    zsys_info ("starting Malamute using config in '%s'", config_file);
    zconfig_t *config = zconfig_load (config_file);
    if (config) {
        //  Do we want to run broker in the background?
        int as_daemon = atoi (zconfig_resolve (config, "server/background", "0"));
        const char *workdir = zconfig_resolve (config, "server/workdir", ".");
        if (as_daemon) {
            zsys_info ("Malamute going into background...");
            if (zsys_daemonize (workdir))
                return -1;
        }
        //  Switch to user/group to run process under, if any
        if (zsys_run_as (
            zconfig_resolve (config, "server/lockfile", NULL),
            zconfig_resolve (config, "server/group", NULL),
            zconfig_resolve (config, "server/user", NULL)))
            return -1;

        zconfig_destroy (&config);
    }
    else {
        zsys_error ("cannot load config file '%s'\n", config_file);
        return 1;
    }
    //  Shutdown all services
    return 0;
}

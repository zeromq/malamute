/*  =========================================================================
    mshell - Malamute command-line shell

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    Accepts either two or three arguments:
    mshell stream pattern         -- show all matching messages
    mshell stream subject body    -- send one message to this stream / subject

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "malamute.h"

int main (int argc, char *argv [])
{
    if (argc < 2) {
        printf ("syntax: mshell stream type [ body ]\n");
        return 0;
    }
    mlm_client_t *client = mlm_client_new ("ipc://@/malamute", 1000, "mshell");
    if (!client) {
        zsys_error ("mshell: server not reachable at ipc://@/malamute");
        return 0;
    }
    if (argc == 3) {
        //  Consume the event subjects specified by the pattern
        mlm_client_set_consumer (client, argv [1], argv [2]);
        while (true) {
            //  Now receive and print any messages we get
            zmsg_t *msg = mlm_client_recv (client);
            if (!msg)
                break;          //  Interrupted
            char *content = zmsg_popstr (msg);
            printf ("Content=%s sender=%s subject=%s\n",
                content, mlm_client_sender (client), mlm_client_subject (client));
            zstr_free (&content);
            zmsg_destroy (&msg);
        }
    }
    else
    if (argc == 4) {
        mlm_client_set_producer (client, argv [1]);
        mlm_client_sendx (client, argv [2], argv [3], NULL);
    }
    mlm_client_destroy (&client);
    return 0;
}

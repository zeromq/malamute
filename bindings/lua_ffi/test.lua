#!/usr/bin/lua5.2

--[[
Copyright (c) the Contributors as noted in the AUTHORS file.
This file is part of the Malamute Project.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

local malamute = require ("malamute_ffi")
local mlm = malamute.mlm
local ffi = malamute.ffi
-- FIXME: test expects broker running as third party service
--        w/o czmq ffi bindings we won't get zactor available
local endpoint = "ipc://@/malamute"

local r = 0
local client = mlm.mlm_client_new ()
assert (client ~= nil)
r = mlm.mlm_client_connect (client, endpoint, 1000, "client")
assert (r == 0)
r = mlm.mlm_client_set_producer (client, "STREAM")
assert (r == 0)

local consumer = mlm.mlm_client_new ()
assert (consumer ~= nil)
r = mlm.mlm_client_connect (consumer, endpoint, 1000, "consumer")
assert (r == 0)
r = mlm.mlm_client_set_consumer (consumer, "STREAM", ".*")
assert (r == 0)

r = mlm.mlm_client_sendx (client, "SUBJECT", "Wine", "from", "Burgundy", ffi.NULL);
assert (r == 0)

-- FIXME: minimal czmq bindings to show at least something on cmdline
local msg = mlm.mlm_client_recv (consumer)
assert (msg ~= nil)
ffi.cdef [[
void zmsg_print (zmsg_t *self);
void zmsg_destroy (zmsg_t **self_p);
]]
local czmq = ffi.load ("czmq")
czmq.zmsg_print (msg)
local msg_p = ffi.new ("zmsg_t*[1]", msg)
czmq.zmsg_destroy (msg_p)

local client_p = ffi.new ("mlm_client_t*[1]", client)
mlm.mlm_client_destroy (client_p)
local consumer_p = ffi.new ("mlm_client_t*[1]", consumer)
mlm.mlm_client_destroy (consumer_p)

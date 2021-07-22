/*  =========================================================================
    app - draft

    Codec header for app.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: application.xml, or
     * The code generation script that built this file: zproto_codec_c_v1
    ************************************************************************

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#pragma once

///  These are the app messages:
///
///    MODULE -
///        name                string
///        params              strings     List of parameters for module
///        args                hash
///
///    DB -
///        op                  number 4
///        params              strings
///        args                hash
///        bin                 chunk       In case we want to store pictures in db, etc...

#define APP_VERSION   1
#define APP_OP_INSERT 0
#define APP_OP_GET    1
#define APP_OP_UPDATE 2
#define APP_OP_DELETE 3

#define APP_MODULE 1
#define APP_DB     2

#include <czmq.h>

typedef struct _app_t app_t;

/// Create a new app
app_t* app_new(int id);

/// Destroy the app
void app_destroy(app_t** self_p);

/// Parse a zmsg_t and decides whether it is app.
/// @note Doesn't destroy or modify the original message.
/// @return true if it is, false otherwise.
bool is_app(zmsg_t* msg_p);

/// Parse a app from zmsg_t.
/// @note Destroys msg and nullifies the msg reference.
/// @return a new object, or NULL if the message could not be parsed, or was NULL.
app_t* app_decode(zmsg_t** msg_p);

/// Encode app into zmsg and destroy it.
/// @note Use when not in control of sending the message.
/// @returns a newly created object or NULL if error.
zmsg_t* app_encode(app_t** self_p);

/// Receive and parse a app from the socket.
/// @note Will block if there's no message waiting.
/// @return new object, or NULL if error.
app_t* app_recv(void* input);

/// Receive and parse a app from the socket.
/// @return new object, or NULL either if there was no input waiting, or the recv was interrupted.
app_t* app_recv_nowait(void* input);

/// Send the app to the output, and destroy it
int app_send(app_t** self_p, void* output);

/// Send the app to the output, and do not destroy it
int app_send_again(app_t* self, void* output);

///  Encode the MODULE
zmsg_t* app_encode_module(const char* name, zlist_t* params, zhash_t* args);

/// Encode the DB
zmsg_t* app_encode_db(uint32_t op, zlist_t* params, zhash_t* args, zchunk_t* bin);


/// Send the MODULE to the output in one step
/// @note WARNING, this call will fail if output is of type ZMQ_ROUTER.
int app_send_module(void* output, const char* name, zlist_t* params, zhash_t* args);

/// Send the DB to the output in one step
/// @note WARNING, this call will fail if output is of type ZMQ_ROUTER.
int app_send_db(void* output, uint32_t op, zlist_t* params, zhash_t* args, zchunk_t* bin);

/// Duplicate the app message
app_t* app_dup(app_t* self);

/// Print contents of message to stdout
void app_print(app_t* self);

/// Get/set the message routing id
zframe_t* app_routing_id(app_t* self);
void      app_set_routing_id(app_t* self, zframe_t* routing_id);

/// Get the app id and printable command
int         app_id(app_t* self);
void        app_set_id(app_t* self, int id);
const char* app_command(app_t* self);

/// Get/set the name field
const char* app_name(app_t* self);
void        app_set_name(app_t* self, const char* format, ...);

/// Get/set the params field
zlist_t* app_params(app_t* self);
/// Get the params field and transfer ownership to caller
zlist_t* app_get_params(app_t* self);
/// Set the params field, transferring ownership from caller
void app_set_params(app_t* self, zlist_t** params_p);

/// Iterate through the params field, and append a params value
const char* app_params_first(app_t* self);
const char* app_params_next(app_t* self);
void        app_params_append(app_t* self, const char* format, ...);
size_t      app_params_size(app_t* self);

/// Get/set the args field
zhash_t* app_args(app_t* self);
/// Get the args field and transfer ownership to caller
zhash_t* app_get_args(app_t* self);
/// Set the args field, transferring ownership from caller
void app_set_args(app_t* self, zhash_t** args_p);

/// Get/set a value in the args dictionary
const char* app_args_string(app_t* self, const char* key, const char* default_value);
uint64_t    app_args_number(app_t* self, const char* key, uint64_t default_value);
void        app_args_insert(app_t* self, const char* key, const char* format, ...);
size_t      app_args_size(app_t* self);

/// Get/set the op field
uint32_t app_op(app_t* self);
void     app_set_op(app_t* self, uint32_t op);

/// Get a copy of the bin field
zchunk_t* app_bin(app_t* self);
/// Get the bin field and transfer ownership to caller
zchunk_t* app_get_bin(app_t* self);
/// Set the bin field, transferring ownership from caller
void app_set_bin(app_t* self, zchunk_t** chunk_p);

/// For backwards compatibility with old codecs
#define app_dump app_print

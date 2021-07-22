/*
 *
 * Copyright (C) 2018 - 2020 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file   utils_json.h
 * \author Nicolas DAVIET <nicolasdaviet@Eaton.com>
 * \brief  some helpers for json message format generation
 */
#ifndef SRC_SHARED_WEB_JSON_H_
#define SRC_SHARED_WEB_JSON_H_

#include <malamute.h>
#include <fty_proto.h>
#include <string>
#include <tntdb.h>

//Return an alert with a json format
std::string getJsonAlert(tntdb::Connection connection, fty_proto_t *alert);

//Return an Asset with a json format
std::string getJsonAsset(mlm_client_t * clientMlm, int64_t elemId);


#endif // SRC_SHARED_WEB_UTILS_H_

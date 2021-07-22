/*
 *
 * Copyright (C) 2014 - 2020 Eaton
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

/// @file   agentstate.h
/// @brief  API for storing agent state as binary entity
/// @author Alena Chernikava <AlenaChernikava@eaton.com>
/// @author TomasHalman <TomasHalman@eaton.com>

#pragma once

#include <string>
#include <tntdb/connect.h>

namespace persist {

/// save agent state
///
/// Function saves data (stored as blob in db) under unique name. It can be used for saving agent state or any other
/// data under unique identifier.
///
/// @param[in] conn       - a database connection
/// @param[in] agent_name - an agent name
/// @param[in] data       - data to be stored
/// @return 0      - in case of success
///                  number - in case of any error
int save_agent_info(tntdb::Connection& conn, const std::string& agent_name, const std::string& data);

/// save agent state
///
/// Function saves data (stored as blob in db) under unique name. It can be used for saving agent state or any other
/// data under unique identifier. Database connection is hadled inside this function. It connects, saves data and
/// disconnects.
///
/// @param[in] agent_name - an agent name
/// @param[in] data       - data to be stored
/// @return 0      - in case of success
///         number - in case of any error
int save_agent_info(const std::string& agent_name, const std::string& data);

/// load agent state
///
/// Function reads data (stored previously in db under unique name). It can be used for loading agent state or any other
/// data under unique identifier.
///
/// @param[in]  conn       - a database connection
/// @param[in]  agent_name - an agent name
/// @param[out] agent_info - data to be retrieved
/// @return 0      - in case of success
///        number - in case of any error
int load_agent_info(tntdb::Connection& conn, const std::string& agent_name, std::string& agent_info);

/// load agent state
///
/// Function reads data (stored previously in db under unique name). It can be used for loading agent state or any other
/// data under unique identifier. Database connection is hadled inside this function. It connects, loads data and
/// disconnects.
///
/// @param[in]  agent_name - an agent name
/// @param[out] agent_info - data to be retrieved
/// @return 0      - in case of success
///         number - in case of any error
int load_agent_info(const std::string& agent_name, std::string& agent_info);

} // namespace persist

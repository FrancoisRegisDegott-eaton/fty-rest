/*
 *
 * Copyright (C) 2015 - 2020 Eaton
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

/// \file   inout.h
/// \brief  Import/Export of csv
/// \author Michal Vyskocil <MichalVyskocil@Eaton.com>
/// \author Alena Chernikava <AlenaChernikava@Eaton.com>
#pragma once

#include "db/dbhelpers.h"
#include "shared/csv.h"
#include <fty_common.h>
#include <fty_common_db.h>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#define CREATE_MODE_ONE_ASSET 1
#define CREATE_MODE_CSV       2

// forward declaration
class MlmClient;

namespace persist {

typedef std::function<void(void)> touch_cb_t;

/// Converts the string priority to number
///
/// get_priority("0") -> 0
/// get_priority("P3") -> 3
/// get_priority("X4") -> 4
/// get_priority("777") -> 5
///
/// @param[in] s - a string with a content
/// @return numeric value of priority or 5 if not detected
int get_priority(const std::string& s);

/// process one asset
///
/// @param[in] cm - an instance of CsvMap (you see this nice evolution, right)
/// @return id of inserted/updated asset
/// @throws execeptions on errors
std::pair<db_a_elmnt_t, persist::asset_operation> process_one_asset(const shared::CsvMap& cm);

/// Processes a csv file
///
/// Resuls are written in DB and into log.
///
/// @param[in]  input    - an input file
/// @param[out] okRows   - a list of short information about inserted rows
/// @param[out] failRows - a list of rejected rows with the message
void load_asset_csv(
    std::istream&                                                   input,
    std::vector<std::pair<db_a_elmnt_t, persist::asset_operation>>& okRows,
    std::map<int, std::string>&                                     failRows,
    touch_cb_t                                                      touch_fn,
    std::string                                                     user = "");

/// Processes a csv map
///
/// Resuls are written in DB and into log.
///
/// @param[in]  cm       - an input csv map
/// @param[out] okRows   - a list of short information about inserted rows
/// @param[out] failRows - a list of rejected rows with the message
void load_asset_csv(
    const shared::CsvMap&                                           cm,
    std::vector<std::pair<db_a_elmnt_t, persist::asset_operation>>& okRows,
    std::map<int, std::string>&                                     failRows,
    touch_cb_t                                                      touch_fn);

/// export csv file and write result to output stream
///
/// @param[out] out - a reference to the standard output stream to which content will be written
/// @param[in] dc_id - limit export to this DC id (default -1 means all DCs)
/// @param[in] generate_bom - generate BOM or not (default true)
void export_asset_csv(std::ostream& out, int64_t dc_id = -1, bool generate_bom = true);

void export_asset_json(std::ostream& out, std::set<std::string>* listElement = NULL);

/// Identify id of row with rackcontroller-0
/// @param[in]   client      Mlm client to send and receieve messages to/from other agents
/// @param[in]   cm          An input CSV map
/// @param[in]   touch_fn    Function to be called after time consuming operation so tntnet request won't time-out
/// @throw runtime_error on failure
/// @return >0 on successfull match
/// @return 0 on failure, all values in such column are empty (so other columns may match)
/// @return -1 on failure, but column has valid content (therefore match is impossible)
int promote_rc0(MlmClient* client, const shared::CsvMap& cm, touch_cb_t touch_fn);

} // namespace persist

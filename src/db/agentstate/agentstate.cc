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

#include "db/agentstate/agentstate.h"
#include "shared/utils.h"
#include <fty_common.h>
#include <fty_common_db_dbpath.h>
#include <inttypes.h>
#include <tntdb/error.h>
#include <tntdb/row.h>

namespace persist {

//=========================
// lowlevel-functions
//=========================
static int update_agent_info(
    tntdb::Connection& conn, const std::string& agent_name, const void* data, size_t size, uint16_t& affected_rows)
{
    LOG_START;

    try {
        tntdb::Statement st = conn.prepare(
            " INSERT INTO t_bios_agent_info (agent_name, info) "
            " VALUES "
            "   (:name, :info) "
            " ON DUPLICATE KEY "
            "   UPDATE "
            "       info = :info ");
        tntdb::Blob blobData(static_cast<const char*>(data), size);

        affected_rows = uint16_t(st.set("name", agent_name).setBlob("info", blobData).execute());
        log_debug("[t_bios_agent_info]: %" PRIu16 " rows ", affected_rows);
        LOG_END;
        return 0;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        return 1;
    }
}

//=========================
// highlevel-functions
//=========================

int save_agent_info(tntdb::Connection& conn, const std::string& agent_name, const std::string& data)
{
    uint16_t rows;

    return update_agent_info(conn, agent_name, data.c_str(), data.size(), rows);
}

int save_agent_info(const std::string& agent_name, const std::string& data)
{
    int result = 1;
    try {
        auto connection = tntdb::connect(DBConn::url);
        result          = save_agent_info(connection, agent_name, data);
        connection.close();
    } catch (const std::exception& e) {
        log_error("Cannot save agent %s info: %s", agent_name.c_str(), e.what());
    }
    return result;
}

int load_agent_info(tntdb::Connection& conn, const std::string& agent_name, std::string& agent_info)
{
    size_t size = 0;
    agent_info  = "";

    try {
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.info "
            " FROM "
            "   v_bios_agent_info v "
            " WHERE "
            "   v.agent_name = :name ");

        tntdb::Row row = st.set("name", agent_name).selectRow();

        tntdb::Blob myBlob;
        row[0].get(myBlob);

        size = myBlob.size();
        std::string data(myBlob.data(), size);
        if (data.empty()) {
            // data is empty
            return 0;
        }
        agent_info = data;
        LOG_END;
        return 0;
    } catch (const tntdb::NotFound& e) {
        log_debug("end: nothing was found");
        size = 0;
        return 0;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int load_agent_info(const std::string& agent_name, std::string& agent_info)
{
    try {
        auto connection = tntdb::connect(DBConn::url);
        return load_agent_info(connection, agent_name, agent_info);
    } catch (const std::exception& e) {
        log_info("Cannot load agent %s info: %s", agent_name.c_str(), e.what());
        return -1;
    }
}

} // namespace persist

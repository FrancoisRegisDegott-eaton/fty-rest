/*
Copyright (C) 2015 Eaton
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*! \file dbhelpers.cc
    \brief Helper functions for direct interacting with db
    \author Alena Chernikava <alenachernikava@eaton.com>
*/

#include <assert.h>

#include <czmq.h>
#include <tntdb/connect.h>
#include <tntdb/row.h>
#include <tntdb/error.h>

#include "log.h"
#include "persist_error.h"
#include "asset_types.h"
#include "dbhelpers.h"

m_dvc_id_t convert_asset_to_monitor(const char* url, 
                a_elmnt_id_t asset_element_id)
{
    assert ( asset_element_id );
    m_dvc_id_t       device_discovered_id = 0;
    a_elmnt_tp_id_t  element_type_id      = 0;
    try{
        tntdb::Connection conn = tntdb::connectCached(url);

        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            "   v.id_discovered_device, v1.id_type"
            " FROM"
            "   v_bios_monitor_asset_relation v"
            " LEFT JOIN t_bios_asset_element v1"
            "   ON (v1.id_asset_element = v.id_asset_element)"
            " WHERE v.id_asset_element = :id"
        );
        
        tntdb::Row row = st.set("id", asset_element_id).
                            selectRow();

        row[0].get(device_discovered_id);

        row[1].get(element_type_id);
        assert (element_type_id);
    }
    catch (const tntdb::NotFound &e){
        // apropriate asset element was not found
        log_info("asset element %d notfound %s ", asset_element_id,"end");
        throw bios::NotFound();
    }
    catch (const std::exception &e) {
        log_warning("abnormal %s ","end");
        throw bios::InternalDBError(e.what());
    }
    if ( element_type_id != asset_type::DEVICE )
    {
        log_info("specified element is not a device %s ","end");
        throw bios::ElementIsNotDevice();
    }
    else if ( device_discovered_id == 0 )
    {
        log_warning("monitor counterpart for the %d was not found %s ", 
                                                asset_element_id, "end");
        throw bios::MonitorCounterpartNotFound ();
    }
    log_info("end: asset element %d converted to %d ", asset_element_id, device_discovered_id);
    return device_discovered_id;
}

a_elmnt_id_t convert_monitor_to_asset(const char* url, 
                    m_dvc_id_t discovered_device_id)
{
    log_info("%s ","start");
    assert ( discovered_device_id );
    a_elmnt_id_t asset_element_id = 0;
    try{
        tntdb::Connection conn = tntdb::connectCached(url);
        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            " id_asset_element"
            " FROM"
            " v_bios_monitor_asset_relation"
            " WHERE id_discovered_device = :id"
        );

        tntdb::Value val = st.set("id", discovered_device_id).
                              selectValue();

        val.get(asset_element_id);
    }
    catch (const tntdb::NotFound &e){
        // apropriate asset element was not found
        log_info("notfound %s ","end");
        throw bios::NotFound();
    }
    catch (const std::exception &e) {
        log_warning("abnormal %s ","end");
        throw bios::InternalDBError(e.what());
    }
    log_info("normal %s ","end");
    return asset_element_id;
}

bool is_ok_element_type (a_elmnt_tp_id_t element_type_id)
{
    switch(element_type_id) {
        case asset_type::DATACENTER:
        case asset_type::ROOM:
        case asset_type::ROW:
        case asset_type::RACK:
        case asset_type::GROUP:
        case asset_type::DEVICE:
            return true;
        default:
            return false;
    }
}

bool is_ok_keytag (const char *keytag)
{
    auto length = strlen(keytag);
    if ( ( length > 0 ) && ( length <= MAX_KEYTAG_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_value (const char *value)
{
    auto length = strlen(value);
    if ( ( length > 0 ) && ( length <= MAX_VALUE_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_asset_device_type (a_dvc_tp_id_t asset_device_type_id)
{
    // TODO: manage device types
    if ( asset_device_type_id > 0 )
        return true;
    else
        return false;
}

bool is_ok_hostname (const char *hostname)
{
    // TODO: Do we need to add more complex restrictions?
    auto length = strlen(hostname);
    if ( ( length > 0 ) && ( length <= MAX_HOSTNAME_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_fullhostname (const char *fullhostname)
{
    // TODO: Do we need to add more complex restrictions?
    auto length = strlen(fullhostname);
    if ( ( length > 0 ) && ( length <= MAX_FULLHOSTNAME_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_ip (const char *ip)
{
    // TODO: Do we need to add more complex restrictions?
    auto length = strlen(ip);
    if ( ( length > 0 ) && ( length <= MAX_IP_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_mac (const char *mac)
{
    // TODO: Do we need to add more complex restrictions?
    auto length = strlen(mac);
    if ( ( length > 0 ) && ( length <= MAX_MAC_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_link_type (a_lnk_tp_id_t link_type_id)
{
    // TODO: manage link types
    if ( link_type_id > 0 )
        return true;
    else
        return false;
}

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

/*!
 * \file data.cc
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \author Tomas Halman <TomasHalman@Eaton.com>
 * \author Jim Klimov <EvgenyKlimov@Eaton.com>
 * \brief Not yet documented file
 */

#include "shared/data.h"
#include "db/asset_general.h"
#include "shared/utils_json.h"
#include <algorithm>
#include <fty_common.h>
#include <fty_common_db_dbpath.h>
#include <fty_common_macros.h>
#include <fty_common_rest.h>

static std::vector<std::tuple<a_elmnt_id_t, std::string, std::string, std::string>> s_get_parents(
    tntdb::Connection& conn, a_elmnt_id_t id)
{

    std::vector<std::tuple<a_elmnt_id_t, std::string, std::string, std::string>> ret{};

    std::function<void(const tntdb::Row&)> cb = [&ret](const tntdb::Row& row) {
        // C++ is c r a z y!! Having static initializer in lambda function made
        // my life easier here, but I did not expected this will work!!
        static const std::vector<std::tuple<std::string, std::string, std::string, std::string>> NAMES = {
            std::make_tuple("id_parent1", "parent_name1", "id_type_parent1", "id_subtype_parent1"),
            std::make_tuple("id_parent2", "parent_name2", "id_type_parent2", "id_subtype_parent2"),
            std::make_tuple("id_parent3", "parent_name3", "id_type_parent3", "id_subtype_parent3"),
            std::make_tuple("id_parent4", "parent_name4", "id_type_parent4", "id_subtype_parent4"),
            std::make_tuple("id_parent5", "parent_name5", "id_type_parent5", "id_subtype_parent5"),
            std::make_tuple("id_parent6", "parent_name6", "id_type_parent6", "id_subtype_parent6"),
            std::make_tuple("id_parent7", "parent_name7", "id_type_parent7", "id_subtype_parent7"),
            std::make_tuple("id_parent8", "parent_name8", "id_type_parent8", "id_subtype_parent8"),
            std::make_tuple("id_parent9", "parent_name9", "id_type_parent9", "id_subtype_parent9"),
            std::make_tuple("id_parent10", "parent_name10", "id_type_parent10", "id_subtype_parent10"),
        };

        for (const auto& it : NAMES) {
            a_elmnt_id_t id1 = 0;
            row[std::get<0>(it)].get(id1);
            std::string name;
            row[std::get<1>(it)].get(name);
            a_elmnt_tp_id_t id_type = 0;
            row[std::get<2>(it)].get(id_type);
            a_elmnt_stp_id_t id_subtype = 0;
            row[std::get<3>(it)].get(id_subtype);
            if (!name.empty())
                ret.push_back(std::make_tuple(
                    id1, name, persist::typeid_to_type(id_type), persist::subtypeid_to_subtype(id_subtype)));
        }
    };

    int r = DBAssets::select_asset_element_super_parent(conn, id, cb);
    if (r == -1) {
        log_error("select_asset_element_super_parent failed");
        throw std::runtime_error(TRANSLATE_ME("DBAssets::select_asset_element_super_parent () failed."));
    }

    return ret;
}

db_reply<db_web_element_t> asset_manager::get_item1(uint32_t id)
{
    db_reply<db_web_element_t> ret;

    try {
        tntdb::Connection conn = tntdb::connect(DBConn::url);
        log_debug("connection was successful");

        auto basic_ret = DBAssets::select_asset_element_web_byId(conn, id);
        log_debug("1/5 basic select is done");

        if (basic_ret.status == 0) {
            ret.status     = basic_ret.status;
            ret.errtype    = basic_ret.errtype;
            ret.errsubtype = basic_ret.errsubtype;
            ret.msg        = JSONIFY(basic_ret.msg.c_str());
            log_warning("%s", ret.msg.c_str());
            return ret;
        }
        log_debug("    1/5 no errors");
        ret.item.basic = basic_ret.item;

        auto ext_ret = DBAssets::select_ext_attributes(conn, id);
        log_debug("2/5 ext select is done");

        if (ext_ret.status == 0) {
            ret.status     = ext_ret.status;
            ret.errtype    = ext_ret.errtype;
            ret.errsubtype = ext_ret.errsubtype;
            ret.msg        = JSONIFY(ext_ret.msg.c_str());
            log_warning("%s", ret.msg.c_str());
            return ret;
        }
        log_debug("    2/5 no errors");
        ret.item.ext = ext_ret.item;

        auto group_ret = DBAssets::select_asset_element_groups(conn, id);
        log_debug("3/5 groups select is done, but next one is only for devices");

        if (group_ret.status == 0) {
            ret.status     = group_ret.status;
            ret.errtype    = group_ret.errtype;
            ret.errsubtype = group_ret.errsubtype;
            ret.msg        = JSONIFY(group_ret.msg.c_str());
            log_warning("%s", ret.msg.c_str());
            return ret;
        }
        log_debug("    3/5 no errors");
        ret.item.groups = group_ret.item;

        if (ret.item.basic.type_id == persist::asset_type::DEVICE) {
            auto powers = DBAssets::select_asset_device_links_to(conn, id, INPUT_POWER_CHAIN);
            log_debug("4/5 powers select is done");

            if (powers.status == 0) {
                ret.status     = powers.status;
                ret.errtype    = powers.errtype;
                ret.errsubtype = powers.errsubtype;
                ret.msg        = JSONIFY(powers.msg.c_str());
                log_warning("%s", ret.msg.c_str());
                return ret;
            }
            log_debug("    4/5 no errors");
            ret.item.powers = powers.item;
        }

        // parents select
        log_debug("5/5 parents select");
        ret.item.parents = s_get_parents(conn, id);
        log_debug("     5/5 no errors");

        ret.status = 1;
        return ret;
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = JSONIFY(e.what());
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply<std::map<uint32_t, std::string>> asset_manager::get_items1(
    const std::string& typeName, const std::string& subtypeName)
{
    LOG_START;
    log_debug("subtypename = '%s', typename = '%s'", subtypeName.c_str(), typeName.c_str());
    a_elmnt_stp_id_t                          subtype_id = 0;
    db_reply<std::map<uint32_t, std::string>> ret;

    a_elmnt_tp_id_t type_id = persist::type_to_typeid(typeName);
    if (type_id == persist::asset_type::TUNKNOWN) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = TRANSLATE_ME("Unsupported type of the elements");
        log_error("%s", ret.msg.c_str());
        // TODO need to have some more precise list of types, so we don't have to change anything here,
        // if something was changed
        bios_error_idx(
            ret.rowid, ret.msg, "request-param-bad", "type", typeName.c_str(), "datacenters,rooms,ros,racks,devices");
        return ret;
    }
    if ((typeName == "device") && (!subtypeName.empty())) {
        subtype_id = persist::subtype_to_subtypeid(subtypeName);
        if (subtype_id == persist::asset_subtype::SUNKNOWN) {
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_INTERNAL;
            ret.msg        = TRANSLATE_ME("Unsupported subtype of the elements");
            log_error("%s", ret.msg.c_str());
            // TODO need to have some more precise list of types, so we don't have to change anything here,
            // if something was changed
            bios_error_idx(
                ret.rowid, ret.msg, "request-param-bad", "subtype", subtypeName.c_str(),
                "ups, epdu, pdu, genset, sts, server, feed");
            return ret;
        }
    }
    log_debug("subtypeid = %" PRIi16 " typeid = %" PRIi16, subtype_id, type_id);

    try {
        tntdb::Connection conn = tntdb::connect(DBConn::url);
        ret                    = DBAssets::select_short_elements(conn, type_id, subtype_id);
        if (ret.status == 0)
            bios_error_idx(ret.rowid, ret.msg, "internal-error", "");
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        bios_error_idx(ret.rowid, ret.msg, "internal-error", "");
        return ret;
    }
}

db_reply_t asset_manager::delete_item(uint32_t id, db_a_elmnt_t& element_info)
{
    db_reply_t ret = db_reply_new();

    // As different types should be deleted in differenct way ->
    // find out the type of the element.
    // Even if in old-style the type is already placed in URL,
    // we will ignore it and discover it by ourselves

    try {
        tntdb::Connection conn = tntdb::connect(DBConn::url);

        db_reply<db_web_basic_element_t> basic_info = DBAssets::select_asset_element_web_byId(conn, id);

        if (basic_info.status == 0) {
            ret.status     = 0;
            ret.errtype    = basic_info.errsubtype;
            ret.errsubtype = DB_ERROR_NOTFOUND;
            ret.msg        = TRANSLATE_ME("problem with selecting basic info");
            log_warning("%s", ret.msg.c_str());
            return ret;
        }
        // here we are only if everything was ok
        element_info.type_id    = basic_info.item.type_id;
        element_info.subtype_id = basic_info.item.subtype_id;
        element_info.name       = basic_info.item.name;

        // disable deleting RC0
        if (RC0_INAME == element_info.name) {
            log_debug("Prevented deleting RC-0");
            ret.status        = 1;
            ret.affected_rows = 0;
            LOG_END;
            return ret;
        }

        // check if a logical_asset refer to the item we are trying to delete
        if (DBAssets::count_keytag(conn, "logical_asset", element_info.name) > 0) {
            ret.status     = 0;
            ret.errtype    = basic_info.errsubtype;
            ret.errsubtype = DB_ERROR_DELETEFAIL;
            ret.msg        = TRANSLATE_ME("a logical_asset (sensor) refers to it");
            log_warning("%s", ret.msg.c_str());
            LOG_END;
            return ret;
        }

        switch (basic_info.item.type_id) {
            case persist::asset_type::DATACENTER:
            case persist::asset_type::ROW:
            case persist::asset_type::ROOM:
            case persist::asset_type::RACK: {
                ret = persist::delete_dc_room_row_rack(conn, id);
                break;
            }
            case persist::asset_type::GROUP: {
                ret = persist::delete_group(conn, id);
                break;
            }
            case persist::asset_type::DEVICE: {
                if (basic_info.item.status == "active") {
                    // we need device JSON in order to delete active device
                    std::string asset_json = getJsonAsset(NULL, id);
                    ret                    = persist::delete_device(conn, id, asset_json);
                } else {
                    ret = persist::delete_device(conn, id);
                }
                break;
            }
            default: {
                ret.status     = 0;
                ret.errtype    = basic_info.errsubtype;
                ret.errsubtype = DB_ERROR_INTERNAL;
                ret.msg        = TRANSLATE_ME("unknown type");
                log_warning("%s", ret.msg.c_str());
            }
        }
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

// ===========================================================================================================

struct CheckException : public std::runtime_error
{
    CheckException(uint32_t _id, const std::string& _msg, int _type = DB_ERR, db_err_nos _subType = DB_ERROR_UNKNOWN)
        : std::runtime_error(_msg)
        , id(_id)
        , errType(_type)
        , errSubType(_subType)
    {
    }

    uint32_t   id;
    int        errType;
    db_err_nos errSubType;
};

struct ElementInfo
{
    db_a_elmnt_t          el;
    std::vector<uint32_t> links;
};

static void checkAndAddInfo(std::vector<ElementInfo>& ids, const ElementInfo& item)
{
    auto it = std::find_if(ids.begin(), ids.end(), [&](const ElementInfo& check) {
        return check.el.id == item.el.id;
    });

    if (it == ids.end()) {
        // disable deleting RC0
        if (RC0_INAME == item.el.name) {
            log_debug("Prevented deleting RC-0");
            throw CheckException(item.el.id, "Prevented deleting RC-0");
        }

        ids.push_back(item);
    }
}

static void collectChildren(tntdb::Connection& conn, uint32_t id, std::vector<ElementInfo>& ids)
{
    // Now corresponding functions to collect child items/filter by parent.
    // TODO: Add to fty-common-db
    DBAssets::select_assets_cb(conn, [&](const tntdb::Row& row) {
        if (id == static_cast<uint32_t>(row["id_parent"].getInt())) {
            ElementInfo item;

            row["id"].get(item.el.id);
            row["id_type"].get(item.el.type_id);
            row["id_subtype"].get(item.el.subtype_id);
            row["name"].get(item.el.name);
            row["id_parent"].get(item.el.parent_id);

            collectChildren(conn, item.el.id, ids);
            checkAndAddInfo(ids, item);
        }
    });
}

static std::vector<uint32_t> selectAnyLinks(tntdb::Connection& conn, const ElementInfo& item)
{
    std::vector<uint32_t> links;
    try {
        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            "   id_asset_device_dest"
            " FROM"
            "   t_bios_asset_link"
            " WHERE"
            "   id_asset_device_src = :src");

        auto result = st.set("src", item.el.id).select();
        for (const auto& row : result) {
            links.push_back(row.getUnsigned32("id_asset_device_dest"));
        }
    } catch (const std::exception& e) {
        throw CheckException(item.el.id, e.what(), DB_ERR, DB_ERROR_INTERNAL);
    }
    return links;
}

static db_reply_t deleteAsset(tntdb::Connection& conn, const ElementInfo& item)
{
    if (DBAssets::count_keytag(conn, "logical_asset", item.el.name) > 0) {
        throw CheckException(
            item.el.id, TRANSLATE_ME("a logical_asset (sensor) refers to it"), DB_ERR, DB_ERROR_DELETEFAIL);
    }

    try {
        switch (item.el.type_id) {
            case persist::asset_type::DATACENTER:
            case persist::asset_type::ROW:
            case persist::asset_type::ROOM:
            case persist::asset_type::RACK:
                // DBAssetsDelete::delete_asset_links_to(conn, item.id);
                return persist::delete_dc_room_row_rack(conn, item.el.id);
            case persist::asset_type::GROUP:
                return persist::delete_group(conn, item.el.id);
            case persist::asset_type::DEVICE:
                if (item.el.status == "active") {
                    // we need device JSON in order to delete active device
                    std::string asset_json = getJsonAsset(nullptr, item.el.id);
                    return persist::delete_device(conn, item.el.id, asset_json);
                }
                return persist::delete_device(conn, item.el.id);
        }
        throw CheckException(item.el.id, TRANSLATE_ME("unknown type"), DB_ERR, DB_ERROR_INTERNAL);
    } catch (const std::exception& e) {
        throw CheckException(item.el.id, e.what(), DB_ERR, DB_ERROR_DELETEFAIL);
    }
}

std::vector<std::pair<uint32_t, db_reply_t>> asset_manager::delete_item(
    const std::vector<uint32_t>& ids, std::vector<db_a_elmnt_t>& element_info)
{
    std::vector<std::pair<uint32_t, db_reply_t>> ret;
    tntdb::Connection                            conn = tntdb::connect(DBConn::url);

    // collect ids recursively
    std::vector<ElementInfo> toDel;
    for (uint32_t id : ids) {
        db_reply<db_web_basic_element_t> basic_info = DBAssets::select_asset_element_web_byId(conn, id);

        if (basic_info.status == 0) {
            db_reply_t answ = db_reply_new();
            answ.status     = basic_info.status;
            answ.errtype    = DB_ERR;
            answ.errsubtype = DB_ERROR_NOTFOUND;
            answ.msg        = TRANSLATE_ME("problem with selecting basic info");
            ret.push_back({id, answ});
        } else {
            try {
                ElementInfo info;
                // here we are only if everything was ok
                info.el.id         = id;
                info.el.type_id    = basic_info.item.type_id;
                info.el.subtype_id = basic_info.item.subtype_id;
                info.el.name       = basic_info.item.name;
                info.el.parent_id  = basic_info.item.parent_id;

                element_info.push_back(info.el);

                std::vector<ElementInfo> children;
                collectChildren(conn, id, children);
                for (uint32_t existsId : ids) {
                    children.erase(
                        std::remove_if(
                            children.begin(), children.end(),
                            [&](const ElementInfo& item) {
                                return item.el.id == existsId;
                            }),
                        children.end());
                }

                auto links = selectAnyLinks(conn, info);
                info.links = links;

                checkAndAddInfo(toDel, info);

                if (!children.empty()) {
                    throw CheckException(id, TRANSLATE_ME("can't delete the asset because it has at least one child"));
                }

                for (uint32_t existsId : ids) {
                    links.erase(std::remove(links.begin(), links.end(), existsId), links.end());
                }

                if (!links.empty()) {
                    throw CheckException(id, TRANSLATE_ME("can't delete the asset because it is linked to others"));
                }

            } catch (const CheckException& e) {
                db_reply_t answ = db_reply_new();
                answ.status     = 0;
                answ.errtype    = e.errType;
                answ.errsubtype = e.errSubType;
                answ.msg        = e.what();
                log_warning("%s", e.what());
                LOG_END;
                ret.push_back({id, answ});
            }
        }
    }

    auto isLinked = [&](const ElementInfo& item, const ElementInfo& parent) {
        auto isLLinked = std::find(item.links.begin(), item.links.end(), parent.el.id);
        if (isLLinked != item.links.end()) {
            return false;
        }

        auto isPLinked = std::find(parent.links.begin(), parent.links.end(), item.el.id);
        if (isPLinked != parent.links.end()) {
            return true;
        }
        return false;
    };

    auto isAnyParent = [&](const ElementInfo& item, const ElementInfo& parent) {
        auto p = parent;
        while (true) {
            if (item.el.parent_id == p.el.id) {
                return true;
            }

            auto found = std::find_if(toDel.begin(), toDel.end(), [&](const ElementInfo& it) {
                return it.el.id == p.el.parent_id;
            });
            if (found == toDel.end()) {
                return false;
            }
            p = *found;
        }
    };

    std::sort(toDel.begin(), toDel.end(), [&](const ElementInfo& l, const ElementInfo& r) {
        return isAnyParent(l, r) || isLinked(l, r);
    });

    for (const auto& item : toDel) {
        try {
            auto it = std::find_if(ret.begin(), ret.end(), [&](const std::pair<uint32_t, db_reply_t>& pair) {
                return pair.first == item.el.id;
            });

            if (it == ret.end() || it->second.status != 0) {
                ret.push_back({item.el.id, deleteAsset(conn, item)});
            }
        } catch (const CheckException& e) {
            db_reply_t answ = db_reply_new();
            answ.status     = 0;
            answ.errtype    = e.errType;
            answ.errsubtype = e.errSubType;
            answ.msg        = e.what();
            log_warning("%s", e.what());
            LOG_END;
            ret.push_back({item.el.id, answ});
        }
    }

    return ret;
}

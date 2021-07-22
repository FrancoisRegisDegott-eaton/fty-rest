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
 * \file sse.cc
 * \author Nicolas DAVIET<nicolasdaviet@Eaton.com>
 * \brief Class and functions used by sse connection
 */
#include <fty_common_rest.h>
#include <fty_common_mlm_utils.h>
#include <fty_common.h>

#include "web/src/sse.h"
#include "shared/data.h"
#include "shared/utils_json.h"

//constructor

Sse::Sse()
{
}

//Clean object in destructor

Sse::~Sse()
{
  if (_clientMlm)
  {
    mlm_client_destroy(&_clientMlm);
  }
  if (_poller)
  {
    zpoller_destroy(&_poller);
  }
}

std::string Sse::connectMalamute()
{
  // connect to malamute
  _clientMlm = mlm_client_new();
  if (!_clientMlm)
  {
    log_fatal("mlm_client_new() failed.");
    return TRANSLATE_ME ("mlm_client_new() failed.");
  }

  std::string client_name = utils::generate_mlm_client_id("web.sse");
  log_debug("malamute client name = '%s'.", client_name.c_str());

  int rv = mlm_client_connect(_clientMlm, MLM_ENDPOINT, 1000, client_name.c_str());
  if (rv == -1)
  {
    log_fatal("mlm_client_connect (endpoint = '%s', timeout = '%" PRIu32"', address = '%s') failed.",
                 MLM_ENDPOINT, 1000, client_name.c_str());
    return TRANSLATE_ME ("mlm_client_connect() failed.");
  }

  _pipe = mlm_client_msgpipe(_clientMlm);
  if (!_pipe)
  {
    log_fatal("mlm_client_msgpipe() failed.");
    return TRANSLATE_ME ("mlm_client_msgpipe()");
  }

  _poller = zpoller_new(_pipe, NULL);
  if (!_poller)
  {
    log_fatal("zpoller_new() failed.");
    return TRANSLATE_ME ("zpoller_new() failed.");
  }

  return std::string("");
}

int Sse::consumeStream(std::string stream, std::string pattern)
{
  return mlm_client_set_consumer(_clientMlm, stream.c_str(), pattern.c_str());
}

long int Sse::checkTokenValidity()
{
  long int tme;
  long int uid;
  long int gid;
  char * user_name;

  if (BiosProfile::Anonymous == tokens::get_instance()->verify_token(_token, &tme, &uid, &gid, &user_name))
  {
    log_info("sse : Token revoked or expired");
    return -1;
  }
  free (user_name);
  return tme;
}

bool Sse::isMessageComesFrom(const char * channel)
{
  if (mlm_client_command(_clientMlm) && channel)
    return streq(mlm_client_command(_clientMlm), channel);
  return false;
}

//Returns the message command, for the latest msg received
std::string Sse::messageCommand()
{
  const char *command = mlm_client_command(_clientMlm);
  return command ? command : "";
}

//Returns the message subject, for the latest msg received
std::string Sse::messageSubject()
{
  const char *subject = mlm_client_subject(_clientMlm);
  return subject ? subject : "";
}

zmsg_t * Sse::getMessageFromMlm()
{
  return mlm_client_recv(_clientMlm);
}

std::string Sse::loadAssetFromDatacenter()
{

  std::map<std::string, int> assets;
  std::map<std::string, int> assetsWithNoLocation;

  //Get all assets from the datacenter
  auto asset_element = DBAssets::select_asset_element_web_byId(_connection, _datacenter_id);
  if (asset_element.status != 1)
  {
    return asset_element.msg.c_str();
  }
  log_debug("Asset element name = '%s'.", asset_element.item.name.c_str());
  assets.emplace(std::make_pair(asset_element.item.name, 5));

  try
  {
    int rv = DBAssets::select_assets_by_container(_connection, _datacenter_id,
      [&assets](const tntdb::Row & row) -> void
      {
        std::string name;
        row[0].get(name);
        assets.emplace(std::make_pair(name, 5));
      });
    if (rv != 0)
    {
      return TRANSLATE_ME ("DBAssets::select_assets_by_container () failed.");
    }
  }
  catch (const tntdb::Error& e)
  {
    return JSONIFY (e.what());
  }
  catch (const std::exception& e)
  {
    return JSONIFY (e.what());
  }
  _assetsOfDatacenter = assets;
  if(ManageFtyLog::getInstanceFtylog()->isLogDebug())
  {
    log_debug("=== These elements are topologically under element id: '%" PRIu32"' ===", _datacenter_id);
    for (auto const& item : _assetsOfDatacenter)
    {
      log_debug("%s", item.first.c_str());
    }
    log_debug("=== end ===");
  }
  //Get all assets without location
  try
  {
    int rv = DBAssets::select_assets_without_container(_connection,{persist::asset_type::DEVICE},{},
      [&assetsWithNoLocation](const tntdb::Row & row) -> void
      {
        std::string name;
        row[0].get(name);
        assetsWithNoLocation.emplace(std::make_pair(name, 5));
      });
    if (rv != 0)
    {
      return TRANSLATE_ME ("DBAssets::select_assets_by_container () failed.");
    }
  }
  catch (const tntdb::Error& e)
  {
    return JSONIFY (e.what());
  }
  catch (const std::exception& e)
  {
    return JSONIFY (e.what());
  }
  _assetsWithNoLocation = assetsWithNoLocation;

  if(ManageFtyLog::getInstanceFtylog()->isLogDebug())
  {
    log_debug("=== These elements are witout location ===");
    for (auto const& item : _assetsWithNoLocation)
    {
      log_debug("%s", item.first.c_str());
    }
    log_debug("=== end ===");
  }
  return std::string("");
}

std::string Sse::changeFtyProtoAlert2Json(fty_proto_t *alert)
{
  std::string json = "";
  if (_assetsOfDatacenter.find(fty_proto_name(alert)) == _assetsOfDatacenter.end())
  {
    log_debug("skipping due to element_src '%s' not being in the list", fty_proto_name(alert));
    return json;
  }

  std::string jsonPayload = getJsonAlert(_connection, alert);
  if (!jsonPayload.empty())
  {
    /*
     * Check if the alert has been published with the same exact state
     * previously. The SSE doesn't have a TTL concept and thus doesn't need to
     * republish, since this will juggle the alerts in the UI because of
     * changing timestamps.
     */
    if (!shouldPublishAlert(alert)) {
      return "";
    }

    const char * rule_name = fty_proto_rule(alert);
    json += "data:{\"topic\":\"alarm/" + std::string(rule_name) + "\",\"payload\":";
    json += jsonPayload;
    json += "}\n\n";
  }
  return json;
}

std::string Sse::changeFtyProtoAsset2Json(fty_proto_t *asset)
{
  log_debug("SSE FtyProto asset message (name: %s, operation: %s)", fty_proto_name(asset), fty_proto_operation(asset));

  //return value
  std::string json = "";

  std::string nameElement = std::string(fty_proto_name(asset));
  //Check operation
  //if delete send json
  if (streq(fty_proto_operation(asset), FTY_PROTO_ASSET_OP_DELETE))
  {
    log_debug("SSE get an delete message");
    if (_assetsOfDatacenter.find(nameElement) == _assetsOfDatacenter.end())
    {
      //The asset is maybe without location
      if (_assetsWithNoLocation.find(nameElement) != _assetsWithNoLocation.end())
      {
        //remove this asset from the list without location
        _assetsWithNoLocation.erase(nameElement);
      }
      else
      {
        log_debug("skipping due to element_src '%s' not being in the list", fty_proto_name(asset));
        return json;
      }
    }
    else
    {
      //remove this asset from the assets list
      _assetsOfDatacenter.erase(nameElement);
    }
    json += "data:{\"topic\":\"asset/" + nameElement + "\",\"payload\":{}}\n\n";
  }
  else if (streq(fty_proto_operation(asset), FTY_PROTO_ASSET_OP_UPDATE)
          || streq(fty_proto_operation(asset), FTY_PROTO_ASSET_OP_INVENTORY)
          || streq(fty_proto_operation(asset), FTY_PROTO_ASSET_OP_CREATE))
  {
    log_debug("SSE get an update, create or inventory message");

    if (streq(fty_proto_operation(asset), FTY_PROTO_ASSET_OP_UPDATE)
        || streq(fty_proto_operation(asset), FTY_PROTO_ASSET_OP_INVENTORY))
    {
      //if update
      //Check if asset is in asset element
      if (_assetsOfDatacenter.find(nameElement) == _assetsOfDatacenter.end())
      {
        //The asset is maybe without location
        if (_assetsWithNoLocation.find(nameElement) != _assetsWithNoLocation.end())
        {
          //remove this asset from the list without location
          _assetsWithNoLocation.erase(nameElement);
          //if not in the datacenter, send a "delete" message by sse
          if (!isAssetInDatacenter(asset))
          {
            json += "data:{\"topic\":\"asset/" + nameElement + "\",\"payload\":{}}\n\n";
          }
          else
          {
            //Add this asset in the list of assets of the datacenter
              _assetsOfDatacenter.emplace(std::make_pair(nameElement, 5));
          }
        }
        else
        {
          log_debug("skipping due to element_src '%s' is not an element of the datacenter",
                    nameElement.c_str());
          return json;
        }
      }
    }
    else
    {
      //fty_proto_operation(asset) ==  FTY_PROTO_ASSET_OP_CREATE
      //Check if parent is null
      log_debug("Asset parent : %s",fty_proto_aux_string(asset,"parent","0"));
      if(streq(fty_proto_aux_string(asset,"parent","0"),"0"))
      {
        //Check if the asset is a device
        const char *type = fty_proto_aux_string(asset, "type", "none");
        log_debug("Asset type : %s", type);

        // XXX: autodiscovered items seems to not have a type, need to take a closer look...
        if (streq(type, "device") || streq(type, "none"))
        {
          //Add in the list of asset without location, will send a normal create message
          _assetsWithNoLocation.emplace(std::make_pair(nameElement, 5));
        }
        else
        {
          //Asset not in the datacenter which is not a device : Don't send any message
          return json;
        }
      }
      else if (!isAssetInDatacenter(asset))
      {
        //Check if the parent is the filtered datacenter
        //if not the same datacenter, return
        log_debug("skipping due to element_src '%s' is not an element of the datacenter",
                  nameElement.c_str());
        return json;
      }
      else
      {
        //Add in the list of asset of the datacenter
        _assetsOfDatacenter.emplace(std::make_pair(nameElement, 5));
      }
    }

    //get id of this element
    int64_t elemId = DBAssets::name_to_asset_id(nameElement);
    if (elemId == -1)
    {
      log_warning("Asset id not found");
      return json;
    }
    else if (elemId == -2)
    {
      log_warning("Error when get asset id");
    }
    log_debug("Sse-update get id Ok !!!");

    //All check Done
    std::string jsonPayload = "";
    if (json.empty())
    {
      jsonPayload = getJsonAsset(_clientMlm, elemId);
    }

    if (!jsonPayload.empty())
    {
      json += "data:{\"topic\":\"asset/" + nameElement + "\",\"payload\":";
      json += jsonPayload;
      json += "}\n\n";
    }
  }
  return json;
}

// decode a SSE generic message to JSON
std::string Sse::changeSseMessage2Json(zmsg_t *message)
{
  if (message == NULL)
    { log_error("message is NULL"); return ""; }

  // pop frames
  std::string topic, jsonPayload, assetID;
  char *aux;

  aux = zmsg_popstr(message);
  topic = aux ? aux : "";
  zstr_free(&aux);

  aux = zmsg_popstr(message);
  jsonPayload = aux ? aux : "";
  zstr_free(&aux);

  aux = zmsg_popstr(message);
  assetID = aux ? aux : "";
  zstr_free(&aux);

  // check asset (assetID is optional)
  if (!assetID.empty()
    && (_assetsOfDatacenter.find(assetID) == _assetsOfDatacenter.end())
  )
  {
    //NOTE: here, message is (partially) consumed and can't be handled elsewhere on return
    log_debug("skipping due to element_src '%s' not being in the list", assetID.c_str());
    return "";
  }

  return "data:{\"topic\":\"" + topic + "\",\"payload\":" + jsonPayload + "}\n\n";
}

bool Sse::isAssetInDatacenter(fty_proto_t *asset)
{
  int i = 1;
  const char * parentName = fty_proto_aux_string(asset, ("parent_name." + std::to_string(i)).c_str(), "not found");
  bool found = streq(parentName, _datacenter.c_str());

  while (!found && !streq(parentName, "not found"))
  {
    i++;
    parentName = fty_proto_aux_string(asset, ("parent_name." + std::to_string(i)).c_str(), "not found");
    found = streq(parentName, _datacenter.c_str());
  }
  log_debug("Sse : Asset %s found",found ? "":"not");
  return found;
}

bool Sse::shouldPublishAlert(fty_proto_t *alert)
{
  bool shouldPublish = true;
  const char *alertRule = fty_proto_rule(alert);
  AlertState alertState(fty_proto_state(alert), fty_proto_severity(alert));
  auto alertStateIt = _alertStates.find(alertRule);

  if (alertStateIt != _alertStates.end())
  {
    if (alertStateIt->second == alertState)
    {
      shouldPublish = false;
    }
    else
    {
      alertStateIt->second = alertState;
    }
  }
  else
  {
    _alertStates.emplace(alertRule, alertState);
  }

  return shouldPublish;
}

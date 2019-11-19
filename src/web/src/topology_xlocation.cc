////////////////////////////////////////////////////////////////////////
// ./src/web/src/topology_xlocation.cc.cpp
// generated with ecppc
//

#include <tnt/ecpp.h>
#include <tnt/convert.h>
#include <tnt/httprequest.h>
#include <tnt/httpreply.h>
#include <tnt/httpheader.h>
#include <tnt/http.h>
#include <tnt/data.h>
#include <tnt/componentfactory.h>
#include <stdexcept>

// <%pre>
#line 25 "./src/web/src/topology_xlocation.ecpp"

#include <fty_common.h>
#include <fty_common_macros.h>
#include <fty_common_rest_helpers.h>
#include <fty_common_db.h>
#include <fty_common_mlm_tntmlm.h>

// set S with MSG popped frame (no memleak, S untouched if NULL frame)
static void zmsg_pop_s (zmsg_t *msg, std::string & s)
{
    char *aux = msg ? zmsg_popstr (msg) : NULL;
    if (aux) s = aux;
    zstr_free (&aux);
}

// </%pre>

namespace
{
class _component_ : public tnt::EcppComponent
{
    _component_& main()  { return *this; }

  protected:
    ~_component_();

  public:
    _component_(const tnt::Compident& ci, const tnt::Urlmapper& um, tnt::Comploader& cl);

    unsigned operator() (tnt::HttpRequest& request, tnt::HttpReply& reply, tnt::QueryParams& qparam);
};

static tnt::ComponentFactoryImpl<_component_> Factory("topology_xlocation");

// <%shared>
// </%shared>

// <%config>
// </%config>

#define SET_LANG(lang) \
     do \
     { \
       request.setLang(lang); \
       reply.setLocale(request.getLocale()); \
     } while (false)

_component_::_component_(const tnt::Compident& ci, const tnt::Urlmapper& um, tnt::Comploader& cl)
  : EcppComponent(ci, um, cl)
{
  // <%init>
  // </%init>
}

_component_::~_component_()
{
  // <%cleanup>
  // </%cleanup>
}

unsigned _component_::operator() (tnt::HttpRequest& request, tnt::HttpReply& reply, tnt::QueryParams& qparam)
 {

#line 41 "./src/web/src/topology_xlocation.ecpp"
  typedef UserInfo user_type;
  TNT_REQUEST_GLOBAL_VAR(user_type, user, "UserInfo user", ());   // <%request> UserInfo user
#line 42 "./src/web/src/topology_xlocation.ecpp"
  typedef bool database_ready_type;
  TNT_REQUEST_GLOBAL_VAR(database_ready_type, database_ready, "bool database_ready", ());   // <%request> bool database_ready
  // <%cpp>
#line 44 "./src/web/src/topology_xlocation.ecpp"

{
    // verify server is ready
    if (!database_ready) {
        log_debug ("Database is not ready yet.");
        std::string err = TRANSLATE_ME("Database is not ready yet, please try again after a while.");
        http_die ("internal-error", err.c_str ());
    }

    // Sanity check end

    // ##################################################
    // check user permissions
    static const std::map <BiosProfile, std::string> PERMISSIONS = {
            {BiosProfile::Dashboard, "R"},
            {BiosProfile::Admin,     "R"}
            };
    CHECK_USER_PERMISSIONS_OR_DIE (PERMISSIONS);

    //ftylog_setVeboseMode(ftylog_getInstance());
    log_trace ("in %s", request.getUrl().c_str ());

    const char *ADDRESS = AGENT_FTY_ASSET; // "asset-agent" 42ty/fty-asset
    const char *SUBJECT = "TOPOLOGY";
    const char *COMMAND = "LOCATION";

    // get params
    std::string parameter_name;
    std::string asset_id;
    {
        std::string from = qparam.param("from");
        std::string to = qparam.param("to");

        // not-empty count
        int ne_count = (from.empty()?0:1) + (to.empty()?0:1);
        if (ne_count != 1) {
            log_error ("unexpected parameter (ne_count: %d)", ne_count);
            if (ne_count == 0) {
                http_die("request-param-required", "from/to");
            }
            else {
                std::string err = TRANSLATE_ME("Only one parameter can be specified at once: 'from' or 'to'");
                http_die("parameter-conflict", err.c_str ());
            }
        }

        if (!from.empty()) {
            parameter_name = "from";
            asset_id = from;
        }
        else if (!to.empty()) {
            parameter_name = "to";
            asset_id = to;
        }
        else {
            log_error ("unexpected parameter");
            std::string err = TRANSLATE_ME("Unexpected parameter");
            http_die ("internal-error", err.c_str());
        }
    }

    log_trace ("%s, parameter_name: '%s', asset_id: '%s'",
        request.getUrl().c_str (), parameter_name.c_str (), asset_id.c_str ());

    std::string argList = parameter_name + ": " + asset_id;

    // get options (depend on parameter_name)
    std::string options;
    {
        std::string filter = qparam.param("filter");
        std::string feed_by = qparam.param("feed_by");
        std::string recursive = qparam.param("recursive");

        if (!filter.empty()) argList += ", filter: " + filter;
        if (!feed_by.empty()) argList += ", feed_by: " + feed_by;
        if (!recursive.empty()) argList += ", recursive: " + recursive;

        if (parameter_name == "to") {
            // not-empty count
            int ne_count = (filter.empty()?0:1) + (feed_by.empty()?0:1) + (recursive.empty()?0:1);

            if (ne_count != 0) {
                log_error("No option allowed by location/to request");
                std::string err = TRANSLATE_ME("No additonal parameter is allowed with parameter 'to'");
                http_die("parameter-conflict", err.c_str ());
            }
        }
        else { // parameter_name == "from"
            // recursive, boolean, 'false' by default
            if (recursive.empty()) recursive = "false";
            std::transform (recursive.begin(), recursive.end(), recursive.begin(), ::tolower);
            if (recursive != "true" && recursive != "false") {
                log_error("Boolean value expected ('%s')", recursive.c_str());
                http_die("request-param-bad", "recursive", recursive.c_str(), "'true'/'false'");
            }

            // filter token, string, empty by default
            std::transform (filter.begin(), filter.end(), filter.begin(), ::tolower);
            if (!filter.empty ()) {
                if (filter != "rooms"
                    && filter != "rows"
                    && filter != "racks"
                    && filter != "devices"
                    && filter != "groups") {
                    http_die("request-param-bad", "filter", filter.c_str(), "'rooms'/'rows'/'racks'/'groups'/'devices'");
                }
            }

            // feed_by device, string, empty by default
            if (!feed_by.empty ()) {
                if (filter != "devices") {
                    std::string err = TRANSLATE_ME("Variable 'feed_by' can be specified only with 'filter=devices'");
                    http_die("parameter-conflict", err.c_str ());
                }
                if (asset_id == "none") {
                    std::string err = TRANSLATE_ME("Variable 'from' can not be 'none' if variable 'feed_by' is set.");
                    http_die("parameter-conflict", err.c_str ());
                }

                //persist::is_power_device coded in db/topology2.cc
                //tntdb::Connection conn = tntdb::connect (DBConn::url);
                //if (!persist::is_power_device (conn, feed_by)) {
                //    std::string expected = TRANSLATE_ME("must be a power device.");
                //    http_die("request-param-bad", "feed_by", feed_by.c_str (), expected.c_str ());
                //}
            }

            // set options (json paylaod)
            options = "{ ";
            options.append("\"filter\": \"").append(filter).append("\", ");
            options.append("\"feed_by\": \"").append(feed_by).append("\", ");
            options.append("\"recursive\": ").append(recursive); // bool
            options.append(" }");
        }
    }

    // accept 'none' asset_id only for 'from' request
    if (asset_id == "none" && parameter_name != "from") {
        log_error ("unexpected 'none' parameter");
        std::string err = TRANSLATE_ME("'none' parameter is not allowed with the '%s' request", parameter_name.c_str());
        http_die("parameter-conflict", err.c_str ());
    }

    // db checks (except if asset_id == 'none')
    if (asset_id != "none") {
        // asset_id valid?
        if (!persist::is_ok_name (asset_id.c_str ()) ) {
            std::string expected = TRANSLATE_ME("valid asset name");
            http_die ("request-param-bad", parameter_name.c_str(), asset_id.c_str (), expected.c_str ());
        }
        // asset_id exist?
        int64_t rv = DBAssets::name_to_asset_id (asset_id);
        if (rv == -1) {
            std::string err = TRANSLATE_ME("existing asset name");
            http_die ("request-param-bad", parameter_name.c_str(), asset_id.c_str (), err.c_str ());
        }
        if (rv == -2) {
            std::string err = TRANSLATE_ME("Connection to database failed.");
            http_die ("internal-error", err.c_str ());
        }
    }

    // connect to mlm client
    MlmClientPool::Ptr client = mlm_pool.get ();
    if (!client.getPointer ()) {
        log_error ("mlm_pool.get () failed");
        std::string err = TRANSLATE_ME("Connection to mlm client failed.");
        http_die ("internal-error", err.c_str());
    }

    // set/send req, recv response
    zmsg_t *req = zmsg_new ();
    if (!req) {
        log_error ("zmsg_new () failed");
        std::string err = TRANSLATE_ME("Memory allocation failed.");
        http_die ("internal-error", err.c_str());
    }

    zmsg_addstr (req, COMMAND);
    zmsg_addstr (req, parameter_name.c_str ());
    zmsg_addstr (req, asset_id.c_str ());
    if (!options.empty()) zmsg_addstr (req, options.c_str ());
    zmsg_t *resp = client->requestreply (ADDRESS, SUBJECT, 5, &req);
    zmsg_destroy (&req);

    #define CLEANUP { zmsg_destroy (&resp); }

    if (!resp) {
        CLEANUP;
        log_error ("client->requestreply (timeout = '5') failed");
        std::string err = TRANSLATE_ME("Request to mlm client failed (timeout reached).");
        http_die ("internal-error", err.c_str());
    }

    // get resp. header
    std::string rx_command, rx_asset_id, rx_status;
    zmsg_pop_s(resp, rx_command);
    zmsg_pop_s(resp, rx_asset_id);
    zmsg_pop_s(resp, rx_status);

    if (rx_command != COMMAND) {
        CLEANUP;
        log_error ("received inconsistent command ('%s')", rx_command.c_str ());
        std::string err = TRANSLATE_ME("Received inconsistent command ('%s').", rx_command.c_str ());
        http_die ("internal-error", err.c_str());
    }
    if (rx_asset_id != asset_id) {
        CLEANUP;
        log_error ("received inconsistent assetID ('%s')", rx_asset_id.c_str ());
        std::string err = TRANSLATE_ME("Received inconsistent asset ID ('%s').", rx_asset_id.c_str ());
        http_die ("internal-error", err.c_str());
    }
    if (rx_status != "OK") {
        std::string reason;
        zmsg_pop_s(resp, reason);
        CLEANUP;
        log_error ("received %s status (reason: %s) from mlm client", rx_status.c_str(), reason.c_str ());
        http_die ("request-param-bad", parameter_name.c_str(), argList.c_str(), JSONIFY (reason.c_str ()).c_str ());
        //http_die ("request-param-bad", parameter_name.c_str(), asset_id.c_str(), JSONIFY (reason.c_str ()).c_str ());
    }

    // result JSON payload
    std::string json;
    zmsg_pop_s(resp, json);
    if (json.empty()) {
        CLEANUP;
        log_error ("empty JSON payload");
        std::string err = TRANSLATE_ME("Received an empty JSON payload.");
        http_die ("internal-error", err.c_str());
    }
    CLEANUP;
    #undef CLEANUP

    // set body (status is 200 OK)
    reply.out () << json;
}

  // <%/cpp>
  return HTTP_OK;
}

} // namespace

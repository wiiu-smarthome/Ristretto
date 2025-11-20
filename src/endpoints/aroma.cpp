#include "aroma.hpp"

#include "../utils/logger.h"

#include <wups_backend/PluginContainer.h>
#include <wups_backend/PluginUtils.h>
#include <wups_backend/api.h>

void AromaEndpoints::registerEndpoints(HttpServer &server) {
    server.when("/aroma/plugins")->requested([](const HttpRequest &req) {
        PluginBackendApiErrorType err = PLUGIN_BACKEND_API_ERROR_NONE;

        std::vector<WUPSBackend::PluginContainer> plugins = WUPSBackend::PluginUtils::getLoadedPlugins(err);
        if (err) {
            DEBUG_FUNCTION_LINE("Failed to get loaded Aroma plugins");
            return HttpResponse{500, "text/plain", "Failed to get loaded Aroma plugins"};
        }


        miniJson::Json::_array res;

        for (WUPSBackend::PluginContainer &plugin : plugins) {
            miniJson::Json::_object plugin_json;

            plugin_json["name"]    = plugin.getMetaInformation().getName();
            plugin_json["author"]  = plugin.getMetaInformation().getAuthor();
            plugin_json["version"] = plugin.getMetaInformation().getVersion();

            res.push_back(plugin_json);
        }

        return HttpResponse{200, res};
    });
}

void AromaEndpoints::on_initialize_plugin() {
    WUPSBackend_InitLibrary();
}

void AromaEndpoints::on_deinitialize_plugin() {
    WUPSBackend_DeInitLibrary();
}

#include "title.h"
#include "../languages.h" // for access to titleLang

inline char *getTitleLongname(ACPMetaXml *meta) {
    char *ret;
    switch (titleLang) {
        case LANG_JAPANESE:
            ret = meta->longname_ja;
            break;
        case LANG_FRENCH:
            ret = meta->longname_fr;
            break;
        case LANG_GERMAN:
            ret = meta->longname_de;
            break;
        case LANG_ITALIAN:
            ret = meta->longname_it;
            break;
        case LANG_SPANISH:
            ret = meta->longname_es;
            break;
        case LANG_SIMPLIFIED_CHINESE:
            ret = meta->longname_zhs;
            break;
        case LANG_KOREAN:
            ret = meta->longname_ko;
            break;
        case LANG_DUTCH:
            ret = meta->longname_nl;
            break;
        case LANG_PORTUGUESE:
            ret = meta->longname_pt;
            break;
        case LANG_RUSSIAN:
            ret = meta->longname_ru;
            break;
        case LANG_TRADITIONAL_CHINESE:
            ret = meta->longname_zht;
            break;
        case LANG_ENGLISH:
        default:
            ret = meta->longname_en;
            break;
    }

    // Fallback for titles which don't have a language-specific translation
    if (ret != NULL && ret[0] == '\0') ret = meta->longname_en;
    return ret;
}

void registerTitleEndpoints(HttpServer &server) {
    server.when("/title/current")->requested([](const HttpRequest &req) {
        ACPTitleId id;
        ACPResult res = ACPGetTitleIdOfMainApplication(&id);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
            return HttpResponse{500, "text/plain", "Couldn't get the current title! Error at ACPGetTitleIdOfMainApplication"};
        }
        ACPMetaXml *meta = new ACPMetaXml;
        res              = ACPGetTitleMetaXml(id, meta);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml");
            return HttpResponse{500, "text/plain", "Couldn't get the title! Error at ACPGetTitleMetaXml"};
        }
        return HttpResponse{200, "text/plain", getTitleLongname(meta)};
    });

    server.when("/title/current/id")->requested([](const HttpRequest &req) {
        ACPTitleId id;
        ACPResult res = ACPGetTitleIdOfMainApplication(&id);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
            return HttpResponse{500, "text/plain", "Couldn't get the current title! Error at ACPGetTitleIdOfMainApplication"};
        }

        return HttpResponse{200, "text/plain", std::to_string(id)};
    });

    server.when("/title/current/info")->requested([](const HttpRequest &req) {
        ACPTitleId id;
        ACPResult res = ACPGetTitleIdOfMainApplication(&id);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
            return HttpResponse{500, "text/plain", "Couldn't get the current title! Error at ACPGetTitleIdOfMainApplication"};
        }
        ACPMetaXml *meta = new ACPMetaXml;
        res              = ACPGetTitleMetaXml(id, meta);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml");
            return HttpResponse{500, "text/plain", "Couldn't get the title! Error at ACPGetTitleMetaXml"};
        }

        int handle = MCP_Open();
        if (handle < 0) {
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }
        MCPTitleListType type;
        MCP_GetTitleInfo(handle, id, &type);

        MCP_Close(handle);

        miniJson::Json::_object ret;
        ret["id"]   = std::to_string(id);
        ret["name"] = getTitleLongname(meta);
        ret["type"] = std::to_string(type.appType);

        return HttpResponse{200, ret};
    });


    server.when("/title/current/name")->requested([](const HttpRequest &req) {
        ACPTitleId id;
        ACPResult res = ACPGetTitleIdOfMainApplication(&id);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
            return HttpResponse{500, "text/plain", "Couldn't get the current title! Error at ACPGetTitleIdOfMainApplication"};
        }
        ACPMetaXml *meta = new ACPMetaXml;
        res              = ACPGetTitleMetaXml(id, meta);
        if (res) {
            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml");
            return HttpResponse{500, "text/plain", "Couldn't get the title! Error at ACPGetTitleMetaXml"};
        }
        return HttpResponse{200, "text/plain", getTitleLongname(meta)};
    });

    server.when("/title/current/type")->requested([](const HttpRequest &req) {
        int handle = MCP_Open();
        if (handle < 0) {
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        // ACP doesn't have the application type, so lets get it from MCP
        uint64_t outId;
        MCPTitleListType type;

        MCP_GetTitleId(handle, &outId);
        MCP_GetTitleInfo(handle, outId, &type);

        MCP_Close(handle);

        // Frontend/API wrapper can translate to the actual app type: https://wut.devkitpro.org/mcp_8h_source.html#l00025
        return HttpResponse{200, "text/plain", std::to_string(type.appType)};
    });

    // NOT FOR HOMEBREW TITLES!!!!!!!
    server.when("/title/list")->requested([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Getting title list.");
        int handle = MCP_Open();
        if (handle < 0) { // some error?
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        uint32_t outCount;
        std::vector<MCPTitleListType> titleList(1000); // arbitrary number so we don't overflow
        MCPError error = MCP_TitleList(handle, &outCount, titleList.data(), titleList.size() * sizeof(MCPTitleListType));
        MCP_Close(handle);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_TitleList");
            return HttpResponse{500, "text/plain", "Couldn't get the title list! Error at MCP_TitleList"};
        }

        miniJson::Json::_object res;

        // This is my first time trying this with C++ vectors, so lets see what happens.
        // Ideally it will just keep rewriting to meta?
        for (auto &title : titleList) {
            ACPMetaXml meta alignas(0x40);

            // not all titles are actual game titles
            // TODO: For vWii titles, allow it under the condition we are able to
            // send back to the server that Ristretto won't be active.
            // All titles under MCP_APP_TYPE_GAME (or any Wii U system title) will
            // allow for Ristretto control inside of it: not sure about homebrew.
            //
            // MCP_APP_TYPE_ACCOUNT_APPS do not work: these things like notifications, account settings,
            // user settings, etc. will not launch or throw an error. (System Transfer for some reason
            // is in this category??? But for console security it should not be exposed anyways).
            if (title.appType == MCP_APP_TYPE_GAME ||
                title.appType == MCP_APP_TYPE_GAME_WII ||
                title.appType == MCP_APP_TYPE_SYSTEM_MENU ||
                title.appType == MCP_APP_TYPE_SYSTEM_APPS ||
                title.appType == MCP_APP_TYPE_SYSTEM_SETTINGS) {
                ACPResult acpError = ACPGetTitleMetaXml(title.titleId, &meta);
                if (acpError) {
                    DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml. Title ID %d", title.titleId);
                    continue;
                }

                // TODO: Consider returning other languages
                if (meta.longname_en[0] != '\0') {
                    DEBUG_FUNCTION_LINE_INFO("Finished %s", meta.longname_en);
                    try {
                        res[std::to_string(title.titleId)] = getTitleLongname(&meta);
                        DEBUG_FUNCTION_LINE_INFO("Written to JSON");
                    } catch (std::exception &e) {
                        DEBUG_FUNCTION_LINE_ERR("Failed to write title to JSON: %s\n", e.what());
                    }
                } else {
                    DEBUG_FUNCTION_LINE_INFO("No English longname - not proceeding");
                }
            }
        }

        return HttpResponse{200, res};
    });
}

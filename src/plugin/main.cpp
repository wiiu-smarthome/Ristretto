#include "../endpoints/account.hpp"
#include "../endpoints/aroma.hpp"
#include "../endpoints/cec.hpp"
#include "../endpoints/device.hpp"
#include "../endpoints/fp.hpp"
#include "../endpoints/gamepad.hpp"
#include "../endpoints/launch.hpp"
#include "../endpoints/odd.hpp"
#include "../endpoints/power.hpp"
#include "../endpoints/remote.hpp"
#include "../endpoints/sdhc.hpp"
#include "../endpoints/switch.hpp"
#include "../endpoints/title.hpp"
#include "../endpoints/vwii.hpp"

#include "../languages.h"
#include "../utils/logger.h"
#include "globals.h"

#include "http.hpp"

#include <coreinit/thread.h>

#include <nn/ac.h>

#include <notifications/notifications.h>

#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

#include <sys/iosupport.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Ristretto")
WUPS_PLUGIN_DESCRIPTION("HTTP Server for IoT communication");
WUPS_PLUGIN_VERSION("v1.2.1");
WUPS_PLUGIN_AUTHOR("ItzSwirlz");
WUPS_PLUGIN_LICENSE("BSD");

WUPS_USE_WUT_DEVOPTAB();       // Use the wut devoptabs
WUPS_USE_STORAGE("ristretto"); // Unique id for the storage api

HttpServer server;
bool server_made = false;

#define ENABLE_CEC_DEFAULT_VALUE    true
#define ENABLE_SERVER_DEFAULT_VALUE true

#define ENABLE_CEC_CONFIG_ID        "enableCEC"
#define ENABLE_SERVER_CONFIG_ID     "enableServer"
#define TITLE_LANG_CONFIG_ID        "titleLang"

bool enableServer = ENABLE_SERVER_DEFAULT_VALUE;
bool enableCEC    = ENABLE_CEC_DEFAULT_VALUE;

void make_server() {
    if (server_made) {
        return;
    }

    server_made = true;
    DEBUG_FUNCTION_LINE("Server started.");

    try {
        // Empty endpoint to allow for device discovery.
        server.when("/")->requested([](const HttpRequest &req) {
            return HttpResponse{200, "text/plain", "Ristretto"};
        });

        // Console hardware
        DeviceEndpoints::registerEndpoints(server);
        GamepadEndpoints::registerEndpoints(server);
        PowerEndpoints::registerEndpoints(server);


        // CEC
        if (enableCEC) {
            CECEndpoints::registerEndpoints(server);
        }


        // Nintendo/Pretendo Network + Services
        AccountEndpoints::registerEndpoints(server);
        FPEndpoints::registerEndpoints(server);


        // Input
        RemoteEndpoints::registerEndpoints(server);


        // Filesystems and Storage
        ODDEndpoints::registerEndpoints(server);
        SDHCEndpoints::registerEndpoints(server);


        // Titles
        LaunchEndpoints::registerEndpoints(server);
        SwitchEndpoints::registerEndpoints(server);
        TitleEndpoints::registerEndpoints(server);


        // vWii
        VWiiEndpoints::registerEndpoints(server);


        // Custom Firmware
        AromaEndpoints::registerEndpoints(server);


        // TODO: Make the port configurable
        server.startListening(8572);
    } catch (std::exception &e) {
        // FIXME: write good strings that can easily be translated
        NotificationModule_AddErrorNotification("Ristretto threw an exception. If the problem persists, check system logs.");
        DEBUG_FUNCTION_LINE_INFO("Exception thrown in the HTTP server: %s\n", e.what());
    }
}

void stop_server() {
    // dont shut down what doesnt exist
    if (!server_made) return;

    server.shutdown();
    server_made = false;

    DEBUG_FUNCTION_LINE("Server shut down.");
}

void make_server_on_thread() {
    try {
        std::jthread thready(make_server);

        auto threadHandle = (OSThread *) thready.native_handle();
        OSSetThreadName(threadHandle, "Ristretto");
        OSSetThreadAffinity(threadHandle, OS_THREAD_ATTRIB_AFFINITY_CPU2);

        thready.detach();
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("Exception thrown trying to make the server thread: %s\n", e.what());
    }
}

void enableCECChanged(ConfigItemBoolean *item, bool newValue) {
    if (newValue != enableCEC) {
        WUPSStorageAPI::Store(ENABLE_CEC_CONFIG_ID, newValue);
    }

    enableCEC = newValue;
}

void enableServerChanged(ConfigItemBoolean *item, bool newValue) {
    // If the value has changed, we store it in the storage.
    if (newValue != enableServer)
        WUPSStorageAPI::Store(ENABLE_SERVER_CONFIG_ID, newValue);

    enableServer = newValue;
}

static void titleLangChanged(ConfigItemMultipleValues *item, uint32_t newValue) {
    if (newValue != titleLang) {
        WUPSStorageAPI::Store(TITLE_LANG_CONFIG_ID, newValue);
    }
    titleLang = newValue;
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    constexpr WUPSConfigItemMultipleValues::ValuePair titleLangMap[] = {
            {LANG_ENGLISH, "English"},
            {LANG_JAPANESE, "Japanese"},
            {LANG_FRENCH, "French"},
            {LANG_GERMAN, "German"},
            {LANG_ITALIAN, "Italian"},
            {LANG_SPANISH, "Spanish"},
            {LANG_SIMPLIFIED_CHINESE, "Chinese (Simplified)"},
            {LANG_KOREAN, "Korean"},
            {LANG_DUTCH, "Dutch"},
            {LANG_PORTUGUESE, "Portuguese"},
            {LANG_RUSSIAN, "Russian"},
            {LANG_TRADITIONAL_CHINESE, "Chinese (Traditional)"}};

    try {
        root.add(WUPSConfigItemBoolean::Create(ENABLE_SERVER_CONFIG_ID, "Enable Server", ENABLE_SERVER_DEFAULT_VALUE, enableServer, enableServerChanged));
        root.add(WUPSConfigItemBoolean::Create(ENABLE_CEC_CONFIG_ID, "Enable HDMI-CEC (experimental)", ENABLE_CEC_DEFAULT_VALUE, enableCEC, enableCECChanged));
        root.add(WUPSConfigItemMultipleValues::CreateFromValue(TITLE_LANG_CONFIG_ID, "Title Language:", TITLE_LANG_DEFAULT_VALUE, titleLang, titleLangMap, titleLangChanged));
        root.add(WUPSConfigItemStub::Create("DISCLAIMER: You use this plugin at your own risk."));
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Creating config menu failed: %s", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();

    if (server_made && !enableServer) stop_server();
    else if (!server_made && enableServer) {
        make_server_on_thread();
    }
}

// Function hooking for the HTTP server logs. Thank you to DanielKO
ssize_t
write_to_log(_reent *, void *, const char *ptr, size_t len) {
    try {
        // only way to guarantee it's null-terminated
        std::string buf{ptr, len};
        if (!WHBLogWrite(buf.c_str()))
            return -1;
        return buf.size();
    } catch (...) {
        return -1;
    }
}

__attribute__((__constructor__)) void
init_stdio() {
    static devoptab_t dev_out;
    dev_out.name           = "stdout";
    dev_out.write_r        = write_to_log;
    devoptab_list[STD_OUT] = &dev_out;
}

// Gets called ONCE when the plugin was loaded.
INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`. See the README for more information.
    WHBLogCafeInit();
    WHBLogUdpInit();

    DEBUG_FUNCTION_LINE("Hello world! - Ristretto");

    NotificationModule_InitLibrary();

    SDHCEndpoints::on_initialize_plugin();
    AromaEndpoints::on_initialize_plugin();


    WUPSConfigAPIOptionsV1 configOptions = {.name = "Ristretto"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }

    WUPSStorageError storageRes;
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(ENABLE_SERVER_CONFIG_ID, enableServer, ENABLE_SERVER_DEFAULT_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(TITLE_LANG_CONFIG_ID, titleLang, (uint32_t) TITLE_LANG_DEFAULT_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("SaveStorage failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }

    if (enableCEC) {
        CECEndpoints::on_initialize_plugin();
    }
}

// Gets called when the plugin will be unloaded.
DEINITIALIZE_PLUGIN() {
    DEBUG_FUNCTION_LINE("Ristretto deinitializing.");
    stop_server();

    AromaEndpoints::on_deinitialize_plugin();
    SDHCEndpoints::on_deinitialize_plugin();

    NotificationModule_DeInitLibrary();

    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
}

// Connections reset every time an application is launched.
ON_APPLICATION_START() {
    if (!enableServer) return;

    nn::ac::Initialize();
    nn::ac::ConnectAsync();

    AccountEndpoints::on_application_starts();

    if (enableCEC) {
        CECEndpoints::on_application_starts();
    }

    make_server_on_thread();
}

ON_APPLICATION_ENDS() {
    if (!enableServer) return;

    AccountEndpoints::on_application_ends();
    stop_server();
}

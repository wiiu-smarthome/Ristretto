#include "gamepad.hpp"

#include "../plugin/globals.h"
#include "../utils/logger.h"

#include <nsysccr/cdc.h>

#include <wups.h>

void GamepadEndpoints::registerEndpoints(HttpServer &server) {
    server.when("/gamepad/battery")->requested([](const HttpRequest &req) {
        std::string ret = std::format("{:d}", vpad_battery);
        return HttpResponse{200, "text/plain", ret};
    });

    // Gets the hardware version in decimal where, when you convert the returning value (int) to hexadecimal:
    // The first two bytes are the major version
    // The second two bytes are the minor version
    // The remaining bytes are the patch version
    // The frontend application/integration should handle parsing this data.
    server.when("/gamepad/version")->requested([](const HttpRequest &req) {
        CCRCDCSoftwareVersion *version = new CCRCDCSoftwareVersion();

        if (CCRCDCSoftwareGetVersion(CCR_CDC_DESTINATION_DRC0, version) != 0) {
            DEBUG_FUNCTION_LINE_ERR("Error at CCRCDCSoftwareGetVersion");
            return HttpResponse{500, "text/plain", "Couldn't get the GamePad software version! Error at CCRCDCSoftwareGetVersion"};
        }

        return HttpResponse{200, "text/plain", std::format("{:d}", version->runningVersion)};
    });
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffers, uint32_t count, VPADReadError *outError) {
    int result = real_VPADRead(chan, buffers, count, outError);
    if (outError != NULL) {
        if (*outError == VPAD_READ_SUCCESS) {
            vpad_battery = buffers->battery;
            if (button_value != 0) {
                buffers->hold |= button_value;
                button_value = 0; // done
            }
        }
    }
    return result;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);

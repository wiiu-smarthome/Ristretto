#include "SDHC.hpp"

#include <sdutils/sdutils.h>

void SDHCEndpoints::registerEndpoints(HttpServer &server) {
    server.when("/sdhc/mounted")->requested([](const HttpRequest &req) {
        bool mounted;
        SDUtils_IsSdCardMounted(&mounted);

        std::string ret = std::format("{:d}", mounted);
        return HttpResponse{200, "text/plain", ret};
    });
}

void SDHCEndpoints::on_initialize_plugin() {
    SDUtils_InitLibrary();
}

void SDHCEndpoints::on_deinitialize_plugin() {
    SDUtils_DeInitLibrary();
}

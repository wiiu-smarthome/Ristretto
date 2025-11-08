#include "account.h"
#include <nn/act/client_cpp.h>

void registerAccountEndpoints(HttpServer &server) {
    server.when("/account/id")->requested([](const HttpRequest &req) {
        char accountId[nn::act::AccountIdSize];

        if (nn::act::GetAccountId(accountId) != 0) {
            return HttpResponse{200, "text/plain", accountId};
        }

        return HttpResponse(500, "text/plain", "Unable to get account ID!");
    });

    server.when("/account/principalid")->requested([](const HttpRequest &req) {
        nn::act::PrincipalId pid = nn::act::GetPrincipalId();
        return HttpResponse{200, "text/plain", std::format("{:d}", pid)};
    });
}

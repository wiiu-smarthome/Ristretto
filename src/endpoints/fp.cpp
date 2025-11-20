#include "fp.hpp"

#include <nn/fp/fp_cpp.h>

void FPEndpoints::registerEndpoints(HttpServer &server) {
    server.when("/fp/comment")->requested([](const HttpRequest &req) {
        nn::fp::Comment myComment;

        if (nn::fp::GetMyComment(&myComment) != 0) {
            miniJson::Json::_object ret;

            // TODO: Find the names
            ret["unk_0x00"] = myComment.unk_0x00;
            ret["unk_0x01"] = myComment.unk_0x01;
            ret["comment"]  = (char *) myComment.comment;
            return HttpResponse{200, ret};
        }

        return HttpResponse(500, "text/plain", "Unable to get FP comment!");
    });

    server.when("/fp/logged_in")->requested([](const HttpRequest &req) {
        return HttpResponse{200, "text/plain", std::format("{:d}", nn::fp::HasLoggedIn())};
    });

    server.when("/fp/online")->requested([](const HttpRequest &req) {
        return HttpResponse{200, "text/plain", std::format("{:d}", nn::fp::IsOnline())};
    });
}

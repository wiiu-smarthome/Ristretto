#include "http.hpp"

namespace AccountEndpoints {
    void on_application_starts();
    void on_application_ends();

    void registerEndpoints(HttpServer &server);
} // namespace AccountEndpoints

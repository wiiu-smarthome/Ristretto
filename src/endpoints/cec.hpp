#include "http.hpp"

namespace CECEndpoints {
    // TODO: Should we also close CEC on plugin deinit or application end?
    void on_application_starts();

    void on_initialize_plugin();

    void registerEndpoints(HttpServer &server);
} // namespace CECEndpoints

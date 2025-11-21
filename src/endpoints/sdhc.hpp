#include <http.hpp>

namespace SDHCEndpoints {
    void on_deinitialize_plugin();
    void on_initialize_plugin();

    void registerEndpoints(HttpServer &server);
}; // namespace SDHCEndpoints

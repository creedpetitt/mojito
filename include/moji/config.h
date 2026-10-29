#pragma once

#include <cstdint>
#include <cstddef>
#include <chrono>
#include <string>
#include <string_view>

namespace moji {

enum class ExecutionModel {
    PerWorker = 0, // Default
    SharedContext = 1
};

// enum class IoMode {
//     Auto = 0, 
//     Sync = 1, 
//     Async = 2
// };

struct Config {
    uint16_t port{8080};
    int threads{1};
    size_t max_body_size{10 * 1024 * 1024}; // 10 MB
    std::chrono::seconds timeout_seconds{30};
    std::chrono::seconds shutdown_timeout{30};
    std::string server_name{"mojito/0.1.0"};
    bool expose_client_ip{false};
    ExecutionModel execution_model{ExecutionModel::PerWorker};
    // IoMode io_mode{IoMode::Async};
};

class ConfigBuilder {
public:
    ConfigBuilder& port(uint16_t);
    ConfigBuilder& threads(int);
    ConfigBuilder& max_body_size(size_t);
    ConfigBuilder& timeout(std::chrono::seconds);
    ConfigBuilder& shutdown(std::chrono::seconds);
    ConfigBuilder& server_name(std::string_view);
    ConfigBuilder& expose_client_ip(bool enabled = false);
    ConfigBuilder& execution_model(ExecutionModel);
    // ConfigBuilder& io_mode(IoMode);    
    Config build();

private:
    Config config_;
};

} // namespace moji

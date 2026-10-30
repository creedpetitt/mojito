#include "moji/config.h"

namespace moji {

    ConfigBuilder&
    ConfigBuilder::port(const u_int16_t port) {
        config_.port = port;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::threads(const int threads) {
        config_.threads = threads;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::max_body_size(const size_t size) {
        config_.max_body_size = size;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::timeout(const std::chrono::seconds timeout) {
        config_.timeout_seconds = timeout;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::shutdown(const std::chrono::seconds shutdown_t) {    
        config_.shutdown_timeout = shutdown_t;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::server_name(const std::string_view name) {
        config_.server_name = name;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::expose_client_ip(const bool enabled) {
        config_.expose_client_ip = enabled;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::execution_model(const ExecutionModel mode) {
        config_.execution_model = mode;
        return *this;
    }

    ConfigBuilder& 
    ConfigBuilder::protocol(const NetworkProtocol protocol) {
        config_.protocol = protocol;
        return *this;
    }

    ConfigBuilder&
    ConfigBuilder::certificate_chain_file(std::string_view certificate) {
        config_.certificate_chain_file = certificate;
        return *this;
    }
    
    ConfigBuilder&
    ConfigBuilder::private_key_file(std::string_view private_key) {
        config_.private_key_file = private_key;
        return *this;
    }
    
    Config ConfigBuilder::build() {
        return config_;
    }
} // namespace moji

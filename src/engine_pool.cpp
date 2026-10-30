#include <moji/config.h>

#include "engine_pool.h"

#include <boost/asio.hpp>

#include <stdexcept>

namespace moji {

EnginePool::EnginePool(ExecutionModel mode, int workers)
    : mode_(mode), worker_count_(workers) {
        if (workers < 1) {
            throw std::invalid_argument("moji: Workers must be at least 1");
        }
        if (mode_ == ExecutionModel::SharedContext) {
            contexts_.reserve(1);
            contexts_.emplace_back(std::make_unique<net::io_context>());
            guards_.push_back(net::make_work_guard(contexts_.at(0)->get_executor()));
        } else { // Per Worker 
            contexts_.reserve(workers);
            for (int i = 0; i < workers; ++i) {
                contexts_.emplace_back(std::make_unique<net::io_context>(1));
                guards_.push_back(net::make_work_guard(contexts_.at(i)->get_executor()));
            }
        }
}

net::io_context& EnginePool::context_for_worker(size_t worker) {
    const auto index = mode_ == ExecutionModel::SharedContext ? 0 : worker;
    return *contexts_.at(index);
}

net::any_io_executor EnginePool::listener_executor() {
    auto& context = *contexts_.at(0);
    if (mode_ == ExecutionModel::SharedContext) {
        return net::make_strand(context);
    }
    return context.get_executor();
}

net::any_io_executor EnginePool::next_connection_executor() {
    if (mode_ == ExecutionModel::SharedContext) {
        return net::make_strand(*contexts_.at(0));
    }
    const auto index = next_connection_;
    next_connection_ = (next_connection_ + 1) % contexts_.size();
    return contexts_.at(index)->get_executor();
}

net::any_io_executor EnginePool::spawn_task_executor() {
    if (mode_ == ExecutionModel::SharedContext) {
        return contexts_.at(0)->get_executor();
    }
    const auto index =
        next_task_.fetch_add(1, std::memory_order_relaxed)
        % contexts_.size();
    
    return contexts_.at(index)->get_executor();
}

void EnginePool::stop() noexcept {
    for (auto& context : contexts_) {
        context->stop();
    }
}

} // namespace moji


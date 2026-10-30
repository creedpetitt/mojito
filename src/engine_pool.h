#pragma once

#include <moji/config.h>

#include <boost/asio.hpp>

#include <atomic> 
#include <cstddef> 
#include <memory> 
#include <vector>

namespace net = boost::asio;

namespace moji {

class EnginePool {
    using WorkGuard = 
        net::executor_work_guard<net::io_context::executor_type>;
    
    public:
        EnginePool(ExecutionModel mode, int workers);

        net::io_context& context_for_worker(size_t worker);

        net::any_io_executor listener_executor();
        net::any_io_executor next_connection_executor();
        net::any_io_executor spawn_task_executor();

        void stop() noexcept;

    private:
       ExecutionModel mode_;
       size_t worker_count_;

       std::vector<std::unique_ptr<net::io_context>> contexts_;
       std::vector<WorkGuard> guards_;
   
       std::size_t next_connection_{0};
       std::atomic<std::size_t> next_task_{0};
        
};

} // namespace moji
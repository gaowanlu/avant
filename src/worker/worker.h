#pragma once
#include <atomic>
#include "event/event_poller.h"
#include "socket/socket_pair.h"
#include <memory>

namespace avant::worker
{
    class worker
    {
    public:
        worker();
        ~worker();
        void operator()();

        bool to_stop{false};
        bool is_stoped{false};
        int worker_id{-1};
        size_t max_client_num{0};

        std::shared_ptr<std::atomic<int>> curr_connection_num{nullptr};

        std::shared_ptr<avant::socket::socket_pair> main_worker_tunnel{nullptr};

        avant::event::event_poller epoller;
    };
};
// 000
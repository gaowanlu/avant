#pragma once

#include "worker/worker.h"
#include "connection/connection.h"

namespace avant::connection
{
    class websocket_ctx
    {
    public:
        websocket_ctx();
        ~websocket_ctx();

        // context create success
        void on_create(connection &conn_obj, worker::worker &worker_obj);
        // context destory
        void on_close();

        void on_event(uint32_t event);
        connection *conn_ptr{nullptr};
        worker::worker *worker_ptr{nullptr};
    };
}
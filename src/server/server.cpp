#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <avant-log/logger.h>
#include <chrono>
#include <vector>
#include "server/server.h"
#include "socket/socket.h"
#include "utility/singleton.h"
#include "task/task_type.h"
#include "socket/server_socket.h"

using namespace std;
using namespace avant::server;
using namespace avant::socket;
using namespace avant::utility;
using namespace avant::task;
using namespace avant::worker;
using namespace avant::socket;

server::server()
{
}

server::~server()
{
    // release SSL_CTX
    if (get_use_ssl() && m_ssl_context)
    {
        SSL_CTX_free(m_ssl_context);
    }
}

void server::set_listen_info(const std::string &ip, int port)
{
    m_ip = ip;
    m_port = port;
}

void server::start()
{
    LOG_ERROR("server::start ...");

    // init OpenSSL CTX
    if (get_use_ssl())
    {
        LOG_ERROR("OpenSSL_version %s", OpenSSL_version(OPENSSL_VERSION));
        LOG_ERROR("SSLeay_version %s", SSLeay_version(SSLEAY_VERSION));
        SSL_library_init();
        SSL_load_error_strings();
        m_ssl_context = SSL_CTX_new(SSLv23_server_method());
        if (!m_ssl_context)
        {
            LOG_ERROR("SSL_CTX_new error");
            return;
        }
        SSL_CTX_set_options(m_ssl_context, SSL_OP_SINGLE_DH_USE);

        std::string crt_pem_path = get_crt_pem();
        int i_ret = SSL_CTX_use_certificate_file(m_ssl_context, crt_pem_path.c_str(), SSL_FILETYPE_PEM);
        if (1 != i_ret)
        {
            LOG_ERROR("SSL_CTX_use_certificate_file error: %s", ERR_error_string(ERR_get_error(), nullptr));
            return;
        }
        std::string key_pem_path = get_key_pem();
        i_ret = SSL_CTX_use_PrivateKey_file(m_ssl_context, key_pem_path.c_str(), SSL_FILETYPE_PEM);
        if (1 != i_ret)
        {
            LOG_ERROR("SSL_CTX_use_PrivateKey_file error: %s", ERR_error_string(ERR_get_error(), nullptr));
            return;
        }
    }

    {
        LOG_ERROR("m_app_id %s", m_app_id.c_str());
        LOG_ERROR("m_ip %s", m_ip.c_str());
        LOG_ERROR("m_port %d", m_port);
        LOG_ERROR("m_worker_cnt %d", m_worker_cnt);
        LOG_ERROR("m_max_client_cnt %d", m_max_client_cnt);
        LOG_ERROR("m_epoll_wait_time %d", m_epoll_wait_time);
        LOG_ERROR("m_accept_per_tick %d", m_accept_per_tick);
        LOG_ERROR("m_http_static_dir %s", m_http_static_dir.c_str());
        LOG_ERROR("m_lua_dir %s", m_lua_dir.c_str());
        LOG_ERROR("m_task_type %s", m_task_type.c_str());
        LOG_ERROR("m_use_ssl %d", (int)m_use_ssl);
        LOG_ERROR("m_crt_pem %s", m_crt_pem.c_str());
        LOG_ERROR("m_key_pem %s", m_key_pem.c_str());
    }

    on_start();
}

void server::set_worker_cnt(size_t worker_cnt)
{
    m_worker_cnt = worker_cnt;
}

void server::set_max_client_cnt(size_t max_client_cnt)
{
    m_max_client_cnt = max_client_cnt;
}

void server::set_epoll_wait_time(size_t epoll_wait_time)
{
    m_epoll_wait_time = epoll_wait_time;
}

void server::set_task_type(std::string task_type)
{
    m_task_type = task_type;
}

void server::set_http_static_dir(std::string http_static_dir)
{
    m_http_static_dir = http_static_dir;
}

const std::string &server::get_http_static_dir()
{
    return m_http_static_dir;
}

void server::set_lua_dir(std::string lua_dir)
{
    m_lua_dir = lua_dir;
}

const std::string &server::get_lua_dir()
{
    return m_lua_dir;
}

task_type server::get_task_type()
{
    return str2task_type(m_task_type);
}

void server::set_use_ssl(bool use_ssl)
{
    m_use_ssl = use_ssl;
}

void server::set_crt_pem(std::string crt_pem)
{
    m_crt_pem = crt_pem;
}

void server::set_key_pem(std::string key_pem)
{
    m_key_pem = key_pem;
}

bool server::get_use_ssl()
{
    return m_use_ssl;
}

std::string server::get_crt_pem()
{
    return m_crt_pem;
}

std::string server::get_key_pem()
{
    return m_key_pem;
}

void server::config(const std::string &app_id,
                    const std::string &ip,
                    int port,
                    size_t worker_cnt,
                    size_t max_client_cnt,
                    size_t epoll_wait_time,
                    size_t accept_per_tick,
                    std::string task_type,
                    std::string http_static_dir,
                    std::string lua_dir,
                    std::string crt_pem /*= ""*/,
                    std::string key_pem /*= ""*/,
                    bool use_ssl /*= false*/)
{
    set_app_id(app_id);
    set_listen_info(ip, port);
    set_worker_cnt(worker_cnt);
    set_max_client_cnt(max_client_cnt);
    set_epoll_wait_time(epoll_wait_time);
    set_accept_per_tick(accept_per_tick);
    set_task_type(task_type);
    set_http_static_dir(http_static_dir);
    set_lua_dir(lua_dir);
    set_crt_pem(crt_pem);
    set_key_pem(key_pem);
    set_use_ssl(use_ssl);
}

bool server::is_stop()
{
    return stop_flag;
}

bool server::on_stop()
{
    if (stop_flag)
    {
        return true; // main process close
    }
    return false;
}

void server::to_stop()
{
    stop_flag = true;
}

SSL_CTX *server::get_ssl_ctx()
{
    return m_ssl_context;
}

void server::on_start()
{
    // main epoller
    int iret = epoller.create(m_max_client_cnt * 2);
    if (iret != 0)
    {
        LOG_ERROR("epoller.create(%d) iret[%d]", (m_max_client_cnt * 2), iret);
        stop_flag = true;
        return;
    }

    // m_curr_connection_num
    {
        m_curr_connection_num.reset(new std::atomic<int>(0));
        if (!m_curr_connection_num)
        {
            LOG_ERROR("new std::atomic<int> m_curr_connection_num err");
            return;
        }
    }

    // main_worker_tunnel
    {
        m_main_worker_tunnel.reset(new std::shared_ptr<avant::socket::socket_pair>[m_worker_cnt]);
        if (!m_main_worker_tunnel)
        {
            LOG_ERROR("new socket_pair err");
            return;
        }
        for (size_t i = 0; i < m_worker_cnt; i++)
        {
            std::shared_ptr<avant::socket::socket_pair> new_item(new avant::socket::socket_pair);
            m_main_worker_tunnel[i] = new_item;
            iret = m_main_worker_tunnel[i]->init();
            if (iret != 0)
            {
                LOG_ERROR("m_main_worker_tunnel[%d] init failed", i);
                return;
            }
        }
    }

    worker::worker *worker_arr = new worker::worker[m_worker_cnt];
    m_workers.reset(worker_arr);
    for (size_t i = 0; i < m_worker_cnt; i++)
    {
        m_workers[i].worker_id = i;
        m_workers[i].curr_connection_num = m_curr_connection_num;
        m_workers[i].main_worker_tunnel = m_main_worker_tunnel[i];
        iret = m_workers[i].epoller.create(m_max_client_cnt * 2);
        if (iret != 0)
        {
            LOG_ERROR("epoller.create(%d) iret[%d]", (m_max_client_cnt * 2), iret);
            stop_flag = true;
            return;
        }
    }

    for (size_t i = 0; i < m_worker_cnt; i++)
    {
        std::thread t(std::ref(m_workers[i]));
        t.detach();
    }

    LOG_ERROR("IP %s PORT %d", m_ip.c_str(), m_port);
    server_socket listen_socket(m_ip, m_port, m_max_client_cnt);
    if (0 > listen_socket.get_fd())
    {
        LOG_ERROR("listen_socket failed get_fd() < 0");
        stop_flag = true;
        return;
    }
    if (0 != epoller.add(listen_socket.get_fd(), nullptr, EPOLLIN | EPOLLERR, true))
    {
        LOG_ERROR("listen_socket epoller add failed");
        return;
    }

    uint32_t counter = 0;
    bool sent = false;
    uint32_t client_counter = 0;

    while (true)
    {
        int num = epoller.wait(10);
        if (num < 0)
        {
            LOG_ERROR("epoller.wait return [%d]", num);
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        for (int i = 0; i < num; i++)
        {
            if (epoller.m_events[i].data.fd == listen_socket.get_fd())
            {
                std::vector<int> clients_fd;
                for (size_t loop = 0; loop < m_accept_per_tick; loop++)
                {
                    int new_client_fd = listen_socket.accept();
                    if (new_client_fd < 0)
                    {
                        break;
                    }
                    else
                    {
                        clients_fd.push_back(new_client_fd);
                        client_counter++;
                        if (client_counter % 1000 == 0)
                        {
                            LOG_ERROR("client_counter[%u]", client_counter);
                        }
                    }
                }
                for (int new_client_fd : clients_fd)
                {
                    // LOG_ERROR("new_client_fd[%d]", new_client_fd);
                    ::close(new_client_fd);
                }
            }
        }

        if (counter != UINT32_MAX)
        {
            counter++;
        }
        else
        {
            if (sent == false)
            {
                sent = true;
                for (size_t i = 0; i < m_worker_cnt; i++)
                {
                    m_workers[i].to_stop = true;
                }
            }
            else
            {
                bool flag = true;
                for (size_t i = 0; i < m_worker_cnt; i++)
                {
                    if (!m_workers[i].is_stoped)
                    {
                        flag = false;
                    }
                }
                if (flag)
                {
                    break;
                }
            }
        }
        if (counter != UINT32_MAX && stop_flag)
        {
            counter = UINT32_MAX;
            sent = true;
            for (size_t i = 0; i < m_worker_cnt; i++)
            {
                m_workers[i].to_stop = true;
            }
        }
    }
    stop_flag = true;
}
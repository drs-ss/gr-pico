// Original concept taken from:
//
// blocking_tcp_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "boost_client.h"

boost_client::boost_client(std::string address, short port) :
m_socket(m_io_service),
m_deadline(m_io_service)
{
    m_port = port;
    m_address = address;
}

boost_client::~boost_client()
{
    disconnect();
}

void
boost_client::disconnect()
{
    if(m_socket.is_open())
        m_socket.close();
}

bool
boost_client::try_connect(int timeout)
{
    boost::posix_time::time_duration dur =
            boost::posix_time::seconds(0);
    if(timeout > 0){
        dur = boost::posix_time::seconds(timeout);
    }

    std::string s_port = (boost::format("%d") % m_port).str();
    boost::asio::ip::tcp::resolver resolver(
            m_io_service);
    boost::asio::ip::tcp::resolver::query query(m_address,
            s_port);
    boost::asio::ip::tcp::resolver::iterator ter =
            resolver.resolve(query);

    m_deadline.expires_from_now(dur);

    boost::system::error_code ec = boost::asio::error::would_block;

    boost::asio::async_connect(m_socket,
            ter, boost::lambda::var(ec) = boost::lambda::_1);

    do{
        m_io_service.run_one();
    }while(ec == boost::asio::error::would_block);

    if (ec || !m_socket.is_open())
        return false;

    return true;
}

std::string
boost_client::send_message(std::string s,
        int timeout)
{
    s.append("\r\n");
    boost::asio::write(m_socket,
            boost::asio::buffer(
                    reinterpret_cast<const void*>(s.c_str()),
                    strlen(s.c_str())));

    if (timeout < 0)
        return "";

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    int check = setsockopt(m_socket.native(), SOL_SOCKET,
    SO_RCVTIMEO,reinterpret_cast<char*>(&tv),sizeof(struct timeval));
    if (check < 0) {
        std::cout << "Error setting socket options."
                << std::endl;
        return "";
    }

    char recv_buff[MAX_RECV_SIZE];

    std::size_t amnt_read = m_socket.read_some(
            boost::asio::buffer(recv_buff, MAX_RECV_SIZE));

    if (amnt_read == 0)
        return "";

    recv_buff[amnt_read + 1] = '\0';
    std::string ret_val(recv_buff);
    return ret_val;
}

void
boost_client::send_data(const boost::asio::const_buffers_1& data)
{
    boost::asio::write(m_socket, data);
}

void
boost_client::check_deadline()
{
    if (m_deadline.expires_at() <=
            boost::asio::deadline_timer::traits_type::now())
    {
      boost::system::error_code ignored_ec;
      m_socket.close(ignored_ec);

      m_deadline.expires_at(boost::posix_time::pos_infin);
    }

    m_deadline.async_wait(
            boost::bind(&boost_client::check_deadline, this));
}


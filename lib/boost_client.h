/**
 * @def MAX_RECV_SIZE
 * @brief The maximum amount of data that can be recieved back
 * from a mnemonic.
 *
 * MAX_RECV_SIZE is the maximum size (in bytes) of a response
 * back from the radio.
 */
#define MAX_RECV_SIZE 1024

#include <boost/lambda/lambda.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <stdlib.h>
#include <iostream>

class boost_client{
public:
    typedef boost::shared_ptr<boost_client> sptr;

    boost_client(std::string ip, short port);
    ~boost_client();

    bool try_connect(int timeout = 3);

    void disconnect();

    std::string send_message(std::string message, int timeout = -1);

    void send_data(const boost::asio::const_buffers_1& data);
private:
    void check_deadline();

    boost::asio::io_service m_io_service;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::deadline_timer m_deadline;

    std::string m_address;
    short m_port;
};

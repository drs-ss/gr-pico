/* -*- c++ -*- */
/* 
 * Copyright 2016 DRS.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <volk/volk.h>
#include "pico_sink_impl.h"

namespace gr
{
namespace pico
{

pico_sink::sptr
pico_sink::make(int input_type, std::string ip,
        short mne_port, short data_port)
{
    return gnuradio::get_initial_sptr(
            new pico_sink_impl(input_type, ip, mne_port,
                    data_port));
}

/*
 * The private constructor
 */
pico_sink_impl::pico_sink_impl(size_t input_size,
        const std::string &host, short mne_port, short data_port) :
        gr::sync_block("pico_sink",
                gr::io_signature::make(1, 1, input_size),
                gr::io_signature::make(0, 0, 0))
{
    m_input_size = input_size;
    set_max_noutput_items(MAX_NOUTPUT_ITEMS);

    const int alignment_multiple =
        volk_get_alignment() / m_input_size;
    set_alignment(std::max(1,alignment_multiple));

    m_waiting_buff.reset(
            new boost::uint8_t[MAX_NOUTPUT_ITEMS
            * sizeof(std::complex<boost::int16_t>)]);
    m_mne_connected = false;
    m_data_connected = false;
    m_data_port = static_cast<boost::uint16_t>(data_port);
    m_mne_port = static_cast<boost::uint16_t>(mne_port);
    m_sample_rate = 0;

    m_total_samples = 0;
    m_samples_per_tick = 0;
    m_samples_per_us = 0;

    // Try to connect to a mne_app server...
    try_connect_mne(host, m_mne_port);
    if (m_mne_connected) {
        std::cout << "Setting up Pico..." << std::endl;
        setup_pico();
        try_connect_data(host, m_data_port);
        if (m_data_connected) {
            std::cout << "Setup successful." << std::endl;
        }else{
            std::cout << "Failed to connect to"
                    " Pico for streaming." << std::endl;
        }
    }
}

/*
 * Our virtual destructor.
 */
pico_sink_impl::~pico_sink_impl()
{
    if (m_data_connected)
        disconnect_data();
    if (m_mne_connected){
        std::stringstream s;
        SEND_STR(s, ENABLE_TX_STREAM_MNE(0));
        disconnect_mne();
    }
}

bool
pico_sink_impl::start()
{
    m_start = boost::get_system_time();
    m_total_samples = 0;
    m_samples_per_tick =
        m_sample_rate
        / boost::posix_time::time_duration::ticks_per_second();
    m_samples_per_us = m_sample_rate / 1e6;
    return true;
}

void
pico_sink_impl::setup_pico()
{
    if(m_mne_connected){
        std::stringstream s;
        SEND_STR(s, CHANNEL_CMD(TX_CHANNEL));
        SEND_STR(s, NETWORKING_CMD(m_pico_address, m_data_port));
        SEND_STR(s, ENABLE_TX_STREAM_MNE(1));
    }
}

void
pico_sink_impl::try_connect_mne(std::string radio,
        boost::uint16_t port)
{
    if (m_mne_connected)
        return;
    m_pico_address = radio;
    std::cout << "Connecting to Pico at " <<
            radio << ":" << port << "..." << std::endl;
    m_mne_client.reset(new boost_client(radio, port));
    m_mne_connected = m_mne_client->try_connect();
    if(m_mne_connected){
        std::cout << "Connected." << std::endl;
    }else{
        std::cout << "Failed to connect to Pico.  "
                "Please make sure that mnemonic app"
                " is running and that the Pico is "
                "connected." << std::endl;
    }
}

void
pico_sink_impl::try_connect_data(std::string host,
        boost::uint16_t port)
{
    if (m_data_connected)
        return;

    std::cout << "Setting up data connection..." << std::endl;
    sleep(ISM_SETUP_TIME);
    m_data_client.reset(new boost_client(host, port));
    m_data_connected = m_data_client->try_connect();
    if(m_data_connected){
        std::cout << "Connected." << std::endl;
    }else{
        std::string response = send_message(HELP_QRY, 2);
        if (response.find(ISM_SEARCH_STR) == std::string::npos) {
            std::cout << "ERROR: This mnemonic app version does not "
                    "support TX streaming." << std::endl;
        }
    }
}

void
pico_sink_impl::disconnect_data()
{
    if (!m_data_connected)
        return;
    m_data_client->disconnect();
    m_data_connected = false;
}

void
pico_sink_impl::disconnect_mne()
{
    if (!m_mne_connected)
        return;
    m_mne_client->disconnect();
    m_mne_connected = false;
}

int
pico_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
{
    if (!m_data_connected || !m_mne_connected)
        return WORK_DONE;

    const char *in = reinterpret_cast<const char *>(input_items[0]);
    ssize_t total_size = noutput_items * m_input_size;

    // Before we do anything with the data, we need to make sure
    // we throttle our process to match the radios sample rate.
    if (m_sample_rate > 0) {
        boost::system_time now = boost::get_system_time();
        boost::int64_t ticks = (now - m_start).ticks();
        boost::uint64_t expected_samps = uint64_t(
                m_samples_per_tick * ticks);
        if (m_total_samples > expected_samps) {
            double sleep_time =
                    (m_total_samples - expected_samps)
                            / m_samples_per_us;
            boost::this_thread::sleep(
                    boost::posix_time::microseconds(
                            long(sleep_time)));
        }
        if (m_input_size != sizeof(gr_complex)) {
            m_total_samples += (total_size
                    / sizeof(std::complex<boost::int16_t>));
        } else {
            m_total_samples += noutput_items;
        }
    }

    if (m_input_size == sizeof(gr_complex)) {
        volk_32f_s32f_convert_16i(
                reinterpret_cast<boost::int16_t*>
                (m_waiting_buff.get()),
                reinterpret_cast<const float*>(in), IQ_SCALE_FACTOR,
                noutput_items * 2);
        m_data_client->send_data(
                boost::asio::buffer(
                        reinterpret_cast<const void*>(
                                m_waiting_buff.get()),
                        noutput_items
                                * sizeof(std::complex<
                                        boost::int16_t>)));
    } else {
        m_data_client->send_data(
                boost::asio::buffer(
                       reinterpret_cast<const void*>(input_items[0]),
                       total_size));
    }

    return noutput_items;
}

std::string
pico_sink_impl::send_message(std::string s,
        int timeout)
{
    if(!m_mne_connected)
        return "";
    return m_mne_client->send_message(s, timeout);
}

void
pico_sink_impl::set_sample_rate(double sr)
{
    if (m_mne_connected) {
        std::stringstream s;
        double mhz_rate = sr / MHZ_SCALE;
        if (mhz_rate <= MAX_SR_MHZ && mhz_rate >= MIN_SR_MHZ) {
            SEND_STR(s, SAMPLE_RATE_CMD(mhz_rate));
            std::string response = "";
            response = send_message(SAMPLE_RATE_QRY, 2);
            response = response.substr(
                    response.find_first_of(" ") + 1, 13);

            m_sample_rate = std::atof(response.c_str());
            if(m_sample_rate > MAX_SR_MHZ){
                SEND_STR(s, SAMPLE_RATE_CMD(0));
                response = send_message(SAMPLE_RATE_QRY, 2);
                response = response.substr(
                response.find_first_of(" ") + 1, 13);
                m_sample_rate = std::atof(response.c_str());
            }
            m_sample_rate *= 1e6;
            m_start = boost::get_system_time();
            m_total_samples = 0;
            m_samples_per_tick =
              m_sample_rate
              / boost::posix_time::time_duration::ticks_per_second();
            m_samples_per_us = m_sample_rate / 1e6;

            if (m_sample_rate != sr) {
                std::cout << "Requested sample rate: "
                        << (sr / 1e6) << " Msps.  Actual "
                                "sample rate: "
                        << (m_sample_rate / 1e6) << " Msps."
                        << std::endl;
                std::cout << "Available sample rates at this "
                        "bandwidth are: " << std::endl;
                response = send_message(SAMPLE_RATE_HLP, 2);
                size_t start = response.find("Values:") + 7;
                size_t stop = response.find_last_of("Msps")
                        - start + 1;
                std::string chopped = response.substr(
                        response.find("Values:") + 7, stop);
                std::vector<double> available_sr = get_values(
                        chopped);
                if (available_sr.size() > 0) {
                    std::ios::fmtflags prev_flags(std::cout.flags());
                    std::size_t i = 1;
                    for (; i <= available_sr.size(); i++) {
                        if (available_sr[i - 1] > 6.7)
                            break;
                        std::cout.precision(4);
                        std::cout << "\t" << std::fixed
                                << available_sr[i - 1]
                                << " Msps";
                        if (i % 3 == 0)
                            std::cout << std::endl;
                    }
                    if ((i - 1) % 3 == 0)
                        std::cout << std::endl;
                    else
                        std::cout << std::endl << std::endl;
                    std::cout.flags(prev_flags);
                }
            }
        } else {
            std::cout << "Please select a sample rate between " <<
                    MAX_SR_MHZ << " and " << MIN_SR_MHZ << " Msps."
                    << std::endl;
        }
    } else {
        std::cout << "No connection established." << std::endl;
    }
}

void
pico_sink_impl::update_sample_rate(double sr)
{
    set_sample_rate(sr);
}

void
pico_sink_impl::set_power(double pwr)
{
    if (m_mne_connected) {
        if (pwr >= MIN_POWER_DBM && pwr <= MAX_POWER_DBM) {
            std::stringstream s;
            SEND_STR(s, POWER_CMD(pwr));
            std::string response = "";
            response = send_message(POWER_QRY, 1);
            //parse out the numbers from the query.
            response = response.substr(
                    response.find_first_of(" ") + 1, 13);
            double actual_txp = std::atof(response.c_str());
            if (actual_txp != pwr) {
                std::cout << "Requested power: " << (pwr)
                        << " dBm.  Actual power: "
                        << (actual_txp) << " dBm." << std::endl;
            }
        } else {
            std::cout << "Power parameter out of range.  Please"
                    " select a value between " << MIN_POWER_DBM
                    << " and " << MAX_POWER_DBM << " dBm."
                    << std::endl;
        }
    }
}

void
pico_sink_impl::update_power(double pwr)
{
    set_power(pwr);
}

void
pico_sink_impl::set_frequency(double freq)
{
    if (m_mne_connected) {
        double freq_mhz = freq / MHZ_SCALE;
        if (freq_mhz > MIN_FREQ_MHZ && freq_mhz < MAX_FREQ_MHZ) {
            std::stringstream s;
            SEND_STR(s, FREQUENCY_CMD(freq_mhz));
        } else {
            std::cout << "Please select a frequency between "
                    << MIN_FREQ_MHZ << " MHz and "
                    << MAX_FREQ_MHZ << " MHz." << std::endl;
        }
    }
}

void
pico_sink_impl::update_frequency(double freq)
{
    set_frequency(freq);
}

void
pico_sink_impl::set_bandwidth(double bw)
{
    if (m_mne_connected) {
        std::stringstream s;
        SEND_STR(s, BANDWIDTH_CMD((bw / 1e6)));
        std::string response = send_message(BANDWIDTH_QRY, 2);
        response = response.substr(
                response.find_first_of(" ") + 1, 13);

        double actual_bw = std::atof(response.c_str()) * 1e6;
        if (actual_bw != bw) {
            std::cout << "Actual bandwidth was set to : "
                    << actual_bw << std::endl;
            response = send_message(BANDWIDTH_HLP, 2);
            size_t start = response.find("Values:") + 7;
            size_t stop = response.find_last_of("MHz") - start
                    + 1;
            std::string chopped = response.substr(
                    response.find("Values:") + 7, stop);
            std::vector<double> available_bw = get_values(
                    chopped);
            if (available_bw.size() > 0) {
                std::ios::fmtflags prev_flags(std::cout.flags());
                std::cout << "Please select from : "
                        << std::endl;
                std::size_t i = 1;
                for (; i <= available_bw.size(); i++) {
                    std::cout.precision(4);
                    std::cout << "\t" << std::fixed
                            << available_bw[i - 1] << " MHz";
                    if (i % 4 == 0)
                        std::cout << std::endl;
                }
                if ((i - 1) % 4 == 0)
                    std::cout << std::endl;
                else
                    std::cout << std::endl << std::endl;
                std::cout.flags(prev_flags);
            }
        }
        if (m_sample_rate > 0)
            set_sample_rate(m_sample_rate);
    }
}

void
pico_sink_impl::update_bandwidth(double bw)
{
    set_bandwidth(bw);
}

std::vector<double>
pico_sink_impl::get_values(std::string s)
{
    std::vector<double> ret_val;
    std::string chopped_string = s;
    std::string search_string = "";
    std::size_t skip_count = 0;
    if (s.find("Msps") != std::string::npos) {
        search_string = "Msps";
        skip_count = 5;
    } else {
        search_string = "MHz";
        skip_count = 4;
    }
    // While there is enough room for a number to be present.
    while (chopped_string.length() > 11) {
        double temp = 0.0;
        std::size_t next_index = chopped_string.find(
                search_string);
        if (next_index == std::string::npos)
            break;
        sscanf(chopped_string.c_str(), "%*[^0-9]%lf", &temp);
        ret_val.push_back(temp);
        next_index += skip_count;
        if (next_index >= chopped_string.length())
            break;
        chopped_string = chopped_string.substr(next_index);
    }
    return ret_val;
}
} /* namespace pico */
} /* namespace gr */


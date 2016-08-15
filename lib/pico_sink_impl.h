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

#ifndef INCLUDED_PICO_PICO_SINK_IMPL_H
#define INCLUDED_PICO_PICO_SINK_IMPL_H

#include <pico/pico_sink.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "mne_helper.h"
#include "complex_manager.h"
#include "boost_client.h"

/**
 * @def MAX_POWER_DBM
 * @brief The highest power the PicoTXR can output.
 *
 * MAX_POWER_DBM defines the highest power level (in dBm)
 * that the radio can output.  The actual value will vary
 * based on frequency, but this is expected to be the
 * highest possible.
 */
#define MAX_POWER_DBM 10

/**
 * @def MIN_POWER_DBM
 * @brief The highest frequency a tuner can hit.
 *
 * MIN_POWER_DBM defines the highest frequency a tuner can tune
 * to (in MHz).
 */
#define MIN_POWER_DBM -89.75

/**
 * @def MAX_FREQ_MHZ
 * @brief The highest frequency the TX channel can hit.
 *
 * MAX_FREQ_MHZ defines the highest frequency the TX channel
 * can tune to (in MHz).
 */
#define MAX_FREQ_MHZ 6302.5

/**
 * @def MIN_FREQ_MHZ
 * @brief The lowest frequency the TX channel can hit.
 *
 * MIN_FREQ_MHZ defines the lowest frequency the TX channel
 * can tune to (in MHz).
 */
#define MIN_FREQ_MHZ 46.857032

/**
 * @def MAX_SR_MHZ
 * @brief The highest possible sample rate for the Pico block.
 *
 * MAX_SR_MHZ defines the highest possible sample rate for the
 * pico to run at taking into account the limitations of the
 * USB to Ethernet.
 */
#define MAX_SR_MHZ 6.7

/**
 * @def MAX_SR_MHZ
 * @brief The lowest possible sample rate for the PicoTXR.
 *
 * MAX_SR_MHZ defines the lowest possible sample rate for
 * the TX channel on the PicoTXR.
 */
#define MIN_SR_MHZ 0.001875

/**
 * @def MAX_NOUTPUT_ITEMS
 * @brief The maximum number of input/output items for this block.
 *
 * MAX_NOUTPUT_ITEMS is used to set the maximum number of input and
 * output items for this block when it is started.
 */
#define MAX_NOUTPUT_ITEMS 10000

/**
 * @def ISM_SETUP_TIME
 * @brief The amount of time for a Pico to setup for streaming.
 *
 * ISM_SETUP_TIME is the amount of time (in seconds) that the Pico
 * takes to setup input streaming mode.  This number should be a
 * little bloated to make sure that we can connect.
 */
#define ISM_SETUP_TIME 3

/**
 * @def ISM_SEARCH_STR
 * @brief A string containing the help message for the ISM mnemonic.
 *
 * ISM_SEARCH_STR is used to search a HELP_QRY response for the ISM
 * mnemonic.
 */
#define ISM_SEARCH_STR "ISM CQ Input Streaming Mode"

namespace gr
{
namespace pico
{

class pico_sink_impl : public pico_sink
{
private:
    // Waiting buffer used for preparing complex samples
    boost::scoped_array<boost::uint8_t> m_waiting_buff;
    // Size of input items in bytes
    std::size_t m_input_size;

    // For throttling...
    boost::system_time m_start;
    boost::uint64_t m_total_samples;
    double m_samples_per_tick;
    double m_samples_per_us;
    double m_sample_rate;

    // For networking...
    bool m_mne_connected;
    bool m_data_connected;
    std::string m_pico_address;
    boost::uint16_t m_data_port;
    boost::uint16_t m_mne_port;
    boost_client::sptr m_data_client;
    boost_client::sptr m_mne_client;

    // Networking helper functions...
    void try_connect_mne(std::string radio, boost::uint16_t port);
    void try_connect_data(std::string host, boost::uint16_t port);
    void disconnect_data();
    void disconnect_mne();

    // Mnemonic communication helper functions...
    void setup_pico();
    std::string send_message(std::string s, int timeout = -1);
    std::vector<double> get_values(std::string s);

    // Setters for radio properties...
    void set_frequency(double freq);
    void set_power(double pwr);
    void set_sample_rate(double sr);
    void set_bandwidth(double bw);
public:
    pico_sink_impl(size_t input_size,
                   const std::string &host,
                   short mne_port, short data_port);
    ~pico_sink_impl();

    //GNURadio Callbacks
    /**
     * Update the sample rate for the output stream.
     *
     * @param sr - The sample rate to set in Hz.
     */
    void update_sample_rate(double sr);

    /**
     * Update the transmit power for the radio.
     * 
     * @param pwr - The power to set in dBm.
     */
    void update_power(double pwr);
   
    /**
     * Update the center frequency for the radio.
     * 
     * @param freq - The frequency to tune to in Hz.
     */
    void update_frequency(double freq);

    /**
     * Update the bandwidth for the output stream
     *
     * @param bw - The bandwidth to set in Hz.
     */
    void update_bandwidth(double bw);

    //gr_block extended functions
    bool start();
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);
};

} // namespace pico
} // namespace gr

#endif /* INCLUDED_PICO_PICO_SINK_IMPL_H */


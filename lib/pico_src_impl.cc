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
#include "pico_src_impl.h"

namespace gr
{
namespace pico
{

pico_src::sptr
pico_src::make(std::string radio_ip, std::string machine_ip, 
               int mne_port, int data_port)
{
    return gnuradio::get_initial_sptr
           (new pico_src_impl(radio_ip, machine_ip,
                   mne_port, data_port));
}

/*
 * The private constructor
 */
pico_src_impl::pico_src_impl(std::string radio,
                             std::string machine,
                             int mne_port, int data_port)
    : gr::sync_block("pico_src",
                     gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    m_data_port = data_port;
    m_mne_port = mne_port;
    m_active_channels[0] = -1;

    try_connect(radio, machine);
    if (m_connected) {
        std::cout << "Setting up Pico..." << std::endl;
        setup_pico();
        std::cout << "Setup successful." << std::endl;
    }
}

/*
 * Our virtual destructor.
 */
pico_src_impl::~pico_src_impl()
{
}

bool
pico_src_impl::start()
{
    if (m_connected) {
        m_complex_manager.reset(new complex_manager(m_machine_address, 
                m_data_port));
        m_complex_manager->update_tuners(m_active_channels, 
                                         NUM_OUTPUT_STREAMS);
        std::stringstream s;
        SEND_STR(s, ENABLE_RX_STREAM_MNE(1));
    }
    return true;
}

bool 
pico_src_impl::stop()
{
    std::stringstream s;
    SEND_STR(s, ENABLE_RX_STREAM_MNE(0));
    usleep(SHORT_USLEEP);
    if (m_complex_manager) {
        m_complex_manager->stop();
    }
    return true;
}

void 
pico_src_impl::try_connect(std::string radio, std::string machine)
{
    m_connected = false;
    m_radio_address = radio;
    m_machine_address = machine;
    std::cout << "Connecting to Pico at " <<
                radio << ":" << m_mne_port << "..." << std::endl;
    m_mne_client.reset(new boost_client(radio, m_mne_port));
    m_connected = m_mne_client->try_connect();
    if(m_connected){
        std::cout << "Connected." << std::endl;
    }else{
        std::cout << "Failed to connect to Pico.  "
                        "Please make sure that mnemonic app"
                        " is running and that the Pico is "
                        "connected." << std::endl;
    }
}

void 
pico_src_impl::setup_pico()
{
    if(m_connected){
        std::stringstream s;
        SEND_STR(s, NETWORKING_CMD(m_machine_address,m_data_port));
        SEND_STR(s, ENDIAN_CMD(0));
    }
}

void
pico_src_impl::set_sample_rate(double sr)
{
    if (m_connected) {
        std::stringstream s;
        double mhz_rate = sr / MHZ_SCALE;
        if (mhz_rate <= MAX_SR_MHZ && mhz_rate >= MIN_SR_MHZ) {
            SEND_STR(s, ENABLE_RX_STREAM_MNE(0));
            SEND_STR(s, SAMPLE_RATE_CMD(mhz_rate));
            SEND_STR(s, ENABLE_RX_STREAM_MNE(1));
        }else if(mhz_rate > MAX_SR_MHZ){
            std::cout<<"Pico cannont support rates"
                       " higher than " << MAX_SR_MHZ <<
                       " Msps."<<std::endl;
            SEND_STR(s, ENABLE_RX_STREAM_MNE(0));
            SEND_STR(s, SAMPLE_RATE_CMD(MAX_SR_MHZ));
            std::string response = "";
            response = send_message(SAMPLE_RATE_QRY, 1);
            // Parse out the numbers from the query.
            response = response.substr(
                response.find_first_of(" ") + 1, 13);
            std::cout<<"Sample rate was set to "<<response
                     <<" Msps"<<std::endl;
            SEND_STR(s, ENABLE_RX_STREAM_MNE(1));
        }else if(mhz_rate < MIN_SR_MHZ){
            std::cout<<"Pico cannont support rates"
                       " lower than " << MIN_SR_MHZ <<
                       " Msps."<<std::endl;
            SEND_STR(s, ENABLE_RX_STREAM_MNE(0));
            SEND_STR(s, SAMPLE_RATE_CMD(MIN_SR_MHZ));
            std::string response = "";
            response = send_message(SAMPLE_RATE_QRY, 1);
            // Parse out the numbers from the query.
            response = response.substr(
                response.find_first_of(" ") + 1, 13);
            std::cout<<"Sample rate was set to "<<response
                     <<" Msps"<<std::endl;
            SEND_STR(s, ENABLE_RX_STREAM_MNE(1));
        }
    }else{
        std::cout<<"No connection established."<<std::endl;
    }
}

void
pico_src_impl::update_sample_rate(double sr)
{
    set_sample_rate(sr);
}

void
pico_src_impl::set_attenuation(double atten)
{
    int int_atten = static_cast<int>(atten);
    if (m_connected && int_atten >= MIN_ATTEN && int_atten <= MAX_ATTEN) {
        if(static_cast<double>(int_atten) != atten)
            std::cout << "Setting attenuation to " << int_atten << std::endl;
        std::stringstream s;
        SEND_STR(s, ATTENUATION_CMD(int_atten));
    }
}

void
pico_src_impl::update_attenuation(double atten)
{
    set_attenuation(atten);
}

void
pico_src_impl::set_frequency(double freq)
{
    double freq_mhz = freq /MHZ_SCALE;
    if (m_connected && freq_mhz > MIN_FREQ_MHZ && 
                                freq_mhz < MAX_FREQ_MHZ) {
        std::stringstream s;
        SEND_STR(s, FREQUENCY_CMD(freq_mhz));
    }else{
        std::cout<<"Requested value is outside of frequency range "
            <<MIN_FREQ_MHZ<<"-"<<MAX_FREQ_MHZ<<" MHz"<<std::endl;
    }
}

void
pico_src_impl::update_frequency(double freq)
{
    set_frequency(freq);
}

void
pico_src_impl::set_channel(int channel)
{
    if (m_connected && channel > 0
            && channel <= NUM_OUTPUT_STREAMS) {
        std::stringstream s;
        SEND_STR(s, CHANNEL_CMD(channel));
        // Update the active streams
        std::string response = "";
        std::stringstream to_hex;
        response = send_message(STREAM_ID_QRY, 1);
        // Parse out the numbers from the query.
        response = response.substr(
                response.find_first_of(" ") + 1, 10);
        to_hex << std::hex << response;
        int resp;
        // Convert the hex response to an int
        to_hex>>resp;
        m_active_channels[0] = resp+1;
    }else{
        std::cout<<"Invalid channel. "
                   "Please set channel to 1."<<std::endl;
        m_active_channels[0] = -1;
    }

    if (m_complex_manager) {
        m_complex_manager->update_tuners(m_active_channels, 1);
    }
}

void
pico_src_impl::update_channel(int channel)
{
    set_channel(channel);
}

int
pico_src_impl::work(int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    int amnt = 0;
    if (m_connected) {
        amnt = m_complex_manager->
            fill_buffers(output_items, 
                         m_active_channels, noutput_items);
    }
    // Tell runtime system how many output items we produced.
    return amnt;
}

std::string
pico_src_impl::send_message(std::string s,
        int timeout)
{
    if(!m_connected)
        return "";
    return m_mne_client->send_message(s,timeout);
}
} /* namespace pico */
} /* namespace gr */


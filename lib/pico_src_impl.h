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

#ifndef INCLUDED_PICO_PICO_SRC_IMPL_H
#define INCLUDED_PICO_PICO_SRC_IMPL_H

#include <pico/pico_src.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/scoped_array.hpp>
#include "complex_manager.h"
#include "mne_helper.h"
#include "boost_client.h"

/**
 * @def NUM_OUTPUT_STREAMS
 * @brief The maximum number of output streams this block can
 * support.
 *
 * NUM_OUTPUT_STREAMS defines the maximum number of outputs
 * this block can create.
 */
#define NUM_OUTPUT_STREAMS 1

/**
 * @def MAX_ATTEN
 * @brief The maximum amount of attenuation a tuner supports.
 *
 * MAX_ATTEN defines the largest amount of attenuation that can
 * be put into a tuner.
 */
#define MAX_ATTEN 46

/**
 * @def MIN_ATTEN
 * @brief The minimum amount of attenuation a tuner supports.
 *
 * MIN_ATTEN defines the smallest amount of attenuation that can
 * be put into a tuner.
 */
#define MIN_ATTEN 0

/**
 * @def MAX_FREQ_MHZ
 * @brief The highest frequency a tuner can hit.
 *
 * MAX_FREQ_MHZ defines the highest frequency a tuner can tune
 * to (in MHz).
 */
#define MAX_FREQ_MHZ 3023.293103

/**
 * @def MIN_FREQ_MHZ
 * @brief The lowest frequency a tuner can hit.
 *
 * MIN_FREQ_MHZ defines the lowest frequency a tuner can tune
 * to (in MHz).
 */
#define MIN_FREQ_MHZ 0.0

/**
 * @def MIN_SR_MHZ
 * @brief The highest sample rate the Pico supports
 *
 * MIN_SR_MHZ defines the highest sample rate the Pico SigInt
 * channel supports (in MHz).
 */
#define MAX_SR_MHZ 6.7

/**
 * @def MIN_SR_MHZ
 * @brief The lowest sample rate the Pico supports
 *
 * MIN_SR_MHZ defines the lowest sample rate the Pico SigInt
 * channel supports (in MHz).
 */
#define MIN_SR_MHZ 0.000356

namespace gr
{
namespace pico
{

class pico_src_impl : public pico_src
{
private:
    // Whether we are connected to the unit or not
    bool m_connected;

    // Networking stuff
    boost_client::sptr m_mne_client;

    // The address of the unit
    std::string m_radio_address;

    // The address of this machine
    std::string m_machine_address;

    //Operation mode
    std::string m_operation_mode;

    // The port to stream to
    int m_data_port;

    // Port mnemonic app is running on.
    int m_mne_port;

    // Pointer to the complex manager that processes our data for us
    boost::scoped_ptr<complex_manager> m_complex_manager;

    // An array that gets passed into fill buffers
    int m_active_channels[NUM_OUTPUT_STREAMS];

    void try_connect(std::string radio, std::string machine);
    void setup_pico();
    std::string send_message(std::string s, int timeout = -1);
    void set_channel(int channel);
    void set_frequency(double freq);
    void set_attenuation(double atten);
    void set_sample_rate(double sr);
public:
    pico_src_impl(std::string radio, std::string machine,
            int mne_port, int data_port);
    ~pico_src_impl();

    //GNURadio Callbacks
    /**
     * Update the sample rate for the output stream.
     * 
     * @param sr - The sample rate to set in Hz.
     */
    void update_sample_rate(double sr);

    /**
     * Update the bandwidth for the output stream
     *
     * @param bw - The bandwidth to set in Hz.
     */
    void update_bandwidth(double bw);

    /**
     * Update the analog attenuation for the output stream. 
     * 
     * @param atten - The attenuation to set in dB.
     */
    void update_attenuation(double atten);
   
    /**
     * Update the center frequency for the active stream.
     * 
     * @param freq - The frequency to tune to in Hz.
     */
    void update_frequency(double freq);
    
    /**
     * Change the active channel of the radio. 
     * 
     * @param channel - The channel to select
     */
    void update_channel(int channel);

    //gr_block extended functions
    bool start();
    bool stop();
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);
};

} // namespace pico
} // namespace gr

#endif /* INCLUDED_PICO_PICO_SRC_IMPL_H */


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

#include "complex_manager.h"

complex_manager::complex_manager(std::string ip, int port)
{
    // Setup processing threads
    for (int i = 0; i < NUM_THREADS; i++) {
        m_sample_count[i] = 0;
        m_packet_buffer[i] = NULL;
        m_target_buffer[i] = NULL;
        m_start_index[i] = 0;
        m_process_tasks[i].reset(new task_impl(
             boost::bind(&complex_manager::process_packet, this, i)));
    }

    m_request_flip = false;
    m_request_amount = 0;
    m_safety_count = 0;
    m_ok_to_parse = true;
    m_last_frame_count = -1;

    memset(m_overflow_buffer, 0, UDP_START_INDEX);

    for (int i = 0; i < NUM_STREAMS; i++) {
        m_mang[i].out_count[0] = 0;
        m_mang[i].out_count[1] = 0;
        m_mang[i].last_count = -1;
        m_mang[i].read_index[0] = 0;
        m_mang[i].read_index[1] = 0;
        m_mang[i].out_pointer = 0;
        m_mang[i].tuner_valid = false;
        m_mang[i].tuner_valid_safe = false;
        m_mang[i].flip = false;
    }

    m_update_valid_streams  = false;

    m_run = true;

    // Create a pool of aligned buffers
    m_aligned_buffs = aligned_buffer::make(
        NUM_STREAMS * 2,
        NUM_COMPLEX * sizeof(std::complex<float>));

    // Save these aligned buffer pointers
    for (int i = 0; i < NUM_STREAMS; i++) {
        m_mang[i].out_buff[0]= reinterpret_cast<std::complex<float> *>
            (m_aligned_buffs->at(i));
        m_mang[i].out_buff[1]= reinterpret_cast<std::complex<float> *>
            (m_aligned_buffs->at(i + NUM_STREAMS));
    }

    m_amnt_saved = 0;

    // Spawn our UDP collection thread.
    m_udp_listener.reset(new udp_listener(ip, port));

    // Create a thread for our own loop to run in.
    m_thread.reset(new boost::thread(boost::ref(*this)));
}

complex_manager::~complex_manager()
{
}

void
complex_manager::stop()
{
    m_udp_listener->stop();

    m_run = false;
    m_thread->join();

    for (int i = 0; i < NUM_THREADS; i++) {
        m_process_tasks[i]->stop_thread();
    }
}

bool
complex_manager::threads_active()
{
    for (int i = 0; i < NUM_THREADS; i++) {
        if (m_process_tasks[i]->is_running()) {
            return true;
        }
    }
    return false;
}

void
complex_manager::update_tuners(int *tuners, int n)
{
    for (int i = 0; i < NUM_STREAMS; i++) {
        m_mang[i].tuner_valid_safe = false;
        for (int j = 0; j < n; j++) {
            if ((i + 1) == tuners[j]) {
                m_mang[i].tuner_valid_safe = true;
            }
        }
    }

    m_update_valid_streams = true;
    while (m_update_valid_streams) {
        usleep(LONG_USLEEP);
    }
}

int
complex_manager::fill_buffers(std::vector<void *> buffs,
                              int *tuners,
                              int count)
{
    if (m_update_valid_streams) return 0;

    // Prevent infinite recursion.
    m_safety_count += 1;
    /* 
    First we have to know what amount of data we have available to
        ship out. 
    */
    int min = count;
    int i = 0;
    for (; i < NUM_STREAMS; i++) {
        m_mang[i].flip = false;
        if (m_mang[i].out_count[!m_mang[i].out_pointer] <= min
            && m_mang[i].tuner_valid) {
            min = m_mang[i].out_count[!m_mang[i].out_pointer];
            if (min == 0) {
                m_mang[i].flip = true;
            }
        }
    }

    if (min == 0) {
        /* 
        If minimum was set to 0 at least 1 of the read buffers
            didn't have any items in it.
        Request the processing loop flip the buffers we set, and
            see if we had any data waiting.
        */
        m_request_flip = true;
        while (m_request_flip) {
            usleep(SHORT_USLEEP);
        }
        if (m_safety_count < NUM_RECURSIVE) {
            // Try to get data again.
            return fill_buffers(buffs, tuners, count);
        } else {
            // Otherwise just return 0 this time, they'll be back.
            m_safety_count = 0;
            return 0;
        }
    }

    // Temporary pointers to copy targets
    void *targets[NUM_STREAMS] = { };
    for (i = 0; i < buffs.size(); i++) {
        if (tuners[i] <= NUM_STREAMS && tuners[i] > 0) {
            targets[tuners[i] - 1] = buffs[i];
        }
    }

    // Remove the minimum amount from each of the buffers.
    int cpy_amnt = min * sizeof(std::complex<float>);
    for (i = 0; i < NUM_STREAMS; i++) {
        if (targets[i] != NULL) {
            memcpy(targets[i], m_mang[i].get_read_buffer(),
                   cpy_amnt);
            m_mang[i].read_index[!m_mang[i].out_pointer] += min;
            m_mang[i].out_count[!m_mang[i].out_pointer] -= min;
        }
    }

    m_safety_count = 0;
    return min;
}

void
complex_manager::handle_flip()
{
    // Flip our read/write buffers,  Wait for our threads to finish.
    while (threads_active()) {
        usleep(SHORT_USLEEP);
    }
    for (int i = 0; i < NUM_STREAMS; i++) {
        if (m_mang[i].tuner_valid && m_mang[i].flip) {
            m_mang[i].out_pointer = !m_mang[i].out_pointer;
            m_mang[i].out_count[m_mang[i].out_pointer] = 0;
            m_mang[i].read_index[0] = 0;
            m_mang[i].read_index[1] = 0;
            m_mang[i].flip = false;
        }
    }
}

void
complex_manager::handle_update()
{
    // Apply updates to our valid streams.  First stop the threads.
    while (threads_active()) {
        usleep(SHORT_USLEEP);
    }
    for (int i = 0; i < NUM_STREAMS; i++) {
        m_mang[i].tuner_valid = m_mang[i].tuner_valid_safe;
        m_mang[i].out_count[0] = 0;
        m_mang[i].out_count[1] = 0;
        m_mang[i].last_count = -1;
        m_mang[i].read_index[0] = 0;
        m_mang[i].read_index[1] = 0;
        m_mang[i].out_pointer = 0;
    }
}

void
complex_manager::operator()()
{
    int length = 0;
    while (m_run) {
        if (m_request_flip) {
            handle_flip();
            m_request_flip = false;
        }
        if (m_update_valid_streams) {
            handle_update();
            m_update_valid_streams = false;
        }
         
        //Make sure we aren't still using packet data before we 
        //request new data.
        
        while (threads_active()) {
            usleep(SHORT_USLEEP);
        }
        // Request a new buffer full of packets.
        length = 0;
        m_saved_packets = m_udp_listener->get_buffer_list(length);
        if (length == 0) {
            // There wasn't anything to process, try again.
            usleep(LONG_USLEEP);
            continue;
        }
        // If we got here, we must have some data to process
        // First setup our local variables with their offsets
        // byte_index must start at the offset location
        int byte_index = UDP_START_INDEX;
        // length returned can be added to the start 
        // index to give us the location of the last byte
        int total_length = length + UDP_START_INDEX;
        // cpy_loc can default to byte_index 
        int cpy_loc = byte_index;

        // If we have some data saved from the 
        // last time we tried to process information...
        if (m_amnt_saved != 0) {
            // Find out where the new start index should be
            cpy_loc = 
                static_cast<int>(UDP_START_INDEX) - (m_amnt_saved);
            // Do the memcpy from the overflow_buffer to it's new home
            memcpy( &(static_cast<boost::uint8_t *>
                      (m_saved_packets->at(0))[cpy_loc]), 
                       m_overflow_buffer, m_amnt_saved);
            // cpy_loc is the starting location now.
            byte_index = cpy_loc;
            // Reset amnt_saved so that it isn't flagged for next time
            m_amnt_saved = 0;
        }

        // While we have data available and are running
        while ((byte_index < total_length) && m_run) {
            if (m_update_valid_streams) {
                handle_update();
                m_update_valid_streams = false;
            }
            if (m_request_flip) {
                handle_flip();
                m_request_flip = false;
            }
            std::ptrdiff_t vrl_index = 0;
            try {
                // Access our buffer at the byte index (then 
                // reinterpret that address location to int 
                // for the parse call)
                VRLFrame::sptr frame = VRLFrame::parse(
                    reinterpret_cast<boost::uint32_t*>
                    (&(static_cast<boost::uint8_t *>
                       (m_saved_packets->at(0))[byte_index])),
                    total_length - byte_index, 
                    &vrl_index);

                // Update our index counter here...
                byte_index = (byte_index + (vrl_index * 4) + 
                              (frame->get_frame_size() * 4));

     
                m_last_frame_count +=1;
                if (m_last_frame_count  == 0) {
                    m_last_frame_count = frame->get_frame_count() ;
                } else if (m_last_frame_count > MAX_FRAME_COUNT ) { 
                    m_last_frame_count  = 0;
                }
                // Use frame counter to check packet loss...
                if (m_last_frame_count != frame->get_frame_count()) {
                    std::cout << FRAME_LOSS_MSG;
                }

                m_last_frame_count = frame->get_frame_count();

                // Loop through all the packets inside of this frame 
                // and process them...
                for (int it = 0; it < frame->get_num_packets(); it++){
                    if (m_update_valid_streams) {
                        handle_update();
                        m_update_valid_streams = false;
                    }
                    if (m_request_flip) {
                        handle_flip();
                        m_request_flip = false;
                    }
                    VRTPacket::sptr packet = frame->get_packet(it);

                    // If this is not a data packet, 
                    // we don't care about it...
                    if (packet->get_packet_type() != 0x1) {
                        continue;
                    }

                    // Grab this stream id and do a bound check...
                    int stream_id = (packet->get_stream_id() 
                                     & 0x000000FF);

                    if (stream_id < 0 || stream_id > NUM_STREAMS) {
                        std::cout << "INVALID STREAM ID (" << 
                            stream_id << ") RECVD" << std::endl;
                        continue;
                    }

                    // Check if this tuner is active
                    if (!m_mang[stream_id].tuner_valid) {
                        std::cout << "STREAM (" << stream_id << 
                            ") IS NOT VALID" << std::endl;
                        continue;
                    }

                    // Check if the packet's samples will 
                    // push us beyond our boundry
                    if (m_mang[stream_id].out_count
                        [m_mang[stream_id].out_pointer] + 
                        (packet->get_payload_size() / 
                         sizeof(std::complex<boost::int16_t>)) >= 
                        NUM_COMPLEX) {
                        it--;
                        continue;
                    }

                    // Grab packet counter...
                    int got = packet->get_packet_count();
                    int expected = m_mang[stream_id].last_count;
                    m_mang[stream_id].last_count = got;

                    // Update our packet counters for comparison.
                    expected += 1;
                    if (expected == 0) {
                        expected = got;
                    } else if (expected > MAX_PACKET_COUNT) {
                        expected = 0;
                    }

                    // If they don't line up, we have lost a packet.
                    if (expected != got) {
                        // Report packet loss to the user.
                        std::cout << PACKET_LOSS_MSG;
                    }

                    // If we got here then we need a 
                    // thread to process this guys data.
                    int setup_thread = -1;
                    while (m_run && setup_thread < 0) {
                        for (int i = 0; i < NUM_THREADS; i++) {
                            if (!m_process_tasks[i]->is_running()) {
                                setup_thread = i;
                                break;
                            }
                        }
                    }

                    // If it still hasn't found a thread, 
                    // run is now false
                    if (setup_thread < 0) {
                        return;
                    }

                    // Setup the variables that this thread will use.
                    m_packet_buffer[setup_thread] = 
                        packet->get_payload();

                    m_sample_count[setup_thread] = 
                       packet->get_payload_size() / 
                        sizeof(std::complex<boost::int16_t>);

                    m_target_buffer[setup_thread] = 
                        m_mang[stream_id].get_active_buffer(
                            m_start_index[setup_thread]);

                    m_mang[stream_id].out_count
                        [m_mang[stream_id].out_pointer] += 
                        (packet->get_payload_size() / 
                         sizeof(std::complex<boost::int16_t>));

                    // Start the thread!
                    m_process_tasks[setup_thread]->wake_up_thread();
                }
            }catch(index_error){
                // If an index error occured, 
                // then the frame was found, 
                // but didn't have enough data.
                // copy whats left of this buffer and save it so 
                // that we can append it to the front
                // of the next incoming data (which will probably 
                // have the last half of this packets data).
                // Find out how much we want to save for later
                m_amnt_saved = total_length - byte_index;
                if (m_amnt_saved < 0) {
                    return;
                }

                // Now memcpy what we have into the overflow_buffer...
                memcpy(m_overflow_buffer, 
                       &(static_cast<boost::uint8_t *>
                         (m_saved_packets->at(0))[byte_index]), 
                             static_cast<size_t>(m_amnt_saved));
                // Update byte index with the new location...
                byte_index += m_amnt_saved;
            } catch (std::exception &e) {
                // If we got here, let's try to increment the 
                // index by one word and then try again.
                std::cout<<e.what()<<std::endl;
                byte_index = total_length;
                m_amnt_saved = 0;
                continue;
            }
        }
    }
    // Wait for any active threads to finish before ending.
    while (threads_active()) {
        usleep(LONG_USLEEP);
    }
}

/* 
Takes a packet out of saved_packets at index i and 
processes it into the current write buffer.
*/
void
inline complex_manager::process_packet(int i)
{
    if (m_packet_buffer[i] == NULL) {
        return;
    }
    // Loop through and create our complex values
    int start = 0;
    int end = start + m_sample_count[i];
    for (int j = start; j < end; j++) {
        boost::uint32_t flipped = 
            static_cast<boost::uint32_t>(m_packet_buffer[i][j]);

        // Get the left and right original values as int16's.
        boost::int16_t left_original =
            (boost::int16_t)
            (((flipped & (boost::uint32_t)0xFFFF0000)) >> 16);
        boost::int16_t right_original =
            (boost::int16_t)(flipped & (boost::uint32_t)0xFFFF);

        // Divide by our scale.
        float real, imag;
        real = (static_cast<float>(left_original) / IQ_SCALE_FACTOR);
        imag = (static_cast<float>(right_original) /IQ_SCALE_FACTOR);

        // Store our complex value.
        m_target_buffer[i][m_start_index[i] + j - start] =
            std::complex<float>(real, imag);
    }
    m_target_buffer[i] = NULL;
    m_packet_buffer[i] = NULL;
}

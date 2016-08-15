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


#ifndef INCLUDED_PICO_PICO_SINK_H
#define INCLUDED_PICO_PICO_SINK_H

#include <pico/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace pico {

    /*!
     * \brief Pico sink
     * \ingroup pico
     *
     */
    class PICO_API pico_sink : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<pico_sink> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of pico::pico_sink.
       *
       * To avoid accidental use of raw pointers, pico::pico_sink's
       * constructor is in a private implementation
       * class. pico::pico_sink::make is the public interface for
       * creating new instances.
       */
      static sptr make(int input_type,
              std::string ip,
              short mne_port,
              short data_port);

      virtual void update_sample_rate(double sr) = 0;
      virtual void update_power(double pwr) = 0;
      virtual void update_frequency(double freq) = 0;
      virtual void update_bandwidth(double bw) = 0;
    };

  } // namespace pico
} // namespace gr

#endif /* INCLUDED_PICO_PICO_SINK_H */


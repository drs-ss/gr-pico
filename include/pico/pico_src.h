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


#ifndef INCLUDED_PICO_PICO_SRC_H
#define INCLUDED_PICO_PICO_SRC_H

#include <pico/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace pico {

    /*!
     * \brief <+description of block+>
     * \ingroup pico
     *
     */
    class PICO_API pico_src : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<pico_src> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of pico::pico_src.
       *
       * To avoid accidental use of raw pointers, pico::pico_src's
       * constructor is in a private implementation
       * class. pico::pico_src::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string radio_ip,
              std::string machine_ip, int mne_port, int data_port);

      virtual void update_sample_rate(double sr) = 0;
      virtual void update_attenuation(double atten) = 0;
      virtual void update_frequency(double freq) = 0;
      virtual void update_channel(int channel) = 0;

    };

  } // namespace pico
} // namespace gr

#endif /* INCLUDED_PICO_PICO_SRC_H */


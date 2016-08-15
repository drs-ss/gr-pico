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

/*******************************************************************/
/*!
** \file        VITA49.hpp
**
** \brief       Classes and methods for handling VITA 49 data
**
** \todo        Handle data streams with 
                callbacks to get more data, etc.
**
********************************************************************/

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

class index_error: public std::exception
{
public:
    /** Constructor (C strings).
     *  @param message C-style string error message.  The string
     *                 contents are copied upon construction. Hence,
     *                 responsibility for deleting the char* lies
     *                 with the caller.
     */
    explicit index_error(const char* message):
      msg_(message)
      {
      }

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit index_error(const std::string& message):
      msg_(message)
      {}

    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~index_error() throw (){}

    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a const char*. The underlying memory
     *          is in posession of the Exception object. Callers must
     *          not attempt to free the memory.
     */
    virtual const char* what() const throw (){
       return msg_.c_str();
    }

protected:
    /** Error message.
     */
    std::string msg_;
};

/******************************************************************/
/*!
** \brief       A VITA Radio Transport (VRT) Packet
**
** Provides the necessary facilities to parse a 
** VRT packet of any type as described in ANSI/VITA 49.0
**
** \note This implementation was based upon Draft 0.21 published
** 31 October 2007
**
********************************************************************/
class VRTPacket
{
public:

    //! Smart pointer for this class
    typedef boost::shared_ptr<VRTPacket> sptr;


    /***************************************************************/
    /*!
    ** \brief       Parse raw data into a VRT packet
    **
    ** This will parse the first packet found 
    **  in the provided block of data.
    **
    ** \warning     This implementation assumes the data 
    **              you are parsing
    **              has a longer life-cyle than this object.
    **
    ** \throw       std::runtime_error
    **                  if the provided data could not be 
    **                  parsed for valid packet data
    **
    ** \throw       index_error
    **                  if the provided data is smaller 
    **                  than the size of the packet
    **
    ** \param       packet_data block of data containing the packet
    ** \param       size        size of packet_data in bytes
    **
    ** \return      A smart pointer to the parsed packet
    ****************************************************************/
    static sptr parse(const boost::uint32_t *packet_data,
            size_t size);


    /***************************************************************/
    /*!
    ** \brief       Get a pretty string of the packet information
    **
    ** \return      The packet info string.
    ****************************************************************/
    virtual std::string packet_info_string() const = 0;

    /**
     * Type of VRT packet.
     *
     * \sa ANSI/VITA 49.0 table 6.1.1-1
     */
    enum PACKET_TYPE
    {
        PT_IF_DATA_NO_IDENT         = 0x0,
        PT_IF_DATA                  = 0x1,
        PT_EXTENSION_DATA_NO_IDENT  = 0x2,
        PT_EXTENSION_DATA           = 0x3,
        PT_IF_CONTEXT               = 0x4,
        PT_EXTENSION_CONTEXT        = 0x5,
        PT_MIN                      = PT_IF_DATA_NO_IDENT,
        PT_MAX                      = PT_EXTENSION_CONTEXT
    };


    /***************************************************************/
    /*!
    ** \brief       Get the type of VRT packet
    **
    ** \return      Type of packet
    ****************************************************************/
    virtual PACKET_TYPE get_packet_type(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Check whether this packet contains a stream ID
    **
    ** Indicates whether or not the VRT packet header specifies that
    ** a stream ID value is included in this packet. The stream ID
    ** is an optional field.
    **
    ** \return      True if a stream ID is available
    ****************************************************************/
    virtual bool has_stream_id(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the 32-bit stream identifier word
    **
    ** \sa          ANSI/VITA 49.0 section 6.1.2
    **
    ** \throw       std::runtime_error
    **                  if the packet contains no stream identifier
    **
    ** \return      The 32-bit stream id word
    ****************************************************************/
    virtual boost::uint32_t get_stream_id(void) const = 0;


    /**
     * VRT class identifier field
     *
     * \sa ANSI/VITA 49.0 section 6.1.3
     */
    struct class_identifier
    {
        boost::uint32_t organization_id;
        boost::uint16_t info_class_code;
        boost::uint16_t packet_class_code;
    };


    /***************************************************************/
    /*!
    ** \brief       Check whether this packet contains a class ID
    **
    ** Indicates whether or not the VRT packet header specifies
    ** that a class identifier is included in the packet. The
    ** class identifier is an optional field.
    **
    ** \return      True if a class ID is available
    ****************************************************************/
    virtual  bool has_class_id(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the class identifier
    **
    ** \throw       std::runtime_error
    **                  if the packet contains no class identifier
    **
    ** \return      The class identifier
    ****************************************************************/
    virtual class_identifier get_class_id(void) const = 0;


    /**
     * Precision of timestamp for context packets
     */
    enum TIMESTAMP_MODE
    {
        TSM_PRECISE = 0x0,
        TSM_GENERAL = 0x1
    };


    /***************************************************************/
    /*!
    ** \brief     Get the precision represented by the timestamp data
    **
    ** Indicates the precision of the packet timestamp(s), 
    **    if it contains any.
    **
    ** \throw       std::runtime_error
    **                  if this is not a context packet
    **
    ** \return      The precision of the timestamp(s)
    ****************************************************************/
    virtual TIMESTAMP_MODE get_timestamp_mode(void) const = 0;


    /**
     * Type of integer timestamp
     *
     * \sa ANSI/VITA 49.0 table 6.1.1-2
     */
    enum INTEGER_TIMESTAMP_TYPE
    {
        TSI_NONE    = 0x0,
        TSI_UTC     = 0x1,
        TSI_GPS     = 0x2,
        TSI_OTHER   = 0x3
    };


    /***************************************************************/
    /*!
    ** \brief   Get the type of integer timestamp the packet contains
    **
    ** Indicates what type of integer timestamp the VRT packet header
    ** specifies. 
    **
    ** \return      Type of integer timestamp included in this packet
    ****************************************************************/
    virtual INTEGER_TIMESTAMP_TYPE
        get_integer_timestamp_type(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the 32-bit integer timestamp
    **
    ** \sa          ANSI/VITA 49.0 section 6.1.4
    **
    ** \throw       std::runtime_error
    **
    ** \return      The 32-bit integer timestamp
    ****************************************************************/
    virtual boost::uint32_t get_integer_timestamp(void) const = 0;

    /**
     * Type of fractional timestamp
     *
     * \sa ANSI/VITA 49.0 table 6.1.1-3
     */
    enum FRACTIONAL_TIMESTAMP_TYPE
    {
        TSF_NONE            = 0x0,
        TSF_SAMPLE_COUNT    = 0x1,
        TSF_REAL_TIME       = 0x2,
        TSF_FREE_RUNNING    = 0x3
    };


    /***************************************************************/
    /*!
    ** \brief Get the type of fractional timestamp the packet
    ** contains
    **
    ** Indicates what type of fractional timestamp 
    **    the VRT packet header specifies. 
    **
    ** \return   Type of fractional timestamp included in this packet
    ****************************************************************/
    virtual FRACTIONAL_TIMESTAMP_TYPE
        get_fractional_timestamp_type(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the 64-bit fractional timestamp
    **
    ** \sa          ANSI/VITA 49.0 section 6.1.5
    **
    ** \throw       std::runtime_error
    **              if there is no fractional timestamp in this packet

    **
    ** \return      The 64-bit fractional timestamp
    ****************************************************************/
    virtual boost::uint64_t get_fractional_timestamp(void) const = 0;

    /**
     * Mask value for packet counter, used to determine rollover
     */
    static const boost::uint8_t PACKET_COUNT_MASK = 0x0F;


    /***************************************************************/
    /*!
    ** \brief       Get the packet count value
    **
    ** The packet count is a modulo-16 count of 
    ** this packet in the stream.
    **
    ** \note    The actual packet count value is only 4 bits, it is
    **          represented in the least significant bits of the
    **          8-bit result.
    **
    ** \return      The packet count
    ****************************************************************/
    virtual boost::uint8_t get_packet_count(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the number of 32-bit words in the packet
    **
    ** This value represents the number of 32-bit words in the packet
    ** including the header, payload, and any optional fields.
    **
    ** \return      The packet size in number of 32-bit words
    ****************************************************************/
    virtual boost::uint16_t get_packet_size(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the size of the payload data
    **
    ** \return      the payload size in bytes
    ****************************************************************/
    virtual size_t get_payload_size(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the packet payload as any type
    **
    **
    **
    **
    ** \return      A pointer to the payload data
    ****************************************************************/
    virtual const boost::uint32_t* get_payload() = 0;

    /***************************************************************/
    /*!
    ** \brief       Get whether this packet contains a trailer
    **
    ** Indicates whether or not the VRT packet header specifies that
    ** a trailer value is included in this packet. The trailer is an
    ** optional field.
    **
    ** \return      True if the trailer is available
    ****************************************************************/
    virtual bool has_trailer(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the trailer data
    **
    ** \throw       std::runtime_error
    **                  if the packet contains no trailer
    **
    ** \todo        Parse the trailer and provide extra functionality
    **
    ** \sa          ANSI/VITA 49.0 section 6.1.7
    **
    ** \return      The trailer data
    ****************************************************************/
    virtual boost::uint32_t get_trailer(void) const = 0;
};


/*******************************************************************/
/*!
** \brief       A VITA Radio Link Layer (VRL) Frame
**
** Provides the necessary facilities to parse a VRL frame as
** described in ANSI/VITA 49.1 and retrieve VRT packets from it.
**
** \note      This implementation was based upon Draft 0.10 published
** 20 December 2007
********************************************************************/
class VRLFrame
{
public:

    //! Smart pointer to this class
    typedef boost::shared_ptr<VRLFrame> sptr;


    /***************************************************************/
    /*!
    ** \brief       Parse raw data into a VRL frame
    **
    ** This will parse the first parse the first frame found in the
    ** provided block of data.
    **
    ** \warning     This implementation assumes the data your parsing
    **              has a longer life-cyle than this object.
    **
    ** \throw       std:::runtime_error
    **                  if the provided data could not be parsed for
    **                  valid frame data
    **
    ** \throw       index_error
    **               if the provided data is smaller than the size of
    **               the frame or a packet therein
    **
    ** \param       frame_data  block of data containing the frame
    ** \param       size    size of frame_data
    ** \param[out]  index   where in the frame_data the packet starts
    **
    ** \return      A smart pointer to the frame parser
    ****************************************************************/
    static sptr parse(const boost::uint32_t *frame_data, size_t size, 
                      std::ptrdiff_t *index = NULL);


    /***************************************************************/
    /*!
    ** \brief       Get a pretty string of the frame information
    **
    ** \return      The frame info string
    ****************************************************************/
    virtual std::string frame_info_string() const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the Frame Alignment Word (FAW)
    **
    ** According to ANSI/VITA 49.1 
    **      this is always the ASCII string "VRLP".
    **
    ** \note     The SI-8649A (Picoceptor) violates this and uses the
    **           ASCII string "VS49" for the FAW
    **
    ** \return      The 32-bit FAW
    ****************************************************************/
    virtual boost::uint32_t get_alignment_word(void) const = 0;


    /**
     * Mask value for packet counter, used to determine rollover
     */
    static const boost::uint16_t FRAME_COUNT_MASK = 0x0FFF;


    /***************************************************************/
    /*!
    ** \brief       Get the rolling frame counter value
    **
    ** The frame count is a modulo-4096 
    **  count of this frome in the stream.
    **
    ** \note   The actual frame count value is only 12 bits, it is
    **         represented in the lowest 12 bits of the 16-bit result
    **
    ** \return      The frame count
    ****************************************************************/
    virtual boost::uint16_t get_frame_count(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the number of 32-bit words in the frame
    **
    ** This value represents the number of 32-bit words in the frame
    ** including the header and trailer.
    **
    ** \return      The frame size
    ****************************************************************/
    virtual boost::uint32_t get_frame_size(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the number of packets in the frame
    **
    ** \return      The number of packets
    ****************************************************************/
    virtual size_t get_num_packets(void) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get a VRT packet
    **
    ** \throw      index_error
    **               if the specified index value is greater than the
    **               number of packets in the frame
    **
    ** \param       index      which packet to get
    **
    ** \return      A smart pointer to the requested VRT packet
    ****************************************************************/
    virtual const VRTPacket::sptr get_packet(size_t index = 0) const = 0;


    /***************************************************************/
    /*!
    ** \brief       Get the frame trailer word
    **
    ** According to ANSI/VITA 49.1 this is always the ASCII string
    ** "VEND" or a CRC32 of the frame.
    **
    ** \todo        Add function to verify CRC32 if trailer is one
    **
    ** \return      The trailer word
    ****************************************************************/
    virtual boost::uint32_t get_trailer() const = 0;
};

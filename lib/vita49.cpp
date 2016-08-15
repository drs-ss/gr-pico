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

/****************************************************************//**
**
** \file        VITA49.cpp
**
** \brief       Implementation of classes and methods for handling 
**              VITA49 data.
**
********************************************************************/

#include <boost/detail/endian.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <sstream> // For info strings
#include <iostream>
#include "vita49.hpp"
#include <exception>

#if defined(BOOST_LITTLE_ENDIAN)

typedef struct
{
    //! Total number of 32-bit words in packet
    boost::uint32_t packet_size   : 16; 
    //! Modulo-16 count of packets
    boost::uint32_t packet_count  : 4; 
    //! Fractional timestamp mode
    boost::uint32_t tsf           : 2; 
    //! Integer timestamp mode
    boost::uint32_t tsi           : 2;
    //! Timestamp mode (Context packets only) 
    boost::uint32_t tsm           : 1; 
    //! Reserved
    boost::uint32_t rsvd          : 1; 
    //! Indicates if packet contains trailer (Data packets only)
    boost::uint32_t t             : 1;
    //! Indicates if a class ID is included
    boost::uint32_t c             : 1; 
    //! Type of packet \sa VITA49Packet::PACKET_TYPE
    boost::uint32_t packet_type   : 4; 
    
} VRTHeader;

typedef struct
{
    boost::uint64_t packet_code : 16;
    boost::uint64_t info_code   : 16;
    boost::uint64_t oui         : 24;
    boost::uint64_t researved   : 8;
} VRTClassID;

#elif defined(BOOST_BIG_ENDIAN)

typedef struct
{
    //! Type of packet \sa VITA49Packet::PACKET_TYPE
    boost::uint32_t packet_type   : 4; 
    //! Indicates if a class ID is included
    boost::uint32_t c             : 1; 
    //! Indicates if packet contains trailer (Data packets only)
    boost::uint32_t t             : 1; 
    //! Reserved
    boost::uint32_t rsvd          : 1; 
    //! Timestamp mode (Context packets only)
    boost::uint32_t tsm           : 1;
    //! Integer timestamp mode 
    boost::uint32_t tsi           : 2; 
    //! Fractional timestamp mode
    boost::uint32_t tsf           : 2; 
    //! Modulo-16 count of packets
    boost::uint32_t packet_count  : 4; 
    //! Total number of 32-bit words in packet
    boost::uint32_t packet_size   : 16; 
} VRTHeader;

typedef struct
{
    boost::uint64_t researved   : 8;
    boost::uint64_t oui         : 24;
    boost::uint64_t info_code   : 16;
    boost::uint64_t packet_code : 16;
} VRTClassID;

#else
#  error The VITA49 parser only supports big and little endian architectures.
#endif

class VRTPacketImpl : public VRTPacket
{
public:

    VRTPacketImpl(const boost::uint32_t *packet_data, size_t size) :
        m_packet_data(packet_data)
    {
        m_header = (VRTHeader *)&packet_data[0];

        PACKET_TYPE pt = (PACKET_TYPE)m_header->packet_type;
        // Ensure we know how to handle this packet type
        if ((pt < PT_MIN) || (pt > PT_MAX))
        {
            throw std::runtime_error(str(boost::format(
                        "Unknown VITA49 packet type %01X") %
                    ((boost::uint8_t)m_header->packet_type)));
        }

        if ((m_header->packet_size) > size / 4)
        {
            //! this needs to be changed to ignore it if its a split 
            //! packet. or ensure that split packets dont get here 
            std::cout << boost::format(
                       "VITA49 packet size (%d words) is larger than"
                       " the supplied buffer size (%d words)."
                       ) % (boost::uint32_t)m_header->packet_size %
                       (size / 4) << std::endl;
            throw index_error(str(boost::format(
                       "VITA49 packet size (%d words) is larger"
                       " than the supplied buffer size (%d words).")%
                       (boost::uint32_t)m_header->packet_size %
                       (size / 4)));
        }
    }

    std::string 
    packet_info_string() const
    {
        std::ostringstream ss;
        PACKET_TYPE pt = get_packet_type();
        ss << boost::format(
            "VITA49 Packet:\n"
            "            Packet Type: 0x%X\n") % pt;

        if (has_stream_id() == true)
        {
            ss << boost::format(
                "              Stream ID: Yes (0x%08X)\n") %
                get_stream_id();
        }
        else
        {
            ss << "              Stream ID: No" << std::endl;
        }

        ss << boost::format(
            "           Packet Count: %d\n"
            "            Packet Size: %d words\n"
            ) % static_cast<unsigned int>(get_packet_count()) % 
                                          get_packet_size();

        if (has_class_id() == true)
        {
            ss << boost::format(
                "               Class ID: Yes\n"
                "        Organization ID: 0x%06X\n"
                "        Info Class Code: 0x%04X\n"
                "      Packet Class Code: 0x%04X\n"
                ) % get_class_id().organization_id % 
                get_class_id().info_class_code %
                get_class_id().packet_class_code;
        }
        else
        {
            ss << "               Class ID: No" << std::endl;
        }

        try
        {
            if (get_timestamp_mode() == TSM_GENERAL)
            {
                ss << "    Timestamp Precision: General" <<std::endl;
            }
            else
            {
                ss << "    Timestamp Precision: Precise" <<std::endl;
            }
        }
        catch (std::runtime_error)
        {
            ss << "    Timestamp Precision: N/A" << std::endl;
        }

        INTEGER_TIMESTAMP_TYPE itt = get_integer_timestamp_type();
        if (itt != TSI_NONE)
        {
            ss << boost::format(
                "        Integer TS Type: 0x%0X (0x%08X)\n") %
                itt % get_integer_timestamp();
        }
        else
        {
            ss << "        Integer TS Type: None" << std::endl;
        }

        if (get_fractional_timestamp_type() != TSF_NONE)
        {
            ss << boost::format(
                "     Fractional TS Type: 0x%0X (0x%016X)\n") %
                itt % get_fractional_timestamp();
        }
        else
        {
            ss << "    Fractional TS Type: None" << std::endl;
        }

        ss << boost::format(
            "           Payload Size: %d bytes\n") %
            get_payload_size();

        if (has_trailer() == true)
        {
            ss << boost::format(
                "                Trailer: 0x%08X") %
                get_trailer();
        }
        else
        {
            ss << "                Trailer: None" << std::endl;
        }

        return ss.str();
    }

    PACKET_TYPE 
    get_packet_type(void) const
    {
        return (PACKET_TYPE)m_header->packet_type;
    }

    boost::uint8_t 
    get_packet_count(void) const
    {
        return (m_header->packet_count);
    }

    boost::uint16_t 
    get_packet_size(void) const
    {
        return (m_header->packet_size);
    }

    bool 
    has_stream_id(void) const
    {
        PACKET_TYPE pt = get_packet_type();
        if((pt == PT_IF_DATA) || (pt == PT_EXTENSION_DATA) || 
           (pt == PT_IF_CONTEXT) || (pt == PT_EXTENSION_CONTEXT))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    boost::uint32_t 
    get_stream_id(void) const
    {
        if (has_stream_id() != true)
        {
            throw std::runtime_error("Cannot get stream ID on "
                                     "this packet type.");
        }
        size_t sid_offset = get_field_offset(STREAM_ID);
        return (m_packet_data[sid_offset]);
    }

    bool 
    has_class_id(void) const
    {
        return (m_header->c == 0x1);
    }

    class_identifier 
    get_class_id(void) const
    {
        if (has_class_id() != true)
        {
            throw std::runtime_error("Cannot get the class ID from "
                                     "this packet, it does not "
                                     "have one.");
        }

        size_t cid_offset = get_field_offset(CLASS_ID);

        VRTClassID *cid_ptr= (VRTClassID*)&m_packet_data[cid_offset];
        class_identifier cid;
        cid.organization_id = cid_ptr->oui;
        cid.info_class_code = cid_ptr->info_code;
        cid.packet_class_code = cid_ptr->packet_code;
        return cid;
    }

    TIMESTAMP_MODE 
    get_timestamp_mode(void) const
    {
        PACKET_TYPE pt = get_packet_type();
        if((pt == PT_IF_CONTEXT) || (pt == PT_EXTENSION_CONTEXT))
        {
            return ((TIMESTAMP_MODE)m_header->tsm);
        }
        else
        {
            throw std::runtime_error("Cannot get the "
                    "timestamp mode on this packet type.");
        }
    }

    INTEGER_TIMESTAMP_TYPE 
    get_integer_timestamp_type(void) const
    {
        if (m_header->tsi == TSI_NONE ||
            m_header->tsi == TSI_UTC  ||
            m_header->tsi == TSI_GPS  ||
            m_header->tsi == TSI_OTHER) 
        {
            return ((INTEGER_TIMESTAMP_TYPE)m_header->tsi);
        }else{
            return ((INTEGER_TIMESTAMP_TYPE)TSI_NONE);
        }
    }

    boost::uint32_t 
    get_integer_timestamp(void) const
    {
        INTEGER_TIMESTAMP_TYPE tsi = get_integer_timestamp_type();
        if (tsi == TSI_NONE)
        {
            throw std::runtime_error("Connot get the integer "
                                     "timestamp from this packet, "
                                     "it does not have one.");
        }

        size_t tsi_offset = get_field_offset(TSI);
        return (m_packet_data[tsi_offset]);
    }

    FRACTIONAL_TIMESTAMP_TYPE 
    get_fractional_timestamp_type(void) const
    {
        return ((FRACTIONAL_TIMESTAMP_TYPE)m_header->tsf);
    }

    boost::uint64_t 
    get_fractional_timestamp(void) const
    {
        FRACTIONAL_TIMESTAMP_TYPE tsf =
                get_fractional_timestamp_type();
        if(tsf == TSF_NONE)
        {
            throw std::runtime_error("Cannot get the fractional "
                                     "timestamp from this packet, "
                                     "it does not have one.");
        }

        size_t tsf_offset = get_field_offset(TSF);
        return ((boost::uint64_t(m_packet_data[tsf_offset]) << 32) | 
               (boost::uint64_t(m_packet_data[tsf_offset + 1]) <<0));

    }

    size_t 
    get_payload_size(void) const
    {
        size_t size = m_header->packet_size;
        size -= get_field_offset(PAYLOAD);
        if (has_trailer() == true)
        {
            size -= 1;
        }
        // API says size in bytes, 
        // packet size is number of 32-bit words
        size = size * 4; 
        return size;
    }

    bool 
    has_trailer(void) const
    {
        PACKET_TYPE pt = get_packet_type();
        if ((pt == PT_IF_CONTEXT) || (pt == PT_EXTENSION_CONTEXT))
        {
            return false;
        }
        else
        {
            return (m_header->t == 0x1);
        }
    }

    boost::uint32_t 
    get_trailer(void) const
    {
        if (has_trailer() != true)
        {
            throw std::runtime_error("Cannot get the trailer "
                                     "from this packet, "
                                     "it does not have one.");
        }
        size_t trailer_offset = get_field_offset(TRAILER);
        return (m_packet_data[trailer_offset]);
    }

private:
    const boost::uint32_t*  m_packet_data;
    VRTHeader*              m_header;

    enum VS49_FIELD
    {
        STREAM_ID   = 0,
        CLASS_ID    = 1,
        TSI         = 2,
        TSF         = 3,
        PAYLOAD     = 4,
        TRAILER     = 5
    };

    /**********************************************************/
    /*!
    ** \brief       Get the offset of a given field in the packet
    **
    ** Uses header information to determine if previous
    ** fields are in the packet and adds their size to the offset.
    **
    ** \warning     Doesn't care if the requested field exists in the
    **              packet or not
    **
    ** \param       field   the field we want the offset of
    **
    ** \return      the offset of the requested field
    *************************************************************/
    size_t 
    get_field_offset(VS49_FIELD field) const
    {
        size_t offset = 1;
        if (field == TRAILER)
        {
            offset = m_header->packet_size - 1;
        }
        else if (field != STREAM_ID)
        {   
            if (has_stream_id() == true)
            {
                offset += 1;
            }
            if (field != CLASS_ID)
            {
                if (has_class_id() == true)
                {
                    offset += 2;
                }
                if (field != TSI)
                {
                    if (get_integer_timestamp_type() !=
                            TSI_NONE)
                    {
                        offset += 1;
                    }
                    if (field != TSF)
                    {
                        if (get_fractional_timestamp_type() !=
                                TSF_NONE)
                        {
                            offset += 2;
                        }

                    }
                }
            }
        }
        return offset;
    }

    const boost::uint32_t* 
    get_payload()
    {
        size_t payload_offset = get_field_offset(PAYLOAD);
        return (&m_packet_data[payload_offset]);
    }

};


typedef struct
{
    boost::uint32_t frame_size  : 20;
    boost::uint32_t frame_count : 12;
} VRLFrameInfo;


class VRLFrameImpl : public VRLFrame
{

public:

    VRLFrameImpl(const boost::uint32_t * frame_data, size_t size, 
                 std::ptrdiff_t *index) :
        m_frame_data(frame_data)
    {

        if (size < sizeof(boost::uint32_t)*5)
        {
            throw index_error("Not enough data for a VRL frame.");
        }

        while ((size >= sizeof(boost::uint32_t)*5) &&
               (m_frame_data[0] != 0x56524C50) && /* VRLP */
               (m_frame_data[0] != 0x56533439)    /* VS49 */
               )
        {
            m_frame_data++;
            size -= sizeof(boost::uint32_t);
        }

        if (index != NULL)
        {
            *index = (m_frame_data - frame_data);
        }

        if (size < sizeof(boost::uint32_t)*5)
        {
            throw std::runtime_error("Unable to find VRL frame "
                                     "alignment word.");
        }

        m_frame_info = reinterpret_cast<VRLFrameInfo *>
            (const_cast<boost::uint32_t *>(&m_frame_data[1]));

        const size_t frame_size = m_frame_info->frame_size;

        if (frame_size > size/sizeof(boost::uint32_t))
        {
            throw index_error(str(boost::format(
                "Frame size (%d words) is larger than the supplied "
                "buffer size (%d words)."
                ) % frame_size % (size / sizeof(boost::uint32_t))));
        }

        if (m_frame_data[frame_size - 1] != 0x56454E44 /* VEND */)
        {
            //! \todo Verify trailer CRC32
        }

        // Parse out all of the packets
        size_t packet_offset = 2;
        do
        {
            // Calculate the remaining 32-bit words in the buffer
            size_t buffer_remaining = frame_size - packet_offset - 1;
            VRTPacket::sptr packet = 
                VRTPacket::parse(&m_frame_data[packet_offset], 
                (buffer_remaining * sizeof(boost::uint32_t)));
            if (packet->get_packet_size() == 0) {
                break;
            }
            m_packets.push_back(packet);
            packet_offset += packet->get_packet_size();
        }
        while ((packet_offset + 1) < frame_size);
    }

    std::string 
    frame_info_string() const
    {
        return str(boost::format(
            "VRL Frame:\n"
            "              FAW: 0x%X\n"
            "    Frame Counter: %d\n"
            "       Frame Size: %d words\n"
            "    Total Packets: %d\n"
            "          Trailer: 0x%X\n"
            ) % get_alignment_word() % get_frame_count() % 
            get_frame_size() % get_num_packets() % get_trailer());
    }

    boost::uint16_t 
    get_frame_count(void) const
    {
        return (m_frame_info->frame_count);
    }

    boost::uint32_t 
    get_alignment_word(void) const
    {
        return (m_frame_data[0]);
    }

    boost::uint32_t 
    get_frame_size(void) const
    {
        return (m_frame_info->frame_size);
    }

    boost::uint32_t 
    get_trailer() const
    {
        size_t trailer_offset = get_frame_size() - 1;
        return (m_frame_data[trailer_offset]);
    }

    const VRTPacket::sptr 
    get_packet(size_t index) const
    {
        return m_packets.at(index);
    }

    size_t 
    get_num_packets(void) const
    {
        return m_packets.size();
    }

private:
    const boost::uint32_t*  m_frame_data;
    VRLFrameInfo*           m_frame_info;
    std::vector<VRTPacket::sptr> m_packets;
};


VRTPacket::sptr
VRTPacket::parse(const boost::uint32_t *packet_data,
                                 size_t size)
{
    return boost::make_shared<VRTPacketImpl>(packet_data, size);
}

VRLFrame::sptr
VRLFrame::parse(const boost::uint32_t *frame_data,
                                size_t size, std::ptrdiff_t *index)
{
    return boost::make_shared<VRLFrameImpl>(frame_data, size, index);
}

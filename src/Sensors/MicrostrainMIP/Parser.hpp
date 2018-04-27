//***************************************************************************
// Copyright 2015-2017 OceanScan - Marine Systems & Technology, Lda.        *
//***************************************************************************
// This file is subject to the terms and conditions defined in file         *
// 'LICENCE.md', which is part of this source code package.                 *
//***************************************************************************
// Author: Ricardo Martins                                                  *
//***************************************************************************

#ifndef SENSORS_MICROSTRAIN_MIP_PARSER_HPP_INCLUDED_
#define SENSORS_MICROSTRAIN_MIP_PARSER_HPP_INCLUDED_

// ISO C++ 98 headers.
#include <cstring>

// DUNE headers.
#include <DUNE/DUNE.hpp>

// Local headers.
#include "Constants.hpp"

namespace Sensors
{
  namespace MicrostrainMIP
  {
    using DUNE_NAMESPACES;

    class Parser
    {
    public:
      Parser(void):
        m_state(STA_SYNC0),
        m_invalid_checksum_count(0),
        m_checksum_msb(0),
        m_checksum_lsb(0),
        m_payload_size(0),
        m_payload_index(0),
        m_timestamp(0)
      { }

      bool
      parse(uint8_t byte)
      {
        switch (m_state)
        {
          case STA_SYNC0:
            if (byte == c_sync0)
            {
              m_timestamp = Clock::getSinceEpoch();
              m_checksum_msb = byte;
              m_checksum_lsb = byte;
              m_payload_size = 0;
              m_payload_index = 0;
              m_state = STA_SYNC1;
            }
            break;

          case STA_SYNC1:
            m_checksum_msb += byte;
            m_checksum_lsb += m_checksum_msb;
            if (byte == c_sync1)
              m_state = STA_DESC;
            else
              m_state = STA_SYNC1;
            break;

          case STA_DESC:
            m_checksum_msb += byte;
            m_checksum_lsb += m_checksum_msb;
            m_desc = byte;
            m_state = STA_PLEN;
            break;

          case STA_PLEN:
            m_checksum_msb += byte;
            m_checksum_lsb += m_checksum_msb;
            m_payload_size = byte;
            m_state = STA_PLOAD;
            break;

          case STA_PLOAD:
            m_checksum_msb += byte;
            m_checksum_lsb += m_checksum_msb;
            m_payload[m_payload_index++] = byte;
            if (m_payload_index == m_payload_size)
              m_state = STA_CHECKSUM_MSB;
            break;

          case STA_CHECKSUM_MSB:
            if (byte == m_checksum_msb)
            {
              m_state = STA_CHECKSUM_LSB;
            }
            else
            {
              m_state = STA_SYNC0;
              ++m_invalid_checksum_count;
            }
            break;

          case STA_CHECKSUM_LSB:
            m_state = STA_SYNC0;
            if (byte == m_checksum_lsb)
              return true;
            else
              ++m_invalid_checksum_count;
            break;
        }

        return false;
      }

      //! Get the number of invalid checksums detected thus far.
      //! @return number of invalid checksums.
      size_t
      getInvalidChecksumCount(void) const
      {
        return m_invalid_checksum_count;
      }

      //! Clear invalid checksum count.
      void
      clearInvalidChecksumCount(void)
      {
        m_invalid_checksum_count = 0;
      }

      uint8_t
      getDescriptorSet(void) const
      {
        return m_desc;
      }

      uint8_t
      getFieldDescriptor(void) const
      {
        return m_payload[1];
      }

      size_t
      getPayloadSize(void) const
      {
        return m_payload_size;
      }

      const uint8_t*
      getPayload(void) const
      {
        return m_payload;
      }

      double
      getTimeStamp(void) const
      {
        return m_timestamp;
      }

    private:
      //! Parser states.
      enum State
      {
        STA_SYNC0,
        STA_SYNC1,
        STA_DESC,
        STA_PLEN,
        STA_PLOAD,
        STA_CHECKSUM_MSB,
        STA_CHECKSUM_LSB
      };

      //! Synchronization byte #0.
      static const uint8_t c_sync0 = 0x75;
      //! Synchronization byte #1.
      static const uint8_t c_sync1 = 0x65;
      //! Current parser state.
      State m_state;
      //! Count of invalid checksums.
      size_t m_invalid_checksum_count;
      //! Computed checksum.
      uint8_t m_checksum_msb;
      //! Computed checksum.
      uint8_t m_checksum_lsb;
      //! Current descriptor set byte.
      uint8_t m_desc;
      //! Payload.
      uint8_t m_payload[256];
      //! Payload size.
      size_t m_payload_size;
      //! Payload write index.
      size_t m_payload_index;
      //! Packet timestamp.
      double m_timestamp;
    };
  }
}

#endif
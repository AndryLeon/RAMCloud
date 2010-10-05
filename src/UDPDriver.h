/* Copyright (c) 2010 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef RAMCLOUD_UDPDRIVER_H
#define RAMCLOUD_UDPDRIVER_H

#include "Common.h"
#include "Driver.h"

namespace RAMCloud {

/**
 * A Driver for kernel-provided UDP communication.  Simple packet send/receive
 * style interface. See Driver for more detail.
 */
class UDPDriver : public Driver {
  public:
    /// The maximum number bytes we can stuff in a UDP packet payload.
    static const uint32_t MAX_PAYLOAD_SIZE = 1400;

    explicit UDPDriver(const ServiceLocator* localServiceLocator = NULL);
    virtual ~UDPDriver();
    virtual uint32_t getMaxPayloadSize();
    virtual void release(char *payload, uint32_t len);
    virtual void sendPacket(const Address *addr,
                            void *header,
                            uint32_t headerLen,
                            Buffer::Iterator *payload);
    virtual bool tryRecvPacket(Received *received);

    virtual Address* newAddress(const ServiceLocator* serviceLocator) {
        return new UDPAddress(serviceLocator);
    }

  private:
    struct UDPAddress : Address {
        UDPAddress() : address() {}
        explicit UDPAddress(const ServiceLocator* serviceLocator);
        UDPAddress(const UDPAddress& other)
            : Address(other), address(other.address) {}
        UDPAddress* clone() const {
            return new UDPAddress(*this);
        }
        sockaddr address;
      private:
        void operator=(UDPAddress&);
    };

    /**
     * Concatenates a UDPAddress and a variable-length payload.
     * Used for received packets.
     */
    struct AddressPayload {
        AddressPayload() : udpAddress() {}
        UDPAddress udpAddress;
        char payload[0];
    };

    /// File descriptor of the UDP socket this driver uses for communication.
    int socketFd;

    /// Tracks number of outstanding allocated payloads.  For detecting leaks.
    int packetBufsUtilized;

    DISALLOW_COPY_AND_ASSIGN(UDPDriver);
};

} // end RAMCloud

#endif  // RAMCLOUD_UDPDRIVER_H

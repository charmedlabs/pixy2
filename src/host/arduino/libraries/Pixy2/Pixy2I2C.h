//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//
// Arduino I2C link class for Pixy2 Camera.
// Provides communication transport over I2C using the Arduino Wire library.
// Includes modifications for robust error handling on I2C communication failures.

#ifndef _PIXY2I2C_H
#define _PIXY2I2C_H

#include "TPixy2.h" // TPixy2 defines PIXY_RESULT_ERROR etc.
#include "Wire.h"

#define PIXY_I2C_DEFAULT_ADDR           0x54
#define PIXY_I2C_MAX_SEND               16 // Maximum bytes to send in a single I2C transmission

class Link2I2C
{
public:
    /**
     * @brief Initializes the I2C link.
     * @param arg I2C address of the Pixy2 camera, or PIXY_DEFAULT_ARGVAL for default.
     * @param wireInstance Reference to the TwoWire instance (e.g., Wire, Wire1) to use.
     * @return 0 on success.
     */
    int8_t open(uint32_t arg, TwoWire& wireInstance = Wire)
    {
        if (arg==PIXY_DEFAULT_ARGVAL)
            m_addr = PIXY_I2C_DEFAULT_ADDR;
        else
            m_addr = (uint8_t)arg; // Store address

        m_wire = &wireInstance; // Store the reference to the I2C bus instance
        m_wire->begin();        // Ensure the bus is started
        return 0;
    }

    /**
     * @brief Closes the I2C link (no specific action needed for Wire library).
     */
    void close()
    {
        // Wire library generally doesn't require an explicit close
    }

    /**
     * @brief Receives data over I2C with error handling.
     * Reads 'len' bytes into 'buf'. Calculates checksum if 'cs' pointer is provided.
     * Handles cases where the I2C peripheral does not respond or returns fewer bytes than expected.
     *
     * @param buf Pointer to the buffer where received data will be stored.
     * @param len Number of bytes to attempt to receive.
     * @param cs Pointer to a variable to store the calculated checksum (optional).
     * @return Number of bytes actually received, or a negative value (PIXY_RESULT_ERROR) on I2C communication failure.
     */
    int16_t recv(uint8_t *buf, uint8_t len, uint16_t *cs = NULL)
    {
        if (len == 0) return 0; // Nothing to receive

        uint8_t bytes_received_total = 0;
        uint8_t bytes_requested_this_pass = 0;
        uint8_t bytes_received_this_pass = 0;
        const uint8_t MAX_I2C_CHUNK = 32; // Wire library buffer limit

        if (cs) *cs = 0; // Initialize checksum if requested

        while (bytes_received_total < len)
        {
            // Determine how many bytes to request in this chunk
            bytes_requested_this_pass = min((uint8_t)(len - bytes_received_total), MAX_I2C_CHUNK);

            // Request bytes from the I2C device
            bytes_received_this_pass = m_wire->requestFrom((uint8_t)m_addr, bytes_requested_this_pass);

            // Check for I2C communication error (NACK, timeout, etc.)
            if (bytes_received_this_pass == 0)
            {
                // If we requested bytes but received none, it's an error.
                // Return error if nothing was read at all, otherwise return partial count.
                return (bytes_received_total > 0) ? bytes_received_total : PIXY_RESULT_ERROR;
            }

            // Read the received bytes from the Wire buffer
            for (uint8_t j = 0; j < bytes_received_this_pass; j++)
            {
                if (m_wire->available()) // Ensure data is actually available
                {
                    buf[bytes_received_total + j] = m_wire->read();
                    if (cs) *cs += buf[bytes_received_total + j]; // Update checksum
                }
                else
                {
                    // Unexpected end of data in buffer, treat as error
                    return (bytes_received_total > 0) ? bytes_received_total : PIXY_RESULT_ERROR;
                }
            }
            bytes_received_total += bytes_received_this_pass;
        }
        // Return the total number of bytes successfully read
        return bytes_received_total;
    }

    /**
     * @brief Sends data over I2C.
     * Sends 'len' bytes from 'buf'. Handles sending in chunks if needed.
     *
     * @param buf Pointer to the buffer containing data to send.
     * @param len Number of bytes to send.
     * @return Number of bytes sent, or a negative value (PIXY_RESULT_ERROR) on I2C transmission error.
     */
    int16_t send(uint8_t *buf, uint8_t len)
    {
        uint8_t i, packet_len;
        for (i = 0; i < len; i += PIXY_I2C_MAX_SEND)
        {
            // Determine size of the current chunk to send
            packet_len = min((uint8_t)(len - i), (uint8_t)PIXY_I2C_MAX_SEND);

            m_wire->beginTransmission(m_addr);
            size_t bytes_written = m_wire->write(buf + i, packet_len);

            // Check if write succeeded locally (buffer space)
            if (bytes_written != packet_len) {
                m_wire->endTransmission(); // Attempt to end transmission even on local write failure
                return PIXY_RESULT_ERROR;  // Local buffer issue likely
            }

            // End transmission and check for I2C errors (like NACK from slave)
            uint8_t error = m_wire->endTransmission();
            if (error != 0)
            {
                // I2C transmission error (NACK etc.)
                // Error codes: 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
                return PIXY_RESULT_ERROR;
            }
        }
        // Return total bytes sent if all transmissions were successful
        return len;
    }

private:
    uint8_t m_addr;     // I2C Address of the Pixy2
    TwoWire* m_wire;    // Pointer to the Wire instance (e.g., &Wire, &Wire1)
};


// Define Pixy2I2C as TPixy2 using the Link2I2C transport
typedef TPixy2<Link2I2C> Pixy2I2C;


#endif // _PIXY2I2C_H
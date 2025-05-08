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
// Main Pixy2 template class.
// Handles packet-based communication over a specified LinkType (e.g., Link2I2C).
// Includes modifications for robust error handling in packet reception.

#ifndef _TPIXY2_H
#define _TPIXY2_H

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Wire.h> // Include Wire for TwoWire type used in init() default

// Enable detailed debug prints to Serial (if uncommented)
// #define PIXY_DEBUG

// --- Constants ---
#define PIXY_DEFAULT_ARGVAL                  0x80000000 // Default argument value for init()
#define PIXY_BUFFERSIZE                      0x104      // Pixy general purpose buffer size (260 bytes)
#define PIXY_CHECKSUM_SYNC                   0xc1af     // Sync bytes for packets with checksum
#define PIXY_NO_CHECKSUM_SYNC                0xc1ae     // Sync bytes for packets without checksum
#define PIXY_SEND_HEADER_SIZE                4          // Header size for sent packets
#define PIXY_MAX_PROGNAME                    33         // Max length for program name in changeProg

// --- Pixy Request/Response Types ---
#define PIXY_TYPE_REQUEST_CHANGE_PROG        0x02
#define PIXY_TYPE_REQUEST_RESOLUTION         0x0c
#define PIXY_TYPE_RESPONSE_RESOLUTION        0x0d
#define PIXY_TYPE_REQUEST_VERSION            0x0e
#define PIXY_TYPE_RESPONSE_VERSION           0x0f
#define PIXY_TYPE_RESPONSE_RESULT            0x01
#define PIXY_TYPE_RESPONSE_ERROR             0x03
#define PIXY_TYPE_REQUEST_BRIGHTNESS         0x10
#define PIXY_TYPE_REQUEST_SERVO              0x12
#define PIXY_TYPE_REQUEST_LED                0x14
#define PIXY_TYPE_REQUEST_LAMP               0x16
#define PIXY_TYPE_REQUEST_FPS                0x18

// --- Pixy Result Codes ---
#define PIXY_RESULT_OK                       0  // Success
#define PIXY_RESULT_ERROR                    -1 // Generic error
#define PIXY_RESULT_BUSY                     -2 // Pixy is busy (e.g., processing)
#define PIXY_RESULT_CHECKSUM_ERROR           -3 // Checksum mismatch on received packet
#define PIXY_RESULT_TIMEOUT                  -4 // Communication timeout
#define PIXY_RESULT_BUTTON_OVERRIDE          -5 // Pixy button override active
#define PIXY_RESULT_PROG_CHANGING            -6 // Pixy program is changing

// --- RC-Servo Values ---
#define PIXY_RCS_MIN_POS                     0
#define PIXY_RCS_MAX_POS                     1000L
#define PIXY_RCS_CENTER_POS                  ((PIXY_RCS_MAX_POS - PIXY_RCS_MIN_POS) / 2)

// --- Include Sub-API Headers ---
#include "Pixy2CCC.h"   // Color Connected Components API
#include "Pixy2Line.h"  // Line Tracking API
#include "Pixy2Video.h" // Video API

/**
 * @brief Structure to hold Pixy version information.
 */
struct Version
{
    uint16_t hardware;      // Hardware version
    uint8_t firmwareMajor;  // Firmware major version
    uint8_t firmwareMinor;  // Firmware minor version
    uint16_t firmwareBuild; // Firmware build number
    char firmwareType[10];  // Firmware type string

    /**
     * @brief Prints version information to the default Serial port.
     */
    void print()
    {
        char buf[80]; // Increased buffer size slightly
        sprintf(buf, "Pixy HW: 0x%X, FW: %d.%d.%d (%s)",
                hardware, firmwareMajor, firmwareMinor, firmwareBuild, firmwareType);
        Serial.println(buf);
    }
};


/**
 * @brief Main template class for Pixy2 communication.
 * @tparam LinkType The communication transport class (e.g., Link2I2C, Link2SPI).
 */
template <class LinkType> class TPixy2
{
public:
    /**
     * @brief Constructor. Allocates communication buffers.
     */
    TPixy2();

    /**
     * @brief Destructor. Closes communication link and frees buffers.
     */
    ~TPixy2();

    /**
     * @brief Initializes the communication link and pings Pixy to ensure it's ready.
     * @param arg Communication link specific argument (e.g., I2C address, SPI SS pin).
     * @param wireInstance Reference to the TwoWire instance (for I2C links only).
     * @return PIXY_RESULT_OK on success, or a negative error code on failure.
     */
    int8_t init(uint32_t arg = PIXY_DEFAULT_ARGVAL, TwoWire& wireInstance = Wire);

    /**
     * @brief Retrieves version information from Pixy.
     * @return Number of bytes in the version structure on success, or a negative error code.
     *         The 'version' member variable will point to the data.
     */
    int8_t getVersion();

    /**
     * @brief Requests Pixy to change its running program.
     * @param prog The name of the program to run (e.g., "line").
     * @return PIXY_RESULT_OK on success, or a negative error code.
     */
    int8_t changeProg(const char *prog);

    /**
     * @brief Sets the position of Pixy's RC servos.
     * @param s0 Position of servo 0 (0-1000).
     * @param s1 Position of servo 1 (0-1000).
     * @return Pixy result code (usually PIXY_RESULT_OK on success).
     */
    int8_t setServos(uint16_t s0, uint16_t s1);

    /**
     * @brief Sets the camera brightness.
     * @param brightness Brightness value (0-255).
     * @return Pixy result code.
     */
    int8_t setCameraBrightness(uint8_t brightness);

    /**
     * @brief Sets the color of Pixy's RGB LED.
     * @param r Red intensity (0-255).
     * @param g Green intensity (0-255).
     * @param b Blue intensity (0-255).
     * @return Pixy result code.
     */
    int8_t setLED(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Controls Pixy's onboard lamps.
     * @param upper Controls the two white LEDs on top (0=off, 1=on).
     * @param lower Controls the RGB LED (0=off, 1=on - overrides setLED).
     * @return Pixy result code.
     */
    int8_t setLamp(uint8_t upper, uint8_t lower);

    /**
     * @brief Gets the current frame resolution used by Pixy's algorithms.
     * @return PIXY_RESULT_OK on success, or a negative error code.
     *         Updates frameWidth and frameHeight members.
     */
    int8_t getResolution();

    /**
     * @brief Gets the current frames-per-second reported by Pixy.
     * @return FPS value on success, or a negative error code.
     */
    int8_t getFPS();

    // --- Public Member Variables ---
    Version *version;       // Pointer to received version information (valid after successful getVersion).
    uint16_t frameWidth;    // Current frame width (valid after successful getResolution).
    uint16_t frameHeight;   // Current frame height (valid after successful getResolution).

    // --- Sub-APIs ---
    Pixy2CCC<LinkType> ccc;    // Access Color Connected Components API.
    Pixy2Line<LinkType> line;  // Access Line Tracking API.
    Pixy2Video<LinkType> video;// Access Video API.

    // Allow friend classes (sub-APIs) access to private members
    friend class Pixy2CCC<LinkType>;
    friend class Pixy2Line<LinkType>;
    friend class Pixy2Video<LinkType>;

    LinkType m_link; // The communication link object (e.g., Link2I2C instance)

// Make methods public for simpler interaction if direct calls are needed, otherwise keep private.
// private:
    /**
     * @brief Searches the communication link for Pixy's sync sequence (0xc1af or 0xc1ae).
     * Handles timeouts and communication errors robustly.
     * @return PIXY_RESULT_OK if sync found, negative error code otherwise (e.g., TIMEOUT, ERROR).
     */
    int16_t getSync();

    /**
     * @brief Receives a complete packet from Pixy (header + payload).
     * Handles sync searching, header parsing, payload reception, and checksum validation.
     * Uses the modified getSync() and robust link recv() methods.
     * @return PIXY_RESULT_OK on success, negative error code on failure
     *         (CHECKSUM_ERROR, TIMEOUT, ERROR, etc.).
     */
    int16_t recvPacket();

    /**
     * @brief Sends a packet to Pixy (header + payload).
     * Constructs the header and sends the data via the communication link.
     * @return Number of bytes sent on success, negative error code (PIXY_RESULT_ERROR) on failure.
     */
    int16_t sendPacket();

    // --- Private Member Variables ---
    uint8_t *m_buf;         // Pointer to the communication buffer.
    uint8_t *m_bufPayload;  // Pointer to the payload section within m_buf.
    uint8_t m_type;         // Type of the last received/sent packet.
    uint8_t m_length;       // Length of the payload in the last received/sent packet.
    bool m_cs;              // Flag indicating if checksum was expected/used for the last received packet.
};


// --- Template Method Implementations ---

template <class LinkType>
TPixy2<LinkType>::TPixy2() : ccc(this), line(this), video(this)
{
    m_buf = (uint8_t *)malloc(PIXY_BUFFERSIZE);
    if (m_buf == nullptr) {
        // Handle allocation failure if necessary
        // Serial.println("FATAL: TPixy2 buffer allocation failed!");
    }
    m_bufPayload = m_buf + PIXY_SEND_HEADER_SIZE;
    frameWidth = frameHeight = 0;
    version = NULL;
}

template <class LinkType>
TPixy2<LinkType>::~TPixy2()
{
    m_link.close();
    free(m_buf);
}

// MODIFIED init METHOD for I2C
template <class LinkType>
int8_t TPixy2<LinkType>::init(uint32_t arg, TwoWire& wireInstance)
{
    uint32_t t0;
    int8_t res;

    // Pass the wire instance to the link's open method
    res = m_link.open(arg, wireInstance);
    if (res < 0)
        return res; // Link opening failed

    // Ping Pixy with getVersion until it responds or timeout occurs
    for(t0 = millis(); millis() - t0 < 7000; ) // Increased timeout
    {
        if (getVersion() >= 0) // Check for non-negative success code
        {
            getResolution(); // Get initial resolution
            return PIXY_RESULT_OK; // Initialization successful
        }
        delayMicroseconds(10000); // Wait longer between pings
    }
    return PIXY_RESULT_TIMEOUT; // Pixy did not respond within timeout
}


// MODIFIED getSync METHOD - Robust timeout and error handling
template <class LinkType>
int16_t TPixy2<LinkType>::getSync()
{
    uint8_t i, sync_attempts, c = 0, cprev = 0;
    int16_t res;
    uint16_t start_word;
    bool byte_received;

    // Loop attempting to find sync pattern with timeout logic
    for (sync_attempts = 0; sync_attempts < 5; sync_attempts++) // Try 5 times
    {
        for (i = 0; i < 4; i++) // Try reading up to 4 bytes per attempt round
        {
            byte_received = false;
            res = m_link.recv(&c, 1); // Attempt to receive one byte

            if (res == 1) // Successfully received 1 byte
            {
                byte_received = true;
                start_word = cprev;          // Previous byte is LSB
                start_word |= (uint16_t)c << 8; // Current byte is MSB
                cprev = c;                     // Store current byte for next iteration

                // Check for valid sync words
                if (start_word == PIXY_CHECKSUM_SYNC) {
                    m_cs = true; // Checksum expected
                    return PIXY_RESULT_OK;
                }
                if (start_word == PIXY_NO_CHECKSUM_SYNC) {
                    m_cs = false; // No checksum expected
                    return PIXY_RESULT_OK;
                }
            }
            else if (res < 0) // Link returned a communication error
            {
                // Propagate the error immediately
                return res; // e.g., PIXY_RESULT_ERROR from Link2I2C
            }
            // else: res == 0 (timeout or no data from link), continue loop

             // Only increment attempt counter if we actually processed a potential byte
            if(byte_received) {
                // Reset inner loop counter 'i' ? No, we want to count total attempts.
            } else {
                 // Add small delay if no byte received to avoid thrashing the bus
                 delayMicroseconds(50);
            }
        } // End inner loop (reading up to 4 bytes)

        // If no sync found after reading a few bytes, wait before next round
        delayMicroseconds(5000); // Wait 5ms between major sync attempt rounds
    } // End outer loop (sync attempts)

    // If loops complete without finding sync, timeout occurred
    #ifdef PIXY_DEBUG
    // Serial.println("getSync Error: no sync pattern found (Timeout)");
    #endif
    return PIXY_RESULT_TIMEOUT;
}


// MODIFIED recvPacket METHOD - Handles errors from getSync and link recv more robustly
template <class LinkType>
int16_t TPixy2<LinkType>::recvPacket()
{
    uint16_t calculated_checksum = 0, received_checksum = 0;
    int16_t res;

    // 1. Find Sync Pattern (handles timeouts and link errors internally)
    res = getSync();
    if (res < 0) return res; // Propagate error (Timeout, Link Error)

    // 2. Receive Header
    uint8_t headerLen = m_cs ? 4 : 2; // Header is 4 bytes with checksum, 2 otherwise
    res = m_link.recv(m_buf, headerLen);
    if (res != headerLen)
    {
        // Error receiving header (link error or short read)
        return PIXY_RESULT_ERROR;
    }

    // 3. Parse Header
    m_type = m_buf[0];
    m_length = m_buf[1]; // Payload length

    // Sanity check payload length against buffer size
    if (m_length > (PIXY_BUFFERSIZE - PIXY_SEND_HEADER_SIZE)) {
         #ifdef PIXY_DEBUG
         Serial.println("recvPacket Error: Declared packet length too large!");
         #endif
         return PIXY_RESULT_ERROR; // Avoid buffer overflow
    }

    // 4. Handle Checksum / Receive Payload
    if (m_cs) // Checksum expected
    {
        received_checksum = *(uint16_t *)&m_buf[2]; // Extract checksum from header

        // Receive payload if length > 0
        if (m_length > 0)
        {
            res = m_link.recv(m_buf, m_length, &calculated_checksum); // Receive payload and calculate its checksum
            if (res != m_length)
            {
                // Error receiving payload (link error or short read)
                return PIXY_RESULT_ERROR;
            }
        }
        else // No payload
        {
             calculated_checksum = 0; // Checksum of empty payload is 0
             res = 0; // Indicate 0 bytes read for payload
        }

        // Verify checksum
        if (received_checksum != calculated_checksum)
        {
            #ifdef PIXY_DEBUG
            // Serial.print("recvPacket: Checksum error. Expected: "); Serial.print(received_checksum);
            // Serial.print(" Calculated: "); Serial.println(calculated_checksum);
            #endif
            return PIXY_RESULT_CHECKSUM_ERROR; // Return specific checksum error
        }
    }
    else // No checksum expected
    {
        // Receive payload if length > 0
        if (m_length > 0)
        {
            res = m_link.recv(m_buf, m_length); // Receive payload without checksum calculation
             if (res != m_length)
             {
                // Error receiving payload (link error or short read)
                return PIXY_RESULT_ERROR;
            }
        }
        else // No payload
        {
             res = 0; // Indicate 0 bytes read for payload
        }
    }

    // If all steps succeeded
    return PIXY_RESULT_OK;
}


// MODIFIED sendPacket METHOD - Returns PIXY_RESULT_ERROR on failure
template <class LinkType>
int16_t TPixy2<LinkType>::sendPacket()
{
    // Construct header in the beginning of the buffer
    m_buf[0] = PIXY_NO_CHECKSUM_SYNC & 0xff; // Use sync indicating no checksum from host
    m_buf[1] = PIXY_NO_CHECKSUM_SYNC >> 8;
    m_buf[2] = m_type;   // Packet type
    m_buf[3] = m_length; // Payload length

    // Send the entire packet (header + payload)
    // Payload data should already be in m_bufPayload (which is m_buf + 4)
    int16_t bytes_to_send = m_length + PIXY_SEND_HEADER_SIZE;
    int16_t sendResult = m_link.send(m_buf, bytes_to_send);

    // Check if the link reported sending the correct number of bytes
    if (sendResult != bytes_to_send)
    {
         return PIXY_RESULT_ERROR; // Indicate send failure
    }
    // Return number of bytes sent on success (consistent with original library, though OK might be better)
    return sendResult;
}


// --- Other High-Level Methods (getVersion, changeProg, setServos, etc.) ---
// These methods use sendPacket() and recvPacket(). The robustness improvements
// in recvPacket() should make these methods return errors more reliably instead
// of potentially hanging or returning incorrect success codes.
// The implementation logic within these methods remains largely the same,
// focusing on checking the return value of recvPacket() appropriately.

template <class LinkType>
int8_t TPixy2<LinkType>::changeProg(const char *prog)
{
    uint32_t pixy_result_code;
    int16_t packet_status;

    // Loop to request program change and wait for confirmation
    while(1)
    {
        // Prepare request payload
        strncpy((char *)m_bufPayload, prog, PIXY_MAX_PROGNAME);
        // Ensure null termination if prog is exactly max length
        m_bufPayload[PIXY_MAX_PROGNAME - 1] = '\0';
        m_length = strlen((char*)m_bufPayload) + 1; // Include null terminator
        m_type = PIXY_TYPE_REQUEST_CHANGE_PROG;

        // Send request
        packet_status = sendPacket();
        if (packet_status < 0) return PIXY_RESULT_ERROR; // Send failed

        // Wait for response
        packet_status = recvPacket();

        if (packet_status == PIXY_RESULT_OK)
        {
            // Check response type and payload
            if (m_type == PIXY_TYPE_RESPONSE_RESULT && m_length == 4)
            {
                pixy_result_code = *(uint32_t *)m_buf;
                if (pixy_result_code == PIXY_RESULT_OK)
                {
                    getResolution(); // Update resolution after program change
                    return PIXY_RESULT_OK; // Success
                }
                else
                {
                    // Pixy reported an error changing program
                    return (int8_t)pixy_result_code; // Return Pixy's error code
                }
            }
            else if (m_type == PIXY_TYPE_RESPONSE_ERROR && m_length > 0)
            {
                // Check if error is "program changing"
                if ((int8_t)m_buf[0] == PIXY_RESULT_PROG_CHANGING) {
                     delayMicroseconds(5000); // Wait longer if program is still changing
                     continue; // Retry
                } else {
                     return (int8_t)m_buf[0]; // Return other error code
                }
            } else {
                 // Unexpected response type or length
                 return PIXY_RESULT_ERROR;
            }
        }
        else if (packet_status == PIXY_RESULT_CHECKSUM_ERROR)
        {
             // Optional: Retry on checksum error? For now, return error.
             return PIXY_RESULT_CHECKSUM_ERROR;
        }
        else if (packet_status == PIXY_RESULT_BUSY || packet_status == PIXY_RESULT_PROG_CHANGING)
        {
             delayMicroseconds(5000); // Wait and retry if busy or still changing
             continue;
        }
        else // Other errors from recvPacket (TIMEOUT, ERROR)
        {
             return (int8_t)packet_status; // Propagate error
        }
    } // end while(1)
}


template <class LinkType>
int8_t TPixy2<LinkType>::getVersion()
{
    int16_t packet_status;
    m_length = 0; // No arguments for getVersion request
    m_type = PIXY_TYPE_REQUEST_VERSION;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK)
    {
        if (m_type == PIXY_TYPE_RESPONSE_VERSION && m_length >= sizeof(Version) - sizeof(version->firmwareType) + 1 ) // Basic check
        {
            version = (Version *)m_buf;
            // Ensure firmwareType string is null-terminated within buffer bounds
            version->firmwareType[sizeof(version->firmwareType) - 1] = '\0';
            return PIXY_RESULT_OK; // Indicate success, caller accesses 'version' member
        }
        else if (m_type == PIXY_TYPE_RESPONSE_ERROR && m_length > 0)
        {
            return (int8_t)m_buf[0]; // Return Pixy's error code (e.g., BUSY)
        }
        else
        {
            return PIXY_RESULT_ERROR; // Unexpected response type or length
        }
    }
    // Propagate error code from recvPacket (CHECKSUM, TIMEOUT, ERROR)
    return (int8_t)packet_status;
}


template <class LinkType>
int8_t TPixy2<LinkType>::getResolution()
{
    int16_t packet_status;
    m_length = 1;
    m_bufPayload[0] = 0; // Reserved argument
    m_type = PIXY_TYPE_REQUEST_RESOLUTION;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK)
    {
        if (m_type == PIXY_TYPE_RESPONSE_RESOLUTION && m_length == 4) // Expect uint16_t width + uint16_t height
        {
            frameWidth = *(uint16_t *)m_buf;
            frameHeight = *(uint16_t *)(m_buf + sizeof(uint16_t));
            return PIXY_RESULT_OK; // Success
        }
        else
        {
            return PIXY_RESULT_ERROR; // Unexpected type or length
        }
    }
    else
        return (int8_t)packet_status;  // Propagate error
}


template <class LinkType>
int8_t TPixy2<LinkType>::setCameraBrightness(uint8_t brightness)
{
    uint32_t pixy_result_code;
    int16_t packet_status;

    m_bufPayload[0] = brightness;
    m_length = 1;
    m_type = PIXY_TYPE_REQUEST_BRIGHTNESS;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK)
    {
        if (m_type == PIXY_TYPE_RESPONSE_RESULT && m_length == 4) {
            pixy_result_code = *(uint32_t *)m_buf;
            return (int8_t)pixy_result_code; // Return the result code from Pixy
        } else {
            return PIXY_RESULT_ERROR; // Unexpected response
        }
    }
    else
        return (int8_t)packet_status; // Propagate error
}


template <class LinkType>
int8_t TPixy2<LinkType>::setServos(uint16_t s0, uint16_t s1)
{
    uint32_t pixy_result_code;
    int16_t packet_status;

    // Ensure values are within Pixy's range
    s0 = constrain(s0, PIXY_RCS_MIN_POS, PIXY_RCS_MAX_POS);
    s1 = constrain(s1, PIXY_RCS_MIN_POS, PIXY_RCS_MAX_POS);

    *(uint16_t *)(m_bufPayload + 0) = s0; // Use uint16_t directly
    *(uint16_t *)(m_bufPayload + 2) = s1;
    m_length = 4;
    m_type = PIXY_TYPE_REQUEST_SERVO;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK) {
        if (m_type == PIXY_TYPE_RESPONSE_RESULT && m_length == 4) {
            pixy_result_code = *(uint32_t *)m_buf;
            return (int8_t)pixy_result_code; // Return Pixy's result code
        } else {
            return PIXY_RESULT_ERROR; // Unexpected response
        }
    } else {
        return (int8_t)packet_status; // Propagate error
    }
}


template <class LinkType>
int8_t TPixy2<LinkType>::setLED(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t pixy_result_code;
    int16_t packet_status;

    m_bufPayload[0] = r;
    m_bufPayload[1] = g;
    m_bufPayload[2] = b;
    m_length = 3;
    m_type = PIXY_TYPE_REQUEST_LED;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK) {
        if (m_type == PIXY_TYPE_RESPONSE_RESULT && m_length == 4) {
            pixy_result_code = *(uint32_t *)m_buf;
            return (int8_t)pixy_result_code; // Return Pixy's result code
        } else {
            return PIXY_RESULT_ERROR; // Unexpected response
        }
    } else {
        return (int8_t)packet_status; // Propagate error
    }
}

template <class LinkType>
int8_t TPixy2<LinkType>::setLamp(uint8_t upper, uint8_t lower)
{
    uint32_t pixy_result_code;
    int16_t packet_status;

    m_bufPayload[0] = upper & 0x01; // Ensure upper is 0 or 1
    m_bufPayload[1] = lower & 0x01; // Ensure lower is 0 or 1
    m_length = 2;
    m_type = PIXY_TYPE_REQUEST_LAMP;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK) {
        if (m_type == PIXY_TYPE_RESPONSE_RESULT && m_length == 4) {
            pixy_result_code = *(uint32_t *)m_buf;
            return (int8_t)pixy_result_code; // Return Pixy's result code
        } else {
            return PIXY_RESULT_ERROR; // Unexpected response
        }
    } else {
        return (int8_t)packet_status; // Propagate error
    }
}

template <class LinkType>
int8_t TPixy2<LinkType>::getFPS()
{
    uint32_t pixy_result_code; // FPS is returned in the result payload
    int16_t packet_status;

    m_length = 0; // no args
    m_type = PIXY_TYPE_REQUEST_FPS;

    packet_status = sendPacket();
    if (packet_status < 0) return PIXY_RESULT_ERROR;

    packet_status = recvPacket();

    if (packet_status == PIXY_RESULT_OK) {
        if (m_type == PIXY_TYPE_RESPONSE_RESULT && m_length == 4) {
            pixy_result_code = *(uint32_t *)m_buf;
            // FPS is the value in the result code for this request
            return (int8_t)pixy_result_code; // Cast might truncate for high FPS, but usually okay
        } else {
            return PIXY_RESULT_ERROR; // Unexpected response
        }
    } else {
        return (int8_t)packet_status; // Propagate error
    }
}


#endif // _TPIXY2_H
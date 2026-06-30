/*****************************************************************
 * POV FAN - TEENSY 4.1
 * Unity -> USB -> Teensy -> UART -> ESP32
 *****************************************************************/

#include <Arduino.h>

#define USB_BAUD        3000000
#define UART_BAUD       3000000


#define START1          0xAA
#define START2          0x55
#define END_BYTE        0xFE

#define MAX_PAYLOAD     25920

// UART to ESP32
#define ESP_SERIAL      Serial1

// USB from Unity
#define PC_SERIAL       Serial

// Frame Buffer
uint8_t frameBuffer[MAX_PAYLOAD];

// Packet Variables
uint16_t payloadLength = 0;
uint16_t frameID = 0;
uint16_t receivedCRC = 0;
uint16_t calculatedCRC = 0;

bool frameReady = false;

// Receiver States
enum RX_STATE
{
    WAIT_START1,
    WAIT_START2,
    READ_FRAMEID,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_CRC,
    READ_END
};

RX_STATE state = WAIT_START1;

uint16_t payloadIndex = 0;
uint8_t crcBytes[2];
uint8_t frameBytes[2];
uint8_t lengthBytes[2];
/*****************************************************************
 * CRC16 MODBUS
 *****************************************************************/
uint16_t calculateCRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    while (length--)
    {
        crc ^= *data++;

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}

/*****************************************************************
 * SETUP
 *****************************************************************/
void setup()
{
    PC_SERIAL.begin(USB_BAUD);

    ESP_SERIAL.begin(UART_BAUD);

    while (!PC_SERIAL)
    {
        ;
    }

    Serial.println("--------------------------------");
    Serial.println("POV FAN TEENSY STARTED");
    Serial.println("--------------------------------");
}
/*****************************************************************
 * LOOP
 *****************************************************************/
/*****************************************************************
 * LOOP
 *****************************************************************/
void loop()
{
    while (PC_SERIAL.available())
    {
        uint8_t data = PC_SERIAL.read();

        switch (state)
        {

        //-------------------------------------------------------
        case WAIT_START1:

            if (data == START1)
                state = WAIT_START2;

            break;

        //-------------------------------------------------------
        case WAIT_START2:

            if (data == START2)
            {
                payloadIndex = 0;
                state = READ_FRAMEID;
            }
            else
            {
                state = WAIT_START1;
            }

            break;

        //-------------------------------------------------------
        case READ_FRAMEID:

            frameBytes[payloadIndex++] = data;

            if (payloadIndex >= 2)
            {
                frameID = frameBytes[0] | (frameBytes[1] << 8);

                payloadIndex = 0;

                state = READ_LENGTH;
            }

            break;

        //-------------------------------------------------------
        case READ_LENGTH:

            lengthBytes[payloadIndex++] = data;

            if (payloadIndex >= 2)
            {
                payloadLength = lengthBytes[0] | (lengthBytes[1] << 8);

                if (payloadLength > MAX_PAYLOAD)
                {
                    state = WAIT_START1;
                    break;
                }

                payloadIndex = 0;

                state = READ_PAYLOAD;
            }

            break;

        //-------------------------------------------------------
        case READ_PAYLOAD:

            frameBuffer[payloadIndex++] = data;

            if (payloadIndex >= payloadLength)
            {
                payloadIndex = 0;

                state = READ_CRC;
            }

            break;

        //-------------------------------------------------------
        case READ_CRC:

            crcBytes[payloadIndex++] = data;

            if (payloadIndex >= 2)
            {
                receivedCRC = crcBytes[0] | (crcBytes[1] << 8);

                calculatedCRC =
                    calculateCRC16(frameBuffer, payloadLength);

                payloadIndex = 0;

                state = READ_END;
            }

            break;

        //-------------------------------------------------------
        case READ_END:

            if (data == END_BYTE)
            {
                if (receivedCRC == calculatedCRC)
                {
                    frameReady = true;
                }
            }

            state = WAIT_START1;

            break;
        }
    }

    //-------------------------------------------------------
    // Forward packet to ESP32
    //-------------------------------------------------------

    if (frameReady)
    {
        frameReady = false;

        ESP_SERIAL.write((uint8_t)START1);
        ESP_SERIAL.write((uint8_t)START2);

        ESP_SERIAL.write((uint8_t *)&frameID, 2);

        ESP_SERIAL.write((uint8_t *)&payloadLength, 2);

        ESP_SERIAL.write(frameBuffer, payloadLength);

        ESP_SERIAL.write((uint8_t *)&receivedCRC, 2);

        ESP_SERIAL.write((uint8_t)END_BYTE);

        ESP_SERIAL.flush();
    }
}
/*****************************************************************
 * POV FAN DISPLAY - FULLY FIXED
 * ESP32-C3 (Seeed XIAO)
 * Receives Frame From Teensy via 3Mbps RS485
 * Drives 4 WS2812 Strips via Parallel Hardware RMT
 * Reads MT6835 SPI Magnetic Encoder for Angle Tracking
 *****************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <FastLED.h>

#define LEDS_PER_BLADE 36
#define BLADES         4
#define TOTAL_LEDS     (LEDS_PER_BLADE * BLADES)
#define BRIGHTNESS     150
#define NUM_SLICES     360 // 360 angular slices per rotation

//------------------------------------------------------
// LED Pins
//------------------------------------------------------
#define BLADE1_PIN 2
#define BLADE2_PIN 3
#define BLADE3_PIN 4
#define BLADE4_PIN 5

//------------------------------------------------------
// UART (RS485) Pins
//------------------------------------------------------
#define RX_PIN    6
#define TX_PIN    7
#define DE_RE_PIN 8

HardwareSerial RS485Serial(1);

//------------------------------------------------------
// MT6835 SPI Pins
//------------------------------------------------------
#define MT_CS   10
#define MT_SCK   9
#define MT_MOSI 11
#define MT_MISO 12

//------------------------------------------------------
// FastLED Buffers
//------------------------------------------------------
CRGB leds1[LEDS_PER_BLADE];
CRGB leds2[LEDS_PER_BLADE];
CRGB leds3[LEDS_PER_BLADE];
CRGB leds4[LEDS_PER_BLADE];

//------------------------------------------------------
// Frame Buffer (360 slices * 36 LEDs * 2 bytes = 25920 bytes)
//------------------------------------------------------
uint8_t frameBuffer[25920];
uint16_t payloadLength = 0;
uint16_t frameID = 0;
volatile bool frameReady = false;

/*****************************************************************
 * Packet Constants
 *****************************************************************/
#define START1      0xAA
#define START2      0x55
#define END_BYTE    0xFE
#define MAX_PAYLOAD 25920

enum RX_STATE {
    WAIT_START1,
    WAIT_START2,
    READ_FRAMEID,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_CRC,
    READ_END
};

RX_STATE state = WAIT_START1;

uint8_t frameBytes[2];
uint8_t lengthBytes[2];
uint8_t crcBytes[2];

uint16_t payloadIndex = 0;
uint16_t receivedCRC = 0;
uint16_t calculatedCRC = 0;

/*****************************************************************
 * CRC16 Calculation
 *****************************************************************/
uint16_t calculateCRC16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    while (length--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/*****************************************************************
 * Read MT6835 Encoder Angle and Convert to Slice (0-359)
 *****************************************************************/
uint16_t getEncoderSlice() {
    // Begin SPI Transaction for MT6835 (Adjust clock speed if necessary)
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE3));
    digitalWrite(MT_CS, LOW);

    // MT6835 21-bit angle read protocol (Example reading registers 0x03, 0x04, 0x05)
    // Transmit Read Command for Angle Register 0x0003
    SPI.transfer(0x30); // Read operation flag + Upper address byte
    SPI.transfer(0x03); // Lower address byte
    
    uint8_t highByte = SPI.transfer(0x00);
    uint8_t midByte  = SPI.transfer(0x00);
    uint8_t lowByte  = SPI.transfer(0x00);

    digitalWrite(MT_CS, HIGH);
    SPI.endTransaction();

    // Reconstruct the raw 21-bit value
    uint32_t rawAngle = ((uint32_t)highByte << 13) | ((uint32_t)midByte << 5) | (lowByte >> 3);
    
    // Convert 21-bit value ($2^{21} = 2097152$) down to a 0-359 slice range
    uint16_t slice = (rawAngle * NUM_SLICES) / 2097152;
    
    return slice % NUM_SLICES;
}

/*****************************************************************
 * Dynamic Slice Rendering Matrix
 *****************************************************************/
void getPixelColorFromBuffer(uint16_t slice, int led, uint8_t &r, uint8_t &g, uint8_t &b) {
    // 36 LEDs * 2 bytes = 72 bytes per individual slice array mapping
    int sliceStartIndex = slice * (LEDS_PER_BLADE * 2); 
    int idx = sliceStartIndex + (led * 2);

    // Bounds check to secure against hard memory faults
    if (idx + 1 >= MAX_PAYLOAD) {
        r = g = b = 0;
        return;
    }

    uint16_t rgb565 = (frameBuffer[idx] << 8) | frameBuffer[idx + 1];
    
    // Scale bit depth cleanly up to RGB888
    r = ((rgb565 >> 11) & 0x1F) << 3;
    g = ((rgb565 >> 5)  & 0x3F) << 2;
    b = (rgb565 & 0x1F) << 3;
}

void displayCurrentSlices(uint16_t targetSlice) {
    uint8_t r, g, b;

    // Process spatial geometry separation offsets for all 4 blades (90-degree steps)
    uint16_t sliceBlade1 = targetSlice;
    uint16_t sliceBlade2 = (targetSlice + 90) % NUM_SLICES;
    uint16_t sliceBlade3 = (targetSlice + 180) % NUM_SLICES;
    uint16_t sliceBlade4 = (targetSlice + 270) % NUM_SLICES;

    for (int led = 0; led < LEDS_PER_BLADE; led++) {
        // Blade 1
        getPixelColorFromBuffer(sliceBlade1, led, r, g, b);
        leds1[led] = CRGB(r, g, b);

        // Blade 2
        getPixelColorFromBuffer(sliceBlade2, led, r, g, b);
        leds2[led] = CRGB(r, g, b);

        // Blade 3
        getPixelColorFromBuffer(sliceBlade3, led, r, g, b);
        leds3[led] = CRGB(r, g, b);

        // Blade 4
        getPixelColorFromBuffer(sliceBlade4, led, r, g, b);
        leds4[led] = CRGB(r, g, b);
    }

    // Hardware non-blocking execution cycle via RMT controllers
    FastLED.show();
}

/*****************************************************************
 * Non-Blocking Serial State Machine Receiver
 *****************************************************************/
void receivePacket() {
    while (RS485Serial.available()) {
        uint8_t data = RS485Serial.read();

        switch (state) {
            case WAIT_START1:
                if (data == START1) state = WAIT_START2;
                break;

            case WAIT_START2:
                if (data == START2) {
                    payloadIndex = 0;
                    state = READ_FRAMEID;
                } else {
                    state = WAIT_START1;
                }
                break;

            case READ_FRAMEID:
                frameBytes[payloadIndex++] = data;
                if (payloadIndex >= 2) {
                    frameID = frameBytes[0] | (frameBytes[1] << 8);
                    payloadIndex = 0;
                    state = READ_LENGTH;
                }
                break;

            case READ_LENGTH:
                lengthBytes[payloadIndex++] = data;
                if (payloadIndex >= 2) {
                    payloadLength = lengthBytes[0] | (lengthBytes[1] << 8);
                    if (payloadLength > MAX_PAYLOAD) {
                        state = WAIT_START1;
                        break;
                    }
                    payloadIndex = 0;
                    state = READ_PAYLOAD;
                }
                break;

            case READ_PAYLOAD:
                frameBuffer[payloadIndex++] = data;
                if (payloadIndex >= payloadLength) {
                    payloadIndex = 0;
                    state = READ_CRC;
                }
                break;

            case READ_CRC:
                crcBytes[payloadIndex++] = data;
                if (payloadIndex >= 2) {
                    receivedCRC = crcBytes[0] | (crcBytes[1] << 8);
                    calculatedCRC = calculateCRC16(frameBuffer, payloadLength);
                    payloadIndex = 0;
                    state = READ_END;
                }
                break;

            case READ_END:
                if (data == END_BYTE) {
                    if (receivedCRC == calculatedCRC) {
                        frameReady = true;
                    }
                }
                state = WAIT_START1;
                break;
        }
    }
}

/*****************************************************************
 * Setup Configurations
 *****************************************************************/
void setup() {
    pinMode(DE_RE_PIN, OUTPUT);
    digitalWrite(DE_RE_PIN, LOW); // Lock transceiver down permanently into RX mode

    Serial.begin(115200);

    // Initialize high-speed hardware UART
    RS485Serial.begin(3000000, SERIAL_8N1, RX_PIN, TX_PIN);

    // Configure SPI Bus for MT6835 Magnetic Encoder
    pinMode(MT_CS, OUTPUT);
    digitalWrite(MT_CS, HIGH);
    SPI.begin(MT_SCK, MT_MISO, MT_MOSI, MT_CS);

    // Initialize parallel hardware execution channels using FastLED 
    FastLED.addLeds<WS2812B, BLADE1_PIN, GRB>(leds1, LEDS_PER_BLADE);
    FastLED.addLeds<WS2812B, BLADE2_PIN, GRB>(leds2, LEDS_PER_BLADE);
    FastLED.addLeds<WS2812B, BLADE3_PIN, GRB>(leds3, LEDS_PER_BLADE);
    FastLED.addLeds<WS2812B, BLADE4_PIN, GRB>(leds4, LEDS_PER_BLADE);
    
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();

    Serial.println("ESP32 Ready");
}

/*****************************************************************
 * Main Execution Loop
 *****************************************************************/
void loop() {
    // 1. Process continuous data packets streaming from the Teensy
    receivePacket();

    // 2. Query encoder position and refresh spatial orientation matrix
    uint16_t currentSlice = getEncoderSlice();
    displayCurrentSlices(currentSlice);

    // 3. Confirm packet delivery statistics safely via hardware monitor UART
    if (frameReady) {
        frameReady = false;
        Serial.print("Frame Sync OK -> ID: ");
        Serial.println(frameID);
    }
}
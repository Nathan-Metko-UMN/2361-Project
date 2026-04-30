// **********************************************************************************
// Driver definition for HopeRF RFM69W/RFM69HW/RFM69CW/RFM69HCW, Semtech SX1231/1231H
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************


// **********************************************************************************
// Converted to AVR environment by Zulkar Nayem
// **********************************************************************************


#include <xc.h>
#include <stdint.h>
#include "spi_lib.h"
#include "RFM69registers.h"
#include "RFM69.h"
#include "get_millis.h"

// Map the library's SPI function to the one we built in spi.c
#define spi_fast_shift spi_transfer 

volatile uint8_t DATALEN;
volatile uint8_t SENDERID;
volatile uint8_t TARGETID;                 // should match _address
volatile uint8_t PAYLOADLEN;
volatile uint8_t ACK_REQUESTED;
volatile uint8_t ACK_RECEIVED;             // should be polled immediately after sending a packet with ACK request
volatile int16_t RSSI;                     // most accurate RSSI during reception (closest to the reception)
volatile uint8_t mode = RF69_MODE_STANDBY; // should be protected?
volatile uint8_t inISR = 0; 
uint8_t isRFM69HW = 1;                     // if RFM69HW model matches high power enable possible
uint8_t address;                           // nodeID
uint8_t powerLevel = 31;
uint8_t promiscuousMode = 0;
unsigned long millis_current;
volatile uint8_t DATA[RF69_MAX_DATA_LEN+1]; 

void rfm69_init(uint16_t freqBand, uint8_t nodeID, uint8_t networkID)
{
    const uint8_t CONFIG[][2] =
    {
        /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
        /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 },
        /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_55555}, 
        /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_55555},
        /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_50000}, 
        /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_50000},
        /* 0x07 */ { REG_FRFMSB, (uint8_t) (freqBand==RF_315MHZ ? RF_FRFMSB_315 : (freqBand==RF_433MHZ ? RF_FRFMSB_433 : (freqBand==RF_868MHZ ? RF_FRFMSB_868 : RF_FRFMSB_915))) },
        /* 0x08 */ { REG_FRFMID, (uint8_t) (freqBand==RF_315MHZ ? RF_FRFMID_315 : (freqBand==RF_433MHZ ? RF_FRFMID_433 : (freqBand==RF_868MHZ ? RF_FRFMID_868 : RF_FRFMID_915))) },
        /* 0x09 */ { REG_FRFLSB, (uint8_t) (freqBand==RF_315MHZ ? RF_FRFLSB_315 : (freqBand==RF_433MHZ ? RF_FRFLSB_433 : (freqBand==RF_868MHZ ? RF_FRFLSB_868 : RF_FRFLSB_915))) },
        /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2 }, 
        /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, 
        /* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, 
        /* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, 
        /* 0x29 */ { REG_RSSITHRESH, 220 }, 
        /* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0 },
        /* 0x2F */ { REG_SYNCVALUE1, 0x2D },      
        /* 0x30 */ { REG_SYNCVALUE2, networkID }, 
        /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON | RF_PACKET1_ADRSFILTERING_OFF },
        /* 0x38 */ { REG_PAYLOADLENGTH, 66 }, 
        /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, 
        /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, 
        /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, 
        {255, 0}
    };
    
    spi_init();                // spi initialize
    
    while (readReg(REG_SYNCVALUE1) != 0xaa) {
        writeReg(REG_SYNCVALUE1, 0xaa);
    }
    while (readReg(REG_SYNCVALUE1) != 0x55) {
        writeReg(REG_SYNCVALUE1, 0x55);
    }

    for (uint8_t i = 0; CONFIG[i][0] != 255; i++)
        writeReg(CONFIG[i][0], CONFIG[i][1]);

    encrypt(0);

    setHighPower(isRFM69HW);        
    setMode(RF69_MODE_STANDBY);
    while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00);
    
    // --- PIC24 INT0 Interrupt Setup ---
    INTCON2bits.INT0EP = 0;  // Trigger INT0 on rising edge
    IFS0bits.INT0IF = 0;     // Clear the interrupt flag
    IEC0bits.INT0IE = 1;     // Enable INT0 Interrupt
    inISR = 0;

    millis_init();                  

    address = nodeID;
    setAddress(address);            
    setNetwork(networkID);
}

void setAddress(uint8_t addr) { writeReg(REG_NODEADRS, addr); }
void setNetwork(uint8_t networkID) { writeReg(REG_SYNCVALUE2, networkID); }

uint8_t canSend() {
    if (mode == RF69_MODE_RX && PAYLOADLEN == 0 && readRSSI(0) < CSMA_LIMIT) {
        setMode(RF69_MODE_STANDBY);
        return 1;
    }
    return 0;
}

void send(uint8_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t requestACK) {
    writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); 
    millis_current = millis();
    while (!canSend() && millis() - millis_current < RF69_CSMA_LIMIT_MS) receiveDone();
    sendFrame(toAddress, buffer, bufferSize, requestACK, 0);
}

uint8_t ACKRequested() { return ACK_REQUESTED && (TARGETID != RF69_BROADCAST_ADDR); }

void sendACK(const void* buffer, uint8_t bufferSize) {
    ACK_REQUESTED = 0;   
    uint8_t sender = SENDERID;
    int16_t _RSSI = RSSI; 
    writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); 
    millis_current = millis();
    while (!canSend() && millis() - millis_current < RF69_CSMA_LIMIT_MS) receiveDone();
    SENDERID = sender;    
    sendFrame(sender, buffer, bufferSize, 0, 1);
    RSSI = _RSSI; 
}

void setPowerLevel(uint8_t powerLevel) {
    uint8_t _powerLevel = powerLevel;
    if (isRFM69HW==1) _powerLevel /= 2;
    writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | _powerLevel);
}

void sleep() { setMode(RF69_MODE_SLEEP); }

uint8_t readTemperature(uint8_t calFactor) {
    setMode(RF69_MODE_STANDBY);
    writeReg(REG_TEMP1, RF_TEMP1_MEAS_START);
    while ((readReg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING));
    return ~readReg(REG_TEMP2) + COURSE_TEMP_COEF + calFactor; 
}

uint32_t getFrequency() { return RF69_FSTEP * (((uint32_t) readReg(REG_FRFMSB) << 16) + ((uint16_t) readReg(REG_FRFMID) << 8) + readReg(REG_FRFLSB)); }

void setFrequency(uint32_t freqHz) {
    uint8_t oldMode = mode;
    if (oldMode == RF69_MODE_TX) setMode(RF69_MODE_RX);
    freqHz /= RF69_FSTEP; 
    writeReg(REG_FRFMSB, freqHz >> 16);
    writeReg(REG_FRFMID, freqHz >> 8);
    writeReg(REG_FRFLSB, freqHz);
    if (oldMode == RF69_MODE_RX) setMode(RF69_MODE_SYNTH);
    setMode(oldMode);
}

uint8_t readReg(uint8_t addr) {
    select();
    spi_fast_shift(addr & 0x7F);
    uint8_t regval = spi_fast_shift(0);
    unselect();
    return regval;
}

void writeReg(uint8_t addr, uint8_t value) {
    select();
    spi_fast_shift(addr | 0x80);
    spi_fast_shift(value);
    unselect();
}

void encrypt(const char* key) {
    setMode(RF69_MODE_STANDBY);
    if (key != 0) {
        select();
        spi_fast_shift(REG_AESKEY1 | 0x80);
        for (uint8_t i = 0; i < 16; i++) spi_fast_shift(key[i]);
        unselect();
    }
    writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1:0));
}

void setMode(uint8_t newMode) {
    if (newMode == mode) return;
    switch (newMode) {
        case RF69_MODE_TX:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
            if (isRFM69HW) setHighPowerRegs(1);
            break;
        case RF69_MODE_RX:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
            if (isRFM69HW) setHighPowerRegs(0);
            break;
        case RF69_MODE_SYNTH:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
            break;
        case RF69_MODE_STANDBY:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
            break;
        case RF69_MODE_SLEEP:
            writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
            break;
        default: return;
    }
    while (mode == RF69_MODE_SLEEP && (readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); 
    mode = newMode;
}
    
void setHighPowerRegs(uint8_t onOff) {
    if(onOff==1) {
        writeReg(REG_TESTPA1, 0x5D);
        writeReg(REG_TESTPA2, 0x7C);
    } else {
        writeReg(REG_TESTPA1, 0x55);
        writeReg(REG_TESTPA2, 0x70);
    }
}
    
void setHighPower(uint8_t onOff) {
    isRFM69HW = onOff;
    writeReg(REG_OCP, isRFM69HW ? RF_OCP_OFF : RF_OCP_ON);
    if (isRFM69HW == 1) 
        writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); 
    else
        writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | powerLevel); 
}

int16_t readRSSI(uint8_t forceTrigger) {
    int16_t rssi = 0;
    if (forceTrigger==1) {
        writeReg(REG_RSSICONFIG, RF_RSSI_START);
        while ((readReg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00); 
    }
    rssi = -readReg(REG_RSSIVALUE);
    rssi >>= 1;
    return rssi;
}

void sendFrame(uint8_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t requestACK, uint8_t sendACK) {
    setMode(RF69_MODE_STANDBY); 
    while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); 
    if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

    uint8_t CTLbyte = 0x00;
    if (sendACK==1) CTLbyte = RFM69_CTL_SENDACK;
    else if (requestACK==1) CTLbyte = RFM69_CTL_REQACK;
    
    if (toAddress > 0xFF) CTLbyte |= (toAddress & 0x300) >> 6; 
    if (address > 0xFF) CTLbyte |= (address & 0x300) >> 8;   

    select(); 
    spi_fast_shift(REG_FIFO | 0x80);
    spi_fast_shift(bufferSize + 3);
    spi_fast_shift(toAddress);
    spi_fast_shift(address);
    spi_fast_shift(CTLbyte);

    for (uint8_t i = 0; i < bufferSize; i++)
        spi_fast_shift(((uint8_t*) buffer)[i]);
    
    unselect();
    setMode(RF69_MODE_TX);
    while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00); 
    setMode(RF69_MODE_STANDBY);
}

void rcCalibration() {
    writeReg(REG_OSC1, RF_OSC1_RCCAL_START);
    while ((readReg(REG_OSC1) & RF_OSC1_RCCAL_DONE) == 0x00);
}

uint8_t sendWithRetry(uint8_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries, uint8_t retryWaitTime) {
    for (uint8_t i = 0; i <= retries; i++) {
        send(toAddress, buffer, bufferSize, 1);
        millis_current = millis();
        while (millis() - millis_current < retryWaitTime) {
            if (ACKReceived(toAddress)) return 1;
        }
    }
    return 0;
}

uint8_t ACKReceived(uint8_t fromNodeID) {
    if (receiveDone())
        return (SENDERID == fromNodeID || fromNodeID == RF69_BROADCAST_ADDR) && ACK_RECEIVED;
    return 0;
}

uint8_t receiveDone() {
    IEC0bits.INT0IE = 0; // Disable INT0 (Replaces cli())
    if (mode == RF69_MODE_RX && PAYLOADLEN > 0) {
        setMode(RF69_MODE_STANDBY); 
        return 1;
    }
    else if (mode == RF69_MODE_RX) {
        IEC0bits.INT0IE = 1; // Enable INT0 (Replaces sei())
        return 0;
    }
    receiveBegin();
    IEC0bits.INT0IE = 1; // Enable INT0
    return 0;
}

void receiveBegin() {
    DATALEN = 0;
    SENDERID = 0;
    TARGETID = 0;
    PAYLOADLEN = 0;
    ACK_REQUESTED = 0;
    ACK_RECEIVED = 0;
    RSSI = 0;
    if (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
        writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); 
    writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); 
    setMode(RF69_MODE_RX);
}

void promiscuous(uint8_t onOff) {
    promiscuousMode = onOff;
    if(promiscuousMode==0)
        writeReg(REG_PACKETCONFIG1, (readReg(REG_PACKETCONFIG1) & 0xF9) | RF_PACKET1_ADRSFILTERING_NODEBROADCAST);
    else
        writeReg(REG_PACKETCONFIG1, (readReg(REG_PACKETCONFIG1) & 0xF9) | RF_PACKET1_ADRSFILTERING_OFF);    
}

void maybeInterrupts() {
    if (!inISR) IEC0bits.INT0IE = 1; // Re-enable INT0 if not inside ISR
}

// Enable SPI transfer (CS is Pin 25 / RB14)
void select() {
<<<<<<< HEAD
    LATBbits.LATB14 = 0; // Pull CS Low
=======
    LATBbits.LATB12 = 0; // Pull CS Low
>>>>>>> 26ebaea92124e1c2927101cab310b0e937cf297f
    IEC0bits.INT0IE = 0; // Disable INT0 (Replaces cli())
}

// Disable SPI transfer (CS is Pin 25 / RB14)
void unselect() {
<<<<<<< HEAD
    LATBbits.LATB14 = 1; // Pull CS High
=======
    LATBbits.LATB12 = 1; // Pull CS High
>>>>>>> 26ebaea92124e1c2927101cab310b0e937cf297f
    maybeInterrupts();
}

// PIC24 Interrupt Service Routine for INT0 (Pin 16)
void __attribute__((interrupt, no_auto_psv)) _INT0Interrupt(void) {
    inISR = 1;
    if (mode == RF69_MODE_RX && (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)) {
        setMode(RF69_MODE_STANDBY);
        select();
        spi_fast_shift(REG_FIFO & 0x7F);
        PAYLOADLEN = spi_fast_shift(0);
        if(PAYLOADLEN>66) PAYLOADLEN=66;
        TARGETID = spi_fast_shift(0);
        
        if(!(promiscuousMode || TARGETID == address || TARGETID == RF69_BROADCAST_ADDR) || PAYLOADLEN < 3) {
            PAYLOADLEN = 0;
            unselect();
            receiveBegin();
            IFS0bits.INT0IF = 0; // Clear interrupt flag before exiting
            return;
        }

        DATALEN = PAYLOADLEN - 3;
        SENDERID = spi_fast_shift(0);
        uint8_t CTLbyte = spi_fast_shift(0);

        ACK_RECEIVED = CTLbyte & RFM69_CTL_SENDACK; 
        ACK_REQUESTED = CTLbyte & RFM69_CTL_REQACK; 

        for (uint8_t i = 0; i < DATALEN; i++) {
            DATA[i] = spi_fast_shift(0);
        }
        if (DATALEN < RF69_MAX_DATA_LEN) DATA[DATALEN] = 0; 
        unselect();
        setMode(RF69_MODE_RX);
    }
    RSSI = readRSSI(0);
    inISR = 0;
    
    // CRITICAL: Clear the PIC24 Interrupt Flag
    IFS0bits.INT0IF = 0; 
}
/* Code modified version Copyright (c) 2021 Philippe Cavenel
 *
 * This Source Code Form is subject to the terms of the
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991.
 *
 * The Psion-specific code is copyright (c) 2019 Ash Wolf.
 * The ARM disassembly code is a modified version of the one used in mGBA by endrift.
 * WindEmu is available under the Mozilla Public License 2.0.
*/

#include "windermere.h"
#include "wind_defs.h"
#include "hardware.h"
#include <time.h>
#include "common.h"
#include <QFileDialog>
#include <QApplication>
#include <QtMultimedia>
#include <QBuffer>


#define INCLUDE_D
// #define INCLUDE_BANK1 // Doesn't work.... to be fixed

namespace Windermere {
////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::Emulator
////////////////////////////////////////////////////////////////////////////
Emulator::Emulator() : EmuBase(true), etna(this) {
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::getRTC
/// \return
////////////////////////////////////////////////////////////////////////////
uint32_t Emulator::getRTC() {
    return time(nullptr) - 946684800;
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::BuzzerStart
////////////////////////////////////////////////////////////////////////////
void Emulator::BuzzerStart(){
    // printf("buzzerVolume(%l)\n",buzzerVolume);fflush(stdout);
    effect.setLoopCount(1);
    effect.setVolume(buzzerVolume/10);
    effect.play();
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::readReg8
/// \param reg
/// \return
////////////////////////////////////////////////////////////////////////////
uint32_t Emulator::readReg8(uint32_t reg) {
	if ((reg & 0xF00) == 0x600) {
        pendingInterrupts &= ~(1 << UART1);
        return uart1.readReg8(reg & 0xFFF);
	} else if ((reg & 0xF00) == 0x700) {
        if (uart2.UART2DATA_fifoBytesAvailable<=0) pendingInterrupts &= ~(1 << UART2);
        return uart2.readReg8(reg & 0xFFF);
	} else if (reg == TC1CTRL) {
		return tc1.config;
	} else if (reg == TC2CTRL) {
		return tc2.config;
	} else if (reg == PADR) {
		return readKeyboard();
	} else if (reg == PBDR) {
		return (portValues >> 16) & 0xFF;
	} else if (reg == PCDR) {
		return (portValues >> 8) & 0xFF;
	} else if (reg == PDDR) {
		return portValues & 0xFF;
	} else if (reg == PADDR) {
		return (portDirections >> 24) & 0xFF;
	} else if (reg == PBDDR) {
		return (portDirections >> 16) & 0xFF;
	} else if (reg == PCDDR) {
		return (portDirections >> 8) & 0xFF;
	} else if (reg == PDDDR) {
		return portDirections & 0xFF;
    } else if (reg == LCDCTL) {
        //printf("LCD control read lcdControl = %d pc=%08x lr=%08x !!!\n", lcdControl, getGPR(15), getGPR(14));fflush(stdout);
        return lcdControl;
    } else if (reg == LCDST) {
        //printf("LCD state read pc=%08x lr=%08x !!!\n", getGPR(15), getGPR(14));
        return 0xFFFFFFFF;
    } else if (reg == PWRSR) {
        // printf("!!! PWRSR read pc=%08x lr=%08x pwrsr=%08x!!!\n", getGPR(15), getGPR(14),pwrsr);
        return pwrsr;
    } else if (reg == INTSR) {
        return pendingInterrupts & interruptMask;
    } else if (reg == INTRSR) {
        return pendingInterrupts;
    } else if (reg == INTENS) {
        return interruptMask;
    } else if (reg == TC1VAL) {
        return tc1.value;
    } else if (reg == TC2VAL) {
        return tc2.value;
    } else if (reg == SSDR) {
        // as per 5000A7B0 in 5mx rom
        uint16_t ssiValue = 0;
        switch (lastSSIRequest) {
        case 0xD0D3: ssiValue = (uint16_t)(50 + (touchX * 5.7)); break;
        case 0x9093: ssiValue = (uint16_t)(3834 - (touchY * 13.225)); break;
        case 0xA4A4: ssiValue = 3100; break; // MainBattery
        case 0xE4E4: ssiValue = 3100; break; // BackupBattery
        }
        uint32_t ret = 0;
        if (ssiReadCounter == 4) ret = (ssiValue >> 5) & 0x7F;
        if (ssiReadCounter == 5) ret = (ssiValue << 3) & 0xF8;
        ssiReadCounter++;
        if (ssiReadCounter == 6) ssiReadCounter = 0;

        // by hardware we should be clearing SSEOTI here, i think
        // but we just leave it on to simplify things
        return ret;
    } else if (reg == SSSR) {
        return 0;
    } else if (reg == RTCDRL) {
        uint16_t v = rtc & 0xFFFF;
//		log("RTCDRL: %04x", v);
        return v;
    } else if (reg == RTCDRU) {
        uint16_t v = rtc >> 16;
//		log("RTCDRU: %04x", v);
        return v;
    } else if (reg == KSCAN) {
        return kScan;
    } else if (reg == CODR) {
        if (!m_recordStart) {
            m_recordStart=true;
            m_audioInput->stop();
            m_audioInput->reset();
            m_inputCodecQueue.clear();
            m_inputDevice=m_audioInput->start();
            printf("start audioInput %d bytesAvailable\n",m_audioInput->bytesAvailable());fflush(stdout);

        }

       // printf("m_inputCodecQueue length = %d\n",m_inputCodecQueue.length());fflush(stdout);
        codrInputCounter++;
        if(codrInputCounter>=8) {
            codrInputCounter=0;
            codecValueInRead=false;
            coflg|=1; // Receive fifo is empty
        }

        if (m_audioInput->bytesAvailable()>0) recordSound();
        if (!m_inputCodecQueue.empty()) { codr=m_inputCodecQueue.dequeue(); }

     //   printf("read CODR :: pc=%08x lr=%08x reg=%03x value=%08x\n", getGPR(15)-4, getGPR(14), reg,codr);
        return codr;
    } else if (reg == CONFG) {
      //  printf("read CONFG :: pc=%08x lr=%08x reg=%03x value=%08x\n", getGPR(15)-4, getGPR(14), reg,confg);
    return confg;
    } else if (reg == COFLG) {
      //  printf("read COFLG :: pc=%08x lr=%08x reg=%03x value=%08x\n", getGPR(15)-4, getGPR(14), reg,coflg);
        return coflg;
    } else if (reg == BZCONT) {
      //  printf("read BZCONT :: pc=%08x lr=%08x reg=%03x value=%08x\n", getGPR(15)-4, getGPR(14), reg,bzcont);
    return bzcont;
    } else {
     //  printf("RegRead unknown:: pc=%08x lr=%08x reg=%03x\n", getGPR(15)-4, getGPR(14), reg);fflush(stdout);
        return 0xFFFFFFFF;
	}
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::writeReg8
/// \param reg
/// \param value
////////////////////////////////////////////////////////////////////////////
void Emulator::writeReg8(uint32_t reg, uint32_t value) {

	if ((reg & 0xF00) == 0x600) {
       pendingInterrupts &= ~(1 << UART1);
        uart1.writeReg8(reg & 0xFFF, value);
	} else if ((reg & 0xF00) == 0x700) {
        if (uart2.UART2DATA_fifoBytesAvailable<=0) pendingInterrupts &= ~(1 << UART2);
        uart2.writeReg8(reg & 0xFFF, value);
	} else if (reg == TC1CTRL) {
		tc1.setConfig(value);
	} else if (reg == TC2CTRL) {
		tc2.setConfig(value);
	} else if (reg == PADR) {
		uint32_t oldPorts = portValues;
		portValues &= 0x00FFFFFF;
		portValues |= (uint32_t)value << 24;
		diffPorts(oldPorts, portValues);
	} else if (reg == PBDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFF00FFFF;
		portValues |= (uint32_t)value << 16;
		if ((portValues & 0x10000) && !(oldPorts & 0x10000))
			etna.setPromBit0High();
		else if (!(portValues & 0x10000) && (oldPorts & 0x10000))
			etna.setPromBit0Low();
		if ((portValues & 0x20000) && !(oldPorts & 0x20000))
			etna.setPromBit1High();
		diffPorts(oldPorts, portValues);
	} else if (reg == PCDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFFFF00FF;
		portValues |= (uint32_t)value << 8;
		diffPorts(oldPorts, portValues);
	} else if (reg == PDDR) {
		uint32_t oldPorts = portValues;
		portValues &= 0xFFFFFF00;
		portValues |= (uint32_t)value;
		diffPorts(oldPorts, portValues);
	} else if (reg == PADDR) {
		portDirections &= 0x00FFFFFF;
		portDirections |= (uint32_t)value << 24;
	} else if (reg == PBDDR) {
		portDirections &= 0xFF00FFFF;
		portDirections |= (uint32_t)value << 16;
	} else if (reg == PCDDR) {
		portDirections &= 0xFFFF00FF;
		portDirections |= (uint32_t)value << 8;
	} else if (reg == PDDDR) {
		portDirections &= 0xFFFFFF00;
		portDirections |= (uint32_t)value;
    } else if (reg == KSCAN) {
        kScan = value;
    } else if (reg == BZCONT) {
        // printf("write reg8 BZCONT :: pc=%08x lr=%08x reg=%03x value=%08x\n", getGPR(15)-4, getGPR(14), reg,bzcont);fflush(stdout);
        bzcont = value;
        if ((bzcont & 1) == 1) {
            buzzerOn=true;
            buzzerVolume++;
        }
        if ((bzcont & 1) == 0) {
            if (buzzerOn) BuzzerStart();
            buzzerOn=false;
            buzzerVolume=0;
        }
    } else	if (reg == LCDCTL) {
        //printf("LCD: ctl write %08x\n", value);fflush(stdout);
        lcdControl = value;
    } else if (reg == LCD_DBAR1) {
    //	printf("LCD: address write %08x\n", value);
        lcdAddress = value;
    } else if (reg == LCDT0) {
        //printf("LCD: horz timing write %08x\n", value);
    } else if (reg == LCDT1) {
        //printf("LCD: vert timing write %08x\n", value);
    } else if (reg == LCDT2) {
        //printf("LCD: clocks write %08x\n", value);
    } else if (reg == INTENS) {
//		diffInterrupts(interruptMask, interruptMask | value);
        interruptMask |= value;
    } else if (reg == INTENC) {
//		diffInterrupts(interruptMask, interruptMask &~ value);
        interruptMask &= ~value;
    } else if (reg == HALT) {
        halted = true;
    // BLEOI = 0x410,
    // MCEOI = 0x414,
    } else if (reg == TEOI) {
        pendingInterrupts &= ~(1 << TINT);
    // TEOI = 0x418,
    // STFCLR = 0x41C,
    // E2EOI = 0x420,
    } else if (reg == SSDR) {
        if (value != 0)
            lastSSIRequest = (lastSSIRequest >> 8) | (value & 0xFF00);
    } else if (reg == TC1LOAD) {
        tc1.load(value);
    } else if (reg == TC1EOI) {
        pendingInterrupts &= ~(1 << TC1OI);
    } else if (reg == TC2LOAD) {
        tc2.load(value);
    } else if (reg == TC2EOI) {
        pendingInterrupts &= ~(1 << TC2OI);
    } else if (reg == RTCDRL) {
        rtc &= 0xFFFF0000;
        rtc |= (value & 0xFFFF);
        //log("RTC write lower: %04x", value);
    } else if (reg == RTCDRU) {
        rtc &= 0x0000FFFF;
        rtc |= (value & 0xFFFF) << 16;
        //log("RTC write upper: %04x", value);
    } else if (reg==CODR){
        m_outputCodecQueue.enqueue((unsigned char) (value & 0xFF));
        codr=value;
        //printf("%02x ", value);fflush(stdout);
        codrOutputCounter++;
        if(codrOutputCounter>=8) {
            codecValueOutReady=true;
            codrOutputCounter=0;
            coflg|=2; // transmit fifo full
        }

     //   printf("CODR write : %08x\n", value);fflush(stdout);
    } else if (reg==CONFG){
     // printf("CONFG write : %08x\n", value);fflush(stdout);
        confg=value;
        if ((confg & 3) == 3) pendingInterrupts |= (1 << CSINT); // Set CODEC irq
        else {
            if (m_recordStart) {
                m_recordStart=false;
                m_audioInput->stop();
                // printf("stop audioInput\n");fflush(stdout);
            }
            m_inputCodecQueue.clear();
            m_outputCodecQueue.clear();
        }
    } else if (reg==COEOI){
      // printf("COEOI write : %08x\n", value);fflush(stdout);
        pendingInterrupts &= ~(1 << CSINT);                  // Unset CODEC irq

    }else {
       //  printf("RegWrite unknown:: pc=%08x reg=%03x value=%02x\n", getGPR(15)-4, reg, value);fflush(stdout);
	}
}


////////////////////////////////////////////////////////////////////////////
/// \brief readReg32
/// \param reg
/// \return
////////////////////////////////////////////////////////////////////////////
uint32_t Emulator::readReg32(uint32_t reg) {
   // printf("Mode 32 bits : ");fflush(stdout);
    return readReg8(reg);
}

////////////////////////////////////////////////////////////////////////////
/// \brief writeReg32
/// \param reg
/// \param value
////////////////////////////////////////////////////////////////////////////
void Emulator::writeReg32(uint32_t reg, uint32_t value) {
   // printf("Mode 32 bits : ");fflush(stdout);
    writeReg8(reg,value);
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::readPhysical
/// \param physAddr
/// \param valueSize
/// \return
////////////////////////////////////////////////////////////////////////////
MaybeU32 Emulator::readPhysical(uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 24) & 0xF1;
	if (valueSize == V8) {
		if (region == 0)
			return ROM[physAddr & 0xFFFFFF];
		else if (region == 0x10)
			return ROM2[physAddr & 0x3FFFF];
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			return etna.readReg8(physAddr & 0xFFF);
        else if (region == 0x80 && physAddr <= 0x80000FFF) {
			return readReg8(physAddr & 0xFFF);
        }
#if defined(INCLUDE_BANK1)
		else if (region == 0xC0)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
		else if (region == 0xC1)
			return MemoryBlockC1[physAddr & MemoryBlockMask];
		else if (region == 0xD0)
			return MemoryBlockD0[physAddr & MemoryBlockMask];
		else if (region == 0xD1)
			return MemoryBlockD1[physAddr & MemoryBlockMask];
#elif defined(INCLUDE_D)
		else if (region == 0xC0 || region == 0xC1)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
		else if (region == 0xD0 || region == 0xD1)
			return MemoryBlockD0[physAddr & MemoryBlockMask];
#else
		else if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0xD1)
			return MemoryBlockC0[physAddr & MemoryBlockMask];
#endif
		else if (region >= 0xC0)
			return 0xFF; // just throw accesses to unmapped RAM away
	} else {
		uint32_t result;
		if (region == 0)
			LOAD_32LE(result, physAddr & 0xFFFFFF, ROM);
		else if (region == 0x10)
			LOAD_32LE(result, physAddr & 0x3FFFF, ROM2);
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			result = etna.readReg32(physAddr & 0xFFF);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			result = readReg32(physAddr & 0xFFF);
#if defined(INCLUDE_BANK1)
		else if (region == 0xC0)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xC1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC1);
		else if (region == 0xD0)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD0);
		else if (region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD1);
#elif defined(INCLUDE_D)
		else if (region == 0xC0 || region == 0xC1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xD0 || region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockD0);
#else
		else if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0xD1)
			LOAD_32LE(result, physAddr & MemoryBlockMask, MemoryBlockC0);
#endif
		else if (region >= 0xC0)
			return 0xFFFFFFFF; // just throw accesses to unmapped RAM away
		else
			return {};
		return result;
	}

	return {};
}
////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::writePhysical
/// \param value
/// \param physAddr
/// \param valueSize
/// \return
////////////////////////////////////////////////////////////////////////////
bool Emulator::writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) {
	uint8_t region = (physAddr >> 24) & 0xF1;

    //check if must refresh the screen
    if ((physAddr & MemoryBlockMask) >= (lcdAddress & MemoryBlockMask) &&
        (physAddr & MemoryBlockMask) <= (lcdAddress & MemoryBlockMask)+(getLCDWidth()*getLCDHeight())+PALETTE_SIZE) {
        m_accessLcdFrameMemory=true;
    }
    else {
        if (m_accessLcdFrameMemory) {
            m_screenRefresh=true;
            m_accessLcdFrameMemory=false;
        }
    }

	if (valueSize == V8) {
#if defined(INCLUDE_BANK1)
		if (region == 0xC0)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xC1)
			MemoryBlockC1[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xD0)
			MemoryBlockD0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xD1)
			MemoryBlockD1[physAddr & MemoryBlockMask] = (uint8_t)value;
#elif defined(INCLUDE_D)
		if (region == 0xC0 || region == 0xC1)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
		else if (region == 0xD0 || region == 0xD1)
			MemoryBlockD0[physAddr & MemoryBlockMask] = (uint8_t)value;
#else
		if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0xD1)
			MemoryBlockC0[physAddr & MemoryBlockMask] = (uint8_t)value;
#endif
		else if (region >= 0xC0)
			return true; // just throw accesses to unmapped RAM away
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			etna.writeReg8(physAddr & 0xFFF, value);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			writeReg8(physAddr & 0xFFF, value);
		else
			return false;
	} else {
		uint8_t region = (physAddr >> 24) & 0xF1;
#if defined(INCLUDE_BANK1)
		if (region == 0xC0)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xC1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC1);
		else if (region == 0xD0)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD0);
		else if (region == 0xD1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD1);
#elif defined(INCLUDE_D)
		if (region == 0xC0 || region == 0xC1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
		else if (region == 0xD0 || region == 0xD1)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockD0);
#else
		if (region == 0xC0 || region == 0xC1 || region == 0xD0 || region == 0x01)
			STORE_32LE(value, physAddr & MemoryBlockMask, MemoryBlockC0);
#endif
		else if (region >= 0xC0)
			return true; // just throw accesses to unmapped RAM away
		else if (region == 0x20 && physAddr <= 0x20000FFF)
			etna.writeReg32(physAddr & 0xFFF, value);
		else if (region == 0x80 && physAddr <= 0x80000FFF)
			writeReg32(physAddr & 0xFFF, value);
		else
			return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::configure
////////////////////////////////////////////////////////////////////////////
void Emulator::configure() {

    if (configured) return;
    configured = true;
	srand(1000);
    uart1.cpu = this;
    uart2.cpu = this;
	memset(&tc1, 0, sizeof(tc1));
	memset(&tc2, 0, sizeof(tc1));
	tc1.clockSpeed = CLOCK_SPEED;
	tc2.clockSpeed = CLOCK_SPEED;

    nextTickAt = TICK_INTERVAL;
    nextCodecAt = CODEC_INTERVAL;
    tc1.nextTickAt = tc1.tickInterval();
	tc2.nextTickAt = tc2.tickInterval();
    rtc = getRTC();
    reset();


    // BUZZER
#ifdef Q_OS_WIN64
     effect.setSource(QUrl::fromLocalFile(qApp->applicationDirPath().append(QString("/../../../Psion5mxEmulator/Psion5mxEmulatorQt/pkg_src/assets/beep.wav"))));

#else // Android
        effect.setSource(QString("assets:beep.wav"));
#endif
     // CODEC

    QAudioFormat audioFormat;
    audioFormat.setSampleRate(8000); // 8KHz
    audioFormat.setChannelCount(1);
    audioFormat.setChannelConfig(QAudioFormat::ChannelConfigMono); // audio/pcm
    audioFormat.setSampleFormat(QAudioFormat::Int16);

    // CODEC INPUT
    // printf("CODEC INPUT");fflush(stdout);

    QAudioDevice DefautDeviceIn = QMediaDevices::defaultAudioInput();
    qWarning() << DefautDeviceIn.description();
    qWarning() << DefautDeviceIn.preferredFormat();
    qWarning() << DefautDeviceIn.maximumSampleRate();
    qWarning() << DefautDeviceIn.minimumSampleRate();
    qWarning() << DefautDeviceIn.minimumChannelCount();
    qWarning() << DefautDeviceIn.supportedSampleFormats();
    //m_audioInput = new QAudioSource(DefautDeviceIn.preferredFormat());
    m_audioInput = new QAudioSource(audioFormat);
    m_audioInput->setVolume(1);
    m_audioInput->setBufferSize(1000);
    qWarning() << m_audioInput->bufferSize();
    qWarning() << m_audioInput->state();
    m_inputDevice=m_audioInput->start();

    // CODEC OUTPUT
    //m_audioOutput = new QAudioSink(DefautDeviceIn.preferredFormat());
    m_audioOutput = new QAudioSink(audioFormat);
    m_audioOutput->setVolume(1);
    m_outputDevice=m_audioOutput->start();
}

uint8_t *Emulator::getROMBuffer() {
	return ROM;
}
size_t Emulator::getROMSize() {
	return sizeof(ROM);
}
void Emulator::loadROM(uint8_t *buffer, size_t size) {
    memcpy(ROM, buffer, min(size, sizeof(ROM)));
}

void Emulator::loadRAMC0(uint8_t *buffer, size_t size) {
    memcpy(MemoryBlockC0, buffer, min(size, sizeof(MemoryBlockC0)));
}
void Emulator::loadRAMC1(uint8_t *buffer, size_t size) {
    memcpy(MemoryBlockC1, buffer, min(size, sizeof(MemoryBlockC1)));
}
void Emulator::loadRAMD0(uint8_t *buffer, size_t size) {
    memcpy(MemoryBlockD0, buffer, min(size, sizeof(MemoryBlockD0)));
}
void Emulator::loadRAMD1(uint8_t *buffer, size_t size) {
    memcpy(MemoryBlockD1, buffer, min(size, sizeof(MemoryBlockD1)));
}

uint8_t *Emulator::getRAMC0() {
    return MemoryBlockC0;
}
uint8_t *Emulator::getRAMC1() {
    return MemoryBlockC1;
}
uint8_t *Emulator::getRAMD0() {
    return MemoryBlockD0;
}
uint8_t *Emulator::getRAMD1() {
    return MemoryBlockD1;
}

uint32_t Emulator::getRAMsizeC0() {
    return sizeof(MemoryBlockC0);
}
uint32_t Emulator::getRAMsizeC1() {
    return sizeof(MemoryBlockC1);
}
uint32_t Emulator::getRAMsizeD0() {
    return sizeof(MemoryBlockD0);
}
uint32_t Emulator::getRAMsizeD1() {
    return sizeof(MemoryBlockD1);
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::executeUntil
/// \param cycles
////////////////////////////////////////////////////////////////////////////
void Emulator::executeUntil(int64_t cycles) {

   // QElapsedTimer timer;
   // timer.start();
	if (!configured)
        configure();

    while (!asleep && passedCycles < cycles) {

        // UART2 external link
        if (uart2.UART2DATA_valueInReady && uart2.UART2DATA_lastValueRead) UartReadData();
        if (uart2.UART2DATA_valueOutReady) UartWriteData();

       if (passedCycles >= nextCodecAt) { // every 1/1000 seconds
            if (codecValueOutReady) {CodecWriteData(); }
            if (!codecValueInRead) { CodecReadData(); }
            nextCodecAt+= CODEC_INTERVAL;
        }

        if (passedCycles >= nextTickAt) { // every 1/64 seconds
			// increment RTCDIV
			if ((pwrsr & 0x3F) == 0x3F) {
                rtc++; // Real Time Clock, every 1 second
				pwrsr &= ~0x3F;
			} else {
				pwrsr++;
			}

			nextTickAt += TICK_INTERVAL;
			pendingInterrupts |= (1<<TINT);
		}
        if (cycles >= nextTickAt) {
            if (tc1.tick(passedCycles))
                pendingInterrupts |= (1<<TC1OI);
            if (tc2.tick(passedCycles))
                pendingInterrupts |= (1<<TC2OI);

        }

        // UART interrupt

        if ((uart1.UART1INTM_value & uart1.PSIONW_UART_TXINT)==uart1.PSIONW_UART_TXINT) {
           pendingInterrupts |= (1 << UART1);
        }

        if ((uart2.UART2INTM_value & uart2.PSIONW_UART_TXINT)==uart2.PSIONW_UART_TXINT) {
           pendingInterrupts |= (1 << UART2);
        }

        if (pendingInterrupts) {
            if ((pendingInterrupts & interruptMask & FIQ_INTERRUPTS) != 0 && canAcceptFIQ()) {
                requestFIQ();
               // printf("The requestFIQ() operation took %d milliseconds pendingInterrupts=0x%x\n",timer.elapsed(),pendingInterrupts);

                halted = false;
            }
            if ((pendingInterrupts & interruptMask & IRQ_INTERRUPTS) != 0 && canAcceptIRQ()) {
                requestIRQ();
                //printf("The requestIRQ() operation took %d milliseconds pendingInterrupts=0x%x\n",timer.elapsed(),pendingInterrupts);
                halted = false;
            }
        }

        int64_t lastpassedCycles=passedCycles; // added to fix a bug during psion backup and restore

        // what's running?
        if (halted) {
			// keep the clock moving
			// when does the next earliest thing happen?
			// this stops us from spinning needlessly
			int64_t nextEvent = nextTickAt;
			if (tc1.nextTickAt < nextEvent) nextEvent = tc1.nextTickAt;
			if (tc2.nextTickAt < nextEvent) nextEvent = tc2.nextTickAt;
            if (cycles < nextEvent) nextEvent = cycles;
            passedCycles = nextEvent;

        }  /*else {
            if (auto v = virtToPhys(getGPR(15) - 0xC); v.has_value() && instructionReady()) {
                // printf("The virtToPhys(getGPR(15) - 0xC); v.has_value() && instructionReady() operation took %d milliseconds",timer.elapsed());
                 debugPC(v.value());
               // printf("The debugPC() operation took %d milliseconds",timer.elapsed());
            }
        }*/
        passedCycles += tick();
        if (lastpassedCycles==passedCycles) return; // added to fix a bug during psion backup and restore

/*
#ifndef __EMSCRIPTEN__
			uint32_t new_pc = getGPR(15) - 0xC;
			if (_breakpoints.find(new_pc) != _breakpoints.end()) {
                //log("?????? Breakpoint triggered at %08x!", new_pc);
				return;
			}
#endif
*/
        //}
	}
    //printf("executeUntil = %d\n",timer.elapsed());

}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::identifyObjectCon
/// \param ptr
/// \return
////////////////////////////////////////////////////////////////////////////
const char *Emulator::identifyObjectCon(uint32_t ptr) {
	if (ptr == readVirtualDebug(0x80000980, V32).value()) return "process";
	if (ptr == readVirtualDebug(0x80000984, V32).value()) return "thread";
	if (ptr == readVirtualDebug(0x80000988, V32).value()) return "chunk";
//	if (ptr == readVirtualDebug(0x8000098C, V32).value()) return "semaphore";
//	if (ptr == readVirtualDebug(0x80000990, V32).value()) return "mutex";
	if (ptr == readVirtualDebug(0x80000994, V32).value()) return "logicaldevice";
	if (ptr == readVirtualDebug(0x80000998, V32).value()) return "physicaldevice";
	if (ptr == readVirtualDebug(0x8000099C, V32).value()) return "channel";
	if (ptr == readVirtualDebug(0x800009A0, V32).value()) return "server";
//	if (ptr == readVirtualDebug(0x800009A4, V32).value()) return "unk9A4"; // name always null
	if (ptr == readVirtualDebug(0x800009AC, V32).value()) return "library";
//	if (ptr == readVirtualDebug(0x800009B0, V32).value()) return "unk9B0"; // name always null
//	if (ptr == readVirtualDebug(0x800009B4, V32).value()) return "unk9B4"; // name always null
	return NULL;
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::fetchStr
/// \param str
/// \param buf
////////////////////////////////////////////////////////////////////////////
void Emulator::fetchStr(uint32_t str, char *buf) {
	if (str == 0) {
		strcpy(buf, "<NULL>");
		return;
	}
	int size = readVirtualDebug(str, V32).value();
	for (int i = 0; i < size; i++) {
		buf[i] = readVirtualDebug(str + 4 + i, V8).value();
	}
	buf[size] = 0;
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::fetchName
/// \param obj
/// \param buf
////////////////////////////////////////////////////////////////////////////
void Emulator::fetchName(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x10, V32).value(), buf);
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::fetchProcessFilename
/// \param obj
/// \param buf
////////////////////////////////////////////////////////////////////////////
void Emulator::fetchProcessFilename(uint32_t obj, char *buf) {
	fetchStr(readVirtualDebug(obj + 0x3C, V32).value(), buf);
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::debugPC
/// \param pc
////////////////////////////////////////////////////////////////////////////
void Emulator::debugPC(uint32_t pc) {
	char objName[1000];

	if (pc == 0x2CBC4) {
		// CObjectCon::AddL()
		uint32_t container = getGPR(0);
		uint32_t obj = getGPR(1);
		const char *wut = identifyObjectCon(container);
		if (wut) {
			fetchName(obj, objName);
			if (strcmp(wut, "process") == 0) {
				char procName[1000];
				fetchProcessFilename(obj, procName);
                //log("OBJS: added %s at %08x <%s> <%s>", wut, obj, objName, procName);
			} else {
                //log("OBJS: added %s at %08x <%s>", wut, obj, objName);
			}
		}
	}

	if (pc == 0x6D8) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t btIndex = getGPR(2);
		uint32_t regionSize = getGPR(3);
        //log("KERNEL MMU SECTION: v:%08x p:%08x size:%08x idx:%02x",
        //	virtAddr, physAddr, regionSize, btIndex);
	}
	if (pc == 0x710) {
		uint32_t virtAddr = getGPR(0);
		uint32_t physAddr = getGPR(1);
		uint32_t btIndex = getGPR(2);
		uint32_t regionSize = getGPR(3);
		uint32_t pageTableA = getGPR(4);
		uint32_t pageTableB = getGPR(5);
        //log("KERNEL MMU PAGES: v:%08x p:%08x size:%08x idx:%02x tableA:%08x tableB:%08x",
        //	virtAddr, physAddr, regionSize, btIndex, pageTableA, pageTableB);
	}

	if (pc == 0x1576C) {
		uint32_t rawEvent = getGPR(0);
		uint32_t evtType = readVirtualDebug(rawEvent, V32).value_or(0);
		uint32_t evtTick = readVirtualDebug(rawEvent + 4, V32).value_or(0);
		uint32_t evtParamA = readVirtualDebug(rawEvent + 8, V32).value_or(0);
		uint32_t evtParamB = readVirtualDebug(rawEvent + 0xC, V32).value_or(0);
		const char *n = "???";
		switch (evtType) {
		case 0: n = "ENone"; break;
		case 1: n = "EPointerMove"; break;
		case 2: n = "EPointerSwitchOn"; break;
		case 3: n = "EKeyDown"; break;
		case 4: n = "EKeyUp"; break;
		case 5: n = "ERedraw"; break;
		case 6: n = "ESwitchOn"; break;
		case 7: n = "EActive"; break;
		case 8: n = "EInactive"; break;
		case 9: n = "EUpdateModifiers"; break;
		case 10: n = "EButton1Down"; break;
		case 11: n = "EButton1Up"; break;
		case 12: n = "EButton2Down"; break;
		case 13: n = "EButton2Up"; break;
		case 14: n = "EButton3Down"; break;
		case 15: n = "EButton3Up"; break;
		case 16: n = "ESwitchOff"; break;
		}
        //log("EVENT %s: tick=%d params=%d,%d", n, evtTick, evtParamA, evtParamB);
	}
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::getDeviceName
/// \return
////////////////////////////////////////////////////////////////////////////
const char *Emulator::getDeviceName() const { return "Series 5mx"; }
int Emulator::getDigitiserWidth()  const { return 695; }
int Emulator::getDigitiserHeight() const { return 280; }
int Emulator::getLCDOffsetX()      const { return 45; }
int Emulator::getLCDOffsetY()      const { return 5; }
int Emulator::getLCDWidth()        const { return 640; }
int Emulator::getLCDHeight()       const { return 240; }

// TODO move this elsewhere
static bool initRgbValues = false;
static uint32_t rgbValues[16];

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::readLCDIntoBuffer
/// \param lines
/// \param is32BitOutput
////////////////////////////////////////////////////////////////////////////
void Emulator::readLCDIntoBuffer(uint8_t **lines, bool is32BitOutput) const {
	if (!initRgbValues) {
		initRgbValues = true;
		for (int i = 0; i < 16; i++) {
			int r = (0x99 * i) / 15;
			int g = (0xAA * i) / 15;
			int b = (0x88 * i) / 15;
			rgbValues[15 - i] = r | (g << 8) | (b << 16) | 0xFF000000;
		}
	}

	if ((lcdAddress >> 24) == 0xC0) {
		const uint8_t *lcdBuf = &MemoryBlockC0[lcdAddress & MemoryBlockMask];
		int width = 640, height = 240;

		// fetch palette
		int bpp = 1 << (lcdBuf[1] >> 4);
		int ppb = 8 / bpp;
		uint16_t palette[16];
		for (int i = 0; i < 16; i++)
			palette[i] = lcdBuf[i*2] | ((lcdBuf[i*2+1] << 8) & 0xF00);

		// build our image out
		int lineWidth = (width * bpp) / 8;
		for (int y = 0; y < height; y++) {
			int lineOffs = 0x20 + (lineWidth * y);
			for (int x = 0; x < width; x++) {
				uint8_t byte = lcdBuf[lineOffs + (x / ppb)];
				int shift = (x & (ppb - 1)) * bpp;
				int mask = (1 << bpp) - 1;
				int palIdx = (byte >> shift) & mask;
				int palValue = palette[palIdx];

				if (is32BitOutput) {
					auto line = (uint32_t *)lines[y];
                    line[x] = rgbValues[palValue];
                } else {
					palValue |= (palValue << 4);
					lines[y][x] = palValue ^ 0xFF;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::readLCDColorIntoBuffer
/// \param lines
/// \param is32BitOutput
////////////////////////////////////////////////////////////////////////////
void Emulator::readLCDColorIntoBuffer(uint8_t **lines, bool is32BitOutput) const {
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::diffPorts
/// \param oldval
/// \param newval
////////////////////////////////////////////////////////////////////////////
void Emulator::diffPorts(uint32_t oldval, uint32_t newval) {

    /*printf("oldval %x newval %x \n",oldval,newval);
	uint32_t changes = oldval ^ newval;
    if (changes & 1) printf("PRT codec enable: %d\n", newval&1);
    if (changes & 2) printf("PRT audio amp enable: %d\n", newval&2);
    if (changes & 4) printf("PRT lcd power: %d\n", newval&4);
    if (changes & 8) printf("PRT etna door: %d\n", newval&8);
    if (changes & 0x10) printf("PRT sled: %d\n", newval&0x10);
    if (changes & 0x20) printf("PRT pump pwr2: %d\n", newval&0x20);
    if (changes & 0x40) printf("PRT pump pwr1: %d\n", newval&0x40);
    if (changes & 0x80) printf("PRT etna err: %d\n", newval&0x80);
    if (changes & 0x100) printf("PRT rs-232 rts: %d\n", newval&0x100);fflush(stdout);
    if (changes & 0x200) printf("PRT rs-232 dtr toggle: %d\n", newval&0x200);fflush(stdout);
    if (changes & 0x400) printf("PRT disable power led: %d\n", newval&0x400);
    if (changes & 0x800) printf("PRT enable uart1: %d\n", newval&0x800);fflush(stdout);
    if (changes & 0x1000) printf("PRT lcd backlight: %d\n", newval&0x1000);
    if (changes & 0x2000) printf("PRT enable uart2: %d\n", newval&0x2000);fflush(stdout);
   // if (changes & 0x4000) printf("PRT dictaphone: %d\n", newval&0x4000);

    // PROM read process makes this super spammy in stdout
    if (changes & 0x10000) printf("PRT EECS: %d\n", newval&0x10000);
    if (changes & 0x20000) printf("PRT EECLK: %d\n", newval&0x20000);
    if (changes & 0x40000) printf("PRT contrast0: %d\n", newval&0x40000);
    if (changes & 0x80000) printf("PRT contrast1: %d\n", newval&0x80000);
    if (changes & 0x100000) printf("PRT contrast2: %d\n", newval&0x100000);
    if (changes & 0x200000) printf("PRT contrast3: %d\n", newval&0x200000);
    if (changes & 0x400000) printf("PRT case open: %d\n", newval&0x400000);
    if (changes & 0x800000) printf("PRT etna cf power: %d\n", newval&0x800000);
    fflush(stdout);*/
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::diffInterrupts
/// \param oldval
/// \param newval
////////////////////////////////////////////////////////////////////////////
void Emulator::diffInterrupts(uint16_t oldval, uint16_t newval) {
	uint16_t changes = oldval ^ newval;
  //  if (changes & 1) printf("INTCHG external=%d\n", newval & 1);
    //if (changes & 2) printf("INTCHG lowbat=%d\n", newval & 2);
    //if (changes & 4) printf("INTCHG watchdog=%d\n", newval & 4);
    //if (changes & 8) printf("INTCHG mediachg=%d\n", newval & 8);
    //if (changes & 0x10) printf("INTCHG codec=%d\n", newval & 0x10);
    //if (changes & 0x20) printf("INTCHG ext1=%d\n", newval & 0x20);
    //if (changes & 0x40) printf("INTCHG ext2=%d\n", newval & 0x40);
    //if (changes & 0x80) printf("INTCHG ext3=%d\n", newval & 0x80);
    //if (changes & 0x100) printf("INTCHG timer1=%d\n", newval & 0x100);
    //if (changes & 0x200) printf("INTCHG timer2=%d\n", newval & 0x200);
    //if (changes & 0x400) printf("INTCHG rtcmatch=%d\n", newval & 0x400);
    //if (changes & 0x800) printf("INTCHG tick=%d\n", newval & 0x800);
    //if (changes & 0x1000) printf("INTCHG uart1=%d\n", newval & 0x1000);fflush(stdout);
    // if (changes & 0x2000) printf("INTCHG uart2=%d\n", newval & 0x2000);fflush(stdout);
    //if (changes & 0x4000) printf("INTCHG lcd=%d\n", newval & 0x4000);
   // if (changes & 0x8000) printf("INTCHG spi=%d\n", newval & 0x8000);
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::readKeyboard
/// \return
////////////////////////////////////////////////////////////////////////////
uint32_t Emulator::readKeyboard() {
	if (kScan & 8) {
		// Select one keyboard
		return keyboardColumns[kScan & 7];
	} else if (kScan == 0) {
		// Report all columns combined
		uint8_t val = 0;
		for (int i = 0; i < 8; i++)
			val |= keyboardColumns[i];
		return val;
	} else {
		return 0;
	}
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::setKeyboardKey
/// \param key
/// \param value
////////////////////////////////////////////////////////////////////////////
void Emulator::setKeyboardKey(EpocKey key, bool value) {
	int idx = -1;
#define KEY(column, bit) idx = (column << 8) | (1 << bit); break

	switch ((int)key) {
	case EStdKeyDictaphoneRecord: KEY(0, 6);
	case '1':                     KEY(0, 5);
	case '2':                     KEY(0, 4);
	case '3':                     KEY(0, 3);
	case '4':                     KEY(0, 2);
	case '5':                     KEY(0, 1);
	case '6':                     KEY(0, 0);

	case EStdKeyDictaphonePlay:   KEY(1, 6);
	case '7':                     KEY(1, 5);
	case '8':                     KEY(1, 4);
	case '9':                     KEY(1, 3);
	case '0':                     KEY(1, 2);
	case EStdKeyBackspace:        KEY(1, 1);
	case EStdKeySingleQuote:      KEY(1, 0);

	case EStdKeyEscape:           KEY(2, 6);
	case 'Q':                     KEY(2, 5);
	case 'W':                     KEY(2, 4);
	case 'E':                     KEY(2, 3);
	case 'R':                     KEY(2, 2);
	case 'T':                     KEY(2, 1);
	case 'Y':                     KEY(2, 0);

	case EStdKeyMenu:             KEY(3, 6);
	case 'U':                     KEY(3, 5);
	case 'I':                     KEY(3, 4);
	case 'O':                     KEY(3, 3);
	case 'P':                     KEY(3, 2);
	case 'L':                     KEY(3, 1);
	case EStdKeyEnter:            KEY(3, 0);

	case EStdKeyLeftCtrl:         KEY(4, 6);
	case EStdKeyTab:              KEY(4, 5);
	case 'A':                     KEY(4, 4);
	case 'S':                     KEY(4, 3);
	case 'D':                     KEY(4, 2);
	case 'F':                     KEY(4, 1);
	case 'G':                     KEY(4, 0);

	case EStdKeyLeftFunc:         KEY(5, 6);
	case 'H':                     KEY(5, 5);
	case 'J':                     KEY(5, 4);
	case 'K':                     KEY(5, 3);
	case 'M':                     KEY(5, 2);
	case EStdKeyFullStop:         KEY(5, 1);
	case EStdKeyDownArrow:        KEY(5, 0);

	case EStdKeyRightShift:       KEY(6, 6);
	case 'Z':                     KEY(6, 5);
	case 'X':                     KEY(6, 4);
	case 'C':                     KEY(6, 3);
	case 'V':                     KEY(6, 2);
	case 'B':                     KEY(6, 1);
	case 'N':                     KEY(6, 0);

	case EStdKeyLeftShift:        KEY(7, 6);
	case EStdKeyDictaphoneStop:   KEY(7, 5);
	case EStdKeySpace:            KEY(7, 4);
	case EStdKeyUpArrow:          KEY(7, 3);
	case EStdKeyComma:            KEY(7, 2);
	case EStdKeyLeftArrow:        KEY(7, 1);
	case EStdKeyRightArrow:       KEY(7, 0);
	}

	if (idx >= 0) {
		if (value)
			keyboardColumns[idx >> 8] |= (idx & 0xFF);
		else
			keyboardColumns[idx >> 8] &= ~(idx & 0xFF);
	}
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::updateTouchInput
/// \param x
/// \param y
/// \param down
////////////////////////////////////////////////////////////////////////////
void Emulator::updateTouchInput(int32_t x, int32_t y, bool down) {
	pendingInterrupts &= ~(1 << EINT3);
	if (down)
		pendingInterrupts |= (1 << EINT3);
	touchX = x;
	touchY = y;
}


////////////////////////////////////////////////////////////////////////////
/// \brief EMulator::CodecWriteData (to produce sound)
////////////////////////////////////////////////////////////////////////////
void Emulator::CodecWriteData() {

    if (codecValueOutReady) {
        // printf("CodecWriteData()\n");fflush(stdout);
        codecValueOutReady=false;
        coflg &= ~2; // Transmit fifo is empty
        pendingInterrupts |= (1 << CSINT);
    }
}

////////////////////////////////////////////////////////////////////////////
/// \brief playSound
////////////////////////////////////////////////////////////////////////////
void Emulator::playSound() {

    if(!m_outputCodecQueue.isEmpty() && (m_outputCodecQueue.length()>16) && ((confg & 3) == 3)) {

        // printf("playSound()\n");fflush(stdout);
        QByteArray byteArray;
        byteArray.resize(2*m_outputCodecQueue.length());
        int byteArrayPtr=0;
        short value;

        while(!m_outputCodecQueue.isEmpty()) {

            value=alaw2pcm[m_outputCodecQueue.dequeue()];
            byteArray[byteArrayPtr++]=(value) & 0xFF;
            byteArray[byteArrayPtr++]=(value>>8) & 0xFF;
        }

        m_outputDevice->write(byteArray,byteArray.length());
    }
}

////////////////////////////////////////////////////////////////////////////
/* linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
 ////////////////////////////////////////////////////////////////////////////

unsigned char  Emulator::linear2alaw(short pcm_val)	/* 2's complement (16-bit range) */
{
   short	 mask;
   short	 seg;
   unsigned char aval;

   pcm_val = pcm_val >> 3;

   if (pcm_val >= 0) {
      mask = 0xD5;		/* sign (7th) bit = 1 */
   } else {
      mask = 0x55;		/* sign bit = 0 */
      pcm_val = -pcm_val - 1;
   }

   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_aend, 8);

   /* Combine the sign, segment, and quantization bits. */

   if (seg >= 8)		/* out of range, return maximum value. */
      return (unsigned char) (0x7F ^ mask);
   else {
      aval = (unsigned char) seg << SEG_SHIFT;
      if (seg < 2)
     aval |= (pcm_val >> 1) & QUANT_MASK;
      else
     aval |= (pcm_val >> seg) & QUANT_MASK;
      return (aval ^ mask);
   }
}

////////////////////////////////////////////////////////////////////////////
/// \brief EMulator::CodecReadData (to record sound)
////////////////////////////////////////////////////////////////////////////
void Emulator::CodecReadData() {

    // printf("CodecReadData()\n");fflush(stdout);
    coflg &= ~1;    // Receive fifo is no more empty
    pendingInterrupts |= (1 << CSINT);
    codecValueInRead=!m_inputCodecQueue.empty();

}
////////////////////////////////////////////////////////////////////////////
/// \brief recordSound
////////////////////////////////////////////////////////////////////////////
void Emulator::recordSound() {

    if(m_inputDevice!=nullptr) {
       // printf("recordSound()\n");fflush(stdout);

        QByteArray byteArray;
        byteArray=m_inputDevice->readAll();
        // printf("read %d data\n",(int)byteArray.length());fflush(stdout);
        short value;
        short value1;
        short value2;

        for (int i=0;i <byteArray.length()-1;i+=2){

            value1 = byteArray[i] & 0xFF;
            value2 = byteArray[i+1] & 0xFF;
            value = value1 + (value2 << 8);
            m_inputCodecQueue.enqueue(linear2alaw(value));
        }
    }
}


////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::UartReadData
////////////////////////////////////////////////////////////////////////////
void Emulator::UartReadData() {

    if (!m_uartReadQueue.isEmpty()) {
        uart2.UART2DATA_valueIn=m_uartReadQueue.dequeue();
        uart2.UART2DATA_fifoBytesAvailable--;
        printf("%d ",uart2.UART2DATA_fifoBytesAvailable);fflush(stdout);
        uart2.UART2DATA_lastValueRead=false;
        //printf("r(0x%x) ",uart2.UART2DATA_valueIn);fflush(stdout);
        uart2.UART2FLG_value &= ~uart2.AMBA_UARTFR_RXFE;    // Input fifo is no more empty
        uart2.UART2INTR_value|=uart2.PSIONW_UART_RXINT ;    //  set IrqUart2 to ask application to pick this data up
        pendingInterrupts |= (1 << UART2);                  // Set irq
    }
    uart2.UART2DATA_valueInReady=!m_uartReadQueue.isEmpty();
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::UartWriteData
////////////////////////////////////////////////////////////////////////////
void Emulator::UartWriteData() {

    char data;
    if (uart2.UART2DATA_valueOutReady) {
        while(!uart2.m_uart2WriteQueue.isEmpty()){
            data=uart2.m_uart2WriteQueue.dequeue();
            m_serial.write(&data,1);
        }
        uart2.UART2DATA_valueOutReady=false;
        uart2.UART2FLG_value &= ~uart2.AMBA_UARTFR_TXFF; // transmit fifo is empty
    }
}

////////////////////////////////////////////////////////////////////////////
/// \brief Emulator::OpenSerialinterface
////////////////////////////////////////////////////////////////////////////
void Emulator::OpenSerialinterface() {
    qDebug()  << "Serial analysis starts";
   foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {

       m_serial.setPort(info);
       if (info.description()=="com0com - serial port emulator") {
            if (m_serial.open(QIODevice::ReadWrite)) {
                m_serial.setBaudRate(m_serial.Baud115200,QSerialPort::AllDirections);
                m_serial.setDataBits(m_serial.Data8);
                m_serial.setStopBits(m_serial.OneStop);
                m_serial.setParity(m_serial.NoParity);
                m_serial.setFlowControl(m_serial.NoFlowControl);
                m_serial.setRequestToSend(true);
                m_serial.setDataTerminalReady(true);

                qDebug() << "Name : " << info.portName();
                qDebug() << "Description : " << info.description();
                qDebug() << "Manufacturer: " << info.manufacturer();
                qDebug() << "OPEN CONNECTION ON SELECTED DEVICE";
            }
       }
       QObject::connect(&m_serial, &QSerialPort::readyRead, [&]
       {

           //this is called when readyRead() is emitted

           char data;
           printf("\n=>");fflush(stdout);
           while(m_serial.read(&data,1)==1){
                m_uartReadQueue.enqueue(data);
                uart2.UART2DATA_fifoBytesAvailable++;
           }
           uart2.UART2DATA_valueInReady=!m_uartReadQueue.isEmpty();
       });
   }
   qDebug()  << "Serial analysis stops";
}
}


/* Code modified version Copyright (c) 2021 Philippe Cavenel
 *
 * This Source Code Form is subject to the terms of the
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991.
 *
 * The Psion-specific code is copyright (c) 2019 Ash Wolf.
 * The ARM disassembly code is a modified version of the one used in mGBA by endrift.
 * WindEmu is available under the Mozilla Public License 2.0.
*/

#pragma once
#include "emubase.h"
#include "wind_defs.h"
#include "hardware.h"
#include "etna.h"
#include <math.h>


#define PALETTE_SIZE 4096 // 4Kbytes

/* Copied from LIBSNDFILE */
static short alaw2pcm[256] =
{
     5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736,
     7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784,
     2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,
     3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
    22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
    30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
    11008, 10496, 12032, 11520,  8960,  8448,  9984,  9472,
    15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
      344,   328,   376,   360,   280,   264,   312,   296,
      472,   456,   504,   488,   408,   392,   440,   424,
       88,    72,   120,   104,    24,     8,    56,    40,
      216,   200,   248,   232,   152,   136,   184,   168,
     1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184,
     1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,
      688,   656,   752,   720,   560,   528,   624,   592,
      944,   912,  1008,   976,   816,   784,   880,   848,
     -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,
     -7552,  -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,
     -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
     -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
    -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
    -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
    -11008, -10496, -12032, -11520,  -8960,  -8448,  -9984,  -9472,
    -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
      -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
      -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
       -88,    -72,   -120,   -104,    -24,     -8,    -56,    -40,
      -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,
     -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
     -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
      -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
      -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848
};

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
                0x1FF, 0x3FF, 0x7FF, 0xFFF};

static short search(
   short val,
   short *table,
   short size)
{
   short i;

   for (i = 0; i < size; i++) {
      if (val <= *table++)
     return (i);
   }
   return (size);
}

#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */


namespace Windermere {
class AudioTest;
class Emulator : public EmuBase {
public:
    uint8_t ROM[0x1000000];
	uint8_t ROM2[0x40000];
    uint8_t MemoryBlockC0[0x800000];
    uint8_t MemoryBlockC1[0x800000];
    uint8_t MemoryBlockD0[0x800000];
    uint8_t MemoryBlockD1[0x800000];
    enum { MemoryBlockMask = 0x7FFFFF };


    bool configured = false;

    uint16_t    pendingInterrupts = 0;
    uint16_t    interruptMask = 0;
    uint32_t    portValues = 0;
    uint32_t    portDirections = 0;
    uint32_t    lcdAddress = 0;
    uint32_t    lcdControl = 0;
    uint32_t    rtc = 0;

    uint16_t    lastSSIRequest = 0;
    int         ssiReadCounter = 0;

    QSerialPort m_serial;

    // CODEC and Sound
    uint32_t    codr = 0;
    int         codrOutputCounter=0;
    int         codrInputCounter=0;
    uint32_t    confg = 0;
    uint32_t    coflg = 0; // Receive fifo is not empty and transmit fifo is empty
    uint32_t    bzcont = 0;
    bool        buzzerOn=false;
    float       buzzerVolume=0;
    bool        codecValueOutReady=false;
    bool        codecValueInRead=true; // Receive fifo is empty
    bool        m_recordStart = false;
  //  bool        codecLastInValueRead=true;
    QAudioSink  *m_audioOutput;
    QAudioSource *m_audioInput;
    QIODevice   *m_inputDevice = nullptr;
    QIODevice   *m_outputDevice = nullptr;

public:
    void BuzzerStart();

private:

	uint32_t kScan = 0;
	uint8_t keyboardColumns[8] = {0,0,0,0,0,0,0};
	int32_t touchX = 0, touchY = 0;

    Timer tc1, tc2;
    UART uart1, uart2;
	bool halted = false, asleep = false;
    Etna etna;
    QQueue<char> m_uartReadQueue;

    // CODEC
    QSoundEffect effect;
    QQueue<unsigned char> m_outputCodecQueue;
    QQueue<unsigned char> m_inputCodecQueue;

    QQueue<unsigned char> m_inputOuputTest;

    uint32_t getRTC();

    uint32_t readReg8(uint32_t reg);
    uint32_t readReg32(uint32_t reg);
    void writeReg8(uint32_t reg, uint32_t value);
    void writeReg32(uint32_t reg, uint32_t value);


public:
	MaybeU32 readPhysical(uint32_t physAddr, ValueSize valueSize) override;
	bool writePhysical(uint32_t value, uint32_t physAddr, ValueSize valueSize) override;

private:
    void configure();

    const char *identifyObjectCon(uint32_t ptr);
    void fetchStr(uint32_t str, char *buf);
    void fetchName(uint32_t obj, char *buf);
    void fetchProcessFilename(uint32_t obj, char *buf);
    void debugPC(uint32_t pc);
	void diffPorts(uint32_t oldval, uint32_t newval);
	void diffInterrupts(uint16_t oldval, uint16_t newval);
	uint32_t readKeyboard();

public:
	Emulator();
	uint8_t *getROMBuffer() override;
	size_t getROMSize() override;
	void loadROM(uint8_t *buffer, size_t size) override;
    void loadRAMC0(uint8_t *buffer, size_t size) override;
    void loadRAMC1(uint8_t *buffer, size_t size) override;
    void loadRAMD0(uint8_t *buffer, size_t size) override;
    void loadRAMD1(uint8_t *buffer, size_t size) override;
    uint8_t *getRAMC0() override;
    uint8_t *getRAMC1() override;
    uint8_t *getRAMD0() override;
    uint8_t *getRAMD1() override;
    uint32_t getRAMsizeC0() override;
    uint32_t getRAMsizeC1() override;
    uint32_t getRAMsizeD0() override;
    uint32_t getRAMsizeD1() override;
    void executeUntil(int64_t cycles) override;
    int32_t getClockSpeed() const override { return CLOCK_SPEED; }
	const char *getDeviceName() const override;
	int getDigitiserWidth() const override;
	int getDigitiserHeight() const override;
	int getLCDOffsetX() const override;
	int getLCDOffsetY() const override;
	int getLCDWidth() const override;
	int getLCDHeight() const override;
    void readLCDIntoBuffer(uint8_t **lines, bool is32BitOutput) const override;
    void readLCDColorIntoBuffer(uint8_t **lines, bool is32BitOutput) const override;
    void setKeyboardKey(EpocKey key, bool value) override;
	void updateTouchInput(int32_t x, int32_t y, bool down) override;
    void UartReadData() override;
    void UartWriteData() override;
    void OpenSerialinterface() override;
    void CodecReadData() override;
    unsigned char linear2alaw(short pcm_val);
    void CodecWriteData() override;
    void playSound() override;
    void recordSound() override;
};

}

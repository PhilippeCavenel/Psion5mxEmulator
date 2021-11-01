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
#include <stdint.h>

class ARM710;

class Etna {
    uint8_t prom[0x80] = {};
    uint16_t promReadAddress = 0, promReadValue = 0;
    bool promReadActive = false;
    int promAddressBitsReceived = 0;

    uint8_t pendingInterrupts = 0, interruptMask = 0;
    uint8_t wake1 = 0, wake2 = 0;

	ARM710 *owner;

public:
	Etna(ARM710 *owner);

    uint32_t readReg8(uint32_t reg);
    uint32_t readReg32(uint32_t reg);
    void writeReg8(uint32_t reg, uint8_t value);
    void writeReg32(uint32_t reg, uint32_t value);

    // PROM
    void setPromBit0High(); // port B, bit 0
    void setPromBit0Low(); // port B, bit 0
    void setPromBit1High(); // port B, bit 1
};

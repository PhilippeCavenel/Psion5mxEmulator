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
#include "arm710.h"
#include <stdio.h>

struct Timer {
	ARM710 *cpu;

	enum {
		MODE_512KHZ = 1<<3,
		PERIODIC = 1<<6,
		ENABLED = 1<<7
	};
    int64_t nextTickAt;
	uint8_t config;
	uint32_t interval;
	int32_t value;
	int clockSpeed;

	int tickInterval() const {
		return (config & MODE_512KHZ) ? (clockSpeed / 512000) : (clockSpeed / 2000);
	}
	void load(uint32_t lval) {
		interval = lval;
		value = lval;
	}
	void setConfig(uint8_t cval) {
		nextTickAt -= tickInterval();
		config = cval;
		nextTickAt += tickInterval();
	}
    bool tick(int64_t cycles) {
		if (cycles >= nextTickAt) {
			nextTickAt += tickInterval();

			if (config & ENABLED) {
				--value;
				if (value == 0) {
					if (config & PERIODIC)
						value = interval;
					return true;
				}
			}
		}
		return false;
	}
	void dump() {
        /*printf("enabled=%s periodic=%s interval=%d value=%d\n",
			(config & ENABLED) ? "true" : "false",
			(config & PERIODIC) ? "true" : "false",
			interval, value
        );*/
	}
};


enum UartRegs { // source serial_psionw.h
    UART1DATA = 0x600,       // Size 8/11  / 8W 11R / UART1 FIFO Data register 1
    UART1FCR = 0x604,        // Size 32 / RW / Frame control register
    UART1LCR = 0x608,        // Size 32 / RW / Line control register, UBRCR
    UART1CON = 0x60C,        // Size 8 / RW / Port control register
    UART1FLG = 0x610,        // Size 8 / R / Flag register (Read only) UARTFLG
    UART1INT = 0x614,        // Size 32 / 32R 8W / Second level interrupt register
    UART1INTM = 0x618,       // Size 8 / RW / Interrupt mask register
    UART1INTR = 0x61C,       // Size 8 / R / Interrupt raw status register (Read only)
    UART1TEST1 = 0x620,      // Size ? / ?? / Test register
    UART1TEST2 = 0x624,      // Size ? / ?? / Test register
    UART1TEST3 = 0x628,      // Size ? / ?? / Test register
    UART2DATA = 0x700,       // Size 8/11  / 8W 11R / UART2 FIFO Data register 1
    UART2FCR = 0x704,        // Size 32 / RW / Frame control register
    UART2LCR = 0x708,        // Size 32 / RW / Line control register, UBRCR
    UART2CON = 0x70C,        // Size 8 / RW / Port control register
    UART2FLG = 0x710,        // Size 8 / R / Flag register (Read only) UARTFLG
    UART2INT = 0x714,        // Size 32 / 32R 8W / Second level interrupt register
    UART2INTM = 0x718,       // Size 8 / RW / Interrupt mask register
    UART2INTR = 0x71C,       // Size 8 / R / Interrupt raw status register (Read only)
    UART2TEST1 = 0x720,      // Size ? / ?? / Test register
    UART2TEST2 = 0x724,      // Size ? / ?? / Test register
    UART2TEST3 = 0x728       // Size ? / ?? / Test register
};

struct UART { // source serial_psionw.h
	ARM710 *cpu;

	enum {
        PSIONW_UART_RXINT = 1,              // (1 << 0)     /* Rx interrupt */
        PSIONW_UART_TXINT = 2,              // (1 << 1)     /* Tx interrupt */
        PSIONW_UART_MSINT = 4,              // (1 << 2)     /* Modem status interrupt */

        PSIONW_UARTCR_UARTEN = 1,           // (1 << 0)     /* Uart enable */
        PSIONW_UARTCR_SIREN = 2,            // (1 << 1)     /* SiR disable, clear to enable IrDA */
        PSIONW_UARTCR_IRTXM = 4,            // (1 << 2)     /* IrDA Tx mode bit, set for power savings */

        PSIONW_UARTFCR_BREAK = 1,           // (1 << 0)
        PSIONW_UARTFCR_PRTEN = 2,           // (1 << 1)
        PSIONW_UARTFCR_EVENPRT = 4,         // (1 << 2)
        PSIONW_UARTFCR_XSTOP = 8,           // (1 << 3)
        PSIONW_UARTFCR_UFIFOEN = 0x10,      // (1 << 4)
        PSIONW_UARTFCR_WRDLEN_MASK = 0x60,  // (3 << 5)

        PSIONW_UARTFCR_WLEN_5 = 0,          // (0 << 5)
        PSIONW_UARTFCR_WLEN_6 = 0x20,       // (1 << 5)
        PSIONW_UARTFCR_WLEN_7 = 0x40,       // (2 << 5)
        PSIONW_UARTFCR_WLEN_8 = 0x60,       // (3 << 5)

        PSIONW_UARTRSR_FE = 0x100,          // (1 << 8)     /* Frame error */
        PSIONW_UARTRSR_PE = 0x200,          // (1 << 9)     /* Parity error */
        PSIONW_UARTRSR_OE = 0x400,          // (1 << 10)	/* Overrun error */

        AMBA_UARTFR_CTS = 1,                // 0x01         /* Same as Psion CTS */
        AMBA_UARTFR_DSR = 2,                // 0x02         /* Same as Psion DSR */
        AMBA_UARTFR_DCD = 4,                // 0x04         /* Same as Psion DCD */
        AMBA_UARTFR_BUSY = 8,               // 0x08         /* Same as Psion UBUSY */
        AMBA_UARTFR_RXFE = 0x10,            // 0x10         /* Same as Psion URXFE */
        AMBA_UARTFR_TXFF = 0x20,            // 0x20         /* Same as Psion UTXFF */
        AMBA_UARTFR_TMSK = AMBA_UARTFR_TXFF + AMBA_UARTFR_BUSY,
        PSIONW_UARTFR_MODEM_ANY	= (AMBA_UARTFR_DCD|AMBA_UARTFR_DSR|AMBA_UARTFR_CTS)
	};

    // UART 0 information

    uint32_t UART1DATA_valueIn=0;                       // UART1 FIFO Data register In
    uint32_t UART1DATA_valueOut=0;                      // UART1 FIFO Data register Out
    bool    UART1DATA_valueOutReady=false;              // UART1 FIFO Data register has new data out
    bool    UART1DATA_valueInReady=false;               // UART1 FIFO Data register has new data in
    bool    UART1DATA_lastValueRead=true;               // UART1 FIFO Data register last value read
    uint32_t UART1FCR_value=0;                          // Frame control register
    uint32_t UART1LCR_value=0;                          // Line control register, UBRCR
    uint32_t UART1CON_value=0;                          // Port control register
  //  uint32_t UART1FLG_value=AMBA_UARTFR_RXFE |PSIONW_UARTFR_MODEM_ANY; // Flag register (Read only) UARTFLG
  //  uint32_t UART1FLG_value=AMBA_UARTFR_RXFE; // Flag register (Read only) UARTFLG
    uint32_t UART1FLG_value=AMBA_UARTFR_RXFE|AMBA_UARTFR_DSR ; // Flag register (Read only) UARTFLG

    uint32_t UART1INT_value=0;                // Second level interrupt register
    uint32_t UART1INTM_value=0;               // Interrupt mask register
    uint32_t UART1INTR_value=PSIONW_UART_TXINT;               // Interrupt raw status register (Read only)
    uint32_t UART1TEST1_value=0;              // Test register
    uint32_t UART1TEST2_value=0;              // Test register
    uint32_t UART1TEST3_value=0;              // Test register

    // UART 1 information

    uint32_t UART2DATA_valueIn=0;                       // UART2 FIFO Data register In
    uint32_t UART2DATA_valueOut=0;                      // UART2 FIFO Data register Out
    bool    UART2DATA_valueOutReady=false;              // UART2 FIFO Data register has new data out
    bool    UART2DATA_valueInReady=false;               // UART2 FIFO Data register has new data in
    bool    UART2DATA_lastValueRead=true;               // UART1 FIFO Data register last value read
    uint32_t UART2FCR_value=0;                          // Frame control register
    uint32_t UART2LCR_value=0;                          // Line control register, UBRCR
    uint32_t UART2CON_value=0;                          // Port control register
   // uint32_t UART2FLG_value=AMBA_UARTFR_RXFE |PSIONW_UARTFR_MODEM_ANY; // Flag register (Read only) UARTFLG
   // uint32_t UART2FLG_value=AMBA_UARTFR_RXFE; // Flag register (Read only) UARTFLG
    uint32_t UART2FLG_value=AMBA_UARTFR_RXFE |AMBA_UARTFR_DSR ; // Flag register (Read only) UARTFLG

    uint32_t UART2INT_value=0;                // Second level interrupt register
    uint32_t UART2INTM_value=0;               // Interrupt mask register
    uint32_t UART2INTR_value=PSIONW_UART_TXINT;               // Interrupt raw status register (Read only)
    uint32_t UART2TEST1_value=0;              // Test register
    uint32_t UART2TEST2_value=0;              // Test register
    uint32_t UART2TEST3_value=0;              // Test register

    ////////////////////////////////////////////////////////////////////////////
    /// \brief readReg8
    /// \param reg
    /// \return
    ////////////////////////////////////////////////////////////////////////////
    uint32_t readReg8(uint32_t reg) {
        switch(reg & 0xFFF) {
        case UART1DATA: // Return received input data to application
           // printf("Read UART1DATA == 0x%x (%c)\n",UART1DATA_valueIn,UART1DATA_valueIn);fflush(stdout);
           // printf("Set received fifo empty =>");
            writeReg8(UART1FLG,AMBA_UARTFR_RXFE | UART1FLG_value); // Data is read, set received fifo empty
           // printf("Remove interrupt flag to read data =>");
            writeReg8(UART1INTR,UART1INTR_value & ~PSIONW_UART_RXINT);
            UART1DATA_lastValueRead=true;
            return(UART1DATA_valueIn);
        case UART1FCR:
           /* printf("Read UART1FCR == break=%d parityEn=%d evenParity=%d extraStop=%d ufifoEn=%d wrdLen=%d\n",
                UART1FCR_value&1,
                UART1FCR_value&2,
                UART1FCR_value&4,
                UART1FCR_value&8,
                UART1FCR_value&0x10,
                ((UART1FCR_value&0x60)>>5)+5);fflush(stdout);*/
            return(UART1FCR_value);
        case UART1LCR:
           // printf("Read UART1LCR == 0x%x",UART1LCR_value);
           /* switch (UART1LCR_value) {
                case 0x2F : printf(" 9600 bauds\n");fflush(stdout);break;
                case 0x17 : printf(" 19200 bauds\n");fflush(stdout);break;
                case 0x0B : printf(" 38400 bauds\n");fflush(stdout);break;
                case 0x07 : printf(" 57600 bauds\n");fflush(stdout);break;
                case 0x03 : printf(" 115200 bauds\n");fflush(stdout);break;
            }*/
            return(UART1LCR_value);
        case UART1CON:
           // printf("Read UART1CON == 0x%x\n",UART1CON_value);fflush(stdout);
           // if(UART1CON_value & PSIONW_UARTCR_UARTEN) { printf("UART1CON_value=PSIONW_UARTCR_UARTEN\n");fflush(stdout);}
           // if(UART1CON_value & PSIONW_UARTCR_SIREN) { printf("UART1CON_value=PSIONW_UARTCR_SIREN\n");fflush(stdout);}
           // if(UART1CON_value & PSIONW_UARTCR_IRTXM) { printf("UART1CON_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            return(UART1CON_value);
        case  UART1FLG:
           // printf("Read UART1FLG == 0x%x\n",UART1FLG_value);fflush(stdout);
         return(UART1FLG_value);
        case UART1INT:
            //printf("Read UART1INT == 0x%x\n",UART1INT_value);fflush(stdout);
            return(UART1INT_value);
        case UART1INTM:
           // printf("Read UART1INTM == 0x%x\n",UART1INTM_value);fflush(stdout);
           // if(UART1INTM_value & PSIONW_UART_RXINT) { printf("UART1INTM_value=PSIONW_UART_RXINT\n");fflush(stdout);}
           // if(UART1INTM_value & PSIONW_UART_TXINT) { printf("UART1INTM_value=PSIONW_UART_TXINT\n");fflush(stdout);}
           // if(UART1INTM_value & PSIONW_UARTCR_IRTXM) { printf("UART1INTM_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            return(UART1INTM_value);
        case  UART1INTR:
           // printf("Read UART1INTR == 0x%x\n",UART1INTR_value);fflush(stdout);
            return(UART1INTR_value & UART1INTM_value);
        case UART1TEST1:
           // printf("Read UART1TEST1 == 0x%x\n",UART1TEST1_value);fflush(stdout);
            return(UART1TEST1_value);
        case UART1TEST2:
           // printf("Read UART1TEST2 == 0x%x\n",UART1TEST2_value);fflush(stdout);
            return(UART1TEST2_value);
        case UART1TEST3:
           // printf("Read UART1TEST3 == 0x%x\n",UART1TEST3_value);fflush(stdout);
            return(UART1TEST3_value);
        case UART2DATA: // Read input data
           // printf("Read UART2DATA == 0x%x (%c)\n",UART2DATA_valueIn,UART2DATA_valueIn);fflush(stdout);
           // printf("Set received fifo empty =>");
            writeReg8(UART2FLG,AMBA_UARTFR_RXFE | UART2FLG_value); // Data is read, set received fifo empty
           // printf("Remove interrupt flag to read data =>");
            writeReg8(UART2INTR,UART2INTR_value & ~PSIONW_UART_RXINT);
            UART2DATA_lastValueRead=true;
            return(UART2DATA_valueIn);
        case UART2FCR:
           /* printf("Read UART2FCR == break=%d parityEn=%d evenParity=%d extraStop=%d ufifoEn=%d wrdLen=%d\n",
                UART2FCR_value&1,
                UART2FCR_value&2,
                UART2FCR_value&4,
                UART2FCR_value&8,
                UART2FCR_value&0x10,
                ((UART2FCR_value&0x60)>>5)+5);fflush(stdout);*/
            return(UART2FCR_value);
        case  UART2LCR:
           // printf("Read UART2LCR == 0x%x",UART2LCR_value);
           /* switch (UART2LCR_value) {
                case 0x2F : printf(" 9600 bauds\n");fflush(stdout);break;
                case 0x17 : printf(" 19200 bauds\n");fflush(stdout);break;
                case 0x0B : printf(" 38400 bauds\n");fflush(stdout);break;
                case 0x07 : printf(" 57600 bauds\n");fflush(stdout);break;
                case 0x03 : printf(" 115200 bauds\n");fflush(stdout);break;
            }*/
            return(UART2LCR_value);
        case  UART2CON:
          //  printf("Read UART2CON == 0x%x\n",UART2CON_value);fflush(stdout);
          //  if(UART2CON_value & PSIONW_UARTCR_UARTEN) { printf("UART2CON_value=PSIONW_UARTCR_UARTEN\n");fflush(stdout);}
          //  if(UART2CON_value & PSIONW_UARTCR_SIREN) { printf("UART2CON_value=PSIONW_UARTCR_SIREN\n");fflush(stdout);}
          //  if(UART2CON_value & PSIONW_UARTCR_IRTXM) { printf("UART2CON_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            return(UART2CON_value);
        case  UART2FLG:
          //  printf("Read UART2FLG == 0x%x\n",UART2FLG_value);fflush(stdout);
            return(UART2FLG_value);
        case UART2INT:
          //  printf("Read UART2INT == 0x%x\n",UART2INT_value);fflush(stdout);
            return(UART2INT_value);
        case UART2INTM:
          //  printf("Read UART2INTM == 0x%x\n",UART2INTM_value);fflush(stdout);
          //  if(UART2INTM_value & PSIONW_UART_RXINT) { printf("UART2INTM_value=PSIONW_UART_RXINT\n");fflush(stdout);}
          //  if(UART2INTM_value & PSIONW_UART_TXINT) { printf("UART2INTM_value=PSIONW_UART_TXINT\n");fflush(stdout);}
          //  if(UART2INTM_value & PSIONW_UARTCR_IRTXM) { printf("UART2INTM_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            return(UART2INTM_value);
        case UART2INTR:
           // printf("Read UART2INTR == 0x%x\n",UART2INTR_value);fflush(stdout);
            return(UART2INTR_value & UART2INTM_value);
        case UART2TEST1:
           // printf("Read UART2TEST1 == 0x%x\n",UART2TEST1_value);fflush(stdout);
            return(UART2TEST1_value);
        case UART2TEST2:
          //  printf("Read UART2TEST2 == 0x%x\n",UART2TEST2_value);fflush(stdout);
            return(UART2TEST2_value);
        case UART2TEST3:
          //  printf("Read UART2TEST3 == 0x%x\n",UART2TEST3_value);fflush(stdout);
            return(UART2TEST3_value);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    /// \brief writeReg8
    /// \param reg
    /// \param value
    ////////////////////////////////////////////////////////////////////////////
    void writeReg8(uint32_t reg, uint8_t value) {
        switch(reg & 0xFFF) {
        case UART1DATA: // Received output data from application to send
            UART1DATA_valueOut=value;
            UART1DATA_valueOutReady=true;
            UART1FLG_value|=AMBA_UARTFR_TXFF;
            // printf("Write UART1DATA = 0x%x (%c)\n",UART1DATA_valueOut,UART1DATA_valueOut);fflush(stdout);
            break;
        case UART1FCR:
            UART1FCR_value=value;
            /*printf("Write UART1FCR = break=%d parityEn=%d evenParity=%d extraStop=%d ufifoEn=%d wrdLen=%d\n",
                UART1FCR_value&1,
                UART1FCR_value&2,
                UART1FCR_value&4,
                UART1FCR_value&8,
                UART1FCR_value&0x10,
                ((UART1FCR_value&0x60)>>5)+5);fflush(stdout);*/
            break;
        case UART1LCR:
            UART1LCR_value=value;
           // printf("Write UART1LCR =0x%x\n",UART1LCR_value);
           /* switch (UART1LCR_value) {
                case 0x2F : printf(" 9600 bauds\n");fflush(stdout);break;
                case 0x17 : printf(" 19200 bauds\n");fflush(stdout);break;
                case 0x0B : printf(" 38400 bauds\n");fflush(stdout);break;
                case 0x07 : printf(" 57600 bauds\n");fflush(stdout);break;
                case 0x03 : printf(" 115200 bauds\n");fflush(stdout);break;
            }*/
            break;
        case UART1CON:
            UART1CON_value=value;
           // printf("Write UART1CON = 0x%x\n",UART1CON_value);fflush(stdout);
           // if(UART1CON_value & PSIONW_UARTCR_UARTEN) { printf("UART1CON_value=PSIONW_UARTCR_UARTEN\n");fflush(stdout);}
           // if(UART1CON_value & PSIONW_UARTCR_SIREN) { printf("UART1CON_value=PSIONW_UARTCR_SIREN\n");fflush(stdout);}
           // if(UART1CON_value & PSIONW_UARTCR_IRTXM) { printf("UART1CON_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            break;
        case  UART1FLG:
            UART1FLG_value=value;
           // printf("Write UART1FLG = 0x%x\n",UART1FLG_value);fflush(stdout);
            break;
        case UART1INT:
            UART1INT_value=value;
           // printf("Write UART1INT = 0x%x\n",UART1INT_value);fflush(stdout);
            break;
        case UART1INTM:
            UART1INTM_value=value;
           // printf("read UART1INTM = 0x%x\n",UART1INTM_value);fflush(stdout);
           // if(UART1INTM_value & PSIONW_UART_RXINT) { printf("UART1INTM_value=PSIONW_UART_RXINT\n");fflush(stdout);}
           // if(UART1INTM_value & PSIONW_UART_TXINT) { printf("UART1INTM_value=PSIONW_UART_TXINT\n");fflush(stdout);}
           // if(UART1INTM_value & PSIONW_UARTCR_IRTXM) { printf("UART1INTM_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            break;
        case  UART1INTR:
            UART1INTR_value=value;
           // printf("Write UART1INTR = 0x%x\n",UART1INTR_value);fflush(stdout);
            break;
        case UART1TEST1:
            UART1TEST1_value=value;
           // printf("Write UART1TEST1 = 0x%x\n",UART1TEST1_value);fflush(stdout);
            break;
        case UART1TEST2:
            UART1TEST2_value=value;
           // printf("Write UART1TEST2 = 0x%x\n",UART1TEST2_value);fflush(stdout);
            break;
        case UART1TEST3:
            UART1TEST3_value=value;
          //  printf("read UART1TEST3 = 0x%x\n",UART1TEST3_value);fflush(stdout);
            break;
        case UART2DATA: // Received output data from application to send
            UART2DATA_valueOut=value;
            UART2DATA_valueOutReady=true;
            UART2FLG_value|=AMBA_UARTFR_TXFF;
            // printf("Write UART2DATA = 0x%x (%c)\n",UART2DATA_valueOut,UART2DATA_valueOut);fflush(stdout);
            break;
        case UART2FCR:
            UART2FCR_value=value;
           /* printf("Write UART2FCR = break=%d parityEn=%d evenParity=%d extraStop=%d ufifoEn=%d wrdLen=%d\n",
                UART2FCR_value&1,
                UART2FCR_value&2,
                UART2FCR_value&4,
                UART2FCR_value&8,
                UART2FCR_value&0x10,
                ((UART2FCR_value&0x60)>>5)+5);fflush(stdout);*/
            break;
        case  UART2LCR:
            UART2LCR_value=value;
          //  printf("Write UART2LCR = 0x%x\n",UART2LCR_value);
          /*  switch (UART2LCR_value) {
                case 0x2F : printf(" 9600 bauds\n");fflush(stdout);break;
                case 0x17 : printf(" 19200 bauds\n");fflush(stdout);break;
                case 0x0B : printf(" 38400 bauds\n");fflush(stdout);break;
                case 0x07 : printf(" 57600 bauds\n");fflush(stdout);break;
                case 0x03 : printf(" 115200 bauds\n");fflush(stdout);break;
            }*/
            break;
        case  UART2CON:
            UART2CON_value=value;
           // printf("Write UART2CON = 0x%x\n",UART2CON_value);fflush(stdout);
           // if(UART2CON_value & PSIONW_UARTCR_UARTEN) { printf("UART2CON_value=PSIONW_UARTCR_UARTEN\n");fflush(stdout);}
           // if(UART2CON_value & PSIONW_UARTCR_SIREN) { printf("UART2CON_value=PSIONW_UARTCR_SIREN\n");fflush(stdout);}
           // if(UART2CON_value & PSIONW_UARTCR_IRTXM) { printf("UART2CON_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            break;
        case UART2FLG:
            UART2FLG_value=value;
           // printf("Write UART2FLG = 0x%x\n",UART2FLG_value);fflush(stdout);
            break;
        case UART2INT:
            UART2INT_value=value;
           // printf("Write UART2INT = 0x%x\n",UART2INT_value);fflush(stdout);
            break;
        case UART2INTM:
            UART2INTM_value=value;
           // printf("Write UART2INTM = 0x%x\n",UART2INTM_value);fflush(stdout);
           // if(UART2INTM_value & PSIONW_UART_RXINT) { printf("UART2INTM_value=PSIONW_UART_RXINT\n");fflush(stdout);}
           // if(UART2INTM_value & PSIONW_UART_TXINT) { printf("UART2INTM_value=PSIONW_UART_TXINT\n");fflush(stdout);}
           // if(UART2INTM_value & PSIONW_UARTCR_IRTXM) { printf("UART2INTM_value=PSIONW_UARTCR_IRTXM\n");fflush(stdout);}
            break;
        case UART2INTR:
            UART2INTR_value=value;
           // printf("Write UART2INTR = 0x%x\n",UART2INTR_value);fflush(stdout);
            break;
        case UART2TEST1:
            UART2TEST1_value=value;
           // printf("Write UART2TEST1 = 0x%x\n",UART2TEST1_value);fflush(stdout);
            break;
        case UART2TEST2:
            UART2TEST2_value=value;
           // printf("Write UART2TEST2 = 0x%x\n",UART2TEST2_value);fflush(stdout);
            break;
        case UART2TEST3:
            UART2TEST3_value=value;
           // printf("Write UART2TEST3 = 0x%x\n",UART2TEST3_value);fflush(stdout);
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    /// \brief readReg32
    /// \param reg
    /// \return
    ////////////////////////////////////////////////////////////////////////////
    uint32_t readReg32(uint32_t reg) {
       // printf("Mode 32 bits : ");fflush(stdout);
        return readReg8(reg);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// \brief writeReg32
    /// \param reg
    /// \param value
    ////////////////////////////////////////////////////////////////////////////
    void writeReg32(uint32_t reg, uint32_t value) {
       // printf("Mode 32 bits : ");fflush(stdout);
        writeReg8(reg,value);
    }

};

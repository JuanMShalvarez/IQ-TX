/* ADRF6755 power-up sequence (datasheet Rev. B, p.35) - LO = 1000 MHz
 * STM32G474, HAL, I2C2 (PA8 = SDA, PA9 = SCL), REFIN = 80 MHz (Y1)
 *
 * Derived values:
 *   LO 1000 MHz -> Table 6: 577.5 < LO <= 1155 -> RFDIV = 001 (/2), CR27[4] = 0
 *   PFD = 80 MHz x (1+D)/(R x (1+T)) with D=0, T=1, R=1 -> 40 MHz
 *   N = 2^RFDIV x LO / fPFD = 2 x 1000 / 40 = 50 -> INT = 50 (0x032), FRAC = 0
 *   CR28 = 0x09 (bit3 = 1, RFDIV = 001)
 *   CR10 = 0x41 (R/2 divider on, R = 1)    CR5 = 0x10 (enable R-div and /2)
 *   CR7  = 0x00 (MUXOUT tristate, INT[11:8] = 0)   CR6 = 0x32
 *   CR3..CR0 = 0x04, 0x00, 0x00, 0x00
 *
 * I2C address (7-bit): CS high = 0x60, CS low = 0x40. HAL wants it shifted left.
 * CS (pin 27) must be tied to 3.3 V or GND.
 */
#include "main.h"

extern I2C_HandleTypeDef hi2c2;

#define ADRF_ADDR   (0x40 << 1)   /* CS = low. Use (0x60 << 1) if CS = high */

static HAL_StatusTypeDef adrf_wr(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(&hi2c2, ADRF_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 10);
}

typedef struct { uint8_t reg; uint8_t val; } reg_t;

/* Steps 1-31, in datasheet order */
static const reg_t init_seq[] = {
    {0x1E, 0x00},  /*  1 CR30 attenuator 0 dB                          */
    {0x1D, 0x80},  /*  2 CR29 modulator powered down                   */
    {0x1C, 0x09},  /*  3 CR28 RFDIV = /2, bit3 = 1                     */
    {0x1B, 0x00},  /*  4 CR27 bit4 = 0 (LO 577.5-1155 MHz)             */
    {0x1A, 0x00},  /*  5 CR26                                          */
    {0x19, 0x64},  /*  6 CR25 autocal timer, PFD = 40 MHz              */
    {0x18, 0x18},  /*  7 CR24 autocal enabled                          */
    {0x17, 0x70},  /*  8 CR23 lock detect enable                       */
    {0x16, 0x80},  /*  9 CR22                                          */
    {0x15, 0x00},  /* 10 CR21                                          */
    {0x14, 0x00},  /* 11 CR20                                          */
    {0x13, 0x80},  /* 12 CR19                                          */
    {0x12, 0x60},  /* 13 CR18                                          */
    {0x11, 0x00},  /* 14 CR17                                          */
    {0x10, 0x00},  /* 15 CR16                                          */
    {0x0F, 0x00},  /* 16 CR15                                          */
    {0x0E, 0x80},  /* 17 CR14 stop LO when TXDIS = 1                   */
    {0x0D, 0xE8},  /* 18 CR13 lock detect timing, PFD = 40 MHz         */
    {0x0C, 0x18},  /* 19 CR12 power up PLL                             */
    {0x0B, 0x00},  /* 20 CR11                                          */
    {0x0A, 0x41},  /* 21 CR10 R/2 on, R = 1, doubler off               */
    {0x09, 0xF0},  /* 22 CR9  CP current 5 mA                          */
    {0x08, 0x00},  /* 23 CR8                                           */
    {0x07, 0x00},  /* 24 CR7  INT[11:8] = 0, MUXOUT tristate           */
    {0x06, 0x32},  /* 25 CR6  INT[7:0] = 50                            */
    {0x05, 0x10},  /* 26 CR5  enable R-divider and /2                  */
    {0x04, 0x01},  /* 27 CR4                                           */
    {0x03, 0x04},  /* 28 CR3  FRAC[24] = 0                             */
    {0x02, 0x00},  /* 29 CR2  FRAC[23:16]                              */
    {0x01, 0x00},  /* 30 CR1  FRAC[15:8]                               */
    {0x00, 0x00},  /* 31 CR0  FRAC[7:0] - last, triggers acquisition   */
};

HAL_StatusTypeDef adrf6755_init(void)
{
    HAL_StatusTypeDef st;

    for (uint32_t i = 0; i < sizeof(init_seq) / sizeof(init_seq[0]); i++) {
        st = adrf_wr(init_seq[i].reg, init_seq[i].val);
        if (st != HAL_OK) return st;
    }

    /* 32 CR27 bit4 per Table 6 (unchanged, 0) */
    st = adrf_wr(0x1B, 0x00);
    if (st != HAL_OK) return st;

    /* 33 Wait for lock: LDET is not routed to the MCU, so wait >170 us */
    HAL_Delay(1);

    /* 34 Power up modulator */
    return adrf_wr(0x1D, 0x81);
}

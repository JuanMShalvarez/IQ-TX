#include "adrf6755.h"
#include <math.h>

extern I2C_HandleTypeDef hi2c2;

#define ADRF6755_PFD_MHZ       	(79.9952/2.0)
#define ADRF6755_FRAC_MOD       33554432.0      /* 2^25 */

/* Error indication: change this one line if blinkProgramLed() has another signature */
#define ADRF6755_ERROR_BLINK()  blinkProgramLed(50)

/* Register addresses */
#define REG_CR0     0x00
#define REG_CR1     0x01
#define REG_CR2     0x02
#define REG_CR3     0x03
#define REG_CR6     0x06
#define REG_CR7     0x07
#define REG_CR27    0x1B
#define REG_CR28    0x1C
#define REG_CR30    0x1E

static HAL_StatusTypeDef wr(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(&hi2c2, ADRF6755_I2C_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, &val, 1, 10);
}

HAL_StatusTypeDef adrf6755_setFrequency(double MHz)
{
    /* Written so that NaN also fails the check */
    if (!(MHz >= ADRF6755_FREQ_MIN_MHZ && MHz <= ADRF6755_FREQ_MAX_MHZ)) {
        ADRF6755_ERROR_BLINK();
        return HAL_ERROR;
    }

    /* Table 6: RFDIV and CR27[4] */
    uint8_t rfdiv;
    uint8_t cr27_b4;
    if      (MHz > 1155.0)   { rfdiv = 0; cr27_b4 = 1; }   /* /1  */
    else if (MHz > 577.5)    { rfdiv = 1; cr27_b4 = 0; }   /* /2  */
    else if (MHz > 288.75)   { rfdiv = 2; cr27_b4 = 0; }   /* /4  */
    else if (MHz > 144.375)  { rfdiv = 3; cr27_b4 = 0; }   /* /8  */
    else                     { rfdiv = 4; cr27_b4 = 0; }   /* /16 */

    /* Equation 4: N = 2^RFDIV * LO / fPFD ; N = INT + FRAC / 2^25 */
    double   n    = (double)(1u << rfdiv) * MHz / ADRF6755_PFD_MHZ;
    uint32_t intp = (uint32_t)n;
    uint32_t frac = (uint32_t)((n - (double)intp) * ADRF6755_FRAC_MOD + 0.5);
    if (frac >= (uint32_t)ADRF6755_FRAC_MOD) {              /* rounding carry */
        frac = 0;
        intp++;
    }

    HAL_StatusTypeDef st;

    /* Double-buffered registers, latched by the CR0 write */
    if ((st = wr(REG_CR28, (uint8_t)(0x08 | rfdiv)))              != HAL_OK) return st;
    if ((st = wr(REG_CR7,  (uint8_t)((intp >> 8) & 0x0F)))        != HAL_OK) return st;
    if ((st = wr(REG_CR6,  (uint8_t)(intp & 0xFF)))               != HAL_OK) return st;
    if ((st = wr(REG_CR3,  (uint8_t)(0x04 | ((frac >> 24) & 1)))) != HAL_OK) return st;
    if ((st = wr(REG_CR2,  (uint8_t)((frac >> 16) & 0xFF)))       != HAL_OK) return st;
    if ((st = wr(REG_CR1,  (uint8_t)((frac >> 8) & 0xFF)))        != HAL_OK) return st;
    if ((st = wr(REG_CR0,  (uint8_t)(frac & 0xFF)))               != HAL_OK) return st;  /* starts acquisition */

    /* CR27[4] last, after CR0 (datasheet: Programming the Correct LO Frequency) */
    return wr(REG_CR27, (uint8_t)(cr27_b4 << 4));
}

HAL_StatusTypeDef adrf6755_setAttenuation(int att)
{
    if (att < ADRF6755_ATT_MIN_DB || att > ADRF6755_ATT_MAX_DB) {
        ADRF6755_ERROR_BLINK();
        return HAL_ERROR;
    }

    /* Table 27: 0..31 dB -> 0x00..0x1F ; 32..47 dB -> 0x30..0x3F */
    uint8_t code = (att < 32) ? (uint8_t)att : (uint8_t)(0x30 | (att - 32));

    return wr(REG_CR30, code);
}

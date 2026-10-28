#ifndef ADRF6755_H
#define ADRF6755_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 8-bit (shifted) I2C address. CS = GND -> 0x40, CS = 3.3 V -> 0x60.
 * Must match the address used in adrf6755_init.c. */
#ifndef ADRF6755_I2C_ADDR
#define ADRF6755_I2C_ADDR   (0x40 << 1)
#endif

#define ADRF6755_FREQ_MIN_MHZ   100.0
#define ADRF6755_FREQ_MAX_MHZ   2400.0
#define ADRF6755_ATT_MIN_DB     0
#define ADRF6755_ATT_MAX_DB     47

/* Power-up sequence, LO = 1000 MHz, modulator on (unchanged, in adrf6755_init.c) */
HAL_StatusTypeDef adrf6755_init(void);

/* Set LO frequency in MHz (100.0 .. 2400.0).
 * Assumes PFD = 40 MHz (as set by adrf6755_init).
 * Out of range: blinks the program LED with 50 ms delays and returns HAL_ERROR. */
HAL_StatusTypeDef adrf6755_setFrequency(double MHz);

/* Set output attenuation in dB (0 .. 47, 1 dB steps).
 * Out of range: blinks the program LED with 50 ms delays and returns HAL_ERROR. */
HAL_StatusTypeDef adrf6755_setAttenuation(int att);

#ifdef __cplusplus
}
#endif

#endif /* ADRF6755_H */

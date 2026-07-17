/*
 * ssd1306.h
 *
 * Minimal SSD1306 128x32 OLED driver over I2C, for STM32 HAL.
 * No external dependencies beyond the HAL I2C driver already in use
 * for the PN532 (same I2C1 bus, different device address).
 */

#ifndef SSD1306_H_
#define SSD1306_H_

#include <stdint.h>

#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          32

/* 7-bit I2C address 0x3C, shifted left by 1 for HAL (8-bit address) */
#define SSD1306_I2C_ADDRESS     0x78

/**
 * @brief  Initialize the SSD1306 display.
 * @note   Call once after I2C is initialized. Blocks for a short delay
 *         while the display powers up.
 */
void SSD1306_Init(void);

/**
 * @brief  Clear the internal framebuffer (does not push to the display yet).
 */
void SSD1306_Clear(void);

/**
 * @brief  Push the internal framebuffer to the physical display.
 * @note   Call this after Clear/WriteString to actually show changes.
 */
void SSD1306_UpdateScreen(void);

/**
 * @brief  Set the cursor position for the next WriteString/WriteChar call.
 * @param  x  Column, 0 ~ 127
 * @param  y  Row, 0 ~ 31
 */
void SSD1306_SetCursor(uint8_t x, uint8_t y);

/**
 * @brief  Write a single character into the framebuffer at the current cursor.
 * @note   Advances the cursor. Supports ASCII 32-126.
 * @param  ch  Character to draw
 */
void SSD1306_WriteChar(char ch);

/**
 * @brief  Write a null-terminated string into the framebuffer at the current cursor.
 * @note   Does not wrap automatically; keep strings short enough for 128px width
 *         (roughly 21 characters at 6px per character).
 * @param  str  String to draw
 */
void SSD1306_WriteString(const char *str);

/**
 * @brief  Clear a single horizontal page (8 pixels tall) in the framebuffer.
 * @note   Does not push to the display; call SSD1306_UpdateScreen() after.
 * @param  page  Page index: 0 = row 0-7, 1 = row 8-15, 2 = row 16-23, 3 = row 24-31
 */
void SSD1306_ClearLine(uint8_t page);

#endif /* SSD1306_H_ */

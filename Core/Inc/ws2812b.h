/*
 * ws2812b.h
 *
 *  Created on: Jul 17, 2026
 *      Author: Kostiantyn
 *
 * Uses TIM17 CH1 with DMA in PWM mode
 *
 * Timing at 32MHz, Prescaler=0, ARR=40:
 *   Timer frequency: 32MHz / 40 = 800kHz (1.25us per period)
 *   Bit 1: CCR = 27 (~67% duty = 0.84us high)
 *   Bit 0: CCR = 13 (~33% duty = 0.41us high)
 *   Reset: 50+ zero values = >62.5us low
 *
 * NOTE: When porting to STM32F103 (Blue Pill, 72MHz):
 *   ARR  = 90  (72MHz / 90 = 800kHz)
 *   BIT1 = 60  (~67%)
 *   BIT0 = 29  (~33%)
 */

#ifndef INC_WS2812B_H_
#define INC_WS2812B_H_

#include <stdint.h>

/* Number of LEDs in the strip */
#define WS2812B_NUM_LEDS        60

/* PWM values for bit encoding */
#define WS2812B_BIT1            27   /* ~67% duty cycle = logical 1 */
#define WS2812B_BIT0            13   /* ~33% duty cycle = logical 0 */
#define WS2812B_RESET_PERIODS   50   /* >50 periods = >62.5us reset pulse */

/* DMA buffer size: 24 bits per LED + reset */
#define WS2812B_BUF_SIZE        (WS2812B_NUM_LEDS * 24 + WS2812B_RESET_PERIODS)

/**
 * @brief  Initialize the WS2812B driver.
 * @note   Call after MX_TIM17_Init() and MX_DMA_Init().
 *         Clears all LEDs to off.
 */
void WS2812B_Init(void);

/**
 * @brief  Set the color of a single LED in the framebuffer.
 * @note   Call WS2812B_Show() to push changes to the strip.
 * @param  index  LED index, 0 ~ WS2812B_NUM_LEDS-1
 * @param  r      Red component,   0~255
 * @param  g      Green component, 0~255
 * @param  b      Blue component,  0~255
 */
void WS2812B_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief  Set all LEDs to the same color.
 * @note   Call WS2812B_Show() after to apply.
 * @param  r  Red,   0~255
 * @param  g  Green, 0~255
 * @param  b  Blue,  0~255
 */
void WS2812B_Fill(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief  Turn off all LEDs.
 * @note   Call WS2812B_Show() after to apply.
 */
void WS2812B_Clear(void);

/**
 * @brief  Push the framebuffer to the physical LED strip via DMA.
 * @note   Non-blocking — DMA runs in background.
 *         Do not call again until the previous transfer completes.
 *         Use WS2812B_IsReady() to check.
 */
void WS2812B_Show(void);

/**
 * @brief  Check if the DMA transfer has completed and strip is ready
 *         for a new Show() call.
 * @return 1 if ready, 0 if transfer still in progress
 */
uint8_t WS2812B_IsReady(void);

#endif /* INC_WS2812B_H_ */

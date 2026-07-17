/*
 * ws2812b.c
 *
 *  Created on: Jul 17, 2026
 *      Author: Kostiantyn
 *
 *  Uses TIM17 CH1 with DMA in PWM mode
 */

#include "ws2812b.h"
#include "main.h"

extern TIM_HandleTypeDef htim17;

/* DMA PWM buffer — uint16_t matches Half Word DMA setting */
static uint16_t dma_buf[WS2812B_BUF_SIZE];

/* RGB framebuffer */
static uint8_t fb_r[WS2812B_NUM_LEDS];
static uint8_t fb_g[WS2812B_NUM_LEDS];
static uint8_t fb_b[WS2812B_NUM_LEDS];

static volatile uint8_t transfer_complete = 1;

/**
 * @brief  Pack one byte into 8 consecutive PWM values in the DMA buffer.
 * @param  buf    Pointer to start of 8 uint16_t slots in dma_buf
 * @param  byte   Byte to encode, MSB first
 */
static void encode_byte(uint16_t *buf, uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        buf[7 - i] = (byte >> i) & 0x01 ? WS2812B_BIT1 : WS2812B_BIT0;
    }
}

/**
 * @brief  Rebuild the DMA buffer from the RGB framebuffer.
 * @note   WS2812B expects GRB order, MSB first.
 */
static void rebuild_dma_buf(void)
{
    for (uint16_t i = 0; i < WS2812B_NUM_LEDS; i++)
    {
        uint16_t *p = &dma_buf[i * 24];
        encode_byte(p,      fb_g[i]); /* Green first */
        encode_byte(p + 8,  fb_r[i]); /* then Red   */
        encode_byte(p + 16, fb_b[i]); /* then Blue  */
    }

    /* Reset pulse — 50 periods of zero duty cycle */
    for (uint16_t i = WS2812B_NUM_LEDS * 24; i < WS2812B_BUF_SIZE; i++)
    {
        dma_buf[i] = 0;
    }
}

void WS2812B_Init(void)
{
    WS2812B_Clear();
    rebuild_dma_buf();
    transfer_complete = 1;
}

void WS2812B_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= WS2812B_NUM_LEDS) return;
    fb_r[index] = r;
    fb_g[index] = g;
    fb_b[index] = b;
}

void WS2812B_Fill(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = 0; i < WS2812B_NUM_LEDS; i++)
    {
        fb_r[i] = r;
        fb_g[i] = g;
        fb_b[i] = b;
    }
}

void WS2812B_Clear(void)
{
    for (uint16_t i = 0; i < WS2812B_NUM_LEDS; i++)
    {
        fb_r[i] = 0;
        fb_g[i] = 0;
        fb_b[i] = 0;
    }
}

void WS2812B_Show(void)
{
    if (!transfer_complete) return; /* Previous transfer still running */

    transfer_complete = 0;
    rebuild_dma_buf();
    HAL_TIM_PWM_Start_DMA(&htim17, TIM_CHANNEL_1, (uint32_t *)dma_buf, WS2812B_BUF_SIZE);
}

uint8_t WS2812B_IsReady(void)
{
    return transfer_complete;
}

/**
 * @brief  TIM PWM Pulse Finished callback — called by HAL when DMA completes.
 * @note   Stop the timer to avoid re-triggering the DMA continuously.
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM17)
    {
        HAL_TIM_PWM_Stop_DMA(&htim17, TIM_CHANNEL_1);
        transfer_complete = 1;
    }
}

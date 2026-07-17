/*
 * stepper.c
 *
 *  Created on: Jul 15, 2026
 *      Author: Kostiantyn
* 28BYJ-48 stepper motor half-step driver for STM32 HAL.
 *
 * Wiring:
 *   ULN2003 IN1 → IN1_MOTOR_GPIO_Port / IN1_MOTOR_Pin  (set User Label in CubeMX)
 *   ULN2003 IN2 → IN2_MOTOR_GPIO_Port / IN2_MOTOR_Pin
 *   ULN2003 IN3 → IN3_MOTOR_GPIO_Port / IN3_MOTOR_Pin
 *   ULN2003 IN4 → IN4_MOTOR_GPIO_Port / IN4_MOTOR_Pin
 *   ULN2003 VCC → 5V
 *   ULN2003 GND → GND (shared with STM32)
 *
 * Half-step sequence (8 steps per electrical cycle):
 *   Step | IN1 IN2 IN3 IN4
 *   -----+----------------
 *     0  |  1   0   0   0
 *     1  |  1   1   0   0
 *     2  |  0   1   0   0
 *     3  |  0   1   1   0
 *     4  |  0   0   1   0
 *     5  |  0   0   1   1
 *     6  |  0   0   0   1
 *     7  |  1   0   0   1
 *
 * Reversing direction: iterate the sequence backwards.
 *
 * Speed control: adjust delay_ms passed to stepper_Run(),
 * or the osDelay() between stepper_Step() calls in your own loop.
 * Minimum reliable delay is ~1ms; typical slow rotation is 3-5ms per half-step.
 */

#ifndef SRC_STEPPER_C_
#define SRC_STEPPER_C_

#include "stepper.h"
#include "main.h"

/* Half-step sequence table: each row is one step, columns are IN1..IN4 */
static const uint8_t half_step_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1},
};

static volatile bool running = false;
static int8_t current_step = 0;

static void stepper_ApplyStep(int8_t step)
{
    const uint8_t *s = half_step_sequence[step];
    HAL_GPIO_WritePin(IN1_MOTOR_GPIO_Port, IN1_MOTOR_Pin, s[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_MOTOR_GPIO_Port, IN2_MOTOR_Pin, s[1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN3_MOTOR_GPIO_Port, IN3_MOTOR_Pin, s[2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_MOTOR_GPIO_Port, IN4_MOTOR_Pin, s[3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void stepper_Init(void)
{
    stepper_Stop();
}

void stepper_Step(bool forward)
{
    if (forward) {
        current_step = (current_step + 1) % 8;
    } else {
        current_step = (current_step + 7) % 8; /* +7 mod 8 = -1 mod 8, avoids negative */
    }

    stepper_ApplyStep(current_step);
}

void stepper_Run(bool forward, uint32_t delay_ms)
{
    running = true;

    while (running) {
        stepper_Step(forward);
        osDelay(delay_ms);
    }
}

void stepper_Stop(void)
{
    running = false;

    /* De-energize all coils to save power and prevent heating */
    HAL_GPIO_WritePin(IN1_MOTOR_GPIO_Port, IN1_MOTOR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_MOTOR_GPIO_Port, IN2_MOTOR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN3_MOTOR_GPIO_Port, IN3_MOTOR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_MOTOR_GPIO_Port, IN4_MOTOR_Pin, GPIO_PIN_RESET);
}

#endif /* SRC_STEPPER_C_ */

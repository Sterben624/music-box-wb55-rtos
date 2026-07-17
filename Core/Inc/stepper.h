/*
 * stepper.h
 *
 *  Created on: Jul 15, 2026
 *      Author: Kostiantyn
 *
 * 28BYJ-48 stepper motor half-step driver for STM32 HAL.
 * Uses 4 GPIO output pins connected to ULN2003 IN1-IN4.
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Initialize the stepper driver.
 * @note   Call once after MX_GPIO_Init().
 *         All motor pins are set LOW (motor de-energized).
 */
void stepper_Init(void);

/**
 * @brief  Advance the motor by one half-step.
 * @param  forward  true = clockwise, false = counter-clockwise
 */
void stepper_Step(bool forward);

/**
 * @brief  Rotate the motor continuously.
 * @note   Blocking call — runs indefinitely until stepper_Stop() is called
 *         from an interrupt or another task. For non-blocking use, call
 *         stepper_Step() in your main loop instead.
 * @param  forward      true = clockwise, false = counter-clockwise
 * @param  delay_ms     Delay between half-steps in milliseconds.
 *                      Minimum ~1ms; below that the motor may skip steps.
 *                      Typical value for slow rotation: 3-5ms.
 */
void stepper_Run(bool forward, uint32_t delay_ms);

/**
 * @brief  Stop the motor and de-energize all coils.
 * @note   Call this when the motor is not needed to save power
 *         and prevent heating. ULN2003 keeps coils energized at
 *         the last step if not explicitly stopped.
 */
void stepper_Stop(void);

#endif /* INC_STEPPER_H_ */

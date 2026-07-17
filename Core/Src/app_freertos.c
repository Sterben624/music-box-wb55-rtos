/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : app_freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pn532_stm32f1.h"
#include "df_player.h"
#include "ssd1306.h"
#include "stepper.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TAG_1 0x53c49bf6630001
#define TAG_2 0x53539af6630001
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
typedef struct {
	uint64_t uid;
	int8_t   track_number;
} rfid_msg_t;

typedef struct {
	char    track_name[32];
	uint8_t volume;
	uint8_t is_paused;
	uint8_t motor_running;
} display_msg_t;

extern PN532 pn532;
uint8_t volume = 5;

osMessageQId RFID_QueueHandle;
osMessageQId Encoder_QueueHandle;
osMessageQId Display_QueueHandle;

SemaphoreHandle_t motor_start_semHandle;
SemaphoreHandle_t motor_stop_semHandle;

extern TIM_HandleTypeDef htim2;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId TaskMotorHandle;
osThreadId TaskRFIDHandle;
osThreadId TaskEncoderHandle;
osThreadId TaskPlayerHandle;
osThreadId TaskDisplayHandle;
osMutexId i2c_mutexHandle;
osSemaphoreId pause_semHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTaskMotor(void const * argument);
void StartTaskRFID(void const * argument);
void StartTaskEncoder(void const * argument);
void StartTaskPlayer(void const * argument);
void StartTaskDisplay(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of i2c_mutex */
  osMutexDef(i2c_mutex);
  i2c_mutexHandle = osMutexCreate(osMutex(i2c_mutex));

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of pause_sem */
  osSemaphoreDef(pause_sem);
  pause_semHandle = osSemaphoreCreate(osSemaphore(pause_sem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	motor_start_semHandle = xSemaphoreCreateBinary();
	motor_stop_semHandle = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	RFID_QueueHandle = xQueueCreate(1, sizeof(rfid_msg_t));
	Encoder_QueueHandle = xQueueCreate(4, sizeof(int8_t));
	Display_QueueHandle = xQueueCreate(2, sizeof(display_msg_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of TaskMotor */
  osThreadDef(TaskMotor, StartTaskMotor, osPriorityAboveNormal, 0, 128);
  TaskMotorHandle = osThreadCreate(osThread(TaskMotor), NULL);

  /* definition and creation of TaskRFID */
  osThreadDef(TaskRFID, StartTaskRFID, osPriorityNormal, 0, 256);
  TaskRFIDHandle = osThreadCreate(osThread(TaskRFID), NULL);

  /* definition and creation of TaskEncoder */
  osThreadDef(TaskEncoder, StartTaskEncoder, osPriorityNormal, 0, 128);
  TaskEncoderHandle = osThreadCreate(osThread(TaskEncoder), NULL);

  /* definition and creation of TaskPlayer */
  osThreadDef(TaskPlayer, StartTaskPlayer, osPriorityNormal, 0, 256);
  TaskPlayerHandle = osThreadCreate(osThread(TaskPlayer), NULL);

  /* definition and creation of TaskDisplay */
  osThreadDef(TaskDisplay, StartTaskDisplay, osPriorityLow, 0, 256);
  TaskDisplayHandle = osThreadCreate(osThread(TaskDisplay), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTaskMotor */
/**
 * @brief Function implementing the TaskMotor thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTaskMotor */
void StartTaskMotor(void const * argument)
{
  /* USER CODE BEGIN StartTaskMotor */
	/* Infinite loop */
	for(;;)
	{
		/* Block until Player signals motor to start */
		osSemaphoreWait(motor_start_semHandle, osWaitForever);

		/* Clear any pending stop signal */
		xSemaphoreTake(motor_stop_semHandle, 0);

		for (;;)
		{
			stepper_Step(true);
			osDelay(1);

			/* Non-blocking check for stop signal from Player */
			if (osSemaphoreWait(motor_stop_semHandle, 0) == osOK)
				break;
		}

		/* De-energize all coils to save power */
		stepper_Stop();

	    /* Clear any pending start signal that arrived during stop */
	    xSemaphoreTake(motor_start_semHandle, 0);
	}
  /* USER CODE END StartTaskMotor */
}

/* USER CODE BEGIN Header_StartTaskRFID */
/**
 * @brief Function implementing the TaskRFID thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTaskRFID */
void StartTaskRFID(void const * argument)
{
  /* USER CODE BEGIN StartTaskRFID */
	uint8_t uid[MIFARE_UID_MAX_LENGTH];
	int32_t uid_len = 0;
	bool tag_was_present = false;
	static uint64_t last_tag_id = 0;

	/* Initialize PN532 */
	osMutexWait(i2c_mutexHandle, osWaitForever);
	PN532_GetFirmwareVersion(&pn532, uid); /* reuse uid buffer temporarily */
	PN532_SamConfiguration(&pn532);
	osMutexRelease(i2c_mutexHandle);
	/* Infinite loop */
	for(;;)
	{
		osMutexWait(i2c_mutexHandle, osWaitForever);
	    uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 100);
	    osMutexRelease(i2c_mutexHandle);

	    if (uid_len > 0)
	    {
	        uint64_t current_id = uid_to_u64(uid, uid_len);

	        if (current_id != last_tag_id)
	        {
	            last_tag_id = current_id;

	            rfid_msg_t msg;
	            msg.uid = current_id;

	            if (current_id == TAG_1)      msg.track_number = 1;
	            else if (current_id == TAG_2) msg.track_number = 2;
	            else                          msg.track_number = -1;

	            xQueueOverwrite(RFID_QueueHandle, &msg);
	            last_tag_id = 0;
	        }
	    }

	    osDelay(50);
	}
  /* USER CODE END StartTaskRFID */
}

/* USER CODE BEGIN Header_StartTaskEncoder */
/**
 * @brief Function implementing the TaskEncoder thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTaskEncoder */
void StartTaskEncoder(void const * argument)
{
  /* USER CODE BEGIN StartTaskEncoder */
	static int32_t last_count = 0;
	int32_t count = 0;
	int32_t diff = 0;
	int8_t delta = 0;
	/* Infinite loop */
	for(;;)
	{
		count = (int32_t)(uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
		diff = count - last_count;

		/* Handle counter overflow/underflow */
		if (diff > 32767)  diff -= 65536;
		if (diff < -32767) diff += 65536;

		if (diff >= 2)
		{
			delta = 1;
			xQueueSend(Encoder_QueueHandle, &delta, 0);
			last_count = count;
		}
		else if (diff <= -2)
		{
			delta = -1;
			xQueueSend(Encoder_QueueHandle, &delta, 0);
			last_count = count;
		}

		osDelay(1);
	}
  /* USER CODE END StartTaskEncoder */
}

/* USER CODE BEGIN Header_StartTaskPlayer */
/**
 * @brief Function implementing the TaskPlayer thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTaskPlayer */
void StartTaskPlayer(void const * argument)
{
  /* USER CODE BEGIN StartTaskPlayer */
	rfid_msg_t rfid_msg;
	int8_t encoder_delta = 0;
	bool is_paused = false;
	int8_t current_track = -1;

	/* Initialize DFPlayer */

	/* Infinite loop */
	for(;;)
	{
        /* Check for new RFID event */
        if (xQueueReceive(RFID_QueueHandle, &rfid_msg, 0) == pdTRUE)
        {
            if (rfid_msg.track_number >= 0)
            {
                /* New tag placed - start playback */
                current_track = rfid_msg.track_number;
                is_paused = false;

                dfplayer_SetTrakNumber(current_track);

                /* Clear pending stop signal before starting motor */
                xSemaphoreTake(motor_stop_semHandle, 0);
                osSemaphoreRelease(motor_start_semHandle);

                display_msg_t disp;
                disp.volume = volume;
                disp.is_paused = 0;
                disp.motor_running = 1;

                if (current_track == 1)
                    strncpy(disp.track_name, "BTS - Spine Breaker", 32);
                else if (current_track == 2)
                    strncpy(disp.track_name, "J-Hope - Arson", 32);
                else
                    strncpy(disp.track_name, "Unknown", 32);

                xQueueSend(Display_QueueHandle, &disp, 0);
            }
            else
            {
                /* Tag removed - stop playback and motor */
                current_track = -1;
                is_paused = false;

                dfplayer_Stop();
                osSemaphoreRelease(motor_stop_semHandle);

                display_msg_t disp;
                disp.volume = volume;
                disp.is_paused = 0;
                disp.motor_running = 0;
                strncpy(disp.track_name, "Waiting...", 32);
                xQueueSend(Display_QueueHandle, &disp, 0);
            }
        }

        /* Drain all encoder messages and update volume once */
        bool volume_changed = false;
        while (xQueueReceive(Encoder_QueueHandle, &encoder_delta, 0) == pdTRUE)
        {
            if (encoder_delta > 0 && volume < 30) volume++;
            if (encoder_delta < 0 && volume > 0)  volume--;
            volume_changed = true;
        }

        if (volume_changed)
        {
            dfplayer_SetVolume(volume);

            display_msg_t disp;
            disp.volume = volume;
            disp.is_paused = is_paused;
            disp.motor_running = (current_track >= 0);
            if (current_track == 1)
                strncpy(disp.track_name, "BTS - Spine Breaker", 32);
            else if (current_track == 2)
                strncpy(disp.track_name, "J-Hope - Arson", 32);
            else
                strncpy(disp.track_name, "Waiting...", 32);

            xQueueSend(Display_QueueHandle, &disp, 0);
        }

        /* Check for pause semaphore from EXTI */
        if (osSemaphoreWait(pause_semHandle, 0) == osOK)
        {
            if (current_track >= 0)
            {
                is_paused = !is_paused;

                if (is_paused)
                {
                    dfplayer_Pause();
                    osSemaphoreRelease(motor_stop_semHandle);
                }
                else
                {
                    dfplayer_Play();
                    xSemaphoreTake(motor_stop_semHandle, 0);
                    osSemaphoreRelease(motor_start_semHandle);
                }

                display_msg_t disp;
                disp.volume = volume;
                disp.is_paused = is_paused;
                disp.motor_running = !is_paused;
                if (current_track == 1)
                    strncpy(disp.track_name, "BTS - Spine Breaker", 32);
                else if (current_track == 2)
                    strncpy(disp.track_name, "J-Hope - Arson", 32);
                else
                    strncpy(disp.track_name, "Waiting...", 32);

                xQueueSend(Display_QueueHandle, &disp, 0);
            }
        }

        osDelay(10);
	}
  /* USER CODE END StartTaskPlayer */
}

/* USER CODE BEGIN Header_StartTaskDisplay */
/**
 * @brief Function implementing the TaskDisplay thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTaskDisplay */
void StartTaskDisplay(void const * argument)
{
  /* USER CODE BEGIN StartTaskDisplay */
	display_msg_t msg;
	/* Infinite loop */
	for(;;)
	{
		/* Block until Player sends a display update */
		if (xQueueReceive(Display_QueueHandle, &msg, osWaitForever) == pdTRUE)
		{
			osMutexWait(i2c_mutexHandle, osWaitForever);

			SSD1306_Clear();

			SSD1306_SetCursor(0, 0);
			SSD1306_WriteString(msg.track_name);

			char status[22];
			sprintf(status, "Vol:%02d %s", msg.volume,
			        msg.is_paused ? "Paused " : "Playing");
			SSD1306_SetCursor(0, 16);
			SSD1306_WriteString(status);

			SSD1306_UpdateScreen();

			osMutexRelease(i2c_mutexHandle);
		}
	}
  /* USER CODE END StartTaskDisplay */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_2) {
		osSemaphoreRelease(pause_semHandle);
	}
}
/* USER CODE END Application */


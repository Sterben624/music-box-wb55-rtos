/*
 * dfplayer.c
 *
 * DFPlayer Mini control driver
 * Rewritten for STM32WB55, USART1, blocking HAL UART (no DMA)
 * Now reads back the DFPlayer's response to report command status.
 */

#include "df_player.h"
#include "main.h"

static uint8_t tx_buf[DFPLAYER_COMMUNICATION_SIZE];
static uint8_t rx_buf[DFPLAYER_COMMUNICATION_SIZE];

extern UART_HandleTypeDef huart1;

/* Internal helpers, not exposed outside this file */
static DFPlayer_Status_t dfplayer_SendControlMessage(uint8_t cmd, uint8_t para_msb, uint8_t para_lsb);
static uint16_t           dfplayer_Checksum(uint8_t *buf);
static DFPlayer_Status_t  dfplayer_ReadResponse(void);

/**
 * @brief  Build, send a control packet to the DFPlayer, and read back its response.
 * @note   Internal use only.
 * @param  cmd       Control command, see dfplayer.h
 * @param  para_msb  Command parameter MSB, 0 if unused
 * @param  para_lsb  Command parameter LSB, 0 if unused
 * @return DFPLAYER_OK if the DFPlayer acknowledged the command,
 *         DFPLAYER_ERROR / DFPLAYER_TIMEOUT / DFPLAYER_BAD_RESPONSE otherwise.
 * @warning If not using USART1, change huart1 below to your actual handle.
 */
static DFPlayer_Status_t dfplayer_SendControlMessage(uint8_t cmd, uint8_t para_msb, uint8_t para_lsb)
{
    tx_buf[START_INDEX]         = START;
    tx_buf[VERSION_INDEX]       = VERSION;
    tx_buf[DATA_LENGTH_INDEX]   = DATA_LENGTH;
    tx_buf[CMD_INDEX]           = cmd;
    tx_buf[FEEDBACK_INDEX]      = NO_FEED_BACK; /* no ask the DFPlayer to confirm the command */
    tx_buf[PARAMETER_MSB_INDEX] = para_msb;
    tx_buf[PARAMETER_LSB_INDEX] = para_lsb;

    uint16_t checksum = dfplayer_Checksum(tx_buf);

    tx_buf[CHECKSUM_MSB_INDEX] = (uint8_t)(checksum >> 8);
    tx_buf[CHECKSUM_LSB_INDEX] = (uint8_t)(checksum & 0xFF);
    tx_buf[END_INDEX]          = END;

    /* Send the whole packet in one go - no loop, no DMA needed for this */
    HAL_UART_Transmit(&huart1, tx_buf, DFPLAYER_COMMUNICATION_SIZE, HAL_MAX_DELAY);

    return DFPLAYER_OK;
}

/**
 * @brief  Wait for and parse the DFPlayer's response frame.
 * @note   Internal use only.
 * @return DFPLAYER_OK            - DFPlayer replied with CMD_Q_REPLY (command accepted)
 *         DFPLAYER_ERROR         - DFPlayer replied with CMD_Q_RETURN_ERROR
 *         DFPLAYER_TIMEOUT       - no response within DFPLAYER_RESPONSE_TIMEOUT_MS
 *         DFPLAYER_BAD_RESPONSE  - response received but frame is malformed
 *
 * DEAD CODE
 */
static DFPlayer_Status_t dfplayer_ReadResponse(void)
{
    HAL_StatusTypeDef hal_status = HAL_UART_Receive(&huart1, rx_buf, DFPLAYER_COMMUNICATION_SIZE,
                                                      DFPLAYER_RESPONSE_TIMEOUT_MS);

    if (hal_status == HAL_TIMEOUT)
    {
        return DFPLAYER_TIMEOUT;
    }

    if (hal_status != HAL_OK)
    {
        return DFPLAYER_BAD_RESPONSE;
    }

    if (rx_buf[START_INDEX] != START || rx_buf[END_INDEX] != END)
    {
        return DFPLAYER_BAD_RESPONSE;
    }

    uint16_t checksum = dfplayer_Checksum(rx_buf);
    uint16_t received_checksum = ((uint16_t)rx_buf[CHECKSUM_MSB_INDEX] << 8) | rx_buf[CHECKSUM_LSB_INDEX];

    if (checksum != received_checksum)
    {
        return DFPLAYER_BAD_RESPONSE;
    }

    if (rx_buf[CMD_INDEX] == CMD_Q_RETURN_ERROR)
    {
        return DFPLAYER_ERROR;
    }

    /* CMD_Q_REPLY, or any other unsolicited status frame, is treated as OK */
    return DFPLAYER_OK;
}

/**
 * @brief  Compute the checksum required by the DFPlayer serial protocol.
 * @note   Internal use only. Works on either the tx or rx buffer.
 * @param  buf  Buffer to compute the checksum over (must be DFPLAYER_COMMUNICATION_SIZE long)
 */
static uint16_t dfplayer_Checksum(uint8_t *buf)
{
    uint16_t checksum = 0;

    for (int i = VERSION_INDEX; i <= PARAMETER_LSB_INDEX; i++)
    {
        checksum += buf[i];
    }

    checksum = 0xFFFF - checksum + 1; /* two's complement, per protocol spec */

    return checksum;
}

/**
 * @brief  Initialize the DFPlayer.
 * @note   The module needs roughly 1.5-3s after power-on before it is ready.
 */
void dfplayer_Init(void)
{
	HAL_Delay(1500);
}

/**
 * @brief  Play the next track.
 */
DFPlayer_Status_t dfplayer_Next(void)
{
    return dfplayer_SendControlMessage(CMD_NEXT, 0, 0);
}

/**
 * @brief  Play the previous track.
 */
DFPlayer_Status_t dfplayer_Previous(void)
{
    return dfplayer_SendControlMessage(CMD_PREVIOUS, 0, 0);
}

/**
 * @brief  Select a track by its global number.
 * @note   Files must be named 0001.mp3 ... in the root of the SD card.
 * @param  track  1 ~ 30000 (or 0 ~ 2999), depending on naming range used.
 */
DFPlayer_Status_t dfplayer_SetTrakNumber(int16_t track)
{
    return dfplayer_SendControlMessage(CMD_SET_TRAK_NUMBER, (uint8_t)(track >> 8), (uint8_t)track & 0xFF);
}

/**
 * @brief  Increase volume by one step.
 */
DFPlayer_Status_t dfplayer_IncreaseVolume(void)
{
    return dfplayer_SendControlMessage(CMD_INC_VOLUME, 0, 0);
}

/**
 * @brief  Decrease volume by one step.
 */
DFPlayer_Status_t dfplayer_DecreaseVolume(void)
{
    return dfplayer_SendControlMessage(CMD_DEC_VOLUME, 0, 0);
}

/**
 * @brief  Set volume directly.
 * @param  volume  0 ~ 30
 */
DFPlayer_Status_t dfplayer_SetVolume(uint8_t volume)
{
    if (volume > 30)
    {
        volume = 30;
    }

    return dfplayer_SendControlMessage(CMD_SET_VOLUME, 0, volume);
}

/**
 * @brief  Set equalizer preset.
 * @param  eq  EQ_NORMAL / EQ_POP / EQ_ROCK / EQ_JAZZ / EQ_CLASSIC / EQ_BASE
 */
DFPlayer_Status_t dfplayer_SetEQ(int8_t eq)
{
    if (eq > EQ_BASE)
    {
        eq = EQ_NORMAL;
    }

    return dfplayer_SendControlMessage(CMD_SET_EQ, 0, eq);
}

/**
 * @brief  Set playback mode to repeat a given track.
 * @param  track  1 ~ 30000 (or 0 ~ 2999), depending on naming range used.
 */
DFPlayer_Status_t dfplayer_RepeatTrack(uint8_t track)
{
    return dfplayer_SendControlMessage(CMD_REPEAT_TRACK, (uint8_t)(track & 0xFF00) >> 8, (uint8_t)track & 0xFF);
}

/**
 * @brief  Select playback source.
 * @param  source  PLAYBACK_SOURCE_U / PLAYBACK_SOURCE_TF / PLAYBACK_SOURCE_AUX /
 *                 PLAYBACK_SOURCE_SLEEP / PLAYBACK_SOURCE_FLASH
 */
DFPlayer_Status_t dfplayer_SetSource(uint8_t source)
{
    if (source > PLAYBACK_SOURCE_FLASH)
    {
        return DFPLAYER_BAD_RESPONSE;
    }

    return dfplayer_SendControlMessage(CMD_SET_PLAYBACK_SOURCE, 0, source);
}

/**
 * @brief  Enter standby (low power) mode.
 */
DFPlayer_Status_t dfplayer_Standby(void)
{
    return dfplayer_SendControlMessage(CMD_ENTER_INTO_STANDBY, 0, 0);
}

/**
 * @brief  Reset the DFPlayer.
 */
DFPlayer_Status_t dfplayer_Reset(void)
{
    return dfplayer_SendControlMessage(CMD_RESET, 0, 0);
}

/**
 * @brief  Resume/start playback.
 */
DFPlayer_Status_t dfplayer_Play(void)
{
    return dfplayer_SendControlMessage(CMD_PLAYBACK, 0, 0);
}

/**
 * @brief  Pause playback.
 */
DFPlayer_Status_t dfplayer_Pause(void)
{
    return dfplayer_SendControlMessage(CMD_PAUSE, 0, 0);
}

/**
 * @brief  Play a track from a numbered folder (01~99), excluding special folders.
 * @note   Track number is limited to 255 in this mode.
 * @param  folder  01 ~ 99
 * @param  track   001 ~ 255
 */
DFPlayer_Status_t dfplayer_PlayTrackInFolder(uint8_t folder, uint8_t track)
{
    if (folder > 99)
    {
        folder = 99;
    }

    if (track > 255)
    {
        track = 255;
    }

    return dfplayer_SendControlMessage(CMD_SET_PLAY_TRACK_FOLDER, folder, track);
}

/**
 * @brief  Adjust output gain.
 * @param  turn_onoff  true: enable gain adjust, false: disable
 * @param  gain        0 ~ 31
 */
DFPlayer_Status_t dfplayer_VolumeAdjust(bool turn_onoff, uint8_t gain)
{
    if (gain > 31)
    {
        gain = 31;
    }

    return dfplayer_SendControlMessage(CMD_VOLUME_ADJUST, turn_onoff, gain);
}

/**
 * @brief  Enable/disable repeat-all playback.
 * @param  repeat  false: stop, true: start repeat play
 */
DFPlayer_Status_t dfplayer_RepeatAll(bool repeat)
{
    return dfplayer_SendControlMessage(CMD_REPEAT, 0, repeat);
}

/**
 * @brief  Play a track from the "MP3" folder.
 * @param  track  1 ~ 30000 (or 0 ~ 2999), depending on naming range used.
 */
DFPlayer_Status_t dfplayer_PlayMP3Folder(uint16_t track)
{
    return dfplayer_SendControlMessage(CMD_SET_MP3_FOLDER, (uint8_t)(track >> 8), (uint8_t)(track & 0xFF));
}

/**
 * @brief  Interrupt current playback with a track from the ADVERT folder.
 * @note   ADVERT folder supports up to 3000 tracks.
 * @param  track  1 ~ 30000 (or 0 ~ 2999), depending on naming range used.
 */
DFPlayer_Status_t dfplayer_InsertAdvertisement(uint16_t track)
{
    return dfplayer_SendControlMessage(CMD_INSERT_ADVERT, (uint8_t)(track >> 8), (uint8_t)(track & 0xFF));
}

/**
 * @brief  Play a track from one of the 01~15 folders (3000-track support).
 * @param  folder  1 ~ 15
 * @param  track   1 ~ 30000 (or 0 ~ 2999), depending on naming range used.
 */
DFPlayer_Status_t dfplayer_Play3KFolder(uint8_t folder, uint16_t track)
{
    if (folder > 15)
    {
        folder = 15;
    }

    if (track > 3000)
    {
        track = 3000;
    }

    return dfplayer_SendControlMessage(CMD_SET_3K_FOLDER,
                                        (folder << 4) | (uint8_t)(track & 0xF00) >> 8,
                                        (uint8_t)(track & 0xFF));
}

/**
 * @brief  Stop the ADVERT interrupt and resume the original track.
 */
DFPlayer_Status_t dfplayer_StopAdvertisement(void)
{
    return dfplayer_SendControlMessage(CMD_STOP_ADVERT_GOBACK, 0, 0);
}

/**
 * @brief  Stop playback.
 * @note   Playing again restarts the track from the beginning.
 */
DFPlayer_Status_t dfplayer_Stop(void)
{
    return dfplayer_SendControlMessage(CMD_STOP, 0, 0);
}

/**
 * @brief  Repeat all tracks within a given folder.
 * @note   Repeats indefinitely until a stop command is received.
 * @param  folder  Folder number
 */
DFPlayer_Status_t dfplayer_RepeatTrackInFolder(uint8_t folder)
{
    if (folder > 99)
    {
        folder = 99;
    }

    return dfplayer_SendControlMessage(CMD_REPEAT_FOLDER_TRACK, 0, folder);
}

/**
 * @brief  Play a random track.
 */
DFPlayer_Status_t dfplayer_RandomTrack(void)
{
    return dfplayer_SendControlMessage(CMD_RANDOM_PLAY, 0, 0);
}

/**
 * @brief  Repeat the current track.
 * @param  turn_onoff  true: enable, false: disable
 * @warning Does not respond while stopped or paused.
 */
DFPlayer_Status_t dfplayer_RepeatCurrentTrack(bool turn_onoff)
{
    return dfplayer_SendControlMessage(CMD_RANDOM_PLAY, 0, !turn_onoff);
}

/**
 * @brief  Enable/disable the DAC output.
 * @note   DAC is on by default after power-up.
 * @param  turn_onoff  true: on, false: off
 */
DFPlayer_Status_t dfplayer_SetDAC(bool turn_onoff)
{
    return dfplayer_SendControlMessage(CMD_SET_DAC, 0, !turn_onoff);
}

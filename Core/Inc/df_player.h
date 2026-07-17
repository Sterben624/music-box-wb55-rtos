/*
 * dfplayer.h
 *
 * DFPlayer Mini control driver - header
 * Adapted for STM32WB55, plain HAL UART, no external framework dependency
 *
 ******************************************************************************
 * @attention
 *
 * #0. Track index numbers refer to file order on the storage device, not file name.
 * #1. Folders support up to 3000 tracks via CMD_SET_TRAK_NUMBER (CMD 0x03).
 * #2. Folders 01 ~ 99 are supported.
 * #3. Folders 01 ~ 15 support up to 3000 tracks each; folders 16 ~ 99 support up to 255 tracks
 *     (CMD_SET_PLAY_TRACK_FOLDER : 0x0F).
 * #4. If a folder named "MP3" exists, it supports up to 65536 tracks, but 3000 is
 *     recommended to avoid slow access (CMD_SET_MP3_FOLDER : 0x12).
 * #5. If a folder named "ADVERT" exists, it supports up to 3000 tracks.
 * #6. The ADVERT folder can be used to interrupt current playback
 *     (CMD_INSERT_ADVERT : 0x13, CMD_STOP_ADVERT_GOBACK : 0x15).
 *
 */

#ifndef DFPLAYER_H_
#define DFPLAYER_H_

#include <stdint.h>
#include <stdbool.h>

/*
 *           Define                 |  CMD  |        Description          |        Parameters (16bit)
 *------------------------------------------------------------------------------------------------------------------------------------
 */

/* Control Commands - sent directly, no response expected */
#define CMD_NEXT                      0x01    // Play next track
#define CMD_PREVIOUS                  0x02    // Play previous track
#define CMD_SET_TRAK_NUMBER           0x03    // Select track by global number      1 ~ 3000 (or 0 ~ 2999)
#define CMD_INC_VOLUME                0x04    // Increase volume
#define CMD_DEC_VOLUME                0x05    // Decrease volume
#define CMD_SET_VOLUME                0x06    // Set specific volume                0 ~ 30
#define CMD_SET_EQ                    0x07    // Set EQ preset                      0:Normal 1:Pop 2:Rock 3:Jazz 4:Classic 5:Base
#define CMD_REPEAT_TRACK              0x08    // Repeat a track
#define CMD_SET_PLAYBACK_SOURCE       0x09    // Select playback source             0:U 1:TF 2:AUX 3:SLEEP 4:FLASH
#define CMD_ENTER_INTO_STANDBY        0x0A    // Enter low power standby mode
#define CMD_RESET                     0x0C    // Reset module
#define CMD_PLAYBACK                  0x0D    // Resume / start playback
#define CMD_PAUSE                     0x0E    // Pause playback
#define CMD_SET_PLAY_TRACK_FOLDER     0x0F    // Select folder + track              folder:01~99, folder 01~15 -> track 001~3000, folder 16~99 -> track 001~255
#define CMD_VOLUME_ADJUST             0x10    // Adjust output gain                 Parameter_MSB=1: enable gain adjust / Parameter_LSB = gain 0~31
#define CMD_REPEAT                    0x11    // Repeat-all playback                0:stop / 1:start
#define CMD_SET_MP3_FOLDER            0x12    // Select track from "MP3" folder
#define CMD_INSERT_ADVERT             0x13    // Interrupt playback with a track
#define CMD_SET_3K_FOLDER             0x14    // Play track from a 3000-track-capable folder
#define CMD_STOP_ADVERT_GOBACK        0x15    // Stop interrupt playback, resume previous track
#define CMD_STOP                      0x16    // Stop playback
#define CMD_REPEAT_FOLDER_TRACK       0x17    // Repeat all tracks within a folder
#define CMD_RANDOM_PLAY                0x18    // Play a random track
#define CMD_REPEAT_CURRENT_TRACK      0x19    // Repeat current track
#define CMD_SET_DAC                   0x1A    // Enable/disable DAC output

/* Query Commands - DFPlayer replies with the requested info */
#define CMD_Q_SEND_INIT_PARA           0x3F   // Send init parameters                0 - 0x0F (each bit = one device, low 4 bits)
#define CMD_Q_RETURN_ERROR             0x40   // Error reply, request resend
#define CMD_Q_REPLY                    0x41   // Generic ack reply (command accepted)
#define CMD_Q_CURRENT_STATUS           0x42   // Query current status
#define CMD_Q_CURRENT_VOLUME           0x43   // Query current volume
#define CMD_Q_CURRENT_EQ               0x44   // Query current EQ
#define CMD_Q_CURRENT_PLAYBACK_MODE    0x45   // Query current playback mode
#define CMD_Q_CURRENT_SOFTWARE_VER     0x46   // Query firmware version
#define CMD_Q_TOTAL_NUM_TF_FILES       0x47   // Query total file count on TF card
#define CMD_Q_TOTAL_NUM_U_FILES        0x48   // Query total file count on U disk
#define CMD_Q_TOTAL_NUM_FLASH_FILES    0x49   // Query total file count on Flash
#define CMD_Q_CURRENT_TRACK_TF         0x4B   // Query current track number on TF card
#define MP3_Q_CURRENT_TRACK_U_DISK     0x4C   // Query current track number on U disk
#define MP3_Q_CURRENT_TRACK_FLASH      0x4D   // Query current track number on Flash

/* Command Parameters */

/* EQ Parameter */
#define EQ_NORMAL                      0
#define EQ_POP                         1
#define EQ_ROCK                        2
#define EQ_JAZZ                        3
#define EQ_CLASSIC                     4
#define EQ_BASE                        5

/* Playback Source Parameter */
#define PLAYBACK_SOURCE_U              0
#define PLAYBACK_SOURCE_TF             1
#define PLAYBACK_SOURCE_AUX            2
#define PLAYBACK_SOURCE_SLEEP          3
#define PLAYBACK_SOURCE_FLASH          4

/* Repeat Parameter */
#define REPEAT_STOP                    0
#define REPEAT_START                   1

/*
 * DFPlayer serial protocol frame format
 * MCU and DFPlayer communicate over UART using this fixed 10-byte frame:
 *
 *     0        1            2          3          4             5               6              7              8           9
 * |  Start | Version | Data_Length |  CMD  |  Feedback  | Parameter_MSB | Parameter_LSB | Checksum_MSB | Checksum_LSB |  End  |
 * -----------------------------------------------------------------------------------------------------------------------------
 * |  0x7E  |  0xFF   |     0x06    |       |  0x01/0x00 |               |               |              |              | 0xEF  |
 * -----------------------------------------------------------------------------------------------------------------------------
 * |  fixed  |  fixed  |    fixed    |       |  used/unused |   AKA "DH"   |   AKA "DL"    |              |              | fixed |
 *
 * @ Start          : start byte
 * @ Version        : protocol version
 * @ Data_Length    : byte count from Version to Parameter_LSB inclusive (always 6)
 * @ Feedback       : command feedback, 0x01 = feedback requested, 0x00 = none
 * @ Parameter_MSB  : parameter, most significant byte
 * @ Parameter_LSB  : parameter, least significant byte
 * @ Checksum_MSB   : checksum, most significant byte
 * @ Checksum_LSB   : checksum, least significant byte
 * @ End            : end byte
 */

/* DFPlayer serial frame size & byte indices */
#define DFPLAYER_COMMUNICATION_SIZE    10

#define START_INDEX                   0
#define VERSION_INDEX                 1
#define DATA_LENGTH_INDEX             2
#define CMD_INDEX                     3
#define FEEDBACK_INDEX                4
#define PARAMETER_MSB_INDEX            5
#define PARAMETER_LSB_INDEX            6
#define CHECKSUM_MSB_INDEX             7
#define CHECKSUM_LSB_INDEX             8
#define END_INDEX                      9

/* DFPlayer serial frame fixed values */
#define START                          0x7E
#define VERSION                        0xFF
#define DATA_LENGTH                    0x06
#define FEED_BACK                      0x01
#define NO_FEED_BACK                   0x00
#define END                            0xEF

/* Response wait timeout, in milliseconds */
#define DFPLAYER_RESPONSE_TIMEOUT_MS   200

/**
 * @brief Result of a command sent to the DFPlayer.
 */
typedef enum
{
    DFPLAYER_OK = 0,            /* Command accepted (CMD_Q_REPLY received) */
    DFPLAYER_ERROR,             /* DFPlayer replied with an error (CMD_Q_RETURN_ERROR) */
    DFPLAYER_TIMEOUT,           /* No response received within timeout */
    DFPLAYER_BAD_RESPONSE       /* Response received but malformed (bad start/end/checksum) */
} DFPlayer_Status_t;

void dfplayer_Init(void);

DFPlayer_Status_t dfplayer_Next(void);
DFPlayer_Status_t dfplayer_Previous(void);
DFPlayer_Status_t dfplayer_SetTrakNumber(int16_t track);
DFPlayer_Status_t dfplayer_IncreaseVolume(void);
DFPlayer_Status_t dfplayer_DecreaseVolume(void);
DFPlayer_Status_t dfplayer_SetVolume(uint8_t volume);
DFPlayer_Status_t dfplayer_SetEQ(int8_t eq);
DFPlayer_Status_t dfplayer_RepeatTrack(uint8_t track);
DFPlayer_Status_t dfplayer_SetSource(uint8_t source);
DFPlayer_Status_t dfplayer_Standby(void);
DFPlayer_Status_t dfplayer_Reset(void);
DFPlayer_Status_t dfplayer_Play(void);
DFPlayer_Status_t dfplayer_Pause(void);
DFPlayer_Status_t dfplayer_PlayTrackInFolder(uint8_t folder, uint8_t track);
DFPlayer_Status_t dfplayer_VolumeAdjust(bool turn_onoff, uint8_t gain);
DFPlayer_Status_t dfplayer_RepeatAll(bool repeat);
DFPlayer_Status_t dfplayer_PlayMP3Folder(uint16_t track);
DFPlayer_Status_t dfplayer_InsertAdvertisement(uint16_t track);
DFPlayer_Status_t dfplayer_Play3KFolder(uint8_t folder, uint16_t track);
DFPlayer_Status_t dfplayer_StopAdvertisement(void);
DFPlayer_Status_t dfplayer_Stop(void);
DFPlayer_Status_t dfplayer_RepeatTrackInFolder(uint8_t folder);
DFPlayer_Status_t dfplayer_RandomTrack(void);
DFPlayer_Status_t dfplayer_RepeatCurrentTrack(bool turn_onoff);
DFPlayer_Status_t dfplayer_SetDAC(bool turn_onoff);

#endif /* DFPLAYER_H_ */

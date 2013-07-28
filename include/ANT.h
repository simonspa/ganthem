/***************************************************************************
 *   Copyright (C) 2010-2012 by Oleg Khudyakov                             *
 *   prcoder@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef ANT_H
#define ANT_H
#include "SerialIO.h"
#include "Log.h"

#include <vector>
#include <string>
#include <map>
#include <pthread.h>

#define Max_Data_Size 255

using namespace std;

enum ANT_Message
{
    MSG_Null                                = 0x00,
    MSG_ChannelEvent                        = 0x01,
    MSG_ResponseEvent                       = 0x40,
    MSG_UnassignChannel                     = 0x41,
    MSG_AssignChannel                       = 0x42,
    MSG_SetChannelPeriod                    = 0x43,
    MSG_SetChannelSearchTimeout             = 0x44,
    MSG_SetChannelRadioFreq                 = 0x45,
    MSG_SetNetworkKey                       = 0x46,
    MSG_SetTransmitPower                    = 0x47,
    MSG_SetCWTestMode                       = 0x48,
    MSG_SetSearchWaveform                   = 0x49,
    MSG_ResetSystem                         = 0x4A,
    MSG_OpenChannel                         = 0x4B,
    MSG_CloseChannel                        = 0x4C,
    MSG_RequestMessage                      = 0x4D,
    MSG_SendBroadcastData                   = 0x4E,
    MSG_SendAcknowledgedData                = 0x4F,
    MSG_SendBurstTransferPacket             = 0x50,
    MSG_SetChannelId                        = 0x51,
    MSG_ChannelStatus                       = 0x52,
    MSG_InitCWTestMode                      = 0x53,
    MSG_Capabilities                        = 0x54,
    MSG_NVMData                             = 0x56,
    MSG_NVMCmd                              = 0x57,
    MSG_NVMString                           = 0x58,
    MSG_ListAddChannel                      = 0x59,
    MSG_ListConfig                          = 0x5A,
    MSG_OpenRxScanMode                      = 0x5B,
    MSG_ExtSetChannelRadioFreq              = 0x5C,
    MSG_ExtSendBroadcastData                = 0x5D,
    MSG_ExtSendAcknowledgedData             = 0x5E,
    MSG_ExtSendBurstTransferPacket          = 0x5F,
    MSG_SetChannelTxPower                   = 0x60,
    MSG_GetSerialNum                        = 0x61,
    MSG_GetTempCal                          = 0x62,
    MSG_SetLowPriorityChannelSearchTimeout  = 0x63,
    MSG_SetTxSearchOnNext                   = 0x64,
    MSG_SetSerialNumChannelId               = 0x65,
    MSG_RxExtMesgsEnable                    = 0x66,
    MSG_RadioConfigAlways                   = 0x67,
    MSG_EnableLED                           = 0x68,
    MSG_AGCConfig                           = 0x6A,
    MSG_CrystalEnable                       = 0x6D,
    MSG_ResponseFunc                        = 0x6F,
    MSG_ConfigFrequencyAgility              = 0x70,
    MSG_SetProximitySearch                  = 0x71,
    MSG_ReadSEGA                            = 0xA0,
    MSG_SEGACmd                             = 0xA1,
    MSG_SEGAData                            = 0xA2,
    MSG_SEGAErase                           = 0xA3,
    MSG_SEGAWrite                           = 0xA4,
//  Free                                    = 0xA5,
    MSG_SEGALock                            = 0xA6,
    MSG_FuseCheck                           = 0xA7,
    MSG_UartReg                             = 0xA8,
    MSG_ManTemp                             = 0xA9,
    MSG_Bist                                = 0xAA,
    MSG_SelfErase                           = 0xAB,
    MSG_SetMFGBits                          = 0xAC,
    MSG_UnlockInterface                     = 0xAD,
    MSG_IOState                             = 0xB0,
    MSG_CfgState                            = 0xB1,
    MSG_RSSI                                = 0xC0,
    MSG_RSSIBroadcastData                   = 0xC1,
    MSG_RSSIAcknowledgedData                = 0xC2,
    MSG_RSSIBurstData                       = 0xC3,
    MSG_RSSISearchThreshold                 = 0xC4,
    MSG_SleepMessage                        = 0xC5,
    MSG_BTHBroadcastData                    = 0xD0,
    MSG_BTHAcknowledgedData                 = 0xD1,
    MSG_BTHBurstData                        = 0xD2,
    MSG_BTHExtBroadcastData                 = 0xD3,
    MSG_BTHExtAcknowledgedData              = 0xD4,
    MSG_BTHExtBurstData                     = 0xD5
};

enum ANT_Event
{
    EventResponseNoError                    = 0x00,
    EventRXSearchTimeout                    = 0x01,
    EventRXFail                             = 0x02,
    EventTX                                 = 0x03,
    EventTransferRXFailed                   = 0x04,
    EventTransferTXCompleted                = 0x05,
    EventTransferTXFailed                   = 0x06,
    EventChannelClosed                      = 0x07,
    EventRXFailGoToSearch                   = 0x08,
    EventChannelCollision                   = 0x09,
    EventTransferTXStart                    = 0x0A,
    EventTransferTXCompletedRSSI            = 0x10,
    EventChannelInWrongState                = 0x15,
    EventChannelNotOpened                   = 0x16,
    EventChannelIdNotSet                    = 0x18,
    EventCloseAllChannels                   = 0x19,
    EventTransferInProgress                 = 0x1F,
    EventTransferSequenceNumberError        = 0x20,
    EventTransferInError                    = 0x21,
    EventTransferBusy                       = 0x22,
    EventInvalidMessage                     = 0x28,
    EventInvalidNetworkNumber               = 0x29,
    EventInvalidList                        = 0x30,
    EventInvalidScanTXChannel               = 0x31,
    EventNVMFullError                       = 0x40,
    EventNVMWriteError                      = 0x41,
    EventNoResponseMessage                  = 0x50,
    EventReturnToMFG                        = 0x51
};

enum ChannelType
{
    ReceiveChannel  = 0x00,
    TransmitChannel = 0x10
};

enum TXPower
{
    TXPowerMinus20db = 0x00,
    TXPowerMinus10db = 0x01,
    TXPowerMinus5db  = 0x02,
    TXPower0db       = 0x03
};

enum NVMCommand
{
    NVMCmdFormat            = 0x00,
    NVMCmdDump              = 0x01,
    NVMCmdSetDefaultSector  = 0x02,
    NVMCmdEndSector         = 0x03,
    NVMCmdEndDump           = 0x04,
    NVMCmdLockNVM           = 0x05
};

enum ChannelParam
{
    ChannelParamTX                       = 0x10,
    ChannelParamShared                   = 0x20,
    ChannelParamNoTXGuardBand            = 0x30,
    ChannelParamAlwaysRXWildCardSearch   = 0x40,
    ChannelParamRSSI                     = 0x80
};

enum ChannelStatus
{
    ChannelStatusUnassigned = 0,
    ChannelStatusAssigned,
    ChannelStatusSearching,
    ChannelStatusTracking
};

enum Capabilities
{
    CapNoRXChannels     = 0x01,
    CapNoTxChannels     = 0x02,
    CapNoRxMessages     = 0x04,
    CapNoTxMessages     = 0x08,
    CapNoAckdMessages   = 0x10,
    CapNoBurstTransfer  = 0x20
};

enum ClientDeviceStates
{
    DeviceStateLink = 0,
    DeviceStateAuthentication,
    DeviceStateTransport,
    DeviceStateBusy
};


enum AuthTypes
{
    PassThrough = 0,
    NA,
    PairingOnly,
    PassKeyAndPairing
};


enum DataPages
{
    DataPageManufacturerInformation = 0x80,
    DataPageProductInformation,
    DataPageBatteryStatus,
    DataPageTimeAndDate,
    DataPageSubfieldData
};

extern const useconds_t burstSleepTime;

class ANTMessage
{
public:
    enum
    {
        TXSync = 0xA4,
        RXSync = 0xA5
    };

    static bool sendMessage(SerialIO &sio, ANT_Message messageId, vector<uint8_t>& messageData);
    static bool getMessage(vector<uint8_t> &receivedData, volatile ANT_Message &messageId, vector<uint8_t> &messageData);

private:
    static uint8_t calculateCRC(vector<uint8_t> &buffer);
};

class ANT
{
public:
    ANT();
    ~ANT();

    bool init(string deviceName, speed_t speed);
    void leave();
    bool receiveBuffer();
    static void* receiveThread(ANT* ant);
    bool parseMessage();
    static void* parseThread(ANT* ant);
    bool waitMessage(uint8_t id);
    bool waitResponse(uint8_t id);
    bool waitBroadcast();
    bool waitBurst(vector<uint8_t> &data);
    bool waitBurst(uint8_t data[], unsigned len);
    bool getChannelStatus(uint8_t &status);
    string getChannelStatusString();

    // ANT commands
    bool resetSystem();
    bool setNetworkKey(uint8_t network, vector<uint8_t> &key);
    bool assignChannel(uint8_t channel, ChannelType type, uint8_t network);
    bool setChannelPeriod(uint8_t channel, uint16_t period);
    bool setChannelSearchTimeout(uint8_t channel, uint8_t timeout);
    bool setChannelRFFreq(uint8_t channel, uint8_t frequency);
    bool setSearchWaveform(uint8_t channel, uint16_t waveform);
    bool setChannelId(uint8_t channel, uint16_t deviceNum, bool pairing,
        uint8_t deviceType, uint8_t transmissionType);
    bool openChannel(uint8_t channel);
    bool requestMessage(uint8_t channel, ANT_Message msgId);
    bool sendAcknowledgedData(uint8_t channel, vector<uint8_t> &ackData);
    bool sendAcknowledgedData(uint8_t channel, uint8_t data[], unsigned len);
    bool sendBurstTransferData(uint8_t channel, vector<uint8_t> &burstData);
    bool sendBurstTransferData(uint8_t channel, uint8_t data[], unsigned len);

protected:
    SerialIO sio;
    pthread_t receiveThreadHandle;
    pthread_t parseThreadHandle;
    static volatile bool leaveFlag;
    volatile ANT_Message messageId;
    volatile uint8_t responseId;
    volatile uint8_t responseCode;
    volatile uint8_t channelStatus;
    volatile uint8_t clientDeviceState;
    volatile bool broadcast;
    volatile bool lastBurst;
    vector<uint8_t> receivedData;
    pthread_mutex_t receivedDataMutex;
    vector<uint8_t> burstData;
    
public:
    map<uint8_t,string> responseIdMap;
    map<uint8_t,string> responseCodeMap;
    map<uint8_t,string> channelStatusMap;
    map<uint8_t,string> channelPeriodMap;
    map<uint8_t,string> clientDeviceStateMap;
    map<uint8_t,string> authTypesMap;
};
#endif

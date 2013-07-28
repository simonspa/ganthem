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

#ifndef ANT_PLUS_H
#define ANT_PLUS_H
#include "ANT.h"

#define MAX_NAME_LEN 16

enum
{
    ANTFSBeacon     = 0x43,
    ANTFSCommand    = 0x44
};

enum ANTFSCommand
{
    CommandLink             = 0x2,
    CommandDisconnect       = 0x3,
    CommandAuthenticate     = 0x4,
    CommandPing             = 0x5,
    CommandDownloadRequest  = 0x9,
    CommandUploadRequest    = 0xA,
    CommandEraseRequest     = 0xB,
    CommandUploadData       = 0xC
};

enum AuthenticationCommandType
{
    ProceedToTransport = 0,
    RequestClientDeviceSerialNumber,
    RequestPairing,
    RequestPasskeyExchange
};

enum AuthenticationResponses
{
    AuthenticationNA = 0,
    AuthenticationAccepted,
    AuthenticationRejected
};

enum DownloadResponseCodes
{
    DownloadResponseOk = 0,
    DownloadResponseNotExist,
    DownloadResponseNotDownloadable,
    DownloadResponseNotReady,
    DownloadResponseRequestInvalid,
    DownloadResponseCRCIncorrect
};

enum ChannelPeriods
{
    ChannelPeriod0_5Hz = 0,
    ChannelPeriod1Hz,
    ChannelPeriod2Hz,
    ChannelPeriod4Hz,
    ChannelPeriod8Hz,
    ChannelPeriodReserved_5,
    ChannelPeriodReserved_6,
    ChannelPeriodEstablished
};


#pragma pack(1)
struct BeaconStatusByte1
{
    uint8_t beaconChannelPeriod:3;  // 0=0.5Hz, 1=1Hz, 2=2Hz, 3=4Hz, 4=8Hz, 5..6 - Reserved, 7=Established Channel Period
    uint8_t pairingEnabled:1;
    uint8_t uploadEnabled:1;
    uint8_t dataAvailable:1;
    uint8_t reserved:2;
};

struct BeaconStatusByte2
{
    uint8_t clientDeviceState:4;
    uint8_t reserved:4;
};

struct ANTFSBeaconFormat
{
    uint8_t beaconID;
    BeaconStatusByte1 status1;
    BeaconStatusByte2 status2;
    uint8_t authType;
    union
    {
        struct
        {
            uint16_t manufacturerID;
            uint16_t deviceType;
        } devDescr;
        uint32_t hostSN;
    };
};

struct ANTFSCommandFormat
{
    uint8_t code;
    uint8_t command;
    uint8_t param1;
    uint8_t param2;
    uint32_t hostSN;
};

struct ANTPlusPairingCommand
{
    ANTFSCommandFormat cmd;
    char name[MAX_NAME_LEN];
};

struct ANTPlusPasskeyAuthenticationCommand
{
    ANTFSCommandFormat cmd;
    uint64_t key;
    uint64_t unkn;
};

struct ANTPlusDownloadCommand
{
    uint8_t code;
    uint8_t command;
    uint16_t fileIndex;
    uint32_t offset;
    uint8_t unknown;
    uint8_t initialRequest;
    uint16_t CRCseed;
    uint32_t maximumBlockSize;
};

struct ANTPlusAnswer
{
    uint32_t unkn1;
    uint32_t hostSN;
    uint16_t unkn2;
    uint8_t responseType;
    uint8_t authStringLength;
    uint32_t unitId;
};

struct ANTPlusRequestSerialAnswer
{
    ANTPlusAnswer ans;
    uint8_t unitName[MAX_NAME_LEN];
};

struct ANTPlusAuthenticateAnswer
{
    ANTPlusAnswer ans;
    uint64_t key;
};

struct ANTPlusDownloadHeader
{
    ANTFSBeaconFormat beacon;
    uint8_t antFsCommand;
    uint8_t responseToCommand;
    uint8_t response;
    uint8_t zeroByte;
    uint32_t dataRemain;
    uint32_t dataOffset;
    uint32_t fileSize;
};

struct ANTPlusDownloadFooter
{
    uint8_t reserved[6];
    uint16_t CRCseed;
};

#pragma pack()

class ANTPlus : public ANT
{
public:
    ANTPlus();
    ~ANTPlus();

    void leave(uint8_t channel);
    bool link(uint8_t channel, uint8_t freq,  uint8_t period, uint32_t hostSN);
    bool disconnect(uint8_t channel, bool returnToBroadcast = false);
    bool requestSN(uint8_t channel, uint32_t hostSN, string& unitName, uint32_t& unitId);
    bool devicePair(uint8_t channel, uint32_t hostSN, string pcName, uint32_t& unitId, uint64_t& key);
    bool authenticate(uint8_t channel, uint32_t hostSN, uint64_t key);
    bool download(uint8_t channel, uint16_t file, vector<uint8_t> &data);

private:
    int maxAttempts;
    map<uint8_t, string> downloadResponseCodesMap;
};

#endif

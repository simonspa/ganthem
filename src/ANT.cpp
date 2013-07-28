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
#include "ANT.h"
#include "ANTPlus.h"
#include "FIT.h"
#include "Log.h"
#include "GarminConvert.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

const useconds_t sleepTime = 15000;
const useconds_t burstSleepTime = 60000;

bool ANTMessage::sendMessage(SerialIO &sio, ANT_Message messageId, vector<uint8_t>& messageData)
{
    size_t messageSize = messageData.size();
    if (messageSize > Max_Data_Size)
    {
        parseThreadLogStream << "Message size " << messageSize << " bytes is too big. Accepted maximum " << Max_Data_Size << " bytes";
        parseThreadLogFlush();
        return false;
    }

    vector<uint8_t> buffer;
    buffer.push_back(uint8_t(TXSync));
    buffer.push_back(uint8_t(messageData.size()));
    buffer.push_back(uint8_t(messageId));
    buffer.insert(buffer.end(), messageData.begin(), messageData.end());
    buffer.push_back(uint8_t(calculateCRC(buffer)));
    
    return sio.sendBuffer(buffer);
}

bool ANTMessage::getMessage(vector<uint8_t> &receivedData, volatile ANT_Message &messageId, vector<uint8_t> &messageData)
{
    size_t index = 0;
    for (index = 0; index < receivedData.size(); index++)
    {
        if (receivedData[index] == TXSync)
        {
            break;
        }
    }
    if (index == receivedData.size())
    {
        parseThreadLogStream << "Sync byte (0x" << hex << (uint8_t)TXSync << ") not found";
        parseThreadLogFlush();
        return false;
    }
    
    vector<uint8_t> buffer;

    uint8_t sync = receivedData[index++];
    buffer.push_back(sync);

    uint8_t msgLength = receivedData[index++];
    buffer.push_back(msgLength);

    messageId = (ANT_Message)receivedData[index++];
    buffer.push_back(messageId);

    messageData.assign(receivedData.begin() + index, receivedData.begin() + index + msgLength);
    buffer.insert(buffer.end(), receivedData.begin() + index, receivedData.begin() + index + msgLength);
    index += msgLength;

    uint8_t crc = receivedData[index++];
    buffer.push_back(crc);

    if (calculateCRC(buffer) != 0)
    {
        parseThreadLogStream << "Bad CRC in ANT packet";
        parseThreadLogFlush();
        return false;
    }
    
    receivedData.erase(receivedData.begin(), receivedData.begin() + index);

    return true;
}

uint8_t ANTMessage::calculateCRC(vector<uint8_t> &buffer)
{
    uint8_t crc = 0;
    for (unsigned i=0; i<buffer.size(); i++)
    {
        crc ^= buffer[i];
    }

    return crc;
}

volatile bool ANT::leaveFlag = false;

ANT::ANT() :
    channelStatus(ChannelStatusUnassigned)
{
    responseIdMap[MSG_ChannelEvent] = "Channel Event";
    responseIdMap[MSG_AssignChannel] = "Assign Channel";
    responseIdMap[MSG_SetNetworkKey] = "Set Network Key";
    responseIdMap[MSG_SetChannelPeriod] = "Set Channel Period";
    responseIdMap[MSG_SetChannelSearchTimeout] = "Set Channel Search Timeout";
    responseIdMap[MSG_SetChannelRadioFreq] = "Set Channel Radio Frequency";
    responseIdMap[MSG_SetSearchWaveform] = "Set Search Waveform";
    responseIdMap[MSG_SetChannelId] = "Set Channel Id";
    responseIdMap[MSG_OpenChannel] = "Open Channel";
    responseIdMap[MSG_SendBurstTransferPacket] = "Send Burst Transfer Packet";

    responseCodeMap[EventResponseNoError] = "Ok";
    responseCodeMap[EventRXSearchTimeout] = "RX Search Timeout";
    responseCodeMap[EventRXFail] = "RX Fail";
    responseCodeMap[EventTransferRXFailed] = "Transfer RX Failed";
    responseCodeMap[EventTransferTXCompleted] = "Transfer TX Completed";
    responseCodeMap[EventTransferTXFailed] = "Transfer TX Failed";
    responseCodeMap[EventTransferSequenceNumberError] = "Transfer Sequence Number Error";

    channelStatusMap[ChannelStatusUnassigned] = "Un-Assigned";
    channelStatusMap[ChannelStatusAssigned] = "Assigned";
    channelStatusMap[ChannelStatusSearching] = "Searching";
    channelStatusMap[ChannelStatusTracking] = "Tracking";
    
    channelPeriodMap[ChannelPeriod0_5Hz] = "0.5Hz";
    channelPeriodMap[ChannelPeriod1Hz] = "1Hz";
    channelPeriodMap[ChannelPeriod2Hz] = "2Hz";
    channelPeriodMap[ChannelPeriod4Hz] = "4Hz";
    channelPeriodMap[ChannelPeriod8Hz] = "8Hz";
    channelPeriodMap[ChannelPeriodReserved_5] = "Reserved(5)";
    channelPeriodMap[ChannelPeriodReserved_6] = "Reserved(6)";
    channelPeriodMap[ChannelPeriodEstablished] = "Established";
    
    clientDeviceStateMap[DeviceStateLink] = "Link";
    clientDeviceStateMap[DeviceStateAuthentication] = "Authentication";
    clientDeviceStateMap[DeviceStateTransport] = "Transport";
    clientDeviceStateMap[DeviceStateBusy] = "Busy";
    
    authTypesMap[PassThrough] = "Pass-through supported";
    authTypesMap[NA] = "N/A for client devices";
    authTypesMap[PairingOnly] = "Pairing only";
    authTypesMap[PassKeyAndPairing] = "PassKey and Pairing";

}

ANT::~ANT()
{
}

bool ANT::init(string deviceName, speed_t speed)
{
    if (!sio.open(deviceName, speed))
    {
        return false;
    }

    int rv = pthread_mutex_init(&receivedDataMutex, NULL);
    if (rv)
    {
        logStream << "Error initializing mutex (" << dec << errno << "): " << strerror(errno);
        logFlush();
        return false;
    }

    rv = pthread_create(&parseThreadHandle, NULL, (void *(*)(void*))&ANT::parseThread, this);
    if (rv)
    {
        logStream << "Error creating parsing thread (" << dec << errno << "): " << strerror(errno);
        logFlush();
        return false;
    }

    rv = pthread_create(&receiveThreadHandle, NULL, (void *(*)(void*))&ANT::receiveThread, this);
    if (rv)
    {
        logStream << "Error creating receiving thread (" << dec << errno << "): " << strerror(errno);
        logFlush();
        return false;
    }
    
    return true;
}

void ANT::leave()
{
    leaveFlag = true;
    
    int rv = pthread_join(parseThreadHandle, NULL);
    if (rv)
    {
        logStream << "Error joining parsing thread (" << dec << errno << "): " << strerror(errno);
        logFlush();
    }

    rv = pthread_join(receiveThreadHandle, NULL);
    if (rv)
    {
        logStream << "Error joining receving thread (" << dec << errno << "): " << strerror(errno);
        logFlush();
    }

    pthread_mutex_destroy(&receivedDataMutex);

    sio.close();
}

bool ANT::receiveBuffer()
{
    vector<uint8_t> buffer;
    if (!sio.receiveBuffer(buffer))
    {
        return false;
    }

    pthread_mutex_lock(&receivedDataMutex);
    receivedData.insert(receivedData.end(), buffer.begin(), buffer.end());
    pthread_mutex_unlock(&receivedDataMutex);

    return true;
}

void* ANT::receiveThread(ANT* ant)
{
    while(!leaveFlag)
    {
        if (!ant->receiveBuffer())
        {
            logStream << "Error receiving buffer !!!";
            logFlush();
            break;
        }
    }

    logStream << "Exitting receive thread";
    logFlush();
    
    leaveFlag = true;    
    
    pthread_exit((void *)0);
}

bool ANT::parseMessage()
{
    pthread_mutex_lock(&receivedDataMutex);
    if (receivedData.size() > 0)
    {
        vector<uint8_t> msgData;
        if (!ANTMessage::getMessage(receivedData, messageId, msgData))
        {
            return false;
        }
        pthread_mutex_unlock(&receivedDataMutex);

        uint8_t messageChannel = msgData[0] & 0x1F;
        //parseThreadLogStream << ">Channel: " << (unsigned)messageChannel << ", ";

        switch(messageId)
        {
            case MSG_ResponseEvent:
            {
                responseId = msgData[1];
                responseCode = msgData[2];
                parseThreadLogStream << "Response: " << responseIdMap[(unsigned)responseId] << "(" << (unsigned)responseId << ") " <<
                    responseCodeMap[(unsigned)responseCode] << "(" << (unsigned)responseCode << ")";
                break;
            }

            case MSG_ChannelStatus:
            {
                channelStatus = msgData[1];
                parseThreadLogStream << "Channel Status: " << channelStatusMap[(unsigned)channelStatus] << "(" << (unsigned)channelStatus << ")";
                break;
            }

            case MSG_SetChannelId:
            {
                uint16_t deviceId = (msgData[2] << 8) | msgData[1];
                uint8_t deviceType = msgData[3];
                uint8_t transmissionType = msgData[4];
                parseThreadLogStream << "Channel Id: DeviceId=" << deviceId << ", DeviceType=" << (unsigned)deviceType << ", TransmissionType=" << (unsigned)transmissionType;
                break;
            }

            case MSG_Capabilities:
            {
                parseThreadLogStream << "Capabilities";
                break;
            }

            case MSG_SendBroadcastData:
            {
                broadcast = true;

                uint8_t page = msgData[1];
                bool pageToggle = page & 0x80;
                page &= 0x7F;
                
                //parseThreadLogStream << "Broadcast data (Page=" << hex << (unsigned)page << "): " << GarminConvert::gHex(msgData) << " : ";

                switch(page)
                {
                    case ANTFSBeacon:
                    {
                        ANTFSBeaconFormat beacon = { 0 };
                        memcpy(&beacon, &msgData.front()+1, sizeof(beacon));

                        uint8_t channelPeriod = beacon.status1.beaconChannelPeriod;
                        /*parseThreadLogStream << "BeaconChannelPeriod=" << channelPeriodMap[channelPeriod] << ", ";
                        parseThreadLogStream << "Pairing=" << (beacon.status1.pairingEnabled?"Enabled":"Disabled") << ", ";
                        parseThreadLogStream << "Upload=" << (beacon.status1.uploadEnabled?"Enabled":"Disabled") << ", ";
                        parseThreadLogStream << "Data=" << (beacon.status1.dataAvailable?"Available":"Not available") << ", ";
			*/
                        clientDeviceState = beacon.status2.clientDeviceState;
                        //parseThreadLogStream << "ClientDeviceState=" << clientDeviceStateMap[(unsigned)clientDeviceState] << ", ";

                        uint8_t authType = beacon.authType;
                        //parseThreadLogStream << "AuthType=" << authTypesMap[authType] << ", ";

                        if (clientDeviceState == DeviceStateLink)
                        {
			  //parseThreadLogStream << "ManufacturerID=" << beacon.devDescr.manufacturerID << ", DeviceType=" << hex << beacon.devDescr.deviceType;
                        }
                        else
                        {
			  //parseThreadLogStream << "SN=" << hex << beacon.hostSN;
                        }
                        
                        break;
                    }

                    case 1:
                    {
#pragma pack(1)
                        struct Page1
                        {
                            uint8_t operTime[3];
                            uint16_t curEventTime; // 1/1024 seconds
                            uint8_t eventCount;
                            uint8_t hr;
                        } msg;
#pragma pack()
                        memcpy(&msg, &msgData.front()+2, sizeof(msg));

                        unsigned opTime = ((msg.operTime[2] << 16) | (msg.operTime[1] << 8) | msg.operTime[0]) << 1;
                        
                        parseThreadLogStream << dec <<
                            "HR=" << (unsigned)msg.hr << ", " <<
                            "Count=" << (unsigned)msg.eventCount << ", " <<
                            "OperationlTime=" << opTime/60 << ":" << opTime%60;
                        
                        break;
                    }

                    case 2:
                    {
#pragma pack(1)
                        struct Page2
                        {
                            uint8_t manufacturerID;
                            uint16_t serialNumber;
                            uint16_t curEventTime; // 1/1024 seconds
                            uint8_t eventCount;
                            uint8_t hr;
                        } msg;
#pragma pack()
                        memcpy(&msg, &msgData.front()+2, sizeof(msg));

                        parseThreadLogStream << dec << 
                            "HR=" << (unsigned)msg.hr << ", " <<
                            "Count=" << (unsigned)msg.eventCount << ", " <<
                            "Manufacturer=" << (unsigned)msg.manufacturerID << ", " <<
                            "SerialNumber=" << msg.serialNumber;
                        
                        break;
                    }
                    
                    case 3:
                    {
#pragma pack(1)
                        struct Page3
                        {
                            uint8_t hwVersion;
                            uint8_t swVersion;
                            uint8_t modelNumber;
                            uint16_t curEventTime; // 1/1024 seconds
                            uint8_t eventCount;
                            uint8_t hr;
                        } msg;
#pragma pack()
                        memcpy(&msg, &msgData.front()+2, sizeof(msg));

                        parseThreadLogStream << dec << 
                            "HR=" << (unsigned)msg.hr << ", " <<
                            "Count=" << (unsigned)msg.eventCount << ", " <<
                            "HWVersion=" << (unsigned)msg.hwVersion << ", " <<
                            "SWVersion=" << (unsigned)msg.swVersion << ", " <<
                            "ModelNumber=" << (unsigned)msg.modelNumber;
                        
                        break;
                    }
                   
                    case 4:
                    {
#pragma pack(1)
                        struct Page4
                        {
                            uint8_t manufacturerSpecific;
                            uint16_t prevEventTime; // 1/1024 seconds
                            uint16_t curEventTime; // 1/1024 seconds
                            uint8_t eventCount;
                            uint8_t hr;
                        } msg;
#pragma pack()
                        memcpy(&msg, &msgData.front()+2, sizeof(msg));

                        unsigned prevTime = msg.prevEventTime;
                        unsigned curTime = msg.curEventTime;
                        if (curTime < prevTime)
                        {
                            curTime += 0x10000;
                        }
                            
                        double rr = ((double)curTime - (double)prevTime)/1024.0;
                        
                        parseThreadLogStream << dec <<
                            "HR=" << (unsigned)msg.hr << ", " <<
                            "Count=" << (unsigned)msg.eventCount << ", " <<
                            "RR=" << rr;
                        
                        break;
                    }
                }

                break;
            }

            case MSG_SendBurstTransferPacket:
            {
                uint8_t seq = (msgData[0] >> 5) & 0x3;
                lastBurst = msgData[0] >> 7;

                msgData.erase(msgData.begin());
                burstData.insert(burstData.end(), msgData.begin(), msgData.end());

                //SSP parseThreadLogStream << "Burst Data: Sequence=" << (unsigned)seq << " Last=" << string(lastBurst?"Yes":"No") << ":\t" << GarminConvert::hexDump(msgData);
                break;
            }

            default:
            {
                parseThreadLogStream << "Message Id: " << hex << messageId;
            }
        }
        
        //parseThreadLogFlush();
    }
    else
    {
        pthread_mutex_unlock(&receivedDataMutex);
        usleep(sleepTime);
    }
    
    return true;
}

void* ANT::parseThread(ANT* ant)
{
    while(!leaveFlag)
    {
        if (!ant->parseMessage())
        {
            logStream << "Error parsing message !!!";
            logFlush();
            break;
        }
    }

    logStream << "Exitting parse thread";
    logFlush();
    
    leaveFlag = true;
    
    pthread_exit((void *)0);
}


bool ANT::waitMessage(uint8_t id)
{
//    messageId = MSG_Null;

    while(messageId != id)
    {
        if (responseId == MSG_ChannelEvent)
        {
            switch (responseId)
            {
                case EventRXFail:
                {
                    logStream << "! EventRXFail" << endl;
                    logFlush();

                    return false;
                }
                case EventTransferTXFailed:
                {
                    logStream << "! EventTransferTXFailed" << endl;
                    logFlush();

                    return false;
                }

                default:
                {
                    usleep(sleepTime);
                }
            }
        }
    }

    return true;
}

bool ANT::waitResponse(uint8_t id)
{
//    responseId = MSG_Null;
    do
    {
        usleep(sleepTime);

        if (responseId == MSG_ChannelEvent)
        {
            switch (responseCode)
            {
                case EventRXFail:
                {
                    logStream << "! RX failed" << endl;
                    logFlush();

                    return false;
                }
                case EventTransferTXFailed:
                {
                    logStream << "! TX failed" << endl;
                    logFlush();

                    return false;
                }
            }
        }
    }
    while(responseId != id);

    return true;
}

bool ANT::waitBroadcast()
{
    broadcast = false;
    do
    {
        usleep(sleepTime);

        if (clientDeviceState == DeviceStateBusy)
        {
            continue;
        }

        if (responseId == MSG_ChannelEvent)
        {
            switch (responseCode)
            {
                case EventRXFail:
                    return false;
            }
        }
    }
    while(!broadcast);

    return true;
}

bool ANT::waitBurst(vector<uint8_t> &data)
{
    burstData.clear();
    lastBurst = false;

    do
    {
        usleep(sleepTime);

        if (responseId == MSG_ChannelEvent)
        {
            switch (responseCode)
            {
                case EventRXFail:
                case EventTransferRXFailed:
                    return false;
            }
        }
    }
    while(!lastBurst);

    data = burstData;

    return true;
}

bool ANT::waitBurst(uint8_t data[], unsigned len)
{
    vector<uint8_t> tmpData;
    if (!waitBurst(tmpData))
    {
        return false;
    }

    size_t tmpLen = tmpData.size();
    if (tmpLen > len)
    {
        tmpLen = len;
    }

    memcpy(data, &tmpData.front(), tmpLen);

    return true;
}

bool ANT::getChannelStatus(uint8_t &status)
{
    status = channelStatus;

    return true;
}

string ANT::getChannelStatusString()
{
    return channelStatusMap[(unsigned)channelStatus];
}

bool ANT::resetSystem()
{
    logStream << "<RESET system";
    logFlush();

    vector<uint8_t> data;
    data.push_back(0);
    if (!ANTMessage::sendMessage(sio, MSG_ResetSystem, data))
    {
        return false;
    }
    usleep(500000);

    return true;
}

bool ANT::setNetworkKey(uint8_t network, vector<uint8_t> &key)
{
    logStream << "<Set network key (network=" << (unsigned)network << ", key=" << GarminConvert::gHex(key) << ")";
    logFlush();

    vector<uint8_t> data;
    data.push_back(network);
    data.insert(data.end(), key.begin(), key.end());
    if (!ANTMessage::sendMessage(sio, MSG_SetNetworkKey, data))
    {
        logStream << "!Error sending SetNetworkKey command";
        logFlush();

        return false;
    }
    if (!waitResponse(MSG_SetNetworkKey))
    {
        logStream << "!Error waiting response to SetNetworkKey command";
        logFlush();

        return false;
    }

    return true;
}

bool ANT::assignChannel(uint8_t channel, ChannelType type, uint8_t network)
{
    logStream << "<Assign channel (channel=" << dec << (unsigned)channel << ", type=" << (unsigned)type << ", network=" << (unsigned)network << ")";
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    data.push_back(type);
    data.push_back(network);
    if (!ANTMessage::sendMessage(sio, MSG_AssignChannel, data))
    {
        return false;
    }
    if (!waitResponse(MSG_AssignChannel))
    {
        return false;
    }

    return true;
}

bool ANT::setChannelPeriod(uint8_t channel, uint16_t period)
{
    logStream << "<Set channel period (channel=" << (unsigned)channel << ", period=" << period << ")";
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    uint8_t *bPeriod = (uint8_t *)&period;
    data.push_back(bPeriod[0]);
    data.push_back(bPeriod[1]);
    if (!ANTMessage::sendMessage(sio, MSG_SetChannelPeriod, data))
    {
        return false;
    }
    if (!waitResponse(MSG_SetChannelPeriod))
    {
        return false;
    }

    return true;
}

bool ANT::setChannelSearchTimeout(uint8_t channel, uint8_t timeout)
{
    logStream << "<Set channel search timeout (channel=" << (unsigned)channel << ", timeout=" << (unsigned)timeout << ")";
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    data.push_back(timeout);
    if (!ANTMessage::sendMessage(sio, MSG_SetChannelSearchTimeout, data))
    {
        return false;
    }
    if (!waitResponse(MSG_SetChannelSearchTimeout))
    {
        return false;
    }

    return true;
}

bool ANT::setChannelRFFreq(uint8_t channel, uint8_t frequency)
{
    logStream << "<Set channel radio frequency (channel=" << (unsigned)channel << ", frequency=24" << (unsigned)frequency << "Mhz)";
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    data.push_back(frequency);
    if (!ANTMessage::sendMessage(sio, MSG_SetChannelRadioFreq, data))
    {
        return false;
    }
    if (!waitResponse(MSG_SetChannelRadioFreq))
    {
        return false;
    }

    return true;
}

bool ANT::setSearchWaveform(uint8_t channel, uint16_t waveform)
{
    logStream << "<Set search waveform (channel=" << (unsigned)channel << ", waveform=" << waveform << ")";
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    uint8_t *bWaveform = (uint8_t *)&waveform;
    data.push_back(bWaveform[0]);
    data.push_back(bWaveform[1]);
    if (!ANTMessage::sendMessage(sio, MSG_SetSearchWaveform, data))
    {
        return false;
    }
    if (!waitResponse(MSG_SetSearchWaveform))
    {
        return false;
    }

    return true;
}

bool ANT::setChannelId(uint8_t channel, uint16_t deviceNum, bool pairing,
    uint8_t deviceType, uint8_t transmissionType)
{
    logStream << "<Set channel ID (channel=" << (unsigned)channel << ", deviceNum=" << deviceNum << ", pairing=" << string(pairing?"Yes":"No") <<
        ", deviceType=" << (unsigned)deviceType << ", transmissionType=" << (unsigned)transmissionType << ")";
    logFlush();

    struct DeviceTypeBits
    {
        uint8_t type:7;
        uint8_t pairing:1;
    };

    union DeviceTypeU
    {
        DeviceTypeBits typeBits;
        uint8_t type;
    };

    vector<uint8_t> data;
    data.push_back(channel);
    uint8_t *bDeviceNum = (uint8_t *)&deviceNum;
    data.push_back(bDeviceNum[0]);
    data.push_back(bDeviceNum[1]);
    DeviceTypeU dtu;
    dtu.typeBits.pairing = pairing;
    dtu.typeBits.type = deviceType;
    data.push_back(dtu.type);
    data.push_back(transmissionType);
    if (!ANTMessage::sendMessage(sio, MSG_SetChannelId, data))
    {
        return false;
    }
    if (!waitResponse(MSG_SetChannelId))
    {
        return false;
    }

    return true;
}

bool ANT::openChannel(uint8_t channel)
{
    logStream << "<Open channel (channel=" << (unsigned)channel << ")";
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    if (!ANTMessage::sendMessage(sio, MSG_OpenChannel, data))
    {
        return false;
    }
    if (!waitResponse(MSG_OpenChannel))
    {
        return false;
    }

    return true;
}

bool ANT::requestMessage(uint8_t channel, ANT_Message msgId)
{
    logStream << "<Request message (channel=" << (unsigned)channel << ", messageId=" << msgId << ") ";
    switch(msgId)
    {
        case MSG_SetChannelId:
        {
            logStream << "(Set Channel Id)";
            break;
        }
        case MSG_ChannelStatus:
        {
            logStream << "(Channel Status)";
            break;
        }
        case MSG_Capabilities:
        {
            logStream << "(Capabilities)";
            break;
        }
    }
    logFlush();

    messageId = MSG_Null;
    vector<uint8_t> data;
    data.push_back(channel);
    data.push_back(msgId);
    if (!ANTMessage::sendMessage(sio, MSG_RequestMessage, data))
    {
        return false;
    }
    
    if (!waitMessage(msgId))
    {
        return false;
    }

    return true;
}

bool ANT::sendAcknowledgedData(uint8_t channel, vector<uint8_t> &ackData)
{
    logStream << "<Send acknowledged data (channel=" << (unsigned)channel << "): " << GarminConvert::gHex(ackData);
    logFlush();

    vector<uint8_t> data;
    data.push_back(channel);
    data.insert(data.end(), ackData.begin(), ackData.end());
    if (!ANTMessage::sendMessage(sio, MSG_SendAcknowledgedData, data))
    {
        return false;
    }

    return true;
}

bool ANT::sendAcknowledgedData(uint8_t channel, uint8_t data[], unsigned len)
{
    vector<uint8_t> ackData(data, data+len);

    return sendAcknowledgedData(channel, ackData);
}

bool ANT::sendBurstTransferData(uint8_t channel, vector<uint8_t> &bData)
{
  //logStream << "<Send burst transfer data (channel=" << (unsigned)channel << "): " << GarminConvert::gHex(bData);
  //logFlush();

    responseId = MSG_Null;

#pragma pack(1)
    struct D1Bits
    {
        uint8_t channel:5;
        uint8_t sequence:2;
        uint8_t last:1;
    };

    union D1TypeU
    {
        D1Bits d1Bits;
        uint8_t d1u;
    };
#pragma pack()
    uint16_t sequence = 0;
    bool last = false;
    for(unsigned i=0; i<bData.size(); i+=8)
    {
        vector<uint8_t>::iterator itFrom, itTo;
        itFrom = bData.begin()+i;
        itTo = itFrom+8;
        if (itTo >= bData.end())
        {
            itTo = bData.end();
            last = true;
        }

        vector<uint8_t> data;
        D1TypeU dtu;
        dtu.d1Bits.channel = channel;
        dtu.d1Bits.sequence = sequence;
        dtu.d1Bits.last = last;
        data.push_back(dtu.d1u);
        data.insert(data.end(), itFrom, itTo);
        if (!ANTMessage::sendMessage(sio, MSG_SendBurstTransferPacket, data))
        {
            return false;
        }
        
        usleep(burstSleepTime);

        if (++sequence > 3)
        {
            sequence = 1;
        }
    }

    return true;
}

bool ANT::sendBurstTransferData(uint8_t channel, uint8_t data[], unsigned len)
{
    vector<uint8_t> burstData(data, data+len);

    return sendBurstTransferData(channel, burstData);
}

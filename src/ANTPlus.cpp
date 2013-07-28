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

#include "ANTPlus.h"
#include "FIT.h"
#include "GarminConvert.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

ANTPlus::ANTPlus() :
    maxAttempts(5)
{
    downloadResponseCodesMap[DownloadResponseOk] = "Download Request Ok";
    downloadResponseCodesMap[DownloadResponseNotExist] = "Data does not exist";
    downloadResponseCodesMap[DownloadResponseNotDownloadable] = "Data exist but is not downloadable";
    downloadResponseCodesMap[DownloadResponseNotReady] = "Not Ready to download";
    downloadResponseCodesMap[DownloadResponseRequestInvalid] = "Request invalid";
    downloadResponseCodesMap[DownloadResponseCRCIncorrect] = "CRC incorrect";
}

ANTPlus::~ANTPlus()
{
}

void ANTPlus::leave(uint8_t channel)
{
    disconnect(channel);
    waitBroadcast();

    ANT::leave();
}

bool ANTPlus::link(uint8_t channel, uint8_t freq, uint8_t beaconPeriod, uint32_t hostSN)
{
    logStream << "# Link at frequency 24" << dec << setw(2) << setfill('0') << (unsigned)freq << " Mhz, beacon period=" << channelPeriodMap[beaconPeriod];
    logFlush();

    for (int i=0; i<maxAttempts; i++)
    {
        if (waitBroadcast())
        {
            ANTFSCommandFormat cmd = { 0 };
            cmd.code = ANTFSCommand;
            cmd.command = CommandLink;
            cmd.param1 = freq;
            cmd.param2 = beaconPeriod;
            cmd.hostSN = hostSN;

            if (sendAcknowledgedData(channel, (uint8_t *)&cmd, sizeof(cmd)))
            {
                if (waitResponse(MSG_ChannelEvent))
                {
                    return true;
                }
            }
        }

        logStream << "Retrying #" << i+1;
        logFlush();
    }
    
    return false;
}

bool ANTPlus::disconnect(uint8_t channel, bool returnToBroadcast)
{
    logStream << "# Disconnect";
    logFlush();

    for (int i=0; i<maxAttempts; i++)
    {
        if (waitBroadcast())
        {
            ANTFSCommandFormat cmd = { 0 };
            cmd.code = ANTFSCommand;
            cmd.command = CommandDisconnect;
            cmd.param1 = returnToBroadcast;
            cmd.param2 = 0;
            cmd.hostSN = 0;

            if (sendAcknowledgedData(channel, (uint8_t *)&cmd, sizeof(cmd)))
            {
                if (waitResponse(MSG_ChannelEvent))
                {
                    return true;
                }
            }
        }

        logStream << "Retrying #" << i+1;
        logFlush();
    }

    return true;
}

bool ANTPlus::requestSN(uint8_t channel, uint32_t hostSN, string& unitName, uint32_t& unitId)
{
    logStream << "# Request device serial number";
    logFlush();

    for (int i=0; i<maxAttempts; i++)
    {
        if (waitBroadcast())
        {
            ANTFSCommandFormat cmd = { 0 };
            cmd.code = ANTFSCommand;
            cmd.command = CommandAuthenticate;
            cmd.param1 = RequestClientDeviceSerialNumber;
            cmd.param2 = 0;
            cmd.hostSN = hostSN;

            if (sendAcknowledgedData(channel, (uint8_t *)&cmd, sizeof(cmd)))
            {
                if (waitResponse(MSG_ChannelEvent))
                {
                    ANTPlusRequestSerialAnswer answer;
                    if (waitBurst((uint8_t *)&answer, sizeof(answer)))
                    {
                        if (waitResponse(MSG_ChannelEvent))
                        {
                            unitId = answer.ans.unitId;
                            unitName = GarminConvert::gString((uint8_t *)&answer.unitName, sizeof(answer.unitName));

                            return true;
                        }
                    }
                }
            }
        }

        logStream << "Retrying #" << i+1;
        logFlush();
    }
    
    return false;
}

bool ANTPlus::devicePair(uint8_t channel, uint32_t hostSN, string pcName, uint32_t& unitId, uint64_t& key)
{
    logStream << "# Device paring";
    logFlush();

    size_t nameLen = pcName.size();
    if (nameLen > MAX_NAME_LEN)
    {
        nameLen = MAX_NAME_LEN;
    }
    
    for (int i=0; i<maxAttempts; i++)
    {
        if (waitBroadcast())
        {
            ANTPlusPairingCommand cmd = { 0 };
            cmd.cmd.code = ANTFSCommand;
            cmd.cmd.command = CommandAuthenticate;
            cmd.cmd.param1 = RequestPairing;
            cmd.cmd.param2 = nameLen;
            cmd.cmd.hostSN = hostSN;
            memset(cmd.name, 0, sizeof(cmd.name));
            strncpy(cmd.name, pcName.c_str(), nameLen);

            if (sendBurstTransferData(channel, (uint8_t *)&cmd, sizeof(cmd)))
            {
                if (waitResponse(MSG_ChannelEvent))
                {
                    ANTPlusAuthenticateAnswer answer;
                    if (waitBurst((uint8_t *)&answer, sizeof(answer)))
                    {
                        unitId = answer.ans.unitId;
                        key = answer.key;
                        return true;
                    }
                }
            }
        }

        logStream << "Retrying #" << i+1;
        logFlush();
    }

    return false;
}

bool ANTPlus::authenticate(uint8_t channel, uint32_t hostSN, uint64_t key)
{
    logStream << "# Authenticate";
    logFlush();

    for (int i=0; i<maxAttempts; i++)
    {
        if (waitBroadcast())
        {
            ANTPlusPasskeyAuthenticationCommand cmd = { 0 };
            cmd.cmd.code = ANTFSCommand;
            cmd.cmd.command = CommandAuthenticate;
            cmd.cmd.param1 = RequestPasskeyExchange;
            cmd.cmd.param2 = sizeof(cmd.key);
            cmd.cmd.hostSN = hostSN;
            cmd.key = key;
            cmd.unkn = 0;

            if (sendBurstTransferData(channel, (uint8_t *)&cmd, sizeof(cmd)))
            {
                if (waitResponse(MSG_ChannelEvent))
                {
                    ANTPlusAnswer answer = { 0 };
                    if (waitBurst((uint8_t *)&answer, sizeof(answer)))
                    {
                        logStream << "Authentication " << ((answer.responseType == AuthenticationAccepted)?"Accepted":"Rejected");
                        logFlush();
                        return true;
                    }
                }
            }
        }

        logStream << "Retrying #" << i+1;
        logFlush();
    }
    
    
    return true;
}

bool ANTPlus::download(uint8_t channel, uint16_t fileIndex, vector<uint8_t> &data)
{
  logStream << "# Downloading file 0x" << hex << fileIndex << " (" << dec << fileIndex << ")";
    logFlush();

    data.clear();

    uint16_t crc = 0;
    uint32_t offset = 0;
    uint16_t CRCseed = 0;
    uint32_t fileSize = 0;
    bool initialRequest = true;

    do
    {
        if (waitBroadcast())
        {
            ANTPlusDownloadCommand cmd = { 0 };
            cmd.code = ANTFSCommand;
            cmd.command = CommandDownloadRequest;
            cmd.fileIndex = fileIndex;
            cmd.offset = offset;
            cmd.initialRequest = initialRequest;
            cmd.unknown = 0;
            cmd.CRCseed = CRCseed;
            cmd.maximumBlockSize = 0;

            vector<uint8_t> packetData;
            if (sendBurstTransferData(channel, (uint8_t *)&cmd, sizeof(cmd)))
            {
                if (waitResponse(MSG_ChannelEvent))
                {
                    if (waitBurst(packetData))
                    {
		      //logStream << "Burst packet size: " << dec << packetData.size();
		      //logFlush();
                        
                        ANTPlusDownloadHeader packetHeader = { 0 };
                        //logStream << "Header length: " << dec << sizeof(packetHeader);
                        //logFlush();

                        if (packetData.size() >= sizeof(packetHeader))
                        {
                            memcpy(&packetHeader, &packetData.front(), sizeof(packetHeader));
                            packetData.erase(packetData.begin(), packetData.begin()+sizeof(packetHeader));
                            //logStream << "Header: [" << GarminConvert::gHex((uint8_t*)&packetHeader, sizeof(packetHeader)) << "]";
                            //logFlush();
                            //logStream << "Response code: " << downloadResponseCodesMap[packetHeader.response] << "(" << dec << (unsigned)packetHeader.response << ")";
                            //logFlush();
                            //logStream << "Data bytes remain: " << dec << packetHeader.dataRemain;
                            //logFlush();
                            //logStream << "Data offset: " << dec << packetHeader.dataOffset;
                            //logFlush();
                            //logStream << "File size: " << dec << packetHeader.fileSize;
			    //logReturn();
			    
			    int proc = (double)packetHeader.dataOffset/(double)packetHeader.fileSize*100;
			    logStream << "Data: " << dec << setw(3) << proc << "% " << packetHeader.dataOffset << "/" << packetHeader.fileSize;
                            logFlush();
                        }
                        else
                        {
                            logStream << "Packet data length " << dec << packetData.size() << " is too short to get header (" << sizeof(packetHeader) << " bytes)" << endl;
                            logFlush();

                            return false;
                        }

                        if (initialRequest)
                        {
                            fileSize = packetHeader.fileSize;
                            if (fileSize == 0)
                            {
                                break;
                            }
                        }
                        
                        //logStream << "Data bytes remain in buffer: " << dec << packetData.size();
                        //logFlush();

                        if (packetData.size() >= packetHeader.dataRemain)
                        {
                            FIT fit;
                            for (int i=0; i<packetHeader.dataRemain; i++)
                            {
                                crc = fit.CRC_byte(crc, packetData[i]);
                            }
                            //logStream << "CRC: " << hex << uppercase << setw(4) << setfill('0') << crc;
                            //logFlush();

                            data.insert(data.end(), packetData.begin(), packetData.begin() + packetHeader.dataRemain);
                            packetData.erase(packetData.begin(), packetData.begin() + packetHeader.dataRemain);

                            offset += packetHeader.dataRemain;
                            
                            //logStream << "Offset=" << dec << offset;
                            //logFlush();
                        }
                        else
                        {
                            logStream << "Packet data length " << dec << packetData.size() << " is too short to get data (" << packetHeader.dataRemain << " bytes)" << endl;
                            logFlush();
                            
                            return false;
                        }
                        
                        ANTPlusDownloadFooter packetFooter;
                        if (packetData.size() >= sizeof(packetFooter))
                        {
                            memcpy(&packetFooter, &packetData.front(), sizeof(packetFooter));
                            packetData.erase(packetData.begin(), packetData.begin() + sizeof(packetFooter));

                            CRCseed = packetFooter.CRCseed;
                            //logStream << "CRCseed: " << hex << uppercase << setw(4) << setfill('0') << CRCseed;
                            //logFlush();
                        }
                        else
                        {
                            logStream << "Packet data length " << dec << packetData.size() << " is too short to get footer (" << sizeof(packetFooter) << " bytes)" << endl;
                            logFlush();
                            
                            return false;
                        }

                        initialRequest = false;
                    }
                }
            }
        }
    }
    while(data.size() < fileSize);

    logStream << "Data size: " << dec << data.size();
    logFlush();
/*    
    std::stringstream fileName;
    fileName << "ANTFile#";
    fileName << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << fileIndex;
    fileName << ".bin";

    std::fstream f;
    f.open(fileName.str().c_str(), std::fstream::out);
    for (int i=0; i<data.size(); i++)
    {
        f << data.at(i);
    }
    f.close();
*/    
    return true;
}

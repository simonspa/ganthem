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
#include "GPX.h"
#include "CommandLineOptions.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>

//#define HRM

#define HOSTSN 0x1

int main(int argc, char *argv[])
{
    const char* optString = "hplu";
    CommandLineOptions clOpt(argc, argv, optString);

    logStream << "Welcome to ganthem!";
    logFlush();

    // Check for already copied acitivies:
    //    if (clOpt.isSet('u')) {    
      ifstream is("activities.dat");
      istream_iterator<int> start(is), end;
      vector<int> numbers(start, end);
      logStream << "Already downloaded " << numbers.size() << " activies:";
      logFlush();

      // print the numbers to stdout
      logStream << hex;
      copy(numbers.begin(), numbers.end(),ostream_iterator<double>(logStream, " "));
      logStream << dec;
      logFlush();
      // }

    ANTPlus ant;
    if(!ant.init("/dev/ttyUSB0", B115200))
    {
        return EXIT_FAILURE;
    }

    if (!ant.resetSystem())
    {
        logStream << "Error resetting ANT system";
        logFlush();
        return EXIT_FAILURE;
    }

    uint8_t channel = 0;
    if (!ant.requestMessage(channel, MSG_ChannelStatus))
    {
        logStream << "Error requesting channel status";
        logFlush();
        return EXIT_FAILURE;
    }
    
    uint8_t channelStatus = 0;
    if (!ant.getChannelStatus(channelStatus))
    {
        logStream << "Error getting channel status";
        logFlush();
        return EXIT_FAILURE;
    }
    
    if (channelStatus != ChannelStatusUnassigned)
    {
        logStream << "Channel " << (unsigned)channel << " is already assigned";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    if (clOpt.isSet('h'))
    {
        uint8_t net = 0;
        uint8_t netKey[8] = { 0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45 }; // ANT+ Network Key (HRM)
        
        vector<uint8_t> networkKey(netKey, netKey+sizeof(netKey));
        if (!ant.setNetworkKey(net, networkKey))
        {
            logStream << "Error setting network key";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }
        
        if (!ant.assignChannel(channel, ReceiveChannel, net))
        {
            logStream << "Error assigning channel";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }
        
        uint16_t period = 8070; // (HRM)
        if (!ant.setChannelPeriod(channel, period))
        {
            logStream << "Error setting channel period";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }

        uint16_t timeout = 5; // timeout = N / 2.5s, 0x00 = immediate, 0xFF = unlimited (HRM)
        if (!ant.setChannelSearchTimeout(channel, timeout))
        {
            logStream << "Error setting search timeout";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }

        uint8_t frequency = 57; // 2400 Mhz + 57 Mhz = 2457 Mhz (ANT+ HRM, Spd, Cad)
        if (!ant.setChannelRFFreq(channel, frequency))
        {
            logStream << "Error setting radio frequency";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }

        uint16_t deviceNum = 0;
        bool pairing = false;
        uint8_t deviceType =  0x78;
        uint8_t transmissionType = 0; // 0x05;

        if (!ant.setChannelId(channel, deviceNum, pairing, deviceType, transmissionType))
        {
            logStream << "Error setting channel ID";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }

        if (!ant.openChannel(channel))
        {
            logStream << "Error opening channel";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }

        for (;;)
        {
            sleep(1);
        }
        
        return EXIT_SUCCESS;
    }
    
    uint8_t net = 0;
    uint8_t netKey[8] = { 0xA8, 0xA4, 0x23, 0xB9, 0xF5, 0x5E, 0x63, 0xC1 }; // ANTFS Network Key
    vector<uint8_t> networkKey(netKey, netKey+sizeof(netKey));
    if (!ant.setNetworkKey(net, networkKey))
    {
        logStream << "Error setting network key";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    if (!ant.assignChannel(channel, ReceiveChannel, net))
    {
        logStream << "Error assigning channel";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    uint16_t period = 4096; // period = 32768 / 8,00 Hz = 4096
    if (!ant.setChannelPeriod(channel, period))
    {
        logStream << "Error setting channel period";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    uint16_t timeout = 0xFF; // timeout = N / 2.5s, 0x00 = immediate, 0xFF = unlimited
    if (!ant.setChannelSearchTimeout(channel, timeout))
    {
        logStream << "Error setting search timeout";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    uint8_t frequency = 50; // 2400 Mhz + 50 Mhz = 2450 Mhz (ANTFS)
    if (!ant.setChannelRFFreq(channel, frequency))
    {
        logStream << "Error setting radio frequency";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    uint16_t waveform = 83;
    if (!ant.setSearchWaveform(channel, waveform))
    {
        logStream << "Error setting wave form";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    uint16_t deviceNum = 0;
    bool pairing = false;
    uint8_t deviceType = 0;
    uint8_t transmissionType = 0; // 0x05;
    if (!ant.setChannelId(channel, deviceNum, pairing, deviceType, transmissionType))
    {
        logStream << "Error setting channel ID";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    if (!ant.openChannel(channel))
    {
        logStream << "Error opening channel";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    if (!ant.requestMessage(channel, MSG_ChannelStatus))
    {
        logStream << "Error requesting channel status";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    channelStatus = 0;
    do
    {
        if (!ant.getChannelStatus(channelStatus))
        {
            logStream << "Error getting channel status";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }
        logStream << "Channel Status: " << ant.channelStatusMap[(unsigned)channelStatus] << "(" << (unsigned)channelStatus << ")";
        logFlush();
    }
    while(channelStatus != ChannelStatusSearching);
        
    uint8_t beaconPeriod = 4;
    if (!ant.link(channel, frequency, beaconPeriod, HOSTSN))
    {
        logStream << "Error establishing link";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    string unitName;
    uint32_t unitId;
    if (!ant.requestSN(channel, HOSTSN, unitName, unitId))
    {
        logStream << "Error requesting serial number";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    logStream << "Unit: " << unitName << ", Id: " << dec << unitId;
    logFlush();

    if (clOpt.isSet('p'))
    {
        logStream << "# Pairing";
        logFlush();

        string name("ganthem");
        uint64_t key;
        if (!ant.devicePair(channel, HOSTSN, name, unitId, key))
        {
            logStream << "Error paring with unit";
            logFlush();
            ant.leave(channel);
            return EXIT_FAILURE;
        }

        logStream << "Paired with unit " << unitId << ", key: " << std::hex << std::setw(16) << std::setfill('0') << std::uppercase << key;
        logFlush();

        std::string fileName("ganthem.ant.key");
        std::fstream f;
        f.open(fileName.c_str(), std::fstream::out);
        f << std::hex << std::setw(16) << std::setfill('0') << std::uppercase << key;
        f.close();
        
        return EXIT_SUCCESS;
    }
    

    // Passkey authentication
    uint64_t authKey = 0;
    std::string fileName("ganthem.ant.key");
    std::fstream f;
    f.open(fileName.c_str(), std::fstream::in);
    f >> std::hex >> authKey;
    f.close();
    
    if (!ant.authenticate(channel, HOSTSN, authKey))
    {
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    vector<uint8_t> data;
    if (!ant.download(channel, 0, data))
    {
        logStream << "Error downloading ANTFS directory";
        logFlush();
        ant.leave(channel);
        return EXIT_FAILURE;
    }

    FIT fit;
    ZeroFileContent zeroFileContent;
    fit.parseZeroFile(data, zeroFileContent);


    logStream << "# " << dec << zeroFileContent.activityFiles.size() << " activity files available.";
    logFlush();
 
    // Prepare file list to be downloaded:
    vector<uint16_t> filelist;

    for (int i=0; i<zeroFileContent.activityFiles.size(); i++) {

      // LATEST mode: Only download the last five activities:      
      if (clOpt.isSet('l') && i < zeroFileContent.activityFiles.size()-5)
	continue;

      // UPDATE mode: Only download unknown activities:
      if (clOpt.isSet('u') && std::find(numbers.begin(), numbers.end(), zeroFileContent.activityFiles[i]) != numbers.end())
	continue;

      filelist.push_back(zeroFileContent.activityFiles[i]);
    }


    logStream << "# " << dec << filelist.size() << " activity files to be downloaded.";
    logFlush();

    // Prepare file for logging downloaded activities:
    ofstream output;
    output.open("activities.dat", ios::out | ios::app );

    for (int i=0; i<filelist.size();i++) {
      GPX gpx;
      logStream << "# Transfer activity file 0x" << hex << (int)filelist[i] 
		<< " (" << dec << i << "/" << dec << filelist.size() << ")";
      logFlush();
      
      if (!ant.download(channel, filelist[i], data))
	break;

      if (data.size() >= sizeof(FITHeader))
	fit.parse(data, gpx);
      
      // Store the track immediately:
      stringstream sstm;
      sstm << "activities/track" << (int)filelist[i] << ".gpx";
      gpx.writeToFile(sstm.str());

      // This activity has been received, store that information:
      output << (int)filelist[i] << endl;

      
    }

    output.close();
      
    logStream << "# Done with donwloading...";
    logFlush();

    ant.leave(channel);
    return EXIT_SUCCESS;
}

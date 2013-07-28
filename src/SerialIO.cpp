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
#include "SerialIO.h"
#include "Log.h"
#include <fcntl.h>
#include <string.h>
#include <sys/poll.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <unistd.h>

SerialIO::SerialIO() : fd(-1), timeout(60)
{
}

SerialIO::~SerialIO()
{
    close();
}

bool SerialIO::open(string deviceName, speed_t speed)
{
    fd = ::open(deviceName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1)
    {
        logStream << "Error opening serial port '" << deviceName <<"'";
        logFlush();
        return false;
    }

    struct termios termios;
    if (::tcgetattr(fd, &termios))
    {
        ::close(fd);
        logStream << "Error getting terminal attributes";
        logFlush();
        return false;
    }

    termios.c_iflag = 0;
    termios.c_iflag |= IGNPAR;      // ignore parity errors

    termios.c_cflag = 0;
    termios.c_cflag |= CS8;         // 8 data bits, 1 stop bit, no parity, disable flow control
    termios.c_cflag |= CREAD;       // enable receiver
    termios.c_cflag |= CLOCAL;      // ignore conrol lines
//    termios.c_cflag |= CRTSCTS;

    termios.c_oflag = 0;
    termios.c_lflag = 0;
    
    termios.c_cc[VMIN] = 0;

    if (::cfsetispeed(&termios, speed))
    {
        ::close(fd);
        logStream << "Error setting terminal input speed";
        logFlush();
        return false;
    }

    if (::cfsetospeed(&termios, speed))
    {
        ::close(fd);
        logStream << "Error setting terminal output speed";
        logFlush();
        return false;
    }

    if (::tcsetattr(fd, TCSAFLUSH, &termios))
    {
        ::close(fd);
        logStream << "Error setting terminal attributes";
        logFlush();
        return false;
    }
   
/*
    int tmp;
    ::ioctl(serialFD, TIOCMGET, &tmp);
    tmp &= ~TIOCM_DTR;  // turn off DTR line (clear uC reset state)
    ::ioctl(serialFD, TIOCMSET, &tmp);
*/
    ::tcflush(fd, TCIOFLUSH);

    return true;
}

void SerialIO::close()
{
    ::tcflush(fd, TCIOFLUSH);
    ::close(fd);
}

bool SerialIO::receiveBuffer(vector<uint8_t> &buffer)
{
    size_t len = 1024;
    buffer.resize(len);
    bool rv = receiveBuffer(&buffer.front(), len);
    if (!rv)
    {
        buffer.clear();
        return false;
    }

    buffer.resize(len);
    
    return true;
}


bool SerialIO::sendBuffer(vector<uint8_t> &buffer)
{
    ::tcflush(fd, TCIFLUSH);
    
    size_t size = buffer.size();

    int bytes = ::write(fd, &buffer.front(), size);
    if (bytes != size)
    {
        logStream << "Error writing data to the port: " << strerror(errno) << "(" << errno << ")";
        logFlush();
        return false;
    }

    ::tcdrain(fd);

    return true;
}

bool SerialIO::receiveBuffer(uint8_t *buffer, size_t &len)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int rv = ::select(fd+1, &fds, NULL, NULL, &tv);
    if (rv < 0)
    {
        logStream << "Error calling select(): " << strerror(errno) << "(" << errno << ")";
        logFlush();
        return false;
    }
    
    if (rv == 0)
    {
        logStream << "Timeout waiting data from port";
        logFlush();
        return false;
    }

    ssize_t bytesInBuffer = 0;
    for(;;)
    {
        ssize_t bytesToRead = len - bytesInBuffer;
        ssize_t bytesRead = read(fd, buffer, bytesToRead);
        if (bytesRead == -1)
        {
            logStream << "Error reading data from port: " << strerror(errno) << "(" << errno << ")";
            logFlush();
            return false;
        }
        
        bytesInBuffer += bytesRead;
        buffer += bytesRead;
        
        if (bytesRead == 0)
        {
            len = bytesInBuffer;
            //logFlush();
            break;
        }
    }
            
    return true;
}

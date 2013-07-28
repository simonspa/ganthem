ganthem
=======

ganthem is a small tool to retreive the recorded GPS data from Garmin ANT+ devices. The only tested device (the only one I have access to) is the Garmin Forerunner 310XT.

Currently we only download activites. Waypoints and courses are ignored, but I might add (back) support for them later.

ganthem is based on frant by Oleg Khudyakov <prcoder@gmail.com>. He wrote all the communication part and a first interface to download data. The original source code can be found [here](http://sourceforge.net/projects/frant/).

Compilation / Installation
--------------------------

Use CMake to compile the ganthem executable:

    mkdir build && cd build/
    cmake ..
    make

In order to be able to access the ANT USB stick you need to tell your system, that is should us it as a serial USB device and provide it under /dev/ttyusb0:

    sudo modprobe usbserial vendor=0x0fcf product=0x1008 

The vendor and product id above belong to a Garmin Forerunner 310XT device, the only tested one. If you have a different device and want to give it a try, check for the ids using

    lsusb -v

...and report back if it works for you...

Usage
-----

Currently ganthem understands three different modes:

* Pairing: the initial setup with your GPS device. Only needed once to get the pairing key (which is then stored in the ganthem.ant.key file and read from there when connecting)

    ./ganthem -p

* HRM receiveing test: tests the communication with your paired device.

    ./ganthem -h

* Data download: the actual transmission of recoded GPS data from the paired device. This mode requires a pairing key to be present (see above). See below for the various options for downloading data.

You have several options for downloading activiy data. All data transfer starts with the transmission of the overview file (0x0) with a table of all available activities. After that you have various possibilities:
* The full download will get all activity files listed in the 0x0 file of the device. This might take some time, sometimes the connection to the devide is lost. If you experience that it is recommended to use the 'update' mode which continues a stopped download. To start a full download just run

    ./ganthem

* Downloading the last 5 recorded activities might be handy to only get the latest runs. This mode can be invoked using

    ./ganthem -l

* The most powerful download mode of ganthem is the 'update' mode. All copied activities are stored in a file (activities.dat). When receiving the initial table from the device (0x0 file) all listed activities are cross-checked against the stored ones. Therefore only newly recorded activities are transmitted. This option can also be used to get the full data set of a device by emptying the activities.dat file. After a lost connection ganthem will just continue downloading the activities which have not yet been transmitted. Invoke this mode using

    ./ganthem -u

All downloaded activities will be decoded from the FIT format into GPX files, which can be read by most sports tracking softwares. Every activity is stored separately as GPX file in the 'activities/' folder.


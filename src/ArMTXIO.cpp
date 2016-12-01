/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2014 Adept Technology

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
#include "ArExport.h"
#include "ariaOSDef.h"
#include "ArMTXIO.h"
#include "ariaInternal.h"
#include <errno.h>

#include <sys/ioctl.h>
#include <fcntl.h>

#define MTX_VERSION_REG			0x0000
#define MTX_VERSION_RR 			0x0000
#define MTX_VERSION_VV			0x0001
#define MTX_VERSION_CC			0x0002
#define MTX_VERSION_TT			0x0003
#define MTX_C2K_SCRATCH			0x0004
#define MTX_LPC_SCRATCH			0x0005
#define MTX_LPC_CTRL			0x0006
#define MTX_XINTF_CTRL			0x0007
#define MTX_COMMON_STATUS		0x0008
#define MTX_COMMON_STATUS2		0x0009
#define MTX_TIMING_CTRL			0x000b
#define MTX_GLOBAL_TIMER 		0x000c
#define MTX_GLOBAL_TIMER0		0x000c
#define MTX_GLOBAL_TIMER1		0x000d
#define MTX_GLOBAL_TIMER2		0x000e
#define MTX_GLOBAL_TIMER3		0x000f
#define MTX_LPC_TIMER0			0x0010
#define MTX_LPC_TIMER1			0x0011
#define MTX_LPC_TIMER2			0x0012
#define MTX_LPC_TIMER3			0x0013
#define MTX_XINTF_TIMER0		0x0014
#define MTX_XINTF_TIMER1		0x0015
#define MTX_XINTF_TIMER2		0x0016
#define MTX_XINTF_TIMER3		0x0017
#define MTX_SEM1			0x0018
#define MTX_SEM2			0x0019
#define MTX_SEM3			0x001a
#define MTX_SEM4			0x001b
#define MTX_DIO_INPUT_MON1		0x0020
#define MTX_DIO_INPUT_MON2		0x0021
#define MTX_DIO_OUTPUT_MON1		0x0022
#define MTX_DIO_OUTPUT_MON2		0x0023
#define MTX_BUMPER_INPUT_MON		0x0024
#define MTX_PWR_STATUS1			0x0028
#define MTX_PWR_STATUS2			0x0029
#define MTX_LIDAR_STATUS		0x002a
#define MTX_ESTOP_STATUS1		0x002c
#define MTX_ESTOP_STATUS2		0x002d
#define MTX_ESTOP_STATUS3		0x002e
#define MTX_ESTOP_STATUS4		0x002f
#define MTX_DIO_OUTPUT_CTRL1		0x0030
#define MTX_DIO_OUTPUT_CTRL2		0x0031
#define MTX_LIGHTPOLE_OUTPUT_CTRL	0x0032
#define MTX_PERIPH_PWR_CTRL1		0x0034
#define MTX_PERIPH_PWR_CTRL2	0x0035
#define MTX_PERIPH_PWR_CTRL3		0x0036
#define MTX_MOTION_PWR_CTRL		0x0038
#define MTX_MOTION_PWR_STATUS		0x0039
#define MTX_LIDAR_CTRL			0x003a

#define MTX_DPRAM_BLOCK1		0x0040
#define MTX_DPRAM_BLOCK2		0x0070
#define MTX_DPRAM_BLOCK3		0x0100
#define MTX_DPRAM_BLOCK4		0x0500

#define MTX_MAGIC 287
#define MTX_READ_REG	_IOR(MTX_MAGIC, 1, MTX_IOREQ *)
#define MTX_WRITE_REG	_IOW(MTX_MAGIC, 2, MTX_IOREQ *)
//#define MTX_READ_REG		_IOR('m', 1, MTX_IOREQ *)
//#define MTX_WRITE_REG		_IOW('m', 2, MTX_IOREQ *)


ArMutex ArMTXIO::ourMutex;

/** Constructor for the ArMTXIO class.  This will open the device
 * named by @a dev (the default is "/dev/amrio" if the argument is omitted).
 It will find the number of digital banks and set the to inputs.  It will also
 attempt to take an analog reading, which will fail if there is not analog chip
 present.  If the conversion fails it will disable the analog portion of the
 code.

 Check isEnabled() to see if the device was properly opened during construction.
*/
AREXPORT ArMTXIO::ArMTXIO(const char * dev) :
  myDisconnectCB(this, &ArMTXIO::closeIO),
  myLPCTimeUSecCB(this, &ArMTXIO::getLPCTimeUSec)
{
  ourMutex.setLogName("ArMTXIO::ourMutex");

	myFirmwareRevision = 0;
	myFirmwareVersion = 0;
	myCompatibilityCode = 0;
	myFPGAType = 0x20;

  ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: opening device %s", dev);
  myFD = ArUtil::open(dev, O_RDWR);

  if (myFD == -1)
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: open %s failed.  Disabling class (errno %d)", dev, errno);
    myEnabled = false;
  }
  else
  {


		// PS 10/30/12 - query the FPGA

		MTX_IOREQ mtxIO;

		mtxIO.myReg = MTX_VERSION_REG;
		mtxIO.mySize = sizeof(unsigned int);

		if (ioctl(myFD, MTX_READ_REG, &mtxIO) != 0)
		{
      ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: failed to get the Version Registers");
      close(myFD);
			myEnabled = false;
		}
		else
		{
      ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: FPGA version registers = 0x%08x", mtxIO.myData.myVal32);

			myFirmwareRevision = (mtxIO.myData.myVal32 & 0x000000ff);
			myFirmwareVersion = (mtxIO.myData.myVal32 & 0x0000ff00) >> 8;
			myCompatibilityCode = (mtxIO.myData.myVal32 & 0x00ff0000) >> 16;
			myFPGAType = (mtxIO.myData.myVal32 & 0xff000000) >> 24;
			myEnabled = true;

			ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: Firmware Revision = 0x%02x", myFirmwareRevision);
			ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: Firmware Version = 0x%02x", myFirmwareVersion);
			ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: Compatibility Code = 0x%02x", myCompatibilityCode);
			ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: FPGA Type = 0x%02x", myFPGAType);

			Aria::addExitCallback(&myDisconnectCB);
			ArLog::log(ArLog::Normal, "ArMTXIO::ArMTXIO: device opened");

			// there will be be 4 banks for MTX, 2 input consisting of 8 bits each
			// and 2 output consisting of 8 bits each

			myNumBanks = 4;


		}
  }
}

/** Destructor.  Attempt to close the device if it was opened
 **/
AREXPORT ArMTXIO::~ArMTXIO(void)
{
  if (myEnabled)
    closeIO();
  Aria::remExitCallback(&myDisconnectCB);
}

/** Close the device when Aria exits
 **/
AREXPORT bool ArMTXIO::closeIO(void)
{
  myEnabled = false;

  if (close(myFD) == -1)
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::~ArMTXIO: close failed on file descriptor!");
    return false;
  }
  else
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::~ArMTXIO: closed device");
    myFD = -1;
    return true;
  }
}

/** Returns the state of the bits as they are currently set in the outputs.
 @param bank which bank to check
 @param val the byte to write the values into
 @return true if the request was satisfied, false otherwise
 **/
AREXPORT bool ArMTXIO::getDigitalBankOutputs(int bank, unsigned char *val)
{
  if (!myEnabled)
    return false;

  if (bank < 0 || bank > myNumBanks - 1)
    return false;

  // should check for input vs. output
  switch (bank)
  {
		case 2:
      return getDigitalOutputControl1(val);
      break;
    case 3:
      return getDigitalOutputControl2(val);
      break;
		default:
			return false;
  }

  return false;
}

/** 

  @param bank the bank number to use for mtx only 2 & 3 are outputs
  @param val the status of the 8-bits in a single byte.  
 **/
AREXPORT bool ArMTXIO::setDigitalBankOutputs(int bank, unsigned char val)
{
  if (!myEnabled)
    return false;

  if (bank < 0 || bank > myNumBanks - 1)
    return false;

  // bitwise negate val because the hardware calls a 'high' off
  switch (bank)
  {
    case 2:
      if (!setDigitalOutputControl1(&val))
				return false;
      myDigitalBank2 = val;
      break;
    case 3:
      if (!setDigitalOutputControl2(&val))
				return false;
      myDigitalBank3 = val;
      break;
    default:
      return false;
  }

  return true;
}

/** Returns the bits of the digital input bank.
 
  @return true if the ioctl call was succcessfull, false otherwise
  **/
AREXPORT bool ArMTXIO::getDigitalBankInputs(int bank, unsigned char *val)
{
  if (!myEnabled)
    return false;

  if (bank < 0 || bank > myNumBanks - 1)
    return false;

  unsigned char tmp;
  switch (bank)
  {
    case 0:
			return getDigitalIOInputMon1(val);
      break;
    case 1:
			return getDigitalIOInputMon2(val);
      break;
    default:
      return false;
  }

  return true;
}


AREXPORT ArMTXIO::Direction ArMTXIO::getDigitalBankDirection(int bank)
{
  if (bank == 0 || bank == 1)
    return DIGITAL_INPUT;
  else if(bank == 2 || bank == 3)
    return DIGITAL_OUTPUT;
  else
    return INVALID;
}

#if 0

/** Returns the raw bit value as read by the chip.

 @param port the port number, between 0 and 7
 @param val the address of the integer to store the reading in
 @return true if a reading was acquired.  false otherwise
 **/
AREXPORT bool ArMTXIO::getAnalogValueRaw(int port, int *val)
{
  if (!myEnabled || !myAnalogEnabled)
    return false;

  unsigned int tmp;

  if (ioctl(myFD, ANALOG_SET_PORT, port) != 0)
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogValueRaw: failed to set analog port %d", port);
    return false;
  }

  if (ioctl(myFD, ANALOG_GET_VALUE, &tmp) != 0)
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogValueRaw: failed to get analog port %d", port);
    return false;
  }

  *val = (int) tmp;

  return true;
}

/** Returns an analog value converted from the raw reading to a scale
 if 0-5V.

 @param port the port number, between 0 and 7
 @param val the address of the double to store the reading in
 @return true if a reading was acquired.  false otherwise
 **/
AREXPORT bool ArMTXIO::getAnalogValue(int port, double *val)
{
  int tmp;

  // the reading is 4096 units, for a range of 0-5V
  if (getAnalogValueRaw(port, &tmp)) {
    *val = 5 * (tmp/4096.0);
    return true;
  }
  else {
    return false;
  }
}

#endif


AREXPORT bool ArMTXIO::getLightPole(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LIGHTPOLE_OUTPUT_CTRL;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLightPole() failed to get Light Pole Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setLightPole(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LIGHTPOLE_OUTPUT_CTRL;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLightPole() failed to set Light Pole Register");
    return false;
  }

  return true;

}

/** Returns the bits of the digital IO Input Monitoring 1
 
  @return true if the ioctl call was succcessfull, false otherwise
  **/
AREXPORT bool ArMTXIO::getDigitalIOInputMon1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;


	req.myReg = MTX_DIO_INPUT_MON1;
	req.mySize = sizeof(unsigned char);

  if (ioctl(myFD, MTX_READ_REG, &req) != 0)
	{
		ArLog::log(ArLog::Normal, "ArMTXIO::getDigitalIOInputMon1() failed to get Digital IO Input Montioring 1 Register");
		return false;
	}

  *val = req.myData.myVal8;

	ArLog::log(ArLog::Verbose, 
    "ArMTXIO::getDigitalIOInputMon1: Input register 1 = %d", *val);

  return true;
}

/** Returns the bits of the digital IO Input Monitoring 1
 
  @return true if the ioctl call was succcessfull, false otherwise
  **/
AREXPORT bool ArMTXIO::getDigitalIOInputMon2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;


	req.myReg = MTX_DIO_INPUT_MON2;
	req.mySize = sizeof(unsigned char);

  if (ioctl(myFD, MTX_READ_REG, &req) != 0)
	{
		ArLog::log(ArLog::Normal, "ArMTXIO::getDigitalIOInputMon2() failed to get Digital IO Input Montioring 2 Register");
		return false;
	}

  *val = req.myData.myVal8;

	ArLog::log(ArLog::Verbose, 
    "ArMTXIO::getDigitalIOInputMon2: Input register 2 = %d", *val);

  return true;
}


AREXPORT bool ArMTXIO::getDigitalIOOutputMon1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_DIO_OUTPUT_MON1;
	req.mySize = sizeof(unsigned char);

  if (ioctl(myFD, MTX_READ_REG, &req) != 0)
	{
		ArLog::log(ArLog::Normal, "ArMTXIO::getDigitalIOOutputMon1() failed to get Digital IO Output Montioring 1 Register");
		return false;
	}

  *val = req.myData.myVal8;
  return true;
}


AREXPORT bool ArMTXIO::getDigitalIOOutputMon2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_DIO_OUTPUT_MON2;
	req.mySize = sizeof(unsigned char);

  if (ioctl(myFD, MTX_READ_REG, &req) != 0)
	{
		ArLog::log(ArLog::Normal, "ArMTXIO::getDigitalIOOutputMon2() failed to get Digital IO Output Montioring 2 Register");
		return false;
	}

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getLPCTimer0(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LPC_TIMER0;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLPCTimer0() failed to get LPC Time 0 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getLPCTimer1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LPC_TIMER1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLPCTimer1() failed to get LPC Time 1 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getLPCTimer2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LPC_TIMER2;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLPCTimer2() failed to get LPC Time 2 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getLPCTimer3(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LPC_TIMER3;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLPCTimer3() failed to get LPC Time 3 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getLPCTimeUSec(ArTypes::UByte4 *timeUSec)
{
  MTX_IOREQ req;

  if (!myEnabled)
    return false;

  req.myReg = MTX_TIMING_CTRL;
  req.mySize = sizeof(unsigned char);
  req.myData.myVal8 = 1;
  
  if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLPCTimeUSec() failed to set TimingControlRegister");
    return false;
  }

  unsigned char time0, time1, time2, time3;
  if (!getLPCTimer0(&time0) || 
      !getLPCTimer1(&time1) || 
      !getLPCTimer2(&time2) || 
      !getLPCTimer3(&time3))
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLPCTimeUSec() failed to get LPC Time  Register");
    return false;
  }

  *timeUSec = (time3 << 24) | (time2 << 16) | (time1 << 8) | time0;  
  
  return true;
}

AREXPORT bool ArMTXIO::getSemaphore1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore1() failed to get Semaphore 1 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setSemaphore1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM1;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore1() failed to set Semaphore 1 Register");
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getSemaphore2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM2;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore2() failed to get Semaphore 2 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setSemaphore2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM2;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore2() failed to set Semaphore 2 Register");
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getSemaphore3(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM3;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore3() failed to get Semaphore 3 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setSemaphore3(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM3;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore3() failed to set Semaphore 3 Register");
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getSemaphore4(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM4;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore4() failed to get Semaphore 4 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setSemaphore4(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_SEM4;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getSemaphore4() failed to set Semaphore 4 Register");
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getBumperInput(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_BUMPER_INPUT_MON;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getBumperInput() failed to get Bumper Input Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getPowerStatus1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PWR_STATUS1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getPowerStatus1() failed to get Power Status 1 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getPowerStatus2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PWR_STATUS2;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getPowerStatus1() failed to get Power Status 2 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getLIDARSafety(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LIDAR_STATUS;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getLIDARSafety() failed to get LIDAR Safety Status Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getESTOPStatus1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_ESTOP_STATUS1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getESTOPStatus1() failed to get ESTOP Status 1 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getESTOPStatus2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_ESTOP_STATUS1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getESTOPStatus2() failed to get ESTOP Status 2 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getESTOPStatus3(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_ESTOP_STATUS3;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getESTOPStatus3() failed to get ESTOP Status 3 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::getESTOPStatus4(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_ESTOP_STATUS4;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getESTOPStatus4() failed to get ESTOP Status 4 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::compareESTOPStatus4HighNibbleAgainst(int val)
{
  unsigned char rawVal;
  
  if (!getESTOPStatus4(&rawVal))
    return false;

  // shift to get the high nibble, then compare against the val
  if ((rawVal >> 4) == val)
    return true;
  else
    return false;
}

AREXPORT bool ArMTXIO::getDigitalOutputControl1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_DIO_OUTPUT_CTRL1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getDigitalOutputControl1() failed to get Digital IO Output Control 1 Register");
    return false;
  }

  *val = req.myData.myVal8;

	ArLog::log(ArLog::Verbose, 
    "ArMTXIO::getDigitalOutputControl1: Output register 1 = 0x%02x", *val);

  return true;
}

AREXPORT bool ArMTXIO::setDigitalOutputControl1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;


	ArLog::log(ArLog::Verbose, 
    "ArMTXIO::setDigitalOutputControl1: Setting Output register 1 = 0x%02x", *val);

	req.myReg = MTX_DIO_OUTPUT_CTRL1;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setDigitalOutputControl1() failed to set Digital IO Output Control 1 Register %d %d %d",
					req.myReg, req.mySize, *val);
    return false;
  }

  return true;

}


AREXPORT bool ArMTXIO::getDigitalOutputControl2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_DIO_OUTPUT_CTRL2;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getDigitalOutputControl2() failed to get Digital IO Output Control 2 Register");
    return false;
  }

  *val = req.myData.myVal8;

	ArLog::log(ArLog::Verbose, 
    "ArMTXIO::getDigitalOutputControl2: Output register 2 = 0x%02x", *val);

  return true;
}

AREXPORT bool ArMTXIO::setDigitalOutputControl2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	ArLog::log(ArLog::Verbose, 
    "ArMTXIO::setDigitalOutputControl2: Setting Output register 2 = 0x%02x", *val);


	req.myReg = MTX_DIO_OUTPUT_CTRL2;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setDigitalOutputControl2() failed to set Digital IO Output Control 2 Register");
    return false;
  }

  return true;

}


AREXPORT bool ArMTXIO::getPeripheralPower1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PERIPH_PWR_CTRL1;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getPeripheralPower1() failed to get Peripheral Power Control 1 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setPeripheralPower1(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PERIPH_PWR_CTRL1;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setPeripheralPower1() failed to set Peripheral Power Control 1 Register - errno = %d", errno);
    return false;
  }

  return true;

}


AREXPORT bool ArMTXIO::getPeripheralPower2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PERIPH_PWR_CTRL2;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getPeripheralPower2() failed to get Peripheral Power Control 2 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setPeripheralPower2(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PERIPH_PWR_CTRL2;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setPeripheralPower2() failed to set Peripheral Power Control 2 Register - errno = %d", errno);
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getPeripheralPower3(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PERIPH_PWR_CTRL3;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getPeripheralPower3() failed to get Peripheral Power Control 3 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setPeripheralPower3(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_PERIPH_PWR_CTRL3;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setPeripheralPower3() failed to set Peripheral Power 3 Register");
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getMotionPowerStatus(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_MOTION_PWR_STATUS;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getMotionPowerStatus() failed to get Peripheral Power Control 3 Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setPeripheralPowerBankOutputs(int bank, unsigned char val)
{
  if(!myEnabled)
    return false;
  switch(bank)
  {
    case 0:
      return setPeripheralPower1(&val);
    case 1:
      return setPeripheralPower2(&val);
    case 2:
      return setPeripheralPower3(&val);
    default: 
      return false;
  }
  return false;
}


AREXPORT bool ArMTXIO::getPeripheralPowerBankOutputs(int bank, unsigned char *val)
{
  if(!myEnabled)
    return false;
  switch(bank)
  {
    case 0:
      return getPeripheralPower1(val);
    case 1:
      return getPeripheralPower2(val);
    case 2:
      return getPeripheralPower3(val);
    default: 
      return false;
  }
  return false;
}

AREXPORT bool ArMTXIO::getLIDARControl(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LIDAR_CTRL;
	req.mySize = sizeof(unsigned char);

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::LIDARControl() failed to get LIDAR Control Register");
    return false;
  }

  *val = req.myData.myVal8;
  return true;
}

AREXPORT bool ArMTXIO::setLIDARControl(unsigned char *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_LIDAR_CTRL;
	req.mySize = sizeof(unsigned char);
	req.myData.myVal8 = *val;

	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setLIDARControl() failed to set LIDAR ControlRegister");
    return false;
  }

  return true;

}

AREXPORT bool ArMTXIO::getAnalogIOBlock1(int analog, unsigned short *val)
{	
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	// we first need to get the semaphore controlling the block 
	// we do this by writing to it - then reading it to make sure
	// we have obtained it

	unsigned char semTmp = 1;
	if (!setSemaphore1(&semTmp)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock1() failed to set (%d) Semaphore 1", semTmp);
    return false;
  }
	
	unsigned char semVal;

	if (!getSemaphore1(&semVal)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock1() failed to get Semaphore 1");
    return false;
  }

	if (semVal == 0) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock1() Semaphore 1 is busy");
    return false;
  }

	req.myReg = MTX_DPRAM_BLOCK1 + analog;
	req.mySize =16;

	bool noErr = true;
	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock1() failed to get Block 1 (%d)", analog);
    noErr = false;
  }

  *val = req.myData.myVal16;

	semTmp = 0;
	if (!setSemaphore1(&semTmp)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock1() failed to set (%d) Semaphore 1", semTmp);
    return false;
  }

	if (!getSemaphore1(&semVal)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock1() failed to get Semaphore 1");
    return false;
  }

	if (semVal != 0) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock1() Semaphore 1 is busy");
    return false;
  }

  return noErr;
}


AREXPORT bool ArMTXIO::getAnalogIOBlock2(int analog, unsigned short *val)
{	
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	// we first need to get the semaphore controlling the block 
	// we do this by writing to it - then reading it to make sure
	// we have obtained it

	unsigned char semTmp = 1;
	if (!setSemaphore2(&semTmp)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to set (%d) Semaphore 2", semTmp);
    return false;
  }
	
	unsigned char semVal;

	if (!getSemaphore2(&semVal)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock2() failed to get Semaphore 2");
    return false;
  }

	if (semVal == 0) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock2() Semaphore 2 is busy");
    return false;
  }

	req.myReg = MTX_DPRAM_BLOCK2 + analog;
	req.mySize =16;

	bool noErr = true;
	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock2() failed to get Block 2 (%d)", analog);
    noErr = false;
  }

  *val = req.myData.myVal16;

	// what happens if this fails?
	semTmp = 0;
	if (!setSemaphore2(&semTmp)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to set (%d) Semaphore 2", semTmp);
    return false;
  }

	if (!getSemaphore2(&semVal)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock2() failed to get Semaphore 2");
    return false;
  }

	if (semVal != 0) {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogIOBlock2() Semaphore 2 is busy");
    return false;
  }

  return noErr;
}


AREXPORT bool ArMTXIO::setAnalogIOBlock2(int analog, unsigned short *val)
{	
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	// we first need to get the semaphore controlling the block 
	// we do this by writing to it - then reading it to make sure
	// we have obtained it

	unsigned char semTmp = 1;
	if (!setSemaphore2(&semTmp)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to set (%d) Semaphore 2", semTmp);
    return false;
  }
	
	unsigned char semVal;

	if (!getSemaphore2(&semVal)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to get Semaphore 2");
    return false;
  }

	if (semVal == 0) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() Semaphore 2 is busy");
    return false;
  }

	req.myReg = MTX_DPRAM_BLOCK2 + analog;
	req.mySize =16;
	req.myData.myVal16 = *val;

	bool noErr = true;
	if (ioctl(myFD, MTX_WRITE_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to get Block 2 (%d)", analog);
    noErr = false;
  }

	semTmp = 1;
	if (!setSemaphore2(&semTmp)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to set (%d) Semaphore 2", semTmp);
    return false;
  }

	if (!getSemaphore2(&semVal)) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() failed to get Semaphore 2");
    return false;
  }

	if (semVal != 0) {
    ArLog::log(ArLog::Normal, "ArMTXIO::setAnalogIOBlock2() Semaphore 2 is busy");
    return false;
  }


  return noErr;
}

AREXPORT bool ArMTXIO::getAnalogValueRaw(int *val)
{
MTX_IOREQ req;

  if (!myEnabled)
    return false;

	req.myReg = MTX_DPRAM_BLOCK2;
	req.mySize =16;

	if (ioctl(myFD, MTX_READ_REG, &req) != 0) 
  {
    ArLog::log(ArLog::Normal, "ArMTXIO::getAnalogValueRaw() failed to get Block 2");
    return false;
  }

	*val = req.myData.myVal16;

  return true;
}

/** Returns an analog value converted from the raw reading to a scale
 if 0-5V.

 @param val the address of the double to store the reading in
 @return true if a reading was acquired.  false otherwise
 **/
AREXPORT bool ArMTXIO::getAnalogValue(double *val)
{
  int tmp;

  // the reading is 4096 units, for a range of 0-5V
  if (getAnalogValueRaw(&tmp)) {
    *val = 5 * (tmp/4096.0);
    return true;
  }
  else {
    return false;
  }
}

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
#ifndef ARMTXIO_H
#define ARMTXIO_H


#include "ariaTypedefs.h"
#include "ArRobot.h"

/** @brief Interface to digital and analog I/O and switched power outputs on MTX
 * core (used in Pioneer LX and other MTX-based robots).
 
  On Linux this class uses the <code>mtx</code> driver to interface with the 
  MTX digital and analog IO, and control switched power outputs. 

  The <code>mtx</code> driver module must be loaded for this interface to work.

  The <code>mtx</code> interface (<code>/dev/mtx</code>) is opened automatically
  in the ArMTXIO constructor.  If successful, isEnabled() will then return true.

  The MTX IO is organized as two sets of 8-bit "banks" of inputs, and two 8-bit
  "banks" of outputs.

  To read the state input, use getDigitalIOInputMon1() and
  getDigitalIOInputMon2(), or getDigitalBankInputs() with bank index 0 or 1.

  To set state of output, use setDigitalOutputControl1() and
  setDigitalOutputControl2(), or setDigitalBankOutputs() with bank index 2 or
  3.

  Multile ArMTXIO objects may be instatiated; a shared may be
  locked and unlocked by calling lock() and unlock().

   @ingroup OptionalClasses
   @ingroup DeviceClasses
   @ingroup MTX

 @linuxonly

*/
 
class ArMTXIO
{
public:

  enum Direction
  {
    DIGITAL_INPUT,
    DIGITAL_OUTPUT,
    INVALID
  };

  /// Constructor
  AREXPORT ArMTXIO(const char * dev = "/dev/mtx");
  /// Destructor
  AREXPORT virtual ~ArMTXIO(void);

  /// tries to close the device.  Returns false if operation failed
  AREXPORT bool closeIO(void);

  /// returns true if the device is opened and operational
  bool isEnabled(void) { return myEnabled; }

  /// returns true if analog values are supported
  AREXPORT bool isAnalogSupported(void) { return myAnalogEnabled; }

  /// @note not yet implemented.
  bool getAnalogValue(int port, double *val) { return true; }
  /// @note not yet implemented
  bool getAnalogValueRaw(int port, int *val) { return true; }

  /// Get/set value of digital IO banks (see class description above)
  /// Banks are 0-indexed.
  /// @{
  AREXPORT Direction getDigitalBankDirection(int bank);
  AREXPORT bool setDigitalBankOutputs(int bank, unsigned char val);
  AREXPORT bool getDigitalBankInputs(int bank, unsigned char *val);
  AREXPORT bool getDigitalBankOutputs(int bank, unsigned char *val);
  /// @}

  /// Set one bit of an output bank. @a bank and @a bit are 0-indexed.
  bool setDigitalOutputBit(int bank, int bit) {
    unsigned char val;
    if(!getDigitalBankOutputs(bank, &val))
      return false;
    return setDigitalBankOutputs( bank, val | (1 << bit) );
  }

  /// Get one bit of an input bank. @a bank and @a bit are 0-indexed.
  bool getDigitalInputBit(int bank, int bit) {
    unsigned char val = 0;
    getDigitalBankInputs(bank, &val);
    return val & (1 << bit);
  }

  /// get/set value of power output controls (see robot manual for information
  /// on what components and user outputs are controlled). Banks are 0-indexed.
  /// @{
  AREXPORT bool setPeripheralPowerBankOutputs(int bank, unsigned char val);
  AREXPORT bool getPeripheralPowerBankOutputs(int bank, unsigned char *val);
  /// @}

  /// Set one power output. @a bank and @a bit are 0-indexed.
  bool setPowerOutput(int bank, int bit, bool on) {
    unsigned char val = 0;
    if(!getPeripheralPowerBankOutputs(bank, &val))
      return false;
    if (on)
      return setPeripheralPowerBankOutputs(bank, val & (1 << bit));
    else
      return setPeripheralPowerBankOutputs(bank, val ^ (1 << bit));
  }

  /// Lock global (shared) mutex for all ArMTXIO instances.
  /// This allows multiple access to MTX IO (through multiple ArMTXIO objects).
  AREXPORT int lock(void){ return(ourMutex.lock()); }
  /// Unlock global (shared) mutex for all ArMTXIO instances.
  /// This allows multiple access to MTX IO (through multiple ArMTXIO objects).
  AREXPORT int unlock(void){ return(ourMutex.unlock()); }

  /// Try to lock without blocking (see ArMutex::tryLock())
  AREXPORT int tryLock() {return(ourMutex.tryLock());}

  /// gets the Firmware Revision 
  AREXPORT unsigned char getFirmwareRevision()
	{ return myFirmwareRevision; }

  /// gets the Firmware Version 
  AREXPORT unsigned char getFirmwareVersion()
	{ return myFirmwareVersion; }

  /// gets the Compatibility Code 
  AREXPORT unsigned char getCompatibilityCode()
	{ return myCompatibilityCode; }

  /// gets the MTX FPGA Type
  AREXPORT unsigned char getFPGAType()
	{ return myFPGAType; }

  /// gets the values of digital input/output monitoring registers 1 & 2
  /// @{
  AREXPORT bool getDigitalIOInputMon1(unsigned char *val);
  AREXPORT bool getDigitalIOInputMon2(unsigned char *val);
  AREXPORT bool getDigitalIOOutputMon1(unsigned char *val);
  AREXPORT bool getDigitalIOOutputMon2(unsigned char *val);
  /// @}

  /// get the Light Pole IO Output Control Register
  AREXPORT bool getLightPole(unsigned char *val);
  /// sets the Light Pole IO Output Control Register
  AREXPORT bool setLightPole(unsigned char *val);

  /// @internal
  AREXPORT bool getLPCTimeUSec(ArTypes::UByte4 *timeUSec);

  /// @internal
  AREXPORT ArRetFunctor1<bool, ArTypes::UByte4 *> *getLPCTimeUSecCB(void)
    { return &myLPCTimeUSecCB; }

  /// gets/sets the Semaphore Registers
  /// @internal
  //@{
	AREXPORT bool setSemaphore1(unsigned char *val);
  AREXPORT bool getSemaphore1(unsigned char *val);
	AREXPORT bool setSemaphore2(unsigned char *val);
  AREXPORT bool getSemaphore2(unsigned char *val);
	AREXPORT bool setSemaphore3(unsigned char *val);
  AREXPORT bool getSemaphore3(unsigned char *val);
	AREXPORT bool setSemaphore4(unsigned char *val);
  AREXPORT bool getSemaphore4(unsigned char *val);
  //@}

  /// gets the bumper Input Monitoring Reg
  /// @internal
  AREXPORT bool getBumperInput(unsigned char *val);

  /// gets the Power Status Register 
  /// @internal
  AREXPORT bool getPowerStatus1(unsigned char *val);
  /// gets the Power Status Register 2
  /// @internal
  AREXPORT bool getPowerStatus2(unsigned char *val);

  /// gets the LIDAR Safety Status Register
  AREXPORT bool getLIDARSafety(unsigned char *val);

  /// gets the ESTOP status Registers
  /// @internal
  //@{
  AREXPORT bool getESTOPStatus1(unsigned char *val);
  AREXPORT bool getESTOPStatus2(unsigned char *val);
  AREXPORT bool getESTOPStatus3(unsigned char *val);
  AREXPORT bool getESTOPStatus4(unsigned char *val);
  /// Compares the high nibble of this byte against the passed in val, returns true if it matches
  AREXPORT bool compareESTOPStatus4HighNibbleAgainst(int val);
  //@}

  /// gets/sets Digital IO Output Control Registers 1 &amp; 2
  //@{
  AREXPORT bool getDigitalOutputControl1(unsigned char *val);
  AREXPORT bool setDigitalOutputControl1(unsigned char *val);
  AREXPORT bool getDigitalOutputControl2(unsigned char *val);
  AREXPORT bool setDigitalOutputControl2(unsigned char *val);
  //@}

  /// gets/sets the Peripheral Power Control Regs 1 &amp; 2
  /// These control power to core and robot components, and to user/auxilliary
  /// power outputs.  Refer to robot manual for information on which components
  /// and outputs are controlled by which bits in the peripheral power bitmasks.
  //@{
  AREXPORT bool getPeripheralPower1(unsigned char *val); ///< bank 0
  AREXPORT bool setPeripheralPower1(unsigned char *val); ///< bank 0
  AREXPORT bool getPeripheralPower2(unsigned char *val); ///< bank 1
  AREXPORT bool setPeripheralPower2(unsigned char *val); ///< bank 1
  AREXPORT bool getPeripheralPower3(unsigned char *val); ///< bank 2
  AREXPORT bool setPeripheralPower3(unsigned char *val); ///< bank 2
  //@}

  /// gets the motion power status
  AREXPORT bool getMotionPowerStatus(unsigned char *val);

  /// gets/sets the LIDAR Control Reg
  /// @internal
  //@{
  AREXPORT bool getLIDARControl(unsigned char *val);
  AREXPORT bool setLIDARControl(unsigned char *val);
  //@}


  /// gets analog Block 1 & 2
  //@{
  AREXPORT bool getAnalogIOBlock1(int analog, unsigned short *val);
  AREXPORT bool getAnalogIOBlock2(int analog, unsigned short *val);

  AREXPORT bool setAnalogIOBlock2(int analog, unsigned short *val);
  //@}

  /// This returns a conversion of the bits to a decimal value,
  /// currently assumed to be in the 0-5V range
  /// @internal
  AREXPORT bool getAnalogValue(double *val);

  /// @internal
  AREXPORT bool getAnalogValueRaw(int *val);

protected:

  bool getLPCTimer0(unsigned char *val);
  bool getLPCTimer1(unsigned char *val);
  bool getLPCTimer2(unsigned char *val);
  bool getLPCTimer3(unsigned char *val);
	

  static ArMutex ourMutex;
  int myFD;

  bool myEnabled;
  bool myAnalogEnabled;

	unsigned char myFirmwareRevision;
	unsigned char myFirmwareVersion;
	unsigned char myCompatibilityCode;
	unsigned char myFPGAType;

	struct MTX_IOREQ{
    unsigned short	myReg;
    unsigned short	mySize;
    union {
        unsigned int	myVal;
        unsigned int	myVal32;
        unsigned short myVal16;
        unsigned char	myVal8;
				//unsigned char myVal128[16];
    } myData;
	};

  int myNumBanks;

  unsigned char myDigitalBank1;
  unsigned char myDigitalBank2;
  unsigned char myDigitalBank3;
  unsigned char myDigitalBank4;

  ArRetFunctorC<bool, ArMTXIO> myDisconnectCB;
  ArRetFunctor1C<bool, ArMTXIO, ArTypes::UByte4 *> myLPCTimeUSecCB;
};

//#endif // SWIG

#endif // ARMTXIO_H

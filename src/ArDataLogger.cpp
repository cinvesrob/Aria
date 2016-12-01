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
#include "ariaOSDef.h"
#include "ArExport.h"
#include "ArRobot.h"
#include "ArConfig.h"
#include "ArDataLogger.h"
#include "ArRobotBatteryPacketReader.h"
#include <vector>

/**
   @param robot the robot to log information from
   
   @param fileName if NULL then the file name is detered by the
   config, if not NULL then the file name data is put into, if the
   filename is used then the data log file is opened in append mode
   not in write mode, so it'll append to whats there (if you want to
   separate this by runs you need to do it on your own by removing or
   moving the file)
 **/
AREXPORT ArDataLogger::ArDataLogger(ArRobot *robot, const char *fileName) :
  myAddStringFunctor(this, &ArDataLogger::addString),
  myConnectCB(this, &ArDataLogger::connectCallback),
  myProcessFileCB(this, &ArDataLogger::processFile),
  myUserTaskCB(this, &ArDataLogger::userTask)
{
  myMutex.setLogName("ArDataLogger::myMutex");
  myRobot = robot;
  if (fileName == NULL || fileName[0] == '\0')
    myPermanentFileName = "";
  else
    myPermanentFileName = fileName;
  myRobot->addUserTask("DataLogger", 50, &myUserTaskCB);
  myRobot->requestIOPackets();
  myConfig = false;
  myAddToConfigAtConnect = false;
  myAddedToConfig = false;
  myConfigLogging = false;
  myConfigLogInterval = 0;
  myConfigFileName[0] = '\0';
  myOpenedFileName[0] = '\0';
  myAnalogCount = 0;
  myAnalogEnabled = NULL;
  myAnalogVoltageCount = 0;
  myAnalogVoltageEnabled = NULL;
  myDigInCount = 0;
  myDigInEnabled = NULL;
  myDigOutCount = 0;
  myDigOutEnabled = NULL;
  myStringsCount = 0;
  //myStringsEnabled = NULL;

  myLogVoltage = false;
  myLogStateOfCharge = false;
  myLogLeftVel = false;
  myLogRightVel = false;
  myLogTransVel = false;
  myLogRotVel = false;
  myLogLatVel = false;
  myLogLeftStalled = false;
  myLogRightStalled = false;
  myLogStallBits = false;
  myLogFlags = false;
  myLogFaultFlags = false;
  myLogPose = false;
  myLogEncoderPose = false;
  myLogCorrectedEncoderPose = false;
  myLogEncoders = false;
  myLogChargeState = false;
  myLogBatteryInfo = false;

  myFile = NULL;
}

AREXPORT ArDataLogger::~ArDataLogger(void)
{

}

AREXPORT void ArDataLogger::addToConfig(ArConfig *config)
{
  if (config == NULL || myAddedToConfig)
    return;
  myConfig = config;
  if (!myRobot->isConnected())
  {
    myAddToConfigAtConnect = true;
    myRobot->addConnectCB(&myConnectCB);
    return;
  }
  else
  {
    connectCallback();
  }

  myAddedToConfig = true;
  ArLog::log(ArLog::Verbose, "ArDataLogger::addToConfig");
  std::string section;
  char name[512];
  char desc[512];
  int i;
  section = "Data logging";
  // add everything to the config
  myConfig->addParam(
	  ArConfigArg("DataLog", &myConfigLogging, "True to log data, false not to"),
	  section.c_str(), ArPriority::NORMAL);

  myConfig->addParam(
	  ArConfigArg("DataLogInterval", &myConfigLogInterval, "Seconds between logs", 0),
	  section.c_str(), ArPriority::NORMAL);

  if (myPermanentFileName.size() == 0)
    myConfig->addParam(
	    ArConfigArg("DataLogFileName", myConfigFileName, 
			"File to log data into", sizeof(myConfigFileName)),
	    section.c_str(), ArPriority::NORMAL);
  
  for (i = 0; i < myStringsCount; i++)
  {
    snprintf(name, sizeof(name), "DataLog%s", myStrings[i]->getName());
    snprintf(desc, sizeof(desc), "Logs %s", myStrings[i]->getName());
    myConfig->addParam(
	    ArConfigArg(name, myStringsEnabled[i], desc),
	    "Custom data logging", ArPriority::NORMAL);
  }

  myConfig->addParam(
	  ArConfigArg("DataLogBatteryVoltage", &myLogVoltage, "True to log battery voltage"),
	  section.c_str(), ArPriority::DETAILED);
  if (myRobot->haveStateOfCharge())
    myConfig->addParam(
	    ArConfigArg("DataLogStateOfCharge", &myLogStateOfCharge, "True to log state of charge"),
	    section.c_str(), ArPriority::DETAILED);
  
  myConfig->addParam(
	  ArConfigArg("DataLogChargeState", &myLogChargeState, 
		      "True to log charge state"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogBatteryInfo", &myLogBatteryInfo, 
		      "True to log battery info (if available)"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogPose", &myLogPose, "True to log robot's pose"),
	  section.c_str(), ArPriority::NORMAL);
  myConfig->addParam(
	  ArConfigArg("DataLogEncoderPose", &myLogEncoderPose, "True to log robot's raw encoder pose"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogCorrectedEncoderPose", &myLogCorrectedEncoderPose, "True to log robot's corrected (by gyro, etc) encoder pose"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogEncoders", &myLogEncoders, "True to log the raw encoder readings"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogLeftVel", &myLogLeftVel, "True to log left wheel velocity"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogRightVel", &myLogRightVel, "True to log right wheel velocity"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogTransVel", &myLogTransVel, "True to log translational wheel velocity"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogRotVel", &myLogRotVel, "True to log rotational wheel velocity"),
	  section.c_str(), ArPriority::DETAILED);
  if (myRobot->hasLatVel())
    myConfig->addParam(
	    ArConfigArg("DataLogLatVel", &myLogRotVel, "True to log lateral wheel velocity"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogLeftStalled", &myLogLeftStalled, "True to log if the left wheel is stalled"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogRightStalled", &myLogRightStalled, "True to log if the right wheel is stalled"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogStallBits", &myLogStallBits, "True to log all the stall bits is stalled"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogFlags", &myLogFlags, "True to log all the flags"),
	  section.c_str(), ArPriority::DETAILED);
  myConfig->addParam(
	  ArConfigArg("DataLogFaultFlags", &myLogFaultFlags, "True to log all the fault flags"),
	  section.c_str(), ArPriority::DETAILED);

  for (i = 0; i < myAnalogCount; i++)
  {
    snprintf(name, sizeof(name), "DataLogAnalog%d", i);
    snprintf(desc, sizeof(desc), 
	     "Logs the value of analog %d as a 10 bit (0-1024) value",
	     i);
    myConfig->addParam(
	    ArConfigArg(name, &myAnalogEnabled[i], desc),
	    section.c_str(), ArPriority::DETAILED);
  }
  for (i = 0; i < myAnalogVoltageCount; i++)
  {
    snprintf(name, sizeof(name), "DataLogAnalogVoltage%d", i);
    snprintf(desc, sizeof(desc), 
	     "Logs the value of analog %d as voltage from 0 to 5",
	     i);
    myConfig->addParam(
	    ArConfigArg(name, &myAnalogVoltageEnabled[i], desc),
	    section.c_str(), ArPriority::DETAILED);
  }
  for (i = 0; i < myDigInCount; i++)
  {
    snprintf(name, sizeof(name), "DataLogDigIn%d", i);
    snprintf(desc, sizeof(desc), "Logs digital in %d", i);
    myConfig->addParam(
	    ArConfigArg(name, &myDigInEnabled[i], desc),
	    section.c_str(), ArPriority::DETAILED);
  }
  for (i = 0; i < myDigOutCount; i++)
  {
    snprintf(name, sizeof(name), "DataLogDigOut%d", i);
    snprintf(desc, sizeof(desc), "Logs digital out %d", i);
    myConfig->addParam(
	    ArConfigArg(name, &myDigOutEnabled[i], desc),
	    section.c_str(), ArPriority::DETAILED);
  }
  myProcessFileCB.setName("ArDataLogger");
  myConfig->addProcessFileWithErrorCB(&myProcessFileCB, 100);
}

AREXPORT void ArDataLogger::connectCallback(void)
{
  int i;
  ArLog::log(ArLog::Verbose, "ArDataLogger::connectCallback");
  // out with the old memory
  if (myAnalogEnabled != NULL)
  {
    delete myAnalogEnabled;
    myAnalogEnabled = NULL;
  }
  if (myAnalogVoltageEnabled != NULL)
  {
    delete myAnalogVoltageEnabled;
    myAnalogVoltageEnabled = NULL;
  }
  if (myDigInEnabled != NULL)
  {
    delete myDigInEnabled;
    myDigInEnabled = NULL;
  }
  if (myDigOutEnabled != NULL)
  {
    delete myDigOutEnabled;
    myDigOutEnabled = NULL;
  }
  /*
  if (myStringsEnabled != NULL)
  {
    delete myStringsEnabled;
    myStringsEnabled = NULL;
  }
  */
  // see how much memory we need
  myAnalogCount = myRobot->getIOAnalogSize();
  myAnalogVoltageCount = myRobot->getIOAnalogSize();
  myDigInCount = myRobot->getIODigInSize();
  myDigOutCount = myRobot->getIODigOutSize();
  //myStringsCount = myStrings.size();
  // make and initialize the new memory
  if (myAnalogCount > 0)
  {
    myAnalogEnabled = new bool[myAnalogCount];
    for (i = 0; i < myAnalogCount; i++)
    {
      myAnalogEnabled[i] = false;
    }
  }
  if (myAnalogVoltageCount > 0)
  {
    myAnalogVoltageEnabled = new bool[myAnalogVoltageCount];
    for (i = 0; i < myAnalogVoltageCount; i++)
      myAnalogVoltageEnabled[i] = false;
  }
  if (myDigInCount > 0)
  {
    myDigInEnabled = new bool[myDigInCount];
    for (i = 0; i < myDigInCount; i++)
      myDigInEnabled[i] = false;
  }
  if (myDigOutCount > 0)
  {
    myDigOutEnabled = new bool[myDigOutCount];
    for (i = 0; i < myDigOutCount; i++)
      myDigOutEnabled[i] = false;
  }
  /* Taking this out since we do it with a map now.
    if (myStringsCount > 0)
  {
    myStringsEnabled = new bool[myStringsCount];
    for (i = 0; i < myStringsCount; i++)
      myStringsEnabled[i] = false;
      }*/
  if (myAddToConfigAtConnect && !myAddedToConfig)
  {
    myAddToConfigAtConnect = false;
    addToConfig(myConfig);
  }
}

AREXPORT bool ArDataLogger::processFile(char *errorBuffer, 
					size_t errorBufferLen)
{
  myMutex.lock();
  // if our file name is different and we're not using a permanent
  // file name or if we're disabled close the old one
  if ((strcmp(myOpenedFileName, myConfigFileName) != 0 && myFile != NULL && 
       myPermanentFileName.size() == 0) ||
       (myFile != NULL && !myConfigLogging))
  {
    ArLog::log(ArLog::Normal, "Closed data log file '%s'", myOpenedFileName);
    fclose(myFile);
    myFile = NULL;
  }
  // try to open the file
  if (myConfigLogging && myFile == NULL)
  {
    if (myPermanentFileName.size() == 0  && strlen(myConfigFileName) == 0)
    {
      ArLog::log(ArLog::Verbose, "ArDataLogger: no log file to open");
      myMutex.unlock();
      return true;
    }
    std::string fileName;
    if (myPermanentFileName.size() > 0)
    {
      if ((myFile = ArUtil::fopen(myPermanentFileName.c_str(), "a")) != NULL)
      {
	ArLog::log(ArLog::Normal, "Opened data log file '%s'", 
		   myPermanentFileName.c_str());
      }
      else
      {
	ArLog::log(ArLog::Normal, "Could not open data log file '%s'", 
		   myPermanentFileName.c_str());
	myMutex.unlock();
	return true;
      }
    }
    else
    {
      // if we couldn't open it fail
      if ((myFile = ArUtil::fopen(myConfigFileName, "w")) != NULL)
      {
	strcpy(myOpenedFileName, myConfigFileName);
	ArLog::log(ArLog::Normal, "Opened data log file '%s'", 
		   myOpenedFileName);
      }
      else
      {
	ArLog::log(ArLog::Normal, "Could not open data log file '%s'", 
		   myConfigFileName);
	myMutex.unlock();
	if (errorBuffer != NULL)
	  snprintf(errorBuffer, errorBufferLen, "DataLogFileName of '%s' cannot be opened", myConfigFileName);
	return false;
      }
    }
  }
  else if (!myConfigLogging)
  {
    myMutex.unlock();
    return true;
  }
  int i;
  // if we could then dump in the header
  fprintf(myFile, ";%12s", "Time");
  std::map<std::string, bool *, ArStrCaseCmpOp>::iterator it;
  for (i = 0; i < myStringsCount; i++)
  {
    if (*(myStringsEnabled[i]))
    {
      char formatBuf[64];
      sprintf(formatBuf, "\t%%0%ds", myStrings[i]->getMaxLength());
      fprintf(myFile, formatBuf, myStrings[i]->getName());
    }
  }
  if (myLogVoltage)
    fprintf(myFile, "\tVolt");
  if (myLogStateOfCharge)
    fprintf(myFile, "\tSoC");
  if (myLogChargeState)
    fprintf(myFile, "\t%015s\t%5s", "ChargeStateName", "csNum");
  if (myLogBatteryInfo && myRobot->getBatteryPacketReader() != NULL)
  {
    myRobot->getBatteryPacketReader()->requestContinuousPackets();
    int battery;
    for (battery = 1; 
	 battery <= myRobot->getBatteryPacketReader()->getNumBatteries();
	 battery++)
    {
      fprintf(myFile, "\tbat%02dflags1 \tbat%02dflags2 \tbat%02dflags3 \tbat%02drelsoc\tbat%02dabssoc", battery, battery, battery, battery, battery);
    }
  }
  if (myLogPose)
    fprintf(myFile, "\t%010s\t%010s\t%010s", "X", "Y", "Th");
  if (myLogEncoderPose)
    fprintf(myFile, "\t%010s\t%010s\t%010s", "encX", "encY", "encTh");
  if (myLogCorrectedEncoderPose)
    fprintf(myFile, "\t%010s\t%010s\t%010s", 
	    "corrEncX", "corrEncY", "corrEncTh");
  if (myLogEncoders)
  {
    fprintf(myFile, "\t%010s\t%010s", "encL", "encR");
    myRobot->requestEncoderPackets();
  }
  if (myLogLeftVel)
    fprintf(myFile, "\tLeftV");
  if (myLogRightVel)
    fprintf(myFile, "\tRightV");
  if (myLogTransVel)
    fprintf(myFile, "\tTransV");
  if (myLogRotVel)
    fprintf(myFile, "\tRotV");
  if (myLogLatVel)
    fprintf(myFile, "\tLatV");
  if (myLogLeftStalled)
    fprintf(myFile, "\tLStall");
  if (myLogRightStalled)
    fprintf(myFile, "\tRStall");
  if (myLogStallBits)
    fprintf(myFile, "\tStllBts%16s", "");
  if (myLogFlags)
    fprintf(myFile, "\tFlags%16s", "");
  if (myLogFaultFlags)
    fprintf(myFile, "\tFault Flags%10s", "");
  for (i = 0; i < myAnalogCount; i++)
  {
    if (myAnalogEnabled[i])
      fprintf(myFile, "\tAn%d", i);
  }
  for (i = 0; i < myAnalogVoltageCount; i++)
  {
    if (myAnalogVoltageEnabled[i])
      fprintf(myFile, "\tAnV%d", i);
  }
  for (i = 0; i < myDigInCount; i++)
  {
    if (myDigInEnabled[i])
      fprintf(myFile, "\tDigIn%d%8s", i, "");
  }
  for (i = 0; i < myDigOutCount; i++)
  {
    if (myDigOutEnabled[i])
      fprintf(myFile, "\tDigOut%d%8s", i, "");
  }

  fprintf(myFile, "\n");
  fflush(myFile);
  myMutex.unlock();
  return true;
}

AREXPORT void ArDataLogger::userTask(void)
{
  myMutex.lock();
  // if we don't need to do anything just return
  if (myFile == NULL || myLastLogged.secSince() < myConfigLogInterval)
  {
    myMutex.unlock();
    return;
  }
  int i;
  int j;
  int val;

  fprintf(myFile, "%ld", time(NULL));

  char *buf;
  buf = new char[myMaxMaxLength];
  ArStringInfoHolder *infoHolder;
  for (i = 0; i < myStringsCount; i++)
  {
    if (*(myStringsEnabled[i]))
    {
      char formatBuf[64];
      infoHolder = myStrings[i];
      sprintf(formatBuf, "\t%%0%ds", myStrings[i]->getMaxLength());
      infoHolder->getFunctor()->invoke(buf, infoHolder->getMaxLength());
      /*
      ArLog::log(ArLog::Normal, "For '%s' want to log with format '%s' and buf '%s' (max %d) (max max %d)",
		 infoHolder->getName(), formatBuf, buf, 
		 infoHolder->getMaxLength(), 
		 myMaxMaxLength);
      */
      fprintf(myFile, formatBuf, buf);
    }
  }
  delete buf;

  if (myLogVoltage)
    fprintf(myFile, "\t%.2f", myRobot->getRealBatteryVoltageNow());
  if (myLogStateOfCharge)
    fprintf(myFile, "\t%.0f", myRobot->getStateOfCharge());
  if (myLogChargeState)
  {  
    ArRobot::ChargeState chargeState = myRobot->getChargeState();
    std::string chargeString;
    if (chargeState == ArRobot::CHARGING_UNKNOWN)
      chargeString = "Unknowable";
    else if (chargeState == ArRobot::CHARGING_NOT)
      chargeString = "Not";
    else if (chargeState == ArRobot::CHARGING_BULK)
      chargeString = "Bulk";
    else if (chargeState == ArRobot::CHARGING_OVERCHARGE)
      chargeString = "Overcharge";
    else if (chargeState == ArRobot::CHARGING_FLOAT)
      chargeString = "Float";
    else if (chargeState == ArRobot::CHARGING_BALANCE)
      chargeString = "Balance";
    else
      chargeString = "Unknown";
    fprintf(myFile, "\t%15s\t%5d", chargeString.c_str(), chargeState);
  }

  if (myLogBatteryInfo && myRobot->getBatteryPacketReader() != NULL)
  {
    int battery;
    int flags;
    for (battery = 1; 
	 battery <= myRobot->getBatteryPacketReader()->getNumBatteries();
	 battery++)
    {
      fprintf(myFile, "\t");
      flags = myRobot->getBatteryPacketReader()->getFlags1(battery);
      for (i = 0, val = 1; i < 8; i++, val *= 2)
	fprintf(myFile, "%d", (bool) (flags & val));
      fprintf(myFile, "   ");

      fprintf(myFile, "\t");
      flags = myRobot->getBatteryPacketReader()->getFlags2(battery);
      for (i = 0, val = 1; i < 8; i++, val *= 2)
	fprintf(myFile, "%d", (bool) (flags & val));
      fprintf(myFile, "   ");

      fprintf(myFile, "\t");
      flags = myRobot->getBatteryPacketReader()->getFlags3(battery);
      for (i = 0, val = 1; i < 8; i++, val *= 2)
	fprintf(myFile, "%d", (bool) (flags & val));
      fprintf(myFile, "   ");
      
      fprintf(myFile, "\t%11d\t%11d", 
	      myRobot->getBatteryPacketReader()->getRelSOC(battery),
	      myRobot->getBatteryPacketReader()->getAbsSOC(battery));      
    }
  }

  if (myLogPose)
    fprintf(myFile, "\t%10.0f\t%10.0f\t%10.0f", myRobot->getX(), 
	    myRobot->getY(), myRobot->getTh());
  if (myLogEncoderPose)
    fprintf(myFile, "\t%10.0f\t%10.0f\t%10.0f",
	    myRobot->getRawEncoderPose().getX(), 
	    myRobot->getRawEncoderPose().getY(), 
	    myRobot->getRawEncoderPose().getTh());
  if (myLogCorrectedEncoderPose)
    fprintf(myFile, "\t%10.0f\t%10.0f\t%10.0f", 
	    myRobot->getEncoderPose().getX(), 
	    myRobot->getEncoderPose().getY(), 
	    myRobot->getEncoderPose().getTh());
  if (myLogEncoders)
    fprintf(myFile, "\t%10d\t%10d", 
	    myRobot->getLeftEncoder(), myRobot->getRightEncoder());
  if (myLogLeftVel)
    fprintf(myFile, "\t%.0f", myRobot->getLeftVel());
  if (myLogRightVel)
    fprintf(myFile, "\t%.0f", myRobot->getRightVel());
  if (myLogTransVel)
    fprintf(myFile, "\t%.0f", myRobot->getVel());
  if (myLogRotVel)
    fprintf(myFile, "\t%.0f", myRobot->getRotVel());
  if (myLogLatVel)
    fprintf(myFile, "\t%.0f", myRobot->getLatVel());
  if (myLogLeftStalled)
    fprintf(myFile, "\t%d", (bool)myRobot->isLeftMotorStalled());
  if (myLogRightStalled)
    fprintf(myFile, "\t%d", (bool)myRobot->isRightMotorStalled());
  if (myLogStallBits)
  {
    fprintf(myFile, "\t");
    for (i = 0, val = 1; i < 16; i++, val *= 2)
      fprintf(myFile, "%d", (bool)(myRobot->getStallValue() & val));
  }
  if (myLogFlags)
  {
    fprintf(myFile, "\t");
    for (i = 0, val = 1; i < 16; i++, val *= 2)
      fprintf(myFile, "%d", (bool)(myRobot->getFlags() & val));
  }
  if (myLogFaultFlags)
  {
    fprintf(myFile, "\t");
    for (i = 0, val = 1; i < 16; i++, val *= 2)
      fprintf(myFile, "%d", (bool)(myRobot->getFaultFlags() & val));
  }
  for (i = 0; i < myAnalogCount; i++)
  {
    if (myAnalogEnabled[i])
      fprintf(myFile, "\t%d", myRobot->getIOAnalog(i));
  }
  for (i = 0; i < myAnalogVoltageCount; i++)
  {
    if (myAnalogVoltageEnabled[i])
      fprintf(myFile, "\t%.2f", myRobot->getIOAnalogVoltage(i));
  }
  for (i = 0; i < myDigInCount; i++)
  {
    if (myDigInEnabled[i])
    {
      fprintf(myFile, "\t");
      for (j = 0, val = 1; j < 8; j++, val *= 2)
	fprintf(myFile, "%d", (bool)(myRobot->getIODigIn(i) & val));
    }
  }
  for (i = 0; i < myDigOutCount; i++)
  {
    if (myDigOutEnabled[i])
    {
      fprintf(myFile, "\t");
      for (j = 0, val = 1; j < 8; j++, val *= 2)
	fprintf(myFile, "%d", (bool)(myRobot->getIODigOut(i) & val));
    }
  }
  
  fprintf(myFile, "\n");
  fflush(myFile);
  myLastLogged.setToNow();
  myMutex.unlock();
}


AREXPORT void ArDataLogger::addString(
	const char *name, ArTypes::UByte2 maxLength,
	ArFunctor2<char *, ArTypes::UByte2> *functor)
{
  ArTypes::UByte2 len;

  myMutex.lock();
  if (maxLength < strlen(name))
    len = strlen(name);
  else
    len = maxLength;  
  if (myMaxMaxLength < len)
    myMaxMaxLength = len;

  myStrings.push_back(new ArStringInfoHolder(name, len, functor));
  bool *boolPtr;
  boolPtr = new bool;
  // if we've added to config we default to true
  if (myAddedToConfig)
    *boolPtr = true;
  else
    *boolPtr = false;
  myStringsEnabled.push_back(boolPtr);
  myStringsCount++;
  myMutex.unlock();
  if (myAddedToConfig)
    processFile(NULL, 0);
}


#include "Aria.h"
#include "ArExport.h"
#include "ArServerBase.h"
#include "ArServerSimpleCommands.h"

AREXPORT ArServerSimpleComUC::ArServerSimpleComUC(
	ArServerHandlerCommands *handlerCommands, ArRobot *robot) :
  myCommandCB(this, &ArServerSimpleComUC::command),
  myMotionCommandCB(this, &ArServerSimpleComUC::motionCommand)
{
  myHandlerCommands = handlerCommands;
  myRobot = robot;
  if (myHandlerCommands != NULL)
  {
    myHandlerCommands->addStringCommand("MicroControllerCommand",
					"MicroController (uC) command mode has three ways to send commands:\ncom: <command>\ncomInt: <command> <int>\ncom2Bytes: <command> <byte1> <byte2>", &myCommandCB);
    myHandlerCommands->addStringCommand("MicroControllerMotionCommand",
					"MicroController (uC) motion commands will suspend normal motion commands to the microcontroller and ONLY the motion commands will be in charge.  This will last until the server mode is changed or stop is pressed (unless the server mode is locked, ie docking).  This has four ways to send commands:\ncom: <command>\ncomInt: <command> <int>\ncom2Bytes: <command> <byte1> <byte2>:", &myMotionCommandCB);
  }
}

AREXPORT ArServerSimpleComUC::~ArServerSimpleComUC()
{
}


AREXPORT void ArServerSimpleComUC::command(ArArgumentBuilder *arg)
{
  processCommand(arg, false);
}

AREXPORT void ArServerSimpleComUC::motionCommand(ArArgumentBuilder *arg)
{
  processCommand(arg, true);
}

void ArServerSimpleComUC::processCommand(ArArgumentBuilder *arg,
						  bool motionCommand)
{
  int command;
  int int1;
  int int2;
  
  std::string prefix;
  
  if (!motionCommand)
    prefix = "uCCommand: ";
  else
    prefix = "uCMotionCommand: ";    
  myRobot->lock();
  if (motionCommand)
    myRobot->stopStateReflection();
  if (arg->getFullString() == NULL || arg->getFullString()[0] == '\0' ||
      arg->getArgc() == 0)
  {
    if (motionCommand)
    {
      ArLog::log(ArLog::Terse, "%sReset to normal control.", 
		 prefix.c_str());
      myRobot->stop();
      myRobot->clearDirectMotion();
    }
    else
      ArLog::log(ArLog::Terse, "%sSyntax error, no arguments.", 
		 prefix.c_str());
  }    
  else if (arg->getArgc() == 1)
  {
    command = arg->getArgInt(0);
    if (command < 0 || command > 255 || !arg->isArgInt(0))
    {
      ArLog::log(ArLog::Terse, 
		 "%sInvalid command, must be an integer between 0 and 255",
		 prefix.c_str());
    }
    else
    {
      ArLog::log(ArLog::Terse, "%scom(%d)", prefix.c_str(), command);
      myRobot->com(command);
    }
  }
  else if (arg->getArgc() == 2)
  {
    command = arg->getArgInt(0);
    int1 = arg->getArgInt(1);
    if (command < 0 || command > 255 || !arg->isArgInt(0))
    {
      ArLog::log(ArLog::Terse, 
		 "%sInvalid command, must be an integer between 0 and 255",
		 prefix.c_str());
    }
    else if (int1 < -32767 || int1 > 65536 || !arg->isArgInt(1))
    {
      ArLog::log(ArLog::Terse, 
	 "%sInvalid integer, must be an integer between -32767 and 65536",
		 prefix.c_str());
    }
    else
    {
      ArLog::log(ArLog::Terse, "%scomInt(%d, %d)", prefix.c_str(), command,
		 int1);
      myRobot->comInt(command, int1);
    }
  }
  else if (arg->getArgc() == 3)
  {
    command = arg->getArgInt(0);
    int1 = arg->getArgInt(1);
    int2 = arg->getArgInt(2);
    if (command < 0 || command > 255 || !arg->isArgInt(0))
    {
      ArLog::log(ArLog::Terse, 
		 "%sInvalid command, must be between 0 and 255",
		 prefix.c_str());
    }
    else if (int1 < -128 || int1 > 255 || !arg->isArgInt(1))
    {
      ArLog::log(ArLog::Terse, 
	 "%sInvalid byte1, must be an integer between -128 and 127, or between 0 and 255", prefix.c_str());
    }
    else if (int2 < -128 || int2 > 255 || !arg->isArgInt(2))
    {
      ArLog::log(ArLog::Terse, 
	 "%sInvalid byte2, must be an integer between -128 and 127, or between 0 and 255", prefix.c_str());
    }
    else
    {
      ArLog::log(ArLog::Terse, 
		 "%scom2Bytes(%d, %d, %d)", 
		 prefix.c_str(), command, int1, int2);
      myRobot->com2Bytes(command, int1, int2);
    }
  }
  else
  {
    ArLog::log(ArLog::Terse, "%sSyntax error, too many arguments",
	       prefix.c_str());
  }
  myRobot->unlock();
}


AREXPORT ArServerSimpleComMovementLogging::ArServerSimpleComMovementLogging(
	ArServerHandlerCommands *handlerCommands, ArRobot *robot,
	ArServerHandlerPopup *popupHandler) :
  myLogMovementSentEnableCB(this, 
		 &ArServerSimpleComMovementLogging::logMovementSentEnable),
  myLogMovementSentDisableCB(this, 
		 &ArServerSimpleComMovementLogging::logMovementSentDisable),
  myLogMovementReceivedEnableCB(this, 
              &ArServerSimpleComMovementLogging::logMovementReceivedEnable),
  myLogMovementReceivedDisableCB(this, 
  	      &ArServerSimpleComMovementLogging::logMovementReceivedDisable),
  myLogVelocitiesReceivedEnableCB(this, 
              &ArServerSimpleComMovementLogging::logVelocitiesReceivedEnable),
  myLogVelocitiesReceivedDisableCB(this, 
  	      &ArServerSimpleComMovementLogging::logVelocitiesReceivedDisable),
  myPacketsReceivedTrackingEnableCB(this, 
	   &ArServerSimpleComMovementLogging::packetsReceivedTrackingEnable),
  myPacketsReceivedTrackingDisableCB(this, 
	    &ArServerSimpleComMovementLogging::packetsReceivedTrackingDisable),
  myPacketsSentTrackingEnableCB(this, 
	   &ArServerSimpleComMovementLogging::packetsSentTrackingEnable),
  myPacketsSentTrackingDisableCB(this, 
	    &ArServerSimpleComMovementLogging::packetsSentTrackingDisable),
  myLogActionsEnableCB(this, 
              &ArServerSimpleComMovementLogging::logActionsEnable),
  myLogActionsDisableCB(this, 
  	      &ArServerSimpleComMovementLogging::logActionsDisable),
  myLogActionsCB(this, &ArServerSimpleComMovementLogging::logActions),
  myPopupMovementParamsCB(this, 
		  &ArServerSimpleComMovementLogging::popupMovementParams),
  myResetOdometerCB(this, &ArServerSimpleComMovementLogging::resetOdometer)
{
  myHandlerCommands = handlerCommands;
  myRobot = robot;
  myPopupHandler = popupHandler;
  myHandlerCommands->addCommand(
	  "LogMovementSentEnable",
	  "Enables logging of the movement commands sent to the robot",
	  &myLogMovementSentEnableCB);
  myHandlerCommands->addCommand(
	  "LogMovementSentDisable",
	  "Disables logging of the movement commands sent to the robot",
	  &myLogMovementSentDisableCB);
  myHandlerCommands->addCommand(
	  "LogMovementReceivedEnable",
	  "Enables logging of the movement data received from the robot",
	  &myLogMovementReceivedEnableCB);
  myHandlerCommands->addCommand(
	  "LogMovementReceivedDisable",
	  "Disables logging of the movement data received from the robot",
	  &myLogMovementReceivedDisableCB);
  myHandlerCommands->addCommand(
	  "LogVelocitiesReceivedEnable",
	  "Enables logging of the velocity data received from the robot",
	  &myLogVelocitiesReceivedEnableCB);
  myHandlerCommands->addCommand(
	  "LogVelocitiesReceivedDisable",
	  "Disables logging of the velocity data received from the robot",
	  &myLogVelocitiesReceivedDisableCB);
  myHandlerCommands->addCommand(
	  "PacketsReceivedTrackingEnable",
	  "Enables tracking of packets received from the robot",
	  &myPacketsReceivedTrackingEnableCB);
  myHandlerCommands->addCommand(
	  "PacketsReceivedTrackingDisable",
	  "Disables tracking of packets received from the robot",
	  &myPacketsReceivedTrackingDisableCB);
  myHandlerCommands->addCommand(
	  "PacketsSentTrackingEnable",
	  "Enables tracking of packets sent to the robot",
	  &myPacketsSentTrackingEnableCB);
  myHandlerCommands->addCommand(
	  "PacketsSentTrackingDisable",
	  "Disables tracking of packets sent to the robot",
	  &myPacketsSentTrackingDisableCB);
  myHandlerCommands->addCommand(
	  "LogActionsEnable",
	  "Enables continual logging of the actions",
	  &myLogActionsEnableCB);
  myHandlerCommands->addCommand(
	  "LogActionsDisable",
	  "Disables continual logging of the actions",
	  &myLogActionsDisableCB);
  myHandlerCommands->addCommand(
	  "LogActions",
	  "Logs the state of the actions on the robot once",
	  &myLogActionsCB);
  if (myPopupHandler != NULL)
  {
    myHandlerCommands->addCommand(
	    "PopupMovementParams",
	    "Creates a popup with the movement parameters for this robot",
	    &myPopupMovementParamsCB);
  }
  myHandlerCommands->addCommand(
	  "ResetTripOdometer",
	  "Resets the robot's trip odometer",
	  &myResetOdometerCB);

}

AREXPORT ArServerSimpleComMovementLogging::~ArServerSimpleComMovementLogging()
{
}

AREXPORT void ArServerSimpleComMovementLogging::logMovementSentEnable(void)
{
  myRobot->lock();
  myRobot->setLogMovementSent(true);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logMovementSentDisable(void)
{
  myRobot->lock();
  myRobot->setLogMovementSent(false);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logMovementReceivedEnable(void)
{
  myRobot->lock();
  myRobot->setLogMovementReceived(true);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logMovementReceivedDisable(void)
{
  myRobot->lock();
  myRobot->setLogMovementReceived(false);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logVelocitiesReceivedEnable(void)
{
  myRobot->lock();
  myRobot->setLogVelocitiesReceived(true);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logVelocitiesReceivedDisable(void)
{
  myRobot->lock();
  myRobot->setLogVelocitiesReceived(false);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::packetsReceivedTrackingEnable(void)
{
  myRobot->lock();
  myRobot->setPacketsReceivedTracking(true);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::packetsReceivedTrackingDisable(void)
{
  myRobot->lock();
  myRobot->setPacketsReceivedTracking(false);
  myRobot->unlock();
}


AREXPORT void ArServerSimpleComMovementLogging::packetsSentTrackingEnable(void)
{
  myRobot->lock();
  myRobot->setPacketsSentTracking(true);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::packetsSentTrackingDisable(void)
{
  myRobot->lock();
  myRobot->setPacketsSentTracking(false);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logActionsEnable(void)
{
  myRobot->lock();
  myRobot->setLogActions(true);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logActionsDisable(void)
{
  myRobot->lock();
  myRobot->setLogActions(false);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::logActions(void)
{
  myRobot->lock();
  myRobot->logActions();
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComMovementLogging::popupMovementParams(void)
{
  char buf[32000];
  myRobot->lock();
  sprintf(buf, "AbsoluteMaxTransVel %.0f AbsoluteMaxTransNegVel %.0f\nTransVelMax %.0f TransNegVelMax %.0f\n\nAbsoluteMaxTransAccel %.0f AbsoluteMaxTransDecel %.0f\nTransAccel %.0f TransDecel %.0f\n\nAbsoluteMaxRotVel %.0f AbsoluteMaxRotAccel %.0f AbsoluteMaxRotDecel %.0f\nRotVelMax %.0f RotAccel %.0f RotDecel %.0f",
	  myRobot->getAbsoluteMaxTransVel(), 
	  myRobot->getAbsoluteMaxTransNegVel(), 
	  myRobot->getTransVelMax(),
	  myRobot->getTransNegVelMax(),
	  myRobot->getAbsoluteMaxTransAccel(),
	  myRobot->getAbsoluteMaxTransDecel(),
	  myRobot->getTransAccel(),
	  myRobot->getTransDecel(),	  
	  myRobot->getAbsoluteMaxRotVel(), 
	  myRobot->getAbsoluteMaxRotAccel(),
	  myRobot->getAbsoluteMaxRotDecel(),
	  myRobot->getRotVelMax(),
	  myRobot->getRotAccel(),
	  myRobot->getRotDecel());
  if (myRobot->hasLatVel())
    sprintf(buf, "%s\n\nAbsoluteMaxLatVel %.0f AbsoluteMaxLatAccel %.0f AbsoluteMaxLatDecel %.0f\nLatVelMax %.0f LatAccel %.0f LatDecel %.0f",
	    buf, 
	    myRobot->getAbsoluteMaxLatVel(), 
	    myRobot->getAbsoluteMaxLatAccel(),
	    myRobot->getAbsoluteMaxLatDecel(),
	    myRobot->getLatVelMax(),
	    myRobot->getLatAccel(),
	    myRobot->getLatDecel());
  
  ArServerHandlerPopupInfo popupInfo(
	  NULL, "Robot Movement Parameters", buf, 
	  ArServerHandlerPopup::INFORMATION,
	  0, 0, 0, NULL, "OK", "Done viewing movement parameters");
  
  myPopupHandler->createPopup(&popupInfo);  
  myRobot->unlock();
}
AREXPORT ArServerSimpleComGyro::ArServerSimpleComGyro(
	ArServerHandlerCommands *handlerCommands, ArRobot *robot,
	ArAnalogGyro *gyro) :
  myGyroEnableCB(this, &ArServerSimpleComGyro::gyroEnable),
  myGyroDisableCB(this, &ArServerSimpleComGyro::gyroDisable)
{
  myHandlerCommands = handlerCommands;
  myRobot = robot;
  myGyro = gyro;
  myHandlerCommands->addCommand("GyroEnable",
				"Enables the gyro",
				&myGyroEnableCB);
  myHandlerCommands->addCommand("GyroDisable",
				"Disables the gyro",
				&myGyroDisableCB);
}

AREXPORT void ArServerSimpleComMovementLogging::resetOdometer(void)
{
  myRobot->lock();
  myRobot->resetTripOdometer();
  myRobot->unlock();
}

AREXPORT ArServerSimpleComGyro::~ArServerSimpleComGyro()
{
}

AREXPORT void ArServerSimpleComGyro::gyroEnable(void)
{
  myRobot->lock();
  if (myGyro != NULL)
    myGyro->activate();
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComGyro::gyroDisable(void)
{
  myRobot->lock();
  if (myGyro != NULL)
    myGyro->deactivate();
  myRobot->unlock();
}

AREXPORT ArServerSimpleComLogRobotConfig::ArServerSimpleComLogRobotConfig(
	ArServerHandlerCommands *commands, ArRobot* robot, 
	ArServerHandlerPopup *popupHandler) :
  myPacketArrivedCB(this, 
		    &ArServerSimpleComLogRobotConfig::configPacketArrived),
  myPacketReader(robot, false, &myPacketArrivedCB),
  myLogConfigCB(this, &ArServerSimpleComLogRobotConfig::logConfig),
  myLogMovementConfigCB(
	  this, &ArServerSimpleComLogRobotConfig::logMovementConfig),
  myLogOrigConfigCB(this, &ArServerSimpleComLogRobotConfig::logOrigConfig),
  myPopupConfigCB(this, &ArServerSimpleComLogRobotConfig::popupConfig),
  myPopupOrigConfigCB(this, &ArServerSimpleComLogRobotConfig::popupOrigConfig),
  myPopupMovementConfigCB(
	  this, &ArServerSimpleComLogRobotConfig::popupMovementConfig)
{
  myRobot = robot;
  myHandlerCommands = commands;
  myPopupHandler = popupHandler;

  myLogConfig = false;
  myPopupConfig = false;
  myLogConfigMovement = false;
  myPopupConfigMovement = false;

  commands->addCommand("LogRobotConfig", 
		       "Get current robot configuration and write it to the server log.", 
		       &myLogConfigCB);
  commands->addCommand("LogRobotConfigOrig", 
		       "Logs the original config packet the robot sent on connection.", 
		       &myLogOrigConfigCB);
  commands->addCommand("LogRobotConfigMovement", 
		       "Get current robot configuration and write the movement part to the server log.", 
		       &myLogMovementConfigCB);
  if (myPopupHandler != NULL)
  {
    commands->addCommand("PopupRobotConfig", 
			 "Popups up the current robot configuration.", 
			 &myPopupConfigCB);
    commands->addCommand("PopupRobotConfigOrig", 
			 "Popups up the original config packet the robot sent on connection.", 
			 &myPopupOrigConfigCB);
    commands->addCommand("PopupRobotConfigMovement", 
	 "Popups up the movement part of the current robot configuration.", 
			 &myPopupMovementConfigCB);

  }
}


AREXPORT void ArServerSimpleComLogRobotConfig::logConfig(void) 
{
  ArLog::log(ArLog::Normal, "Server received logRobotConfig command. Config packet requested");
  myRobot->lock();
  myPacketReader.requestPacket();
  myLogConfig = true;
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComLogRobotConfig::logMovementConfig(void) 
{
  ArLog::log(ArLog::Normal, "Server received logRobotConfigMovement command. Config packet requested");
  myRobot->lock();
  myPacketReader.requestPacket();
  myLogConfigMovement = true;
  myRobot->unlock();
}


AREXPORT void ArServerSimpleComLogRobotConfig::logOrigConfig(void) 
{
  myRobot->lock();
  if (myRobot->getOrigRobotConfig() != NULL && 
      myRobot->getOrigRobotConfig()->hasPacketArrived())
  {
    ArLog::log(ArLog::Normal, "-- Orig Config Packet: --");
    myRobot->getOrigRobotConfig()->log();
    ArLog::log(ArLog::Normal, "-- End Orig Config --");
  }
  else
  {
    ArLog::log(ArLog::Normal, 
	       "Cannot log Orig Config since it was never received.");
  }
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComLogRobotConfig::popupConfig(void) 
{
  ArLog::log(ArLog::Normal, "Server received popupRobotConfig command. Config packet requested");
  myRobot->lock();
  myPacketReader.requestPacket();
  myPopupConfig = true;
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComLogRobotConfig::popupMovementConfig(void) 
{
  ArLog::log(ArLog::Normal, "Server received popupRobotConfigMovement command. Config packet requested");
  myRobot->lock();
  myPacketReader.requestPacket();
  myPopupConfigMovement = true;
  myRobot->unlock();
}

AREXPORT void ArServerSimpleComLogRobotConfig::popupOrigConfig(void) 
{
  myRobot->lock();
  if (myRobot->getOrigRobotConfig() != NULL && 
      myRobot->getOrigRobotConfig()->hasPacketArrived())
  {
    std::string str;
    str = myRobot->getOrigRobotConfig()->buildString();
    ArServerHandlerPopupInfo popupInfo(
	    NULL, "Orig robot config", str.c_str(), 
	    ArServerHandlerPopup::INFORMATION,
	    0, 0, 0, NULL, "OK", "Done viewing orig robot config parameters");
    
    myPopupHandler->createPopup(&popupInfo);  
  }
  else
  {
    ArLog::log(ArLog::Normal, 
	       "Cannot log Orig Config since it was never received.");
  }
  myRobot->unlock();
}

void ArServerSimpleComLogRobotConfig::configPacketArrived(void)
{
  if (myLogConfig)
  {
    ArLog::log(ArLog::Normal, "-- Robot Config Movement Packet Returned: --");
    myPacketReader.log();
    ArLog::log(ArLog::Normal, "-- End Config --");
  }
  if (myLogConfigMovement)
  {
    ArLog::log(ArLog::Normal, "-- Robot Config Movement Packet Returned: --");
    myPacketReader.logMovement();
    ArLog::log(ArLog::Normal, "-- End Config --");
  }
  if (myPopupConfig)
  {
    std::string str;
    str = myPacketReader.buildString();
    ArServerHandlerPopupInfo popupInfo(
	    NULL, "Robot config", str.c_str(), 
	    ArServerHandlerPopup::INFORMATION,
	    0, 0, 0, NULL, "OK", "Done viewing robot config parameters");
    
    myPopupHandler->createPopup(&popupInfo);  
    
  }
  if (myPopupConfigMovement)
  {
    std::string str;
    str = "_______________________________________________________\n";
    str += myPacketReader.buildStringMovement();
    ArServerHandlerPopupInfo popupInfo(
	    NULL, "Robot config movement", str.c_str(), 
	    ArServerHandlerPopup::INFORMATION,
	    0, 0, 0, NULL, "OK", 
	    "Done viewing robot config movement parameters");
    myPopupHandler->createPopup(&popupInfo);  
  }
  myLogConfig = false;
  myPopupConfig = false;
  myLogConfigMovement = false;
  myPopupConfigMovement = false;
}

AREXPORT ArServerSimpleComLogActions::ArServerSimpleComLogActions(ArServerHandlerCommands *commands, ArRobot* robot) :
  myRobot(robot), myCallback(this, &ArServerSimpleComLogActions::logActions) 
{
  commands->addCommand("LogActions", "Write current ArRobot actions to server log file.",
      &myCallback);
}

AREXPORT void ArServerSimpleComLogActions::logActions() 
{
  ArLog::log(ArLog::Normal, "Server simple command logActions: current ArRobot actions are:");
    myRobot->logActions();
}

AREXPORT ArServerSimpleServerCommands::ArServerSimpleServerCommands(
	ArServerHandlerCommands *commands, ArServerBase *server, 
	bool addLogConnections) :
  myTerseTrackingCB(this, &ArServerSimpleServerCommands::logTerseTracking),
  myVerboseTrackingCB(this, &ArServerSimpleServerCommands::logVerboseTracking),
  myResetTrackingCB(this, &ArServerSimpleServerCommands::resetTracking),
  myLogConnectionsCB(this, &ArServerSimpleServerCommands::logConnections) 
{
  myServer = server;
  commands->addCommand("NetworkTrackingLogTerse", 
		       "Logs the information about client commands sent and received:",
		       &myTerseTrackingCB);
  commands->addCommand("NetworkTrackingLogVerbose", 
		       "Logs verbosely (broken up by tcp/udp) the information about client commands sent and received:",
		       &myVerboseTrackingCB);

  commands->addCommand("NetworkTrackingReset", 
		       "Resets the information about client commands sent and received:",
		       &myResetTrackingCB);
  if (addLogConnections)
    commands->addCommand("NetworkLogConnections", 
			 "Logs the connections to the server", 
			 &myLogConnectionsCB);
}

AREXPORT ArServerSimpleServerCommands::~ArServerSimpleServerCommands()
{
}

AREXPORT void ArServerSimpleServerCommands::logTerseTracking() 
{
  myServer->logTracking(true);
}

AREXPORT void ArServerSimpleServerCommands::logVerboseTracking() 
{
  myServer->logTracking(false);
}

AREXPORT void ArServerSimpleServerCommands::resetTracking() 
{
  myServer->resetTracking();
}

AREXPORT void ArServerSimpleServerCommands::logConnections() 
{
  myServer->logConnections();
}

AREXPORT ArServerSimplePopup::ArServerSimplePopup(
	ArServerHandlerCommands *commands, 
	ArServerHandlerPopup *popupHandler) :
  mySimplePopupCB(this, &ArServerSimplePopup::simplePopup)
{
  myCommands = commands;
  myPopupHandler = popupHandler;
  myCommands->addStringCommand(
	  "SimplePopup",
	  "\"<title>\" \"<message>\" \"<button>\" <int:timeout>", 
	  &mySimplePopupCB);
}

AREXPORT ArServerSimplePopup::~ArServerSimplePopup()
{
}

AREXPORT void ArServerSimplePopup::simplePopup(ArArgumentBuilder *arg)
{

  arg->compressQuoted(true);

  if (arg->getArgc() < 4)
  {
    ArLog::log(ArLog::Terse, 
	       "simplePopup: Syntax error, not enough arguments.");
    return;
  }    

  ArServerHandlerPopupInfo popupInfo(
	  arg->getArg(0), arg->getArg(0), 
	  arg->getArg(1), ArServerHandlerPopup::INFORMATION, 
	  0, 0, arg->getArgInt(3), arg->getArg(1),
	  arg->getArg(2), arg->getArg(1));
  myPopupHandler->createPopup(&popupInfo);
}

AREXPORT ArServerSimpleLogRobotDebugPackets::ArServerSimpleLogRobotDebugPackets(
	ArServerHandlerCommands *commands, ArRobot *robot, 
	const char *baseDirectory) :
  myPacketHandlerCB(this, &ArServerSimpleLogRobotDebugPackets::packetHandler),
  myStartLoggingCB(this, &ArServerSimpleLogRobotDebugPackets::startLogging,
		   "robotDebugPacketsLog.txt"),
  myStopLoggingCB(this, &ArServerSimpleLogRobotDebugPackets::stopLogging)
{
  myRobot = robot;
  myCommands = commands;

  myPacketHandlerCB.setName("ArServerSimpleLogRobotDebugPackets::packetHandler");
  myStartLoggingCB.setName("ArLogRobotDebugPacktets::startLogging");
  myStopLoggingCB.setName("ArLogRobotDebugPacktets::stopLogging");
  
  myRobot->addPacketHandler(&myPacketHandlerCB);
  myCommands->addCommand("RobotDebugPacketLoggingEnable", 
			 "Starts logging robot debug packets to file",
			 &myStartLoggingCB);

  myCommands->addCommand("RobotDebugPacketLoggingDisable", 
			 "Stops logging robot debug packets to file",
			 &myStopLoggingCB);
  
  myFile = NULL;
  if (baseDirectory != NULL && baseDirectory[0] != '\0')
    myBaseDir = baseDirectory;
  else
    myBaseDir = "";
}

AREXPORT ArServerSimpleLogRobotDebugPackets::~ArServerSimpleLogRobotDebugPackets()
{
  myRobot->remPacketHandler(&myPacketHandlerCB);
}

AREXPORT bool ArServerSimpleLogRobotDebugPackets::packetHandler(ArRobotPacket *packet)
{
  if (packet->getID() != 0xfc || myFile == NULL)
    return false;

  int numIntsInGroup = 0;
  int numGroups = 0;
  
  numIntsInGroup = packet->bufToByte();
  numGroups = packet->bufToByte();

  myNumVals = numIntsInGroup;
  
  if (numIntsInGroup == 0 || numGroups == 0)
  {
    ArLog::log(ArLog::Normal, "numIntsInGroup %d numGroups %d", 
	       numIntsInGroup, numGroups);
    return true;
  }

  int i;
  int j;
  int val;
  char buf[2048];
  char *timeStr;
  int timeLen = 20; // this is a value based on the standard length of
  // ctime return
  time_t now;
  for (i = 0; i < numGroups; i++)
  {
    now = time(NULL);
    timeStr = ctime(&now);
    // get take just the portion of the time we want
    strncpy(buf, timeStr, timeLen);
    buf[timeLen] = '\0';
    for (j = 0; j < numIntsInGroup; j++)
    {
      val = packet->bufToByte2();
      myVals[j] = val;
      sprintf(buf, "%s\t%d", buf, val);
    }
    fprintf(myFile, "%s\n", buf);
  }
  return true;
}

AREXPORT bool ArServerSimpleLogRobotDebugPackets::startLogging(const char *fileName)
{
  std::string realFileName;

  if (myFile != NULL)
    stopLogging();

  if (fileName == NULL || fileName[0] == '\0')
  {
    ArLog::log(ArLog::Normal, "ArServerSimpleLogRobotDebugPackets::startLogging: Empty filename given, cannot log");
    return false;
  }
  
  if (fileName[0] == '/' || fileName[0] == '\\')
  {
    realFileName = fileName;
  }
  else
  {
    realFileName = myBaseDir;
    realFileName += fileName;
  }

  
  if ((myFile = ArUtil::fopen(realFileName.c_str(), "w")) == NULL)
  {
    ArLog::log(ArLog::Terse, 
	       "ArServerSimpleLogRobotDebugPackets::startLogging: Could not open file %s for writing",
	       realFileName.c_str());
    return false;
  }
  
  ArLog::log(ArLog::Normal, 
	     "ArServerSimpleLogRobotDebugPackets::startLogging: Logging to file %s",
	     realFileName.c_str());
  myRobot->lock();
  myRobot->comInt(251, 1);
  myRobot->unlock();
  return true;
}

AREXPORT bool ArServerSimpleLogRobotDebugPackets::stopLogging(void)
{
  if (myFile != NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerSimpleLogRobotDebugPackets::stopLogging: Stopping");
    fclose(myFile);
    myFile = NULL;
  }
  myRobot->lock();
  myRobot->comInt(251, 0);
  myRobot->unlock();
  return true;
}

AREXPORT void ArServerSimpleLogRobotDebugPackets::addToInfoGroup(
	ArStringInfoGroup *infoGroup, const char *name, int whichVal,
	const char *format)
{
  infoGroup->addStringInt(
	  name, 8, 
	  new ArRetFunctor1C<int, ArServerSimpleLogRobotDebugPackets, int>(
		  this, 
		  &ArServerSimpleLogRobotDebugPackets::getValue, whichVal),
	  format);
}

AREXPORT int ArServerSimpleLogRobotDebugPackets::getNumValues(void)
{
  return myNumVals;
}

AREXPORT int ArServerSimpleLogRobotDebugPackets::getValue(int whichVal)
{
  if (myVals.find(whichVal) == myVals.end())
    return 0;
  else
    return myVals[whichVal];
}

AREXPORT ArServerSimpleConnectionTester::ArServerSimpleConnectionTester(
	ArServerHandlerCommands *handlerCommands, ArRobot *robot) :
  myConnectionTestStartCB(this, &ArServerSimpleConnectionTester::connectionTestStart),
  myConnectionTestStopCB(this, &ArServerSimpleConnectionTester::connectionTestStop),
  myPacketHandlerCB(this, &ArServerSimpleConnectionTester::packetHandler),
  myUserTaskCB(this, &ArServerSimpleConnectionTester::userTask)
{
  myRobot = robot;
  myHandlerCommands = handlerCommands;

  if (myHandlerCommands) 
  {
    myHandlerCommands->addCommand(
	    "ConnectionTesterStart",
	    "Starts testing the connection to the robot", 
	    &myConnectionTestStartCB);
    myHandlerCommands->addCommand(
	    "ConnectionTesterStop",
	    "Stops testing the connection to the robot", 
	    &myConnectionTestStopCB);
  }

  myPacketHandlerCB.setName("ArServerSimpleConnectionTester::packetHandler");
  myUserTaskCB.setName("ArServerSimpleConnectionTester::userTask");
  
}

AREXPORT ArServerSimpleConnectionTester::~ArServerSimpleConnectionTester()
{
}


AREXPORT void ArServerSimpleConnectionTester::connectionTestStart(void)
{
  ArLog::log(ArLog::Normal, "ConnectionTester: Starting test");
  myRobot->lock();
  myFirst = true;
  myRobot->remPacketHandler(&myPacketHandlerCB);
  myRobot->addPacketHandler(&myPacketHandlerCB, ArListPos::FIRST);
  myRobot->remUserTask(&myUserTaskCB);
  myRobot->addUserTask("connectionTester", 50, &myUserTaskCB);
  myRobot->unlock();
}

AREXPORT void ArServerSimpleConnectionTester::connectionTestStop(void)
{
  ArLog::log(ArLog::Normal, "ConnectionTester: Stopping test");
  myRobot->lock();
  log();
  myRobot->remPacketHandler(&myPacketHandlerCB);
  myRobot->remUserTask(&myUserTaskCB);
  myRobot->unlock();
}

bool ArServerSimpleConnectionTester::packetHandler(ArRobotPacket *packet)
{
  if (packet->getID() == 0x90 || 
      packet->getID() == ArCommands::SAFETY_STATE_INFO)
  {
    myReceivedPackets++;
    myPacketsThisCycle++;
    myPacketReceived.setToNow();
    return true;
  }
  return false;
}

void ArServerSimpleConnectionTester::userTask(void)
{
  if (myFirst)
  {
    myRobot->comInt(ArCommands::ENCODER, 1);
    myRobot->comInt(ArCommands::SAFETY_STATE_INFO, 1);
    myFirst = false;
    myReceivedPackets = 0;
    myMissedPackets = 0;
    //myMissedMotorPackets = 0;
    myPacketsThisCycle = 0;
    myCyclesSincePacket = 0;
    myPacketReceived.setToNow();
    myLastLog.setToNow();
    log();
    return;
  }
  
  if (myCyclesSincePacket > 2 && myPacketReceived.mSecSince() > 200)
  {
    myMissedPackets++;
    log();
    myRobot->comInt(ArCommands::ENCODER, 1); 
    myRobot->comInt(ArCommands::SAFETY_STATE_INFO, 1);
  }
  /*
  if (myPacketsThisCycle > 1)
  {
    myMissedMotorPackets++;
    log();
  }
  */
  if (myPacketsThisCycle == 0)
    myCyclesSincePacket++;

  if (myPacketsThisCycle > 0)
  {
    myRobot->comInt(ArCommands::ENCODER, 1);
    myRobot->comInt(ArCommands::SAFETY_STATE_INFO, 1);
  }

  if (myLastLog.secSince() >= 15)
  {
    log();
    myLastLog.setToNow();
  }

  myPacketsThisCycle = 0;
}

void ArServerSimpleConnectionTester::log(void)
{
  ArLog::log(ArLog::Normal, "ConnectionTester: Received %d packets, missed %d packets, mpacs %d",
	     myReceivedPackets, myMissedPackets, myRobot->getMotorPacCount());
	     
}



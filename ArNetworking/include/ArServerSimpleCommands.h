#ifndef ARSERVERSIMPLECOMMANDS_H
#define ARSERVERSIMPLECOMMANDS_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArServerHandlerCommands.h"
#include "ArServerHandlerPopup.h"

/// Class for sending commands to the microcontroller (uC)
/**
   This class will let you send commands to the microcontroller (com,
   comInt, and com2Bytes).
 **/
class ArServerSimpleComUC
{
public:
  /// Constructor
  AREXPORT ArServerSimpleComUC(ArServerHandlerCommands *handlerCommands, 
			       ArRobot *robot);
  /// Destructor
  AREXPORT virtual ~ArServerSimpleComUC();
  /// Handles the command
  AREXPORT void command(ArArgumentBuilder *arg);
  /// Handles the motor commands
  AREXPORT void motionCommand(ArArgumentBuilder *arg);
protected:
  void processCommand(ArArgumentBuilder *arg, bool motionCommand);
  ArRobot *myRobot;
  ArServerHandlerCommands *myHandlerCommands;
  ArFunctor1C<ArServerSimpleComUC, ArArgumentBuilder *> myCommandCB;
  ArFunctor1C<ArServerSimpleComUC, ArArgumentBuilder *> myMotionCommandCB;
};

/// Class for enabling or disabling logging of movement commands and data
/**
   This just calls ArRobot::setLogMovementSent and
   ArRobot::setLogLovementReceived.  It makes these available for easy
   enabling or disabling on the client side.
**/
class ArServerSimpleComMovementLogging
{
public:
    /// Constructor
  AREXPORT ArServerSimpleComMovementLogging(
	  ArServerHandlerCommands *handlerCommands, ArRobot *robot,
	  ArServerHandlerPopup *popupHandler = NULL);
  /// Destructor
  AREXPORT virtual ~ArServerSimpleComMovementLogging();
  /// Enable logging of movement commands sent to the robot 
  AREXPORT void logMovementSentEnable(void);
  /// Disable logging of movement commands sent to the robot
  AREXPORT void logMovementSentDisable(void);
  /// Enable logging of movement received from the robot
  AREXPORT void logMovementReceivedEnable(void);
  /// Disable logging of movement received from the robot
  AREXPORT void logMovementReceivedDisable(void);
  /// Enable logging of velocities received from the robot
  AREXPORT void logVelocitiesReceivedEnable(void);
  /// Disable logging of velocities received from the robot
  AREXPORT void logVelocitiesReceivedDisable(void);
  /// Enable tracking of packets from the robot
  AREXPORT void packetsReceivedTrackingEnable(void);
  /// Disable tracking of packets from the robot
  AREXPORT void packetsReceivedTrackingDisable(void);
  /// Enable tracking of packets from the robot
  AREXPORT void packetsSentTrackingEnable(void);
  /// Disable tracking of packets from the robot
  AREXPORT void packetsSentTrackingDisable(void);
  /// Enable logging of velocities received from the robot
  AREXPORT void logActionsEnable(void);
  /// Disable logging of velocities received from the robot
  AREXPORT void logActionsDisable(void);
  /// Log the status of the actions on the robot
  AREXPORT void logActions(void);
  /// Popups up the movement parameters
  AREXPORT void popupMovementParams(void);
  /// Resets the odometer
  AREXPORT void resetOdometer(void);
protected:
  ArRobot *myRobot;
  ArServerHandlerCommands *myHandlerCommands;
  ArServerHandlerPopup *myPopupHandler;

  ArFunctorC<ArServerSimpleComMovementLogging> myLogMovementSentEnableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogMovementSentDisableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogMovementReceivedEnableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogMovementReceivedDisableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogVelocitiesReceivedEnableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogVelocitiesReceivedDisableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myPacketsReceivedTrackingEnableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myPacketsReceivedTrackingDisableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myPacketsSentTrackingEnableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myPacketsSentTrackingDisableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogActionsEnableCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myLogActionsDisableCB;

  ArFunctorC<ArServerSimpleComMovementLogging> myLogActionsCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myPopupMovementParamsCB;
  ArFunctorC<ArServerSimpleComMovementLogging> myResetOdometerCB;
};

/// Class for enabling or disabling the gyro
/**
   This just calls ArAnalogGyro::activate and
   ArAnalogGyro::deactivate.  If you want to see the gyro data if you
   use the robot's movement received (either
   ArRobot::setLogMovementReceived or use
   ArServerSimpleComMovementLogging)
**/
class ArServerSimpleComGyro
{
public:
    /// Constructor
  AREXPORT ArServerSimpleComGyro(
	  ArServerHandlerCommands *handlerCommands, ArRobot *robot,
	  ArAnalogGyro *gyro);
  /// Destructor
  AREXPORT virtual ~ArServerSimpleComGyro();
  /// Enables the gyro
  AREXPORT void gyroEnable(void);
  /// Disables the gyro
  AREXPORT void gyroDisable(void);
protected:
  ArRobot *myRobot;
  ArAnalogGyro *myGyro;
  ArServerHandlerCommands *myHandlerCommands;
  ArFunctorC<ArServerSimpleComGyro> myGyroEnableCB;
  ArFunctorC<ArServerSimpleComGyro> myGyroDisableCB;
};


/// Request configuration from robot controller and log the result
class ArServerSimpleComLogRobotConfig 
{
public:
  AREXPORT ArServerSimpleComLogRobotConfig(
	  ArServerHandlerCommands *commands, ArRobot* robot, 	  
	  ArServerHandlerPopup *popupHandler = NULL);
  AREXPORT void logConfig(void); 
  AREXPORT void logMovementConfig(void); 
  AREXPORT void logOrigConfig(void); 
  AREXPORT void popupConfig(void); 
  AREXPORT void popupOrigConfig(void); 
  AREXPORT void popupMovementConfig(void); 
private:
  void configPacketArrived(void);
  ArServerHandlerCommands *myHandlerCommands;
  ArRobot* myRobot;
  ArServerHandlerPopup *myPopupHandler;

  bool myLogConfig;
  bool myPopupConfig;
  bool myLogConfigMovement;
  bool myPopupConfigMovement;


  ArFunctorC<ArServerSimpleComLogRobotConfig> myPacketArrivedCB;
  ArRobotConfigPacketReader myPacketReader;
  ArFunctorC<ArServerSimpleComLogRobotConfig> myLogConfigCB;
  ArFunctorC<ArServerSimpleComLogRobotConfig> myLogMovementConfigCB;
  ArFunctorC<ArServerSimpleComLogRobotConfig> myLogOrigConfigCB;
  ArFunctorC<ArServerSimpleComLogRobotConfig> myPopupConfigCB;
  ArFunctorC<ArServerSimpleComLogRobotConfig> myPopupOrigConfigCB;
  ArFunctorC<ArServerSimpleComLogRobotConfig> myPopupMovementConfigCB;
};

/// Log current ArRobot actions.
class ArServerSimpleComLogActions 
{
public:
  AREXPORT ArServerSimpleComLogActions(ArServerHandlerCommands *commands, ArRobot* robot);  
  AREXPORT void logActions(); 
private:
  ArRobot* myRobot;
  ArFunctorC<ArServerSimpleComLogActions> myCallback;
};

class ArServerSimpleServerCommands 
{ 
public:
  AREXPORT ArServerSimpleServerCommands(
	  ArServerHandlerCommands *commands, ArServerBase *server,
	  bool addLogConnections = true);
  AREXPORT virtual ~ArServerSimpleServerCommands();
  AREXPORT void logTerseTracking(void); 
  AREXPORT void logVerboseTracking(void); 
  AREXPORT void resetTracking(void);
  AREXPORT void logConnections(void); 
private:
  ArServerBase *myServer;
  ArFunctorC<ArServerSimpleServerCommands> myTerseTrackingCB;
  ArFunctorC<ArServerSimpleServerCommands> myVerboseTrackingCB;
  ArFunctorC<ArServerSimpleServerCommands> myResetTrackingCB;
  ArFunctorC<ArServerSimpleServerCommands> myLogConnectionsCB;

};

/// Class for sending popups out the server
/**
   This class will let you make a popup in MobileEyes, mostly for testing
 **/
class ArServerSimplePopup
{
public:
  /// Constructor
  AREXPORT ArServerSimplePopup(ArServerHandlerCommands *commands, 
			       ArServerHandlerPopup *popupHandler);
  /// Destructor
  AREXPORT virtual ~ArServerSimplePopup();
  /// Handles the command
  AREXPORT void simplePopup(ArArgumentBuilder *arg);
protected:
  ArServerHandlerCommands *myCommands;
  ArServerHandlerPopup *myPopupHandler;
  ArFunctor1C<ArServerSimplePopup, ArArgumentBuilder *> mySimplePopupCB;
};

/// Class that logs a special debug packet from the robot
class ArServerSimpleLogRobotDebugPackets
{
public:
  /// Constructor
  AREXPORT ArServerSimpleLogRobotDebugPackets(
	  ArServerHandlerCommands *commands, ArRobot *robot, 
	  const char *baseDirectory = "");
  /// Destructor
  AREXPORT virtual ~ArServerSimpleLogRobotDebugPackets();
  /// Starts the logging
  AREXPORT bool startLogging(
	  const char *fileName = "robotDebugPacketsLog.txt");
  /// Ends the logging
  AREXPORT bool stopLogging(void);
  /// Internal function that parses the packet
  AREXPORT bool packetHandler(ArRobotPacket *packet);
  /// Adds the robot debug info to the info group
  AREXPORT void addToInfoGroup(ArStringInfoGroup *infoGroup,
			       const char *name, int whichVal,
			       const char *format = "%d");
  /// Gets the number of values there are
  AREXPORT int getNumValues(void);
  /// Gets a particular value
  AREXPORT int getValue(int whichVal);
protected:
  ArRobot *myRobot;
  ArServerHandlerCommands *myCommands;
  // the file we write to, if its NULL we're not logging
  FILE *myFile;
  std::string myBaseDir;
  std::map<int, int> myVals;
  int myNumVals;
  ArRetFunctor1C<bool, ArServerSimpleLogRobotDebugPackets, 
      ArRobotPacket *> myPacketHandlerCB;
  ArRetFunctor1C<bool, ArServerSimpleLogRobotDebugPackets,
		 const char *> myStartLoggingCB;
  ArRetFunctorC<bool, ArServerSimpleLogRobotDebugPackets> myStopLoggingCB;
};


/// Class for sending commands to the microcontroller (uC)
/**
   This class will let you send commands to the microcontroller (com,
   comInt, and com2Bytes).
 **/
class ArServerSimpleConnectionTester
{
public:
  /// Constructor
  AREXPORT ArServerSimpleConnectionTester(
	  ArServerHandlerCommands *handlerCommands, ArRobot *robot);
  /// Destructor
  AREXPORT virtual ~ArServerSimpleConnectionTester();
  /// Handles the command
  AREXPORT void connectionTestStart(void);
  /// Handles the motor commands
  AREXPORT void connectionTestStop(void);
protected:
  ArRobot *myRobot;
  ArServerHandlerCommands *myHandlerCommands;

  bool packetHandler(ArRobotPacket *packet);
  void userTask(void);
  void log(void);

  bool myFirst;
  int myReceivedPackets;
  int myMissedPackets;
  int myMissedMotorPackets;
  int myPacketsThisCycle;
  int myCyclesSincePacket;
  ArTime myPacketReceived;
  ArTime myLastLog;

  ArFunctorC<ArServerSimpleConnectionTester> myConnectionTestStartCB;
  ArFunctorC<ArServerSimpleConnectionTester> myConnectionTestStopCB;
  ArRetFunctor1C<bool, ArServerSimpleConnectionTester, 
		 ArRobotPacket *> myPacketHandlerCB;
  ArFunctorC<ArServerSimpleConnectionTester> myUserTaskCB;
};


#endif 

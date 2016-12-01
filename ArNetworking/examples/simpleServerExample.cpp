#include "Aria.h"
#include "ArNetworking.h"


/** @example simpleServerExample.cpp This is a simple example of an ArNetworking server.
 * This server provides range sensor data to the client, and teleoperation
 * controls. It also includes an example of a custom command (using
 * ArServerHandlerCommands).
 *
 * For a more complete server, see serverDemo.cpp.
 */


/* This function is called ArServerHandlerCommands when our custom command is
 * recieved. */
void customCommandHandler(ArArgumentBuilder *args)
{
  if(args && args->getArg(0))
    ArLog::log(ArLog::Normal, "Recieved custom command with argument \"%s\".", args->getArg(0));
  else 
    ArLog::log(ArLog::Normal, "Recieved custom command with no arguments.");
}


int main(int argc, char **argv)
{
  Aria::init();
  ArRobot robot;
  ArArgumentParser parser(&argc, argv);
  ArSimpleConnector simpleConnector(&parser);

  // The base server object, manages all connections to clients.
  ArServerBase server;

  // This object simplifies configuration and opening of the ArServerBase
  // object.
  ArServerSimpleOpener simpleOpener(&parser);

  // parse the command line. fail and print the help if the parsing fails
  // or if the help was requested with -help
  parser.loadDefaultArguments();
  if (!simpleConnector.parseArgs() || !simpleOpener.parseArgs() || 
      !parser.checkHelpAndWarnUnparsed())
  {    
    simpleConnector.logOptions();
    simpleOpener.logOptions();
    exit(1);
  }

  // Use the ArSimpleOpener to open the server port
  if (!simpleOpener.open(&server))
  {
    ArLog::log(ArLog::Terse, "Error: Could not open server on port %d", simpleOpener.getPort());
    exit(1);
  }


  //
  // Create services attached to the base server:
  // 
  
  // Robot position etc.:
  ArServerInfoRobot serverInfoRobot(&server, &robot);

  // Robot control modes (only one mode can be active at once):
  ArServerModeStop modeStop(&server, &robot);    
  // old ArServerModeDrive modeDrive(&server, &robot); 
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);
  ArServerModeWander modeWander(&server, &robot);
  modeStop.addAsDefaultMode();
  modeStop.activate();

  // This provides a simple way to add new commands.
  ArServerHandlerCommands commands(&server);

  // Add our custom command. ArServerHandlerCommands also has other methods
  // for adding commands taht take different kinds of arguments, or no
  // arguments.
  ArGlobalFunctor1<ArArgumentBuilder*> customCommandFunctor(&customCommandHandler);
  commands.addStringCommand("ExampleCustomCommand", "Example of a custom command. simpleServerExample will print out the text sent with the command.", &customCommandFunctor);
 
  // These objects provide various debugging and diagnostic custom commands:
  ArServerSimpleComUC uCCommands(&commands, &robot);  // Get information about the robot
  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot);  // Control logging
  modeRatioDrive.addControlCommands(&commands);  // Drive mode diagnostics

  // This provides the client (e.g. MobileEyes) with a simple table of string values
  // (called an InfoGroup). An InfoGroup is kept globally by Aria.
  // The values in the table sent to clients are retrieved periodically by calling a 
  // functor.
  ArServerInfoStrings stringInfo(&server);
  Aria::getInfoGroup()->addAddStringCallback(stringInfo.getAddStringFunctor());

  // Here are some example entries in the InfoGroup:
  Aria::getInfoGroup()->addStringInt(
	  "Motor Packet Count", 10, 
	  new ArConstRetFunctorC<int, ArRobot>(&robot, 
					       &ArRobot::getMotorPacCount));

  //
  // Connect to the robot:
  // 
  
  if (!simpleConnector.connectRobot(&robot))
  {
    printf("Error: Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }


  robot.enableMotors();
  robot.runAsync(true);

  // The simple opener might have information to display right before starting 
  // the server thread:
  simpleOpener.checkAndLog();

  // now let the server base run in a new thread, accepting client connections.
  server.runAsync();

  ArLog::log(ArLog::Normal, "Server is now running... Press Ctrl-C to exit.");

  robot.waitForRunExit();
  Aria::shutdown();
  exit(0);  
}



/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2015 Adept Technology, Inc.
Copyright (C) 2016 Omron Adept Technologies, Inc.

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

#include "Aria.h"
#include "ArNetworking.h"

/** @example popupExample.cpp Shows how to create popup windows in a client like MobileEyes
 *
 * This example server program connects to a robot, and sends a message to clients (e.g. MobileEyes) to display
 * in a dialog box when a sensor reading is detected in front of the robot
 * within 1 meter. It also checks to see if that obstacle is not at the same
 * angle as the previous detected obstacle -- it's probably the same one,
 * unmoved -- to avoid sending repeated popups. The popup offers three choices,
 * acknowlege and do nothing, turn the robot around 180 degrees, or exit the server.
 */


class SensorDetectPopup
{
public:
  SensorDetectPopup(ArRobot *robot, ArServerHandlerPopup *popupServer);
protected:
  ArRobot *myRobot;
  ArServerHandlerPopup *myPopupServer;
  bool myPopupDisplayed;
  double myPrevObstacleAngle;
  bool myPrevObstacleAngleValid;
  ArFunctor2C<SensorDetectPopup, ArTypes::Byte4, int> *myPopupClosedCB;

  void popupClosed(ArTypes::Byte4 popupID, int button);
  void sensorTask(void)  ;
};


int main(int argc, char **argv)
{
  Aria::init();
  ArRobot robot;
  ArSonarDevice sonar;
  ArSick sick;
  robot.addRangeDevice(&sonar);
  ArServerBase server;

  // Argument parser:
  ArArgumentParser parser(&argc, argv);
  parser.loadDefaultArguments();

  // Connector and server opener:
  ArRobotConnector robotConnector(&parser, &robot);
  if(!robotConnector.connectRobot())
  {
    ArLog::log(ArLog::Terse, "popupExample: Could not connect to the robot.");
    if(parser.checkHelpAndWarnUnparsed())
    {
      Aria::logOptions();
    }
    Aria::exit(1);
  }

  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);

  ArServerSimpleOpener simpleOpener(&parser);

  // Get command-line and other parameters
  if(!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    Aria::exit(1);
  }

  robot.runAsync(true);

  // connect to the laser
  if(!laserConnector.connectLasers())
  {
    ArLog::log(ArLog::Normal, "popupExample: Warning: Could not connect to lasers.");
  }


  // Open the server
  if(!simpleOpener.open(&server))
  {
    ArLog::log(ArLog::Terse, "popupExample: Error, could not open server.");
    return 1;
  }
  server.runAsync();
  ArLog::log(ArLog::Normal, "popupExample: Server running. Press control-C to exit.");
  ArLog::log(ArLog::Normal, "popupExample: Each time an obstacle is detected near the robot, a new popup message will be created. Connect with MobileEyes to see them.");

  // Sends robot position etc.
  ArServerInfoRobot robotInfoServer(&server, &robot);

  // This service sends drawings e.g. showing range device positions
  ArServerInfoDrawings drawingsServer(&server);
  drawingsServer.addRobotsRangeDevices(&robot);

  // This service can send messages to clients to display as popup dialogs:
  ArServerHandlerPopup popupServer(&server);

  // This object contains the robot sensor interpretation task and creates
  // popups:
  SensorDetectPopup(&robot, &popupServer);

  // modes for controlling robot movement
  ArServerModeStop modeStop(&server, &robot);
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);  
  ArServerModeWander modeWander(&server, &robot);
  modeStop.addAsDefaultMode();
  modeStop.activate();

  // allow configuration of driving and other settings
  ArServerHandlerConfig serverHandlerConfig(&server, Aria::getConfig()); // make a config handler
  ArLog::addToConfig(Aria::getConfig()); // let people configure logging

  modeRatioDrive.addToConfig(Aria::getConfig(), "Teleop settings"); // able to configure teleop settings

  robot.enableMotors();
  robot.waitForRunExit();

  Aria::exit(0);
}

SensorDetectPopup::SensorDetectPopup(ArRobot *robot, ArServerHandlerPopup *popupServer) :
  myRobot(robot),
  myPopupServer(popupServer),
  myPopupDisplayed(false),
  myPrevObstacleAngleValid(false)
{
 myRobot->lock();
 myRobot->addSensorInterpTask("sensorDetectPopup", 50, new ArFunctorC<SensorDetectPopup>(this, &SensorDetectPopup::sensorTask));
 myPopupClosedCB = new ArFunctor2C<SensorDetectPopup, ArTypes::Byte4, int>(this, &SensorDetectPopup::popupClosed);
 myRobot->unlock();
}

void SensorDetectPopup::sensorTask(void)
{
  // Basic obstacle detection
  
  if (myPopupDisplayed) return;
  double detectAngle, detectRange;
  detectRange = myRobot->checkRangeDevicesCurrentPolar(-90, 90, &detectAngle);
  if (detectRange > 0 && detectRange <= 500)
  {
    if(myPrevObstacleAngleValid && fabs(detectAngle - myPrevObstacleAngle) < 0.0001)
      return;
    ArLog::log(ArLog::Normal, "popupExample: New obstacle detected at range %f, angle %f. Displaying popup dialog on client...", detectRange, detectAngle);

    ArServerHandlerPopupInfo info("popupExample", // ID
              "Object Detected",                  // Title
              "A range sensor detected a reading within 0.5 meters of the robot.", // Message
              ArServerHandlerPopup::INFORMATION,  // Type
              0,                                  // Default button
              0,                                  // Cancel/escape button
              5,                                 // Timeout (sec.)
              NULL,                               // Timeout String
              "OK", "Acknowleged.",               // Button 0 Label/Acknowlegement
              "Turn Around", "Requested rotate...",   // Button 1 Label/Acknowlegement
              "Shut Down", "Shutting down server..."  // Button 2 Label/Acknowlegement
             );
    int id = myPopupServer->createPopup(&info, myPopupClosedCB);
    ArLog::log(ArLog::Normal, "\t...Created a popup with ID=%d", id);
    myPopupDisplayed = true;
    myPrevObstacleAngle = detectAngle;
    myPrevObstacleAngleValid = true;
  }
}

void SensorDetectPopup::popupClosed(ArTypes::Byte4 popupID, int button)
{
  // A client closed the popup
  ArLog::log(ArLog::Normal, "popupExample: a client closed popup dialog window with id=%d. Button=%d...", popupID, button);
  myPopupDisplayed = false;

  if(button < 0)
  {
    ArLog::log(ArLog::Normal, "\t...popup timed out or closed due to an error.");
    return;
  }

  if (button == 0)
  {
    ArLog::log(ArLog::Normal, "\t...OK pressed.");
    return;
  }

  if(button == 1)
  {
    ArLog::log(ArLog::Normal, "\t...180 degree rotate requested.");
    myRobot->lock();
    myRobot->setDeltaHeading(180);
    myRobot->unlock();
    return;
  }

  if(button == 2)
  {
    ArLog::log(ArLog::Normal, "\t...exit requested.");
    myRobot->stopRunning();
    Aria::shutdown();
    Aria::exit(0);
  }
}



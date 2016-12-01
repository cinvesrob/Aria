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

class InputHandler
{
public:
  InputHandler(ArServerBase *server, ArRobot *robot);
  virtual ~InputHandler(void);
  void setVel(ArServerClient *client, ArNetPacket *packet);
  void deltaVel(ArServerClient *client, ArNetPacket *packet);
  void deltaHeading(ArServerClient *client, ArNetPacket *packet);
protected:
  ArActionInput myActionInput;
  ArRobot *myRobot;
  ArServerBase *myServer;
  ArFunctor2C<InputHandler, ArServerClient *, ArNetPacket *> mySetVelCB;
  ArFunctor2C<InputHandler, ArServerClient *, ArNetPacket *> myDeltaVelCB;
  ArFunctor2C<InputHandler, ArServerClient *, ArNetPacket *> myDeltaHeadingCB;
};

InputHandler::InputHandler(ArServerBase *server, ArRobot *robot) :
  mySetVelCB(this, &InputHandler::setVel),
  myDeltaVelCB(this, &InputHandler::deltaVel),
  myDeltaHeadingCB(this, &InputHandler::deltaHeading)
{
  myRobot = robot;
  myServer = server;
  myRobot->addAction(&myActionInput, 50);
  myServer->addData("setVel", "sets the velocity of the robot", &mySetVelCB,
		    "double: vel", "none");
  myServer->addData("deltaVel", "changes the velocity of the robot", 
		    &myDeltaVelCB, "double: deltaVel", "none");
  myServer->addData("deltaHeading", "changes the heading of the robot", 
		    &myDeltaHeadingCB, "double: deltaHeading", "none");
}

InputHandler::~InputHandler(void)
{
  myRobot->remAction(&myActionInput);
  /*myServer->remData("setVel");
  myServer->remData("deltaVel");
  myServer->remData("deltaHeading");*/
}

void InputHandler::setVel(ArServerClient *client, ArNetPacket *packet)
{
  double vel = packet->bufToDouble();
  //printf("Vel %g\n", vel);
  myActionInput.setVel(vel);
}
 
void InputHandler::deltaVel(ArServerClient *client, ArNetPacket *packet)
{
  double delta = packet->bufToDouble();
  //printf("DeltaVel %g\n", delta);
  //myActionInput.deltaVel(delta); // deltaVel has been removed from ArActionInput 
  myActionInput.setVel(myRobot->getVel() + delta);
}
 
void InputHandler::deltaHeading(ArServerClient *client, ArNetPacket *packet)
{
  double delta = packet->bufToDouble();
  //printf("DeltaHeading %g\n", delta);
  myActionInput.deltaHeadingFromCurrent(delta);
}

class OutputHandler
{
public:
  OutputHandler(ArServerBase *server, ArRobot *robot);
  virtual ~OutputHandler(void);
  void buildOutput(void);
  void output(ArServerClient *client, ArNetPacket *packet);

protected:
  ArServerBase *myServer;
  ArRobot *myRobot;
  ArMutex myPacketMutex;
  ArNetPacket myBuiltPacket;
  ArNetPacket mySendingPacket;
  ArFunctor2C<OutputHandler, ArServerClient *, ArNetPacket *> myOutputCB;
  ArFunctorC<OutputHandler> myTaskCB;
};

OutputHandler::OutputHandler(ArServerBase *server, ArRobot *robot) :
  myOutputCB(this, &OutputHandler::output),
  myTaskCB(this, &OutputHandler::buildOutput)
{
  myServer = server;
  myRobot = robot;
  myServer->addData("output", "gives the status of the robot", &myOutputCB,
		    "none", "byte4: x, byte4: y, byte2: th*10, byte2: vel, byte2: rotvel, byte2: battery*10");
  myRobot->addUserTask("output", 50, &myTaskCB);
}

OutputHandler::~OutputHandler(void)
{

}

void OutputHandler::buildOutput(void)
{
  myPacketMutex.lock();
  myBuiltPacket.empty();
  myBuiltPacket.byte4ToBuf(ArMath::roundInt(myRobot->getX()));
  myBuiltPacket.byte4ToBuf(ArMath::roundInt(myRobot->getY()));
  myBuiltPacket.byte2ToBuf(ArMath::roundInt(myRobot->getTh()));
  myBuiltPacket.byte2ToBuf(ArMath::roundInt(myRobot->getVel()));
  myBuiltPacket.byte2ToBuf(ArMath::roundInt(myRobot->getRotVel()));
  myBuiltPacket.byte2ToBuf(ArMath::roundInt(
	  myRobot->getBatteryVoltage() * 10));
  myPacketMutex.unlock();
}

void OutputHandler::output(ArServerClient *client, ArNetPacket *packet)
{
  myPacketMutex.lock();
  mySendingPacket.duplicatePacket(&myBuiltPacket);
  myPacketMutex.unlock();
  client->sendPacketTcp(&mySendingPacket);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;
  // the serial connection (robot)
  ArSerialConnection serConn;
  // tcp connection (sim)
  ArTcpConnection tcpConn;
  // robot
  ArRobot robot;

  // first open the server up
  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }
  // attach stuff to the server
  InputHandler inputHandler(&server, &robot);
  OutputHandler outputHandler(&server, &robot);
  // now let it spin off in its own thread
  server.runAsync();

  
  tcpConn.setPort();
  // see if we can get to the simulator  (true is success)
  if (tcpConn.openSimple())
  {
    // we could get to the sim, so set the robots device connection to the sim
    printf("Connecting to simulator through tcp.\n");
    robot.setDeviceConnection(&tcpConn);
  }
  else
  {
    // we couldn't get to the sim, so set the port on the serial
    // connection and then set the serial connection as the robots
    // device

    // modify the next line if you're not using the first serial port
    // to talk to your robot
    serConn.setPort();
    printf(
      "Could not connect to simulator, connecting to robot through serial.\n");
    robot.setDeviceConnection(&serConn);
  }

  // try to connect, if we fail exit
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }
  // enable the motors, disable amigobot sounds
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::BUMPSTALL, 0);
  
  // run the robot, true here so that the run will exit if connection lost
  robot.run(true);
  
  // now exit
  Aria::shutdown();



  return 0;
}



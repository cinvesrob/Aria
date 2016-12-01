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
  InputHandler(ArClientBase *client, ArKeyHandler *keyHandler);
  virtual ~InputHandler(void);
  void up(void);
  void down(void);
  void left(void);
  void right(void);
  void space(void);
protected:
  ArClientBase *myClient;
  ArKeyHandler *myKeyHandler;
  ArFunctorC<InputHandler> myUpCB;
  ArFunctorC<InputHandler> myDownCB;
  ArFunctorC<InputHandler> myLeftCB;
  ArFunctorC<InputHandler> myRightCB;
  ArFunctorC<InputHandler> mySpaceCB;
};

InputHandler::InputHandler(ArClientBase *client, ArKeyHandler *keyHandler) : 
  myUpCB(this, &InputHandler::up),
  myDownCB(this, &InputHandler::down),
  myLeftCB(this, &InputHandler::left),
  myRightCB(this, &InputHandler::right),
  mySpaceCB(this, &InputHandler::space)
{
  myClient = client;
  myKeyHandler = keyHandler;
  myKeyHandler->addKeyHandler(ArKeyHandler::UP, &myUpCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::DOWN, &myDownCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::LEFT, &myLeftCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::RIGHT, &myRightCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::SPACE, &mySpaceCB);
}

InputHandler::~InputHandler(void)
{

}

void InputHandler::up(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(25);
  myClient->requestOnce("deltaVel", &packet);
}

void InputHandler::down(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(-25);
  myClient->requestOnce("deltaVel", &packet);
}

void InputHandler::left(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(3.2);
  myClient->requestOnce("deltaHeading", &packet);
}

void InputHandler::right(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(-3.2);
  myClient->requestOnce("deltaHeading", &packet);
}

void InputHandler::space(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(0.00001);
  myClient->requestOnce("setVel", &packet);
  myClient->requestOnce("deltaHeading", &packet);
}

class OutputHandler
{
public:
  OutputHandler(ArClientBase *client);
  virtual ~OutputHandler(void);
  void handleOutput(ArNetPacket *packet);
protected:
  double myX;
  double myY;
  double myTh;
  double myVel;
  double myRotVel;
  double myVoltage;
  ArClientBase *myClient;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleOutputCB;

};

OutputHandler::OutputHandler(ArClientBase *client) :
  myHandleOutputCB(this, &OutputHandler::handleOutput)
{
  myClient = client;
  myClient->addHandler("output", &myHandleOutputCB);
  myClient->request("output", 100);
}

OutputHandler::~OutputHandler(void)
{
  myClient->requestStop("output");
}

void OutputHandler::handleOutput(ArNetPacket *packet)
{
  myX = packet->bufToByte4();
  myY = packet->bufToByte4();
  myTh = packet->bufToByte2()/10.0;
  myVel = packet->bufToByte2();
  myRotVel = packet->bufToByte2();
  myVoltage = packet->bufToByte2()/10.0;
  printf("\rx: %6.0f y: %6.0f th: %5.1f vel: %4.0f rotVel: %3.0f voltage: %4.1f",
	 myX, myY, myTh, myVel, myRotVel, myVoltage);
  fflush(stdout);
}


void escape(void)
{
  Aria::shutdown();
}

int main(int argc, char **argv)
{
  char* host = "localhost";
  if(argc > 1)
    host = argv[1];
  Aria::init();
  ArClientBase client;
  ArGlobalFunctor escapeCB(&escape);
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);


  printf("Connecting to standaloneServerDemo at %s:%d...\n", host, 7272);
  if (!client.blockingConnect(host, 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  } 
  InputHandler inputHandler(&client, &keyHandler);
  OutputHandler outputHandler(&client);
  keyHandler.addKeyHandler(ArKeyHandler::ESCAPE, &escapeCB);
  client.runAsync();
  while (client.getRunningWithLock())
  {
    keyHandler.checkKeys();
    ArUtil::sleep(1);
  }
  keyHandler.restore();
  Aria::shutdown();
  return 0;
}

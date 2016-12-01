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
#include "ArExport.h"
#include <stdio.h>
#include "ArClientRatioDrive.h"
#include "ArClientBase.h"



AREXPORT ArClientRatioDrive::ArClientRatioDrive(ArClientBase *client) : 
  myClient(client), myPrinting(false), myTransRatio(0.0), myRotRatio(0.0), myLatRatio(0.0), myThrottle(100.0),
  myStop(true), myCycleCB(this, &ArClientRatioDrive::task)
{
  if(myPrinting)
    puts("ArClientRatioDrive: adding client cycle callback");
  myClient->addCycleCallback(&myCycleCB);
}

AREXPORT ArClientRatioDrive::~ArClientRatioDrive() 
{
  if(myPrinting)
    puts("ArClientRatioDrive: destructor");
  stop();
  if(myClient)
    myClient->remCycleCallback(&myCycleCB);
}

AREXPORT void ArClientRatioDrive::setTransVelRatio(double r)
{
  if(myPrinting) printf("ArClientRatioDrive: set trans ratio %f\n", r);
  myTransRatio = r;
  myStop = false;
}

AREXPORT void ArClientRatioDrive::setRotVelRatio(double r)
{
  if(myPrinting) printf("ArClientRatioDrive: set rot ratio %f\n", r);
  myRotRatio = r;
  myStop = false;
}

AREXPORT void ArClientRatioDrive::setLatVelRatio(double r)
{
  if(myPrinting) printf("ArClientRatioDrive: set lat ratio %f\n", r);
  myLatRatio = r;
  myStop = false;
}


AREXPORT void ArClientRatioDrive::safeDrive()
{
  ArNetPacket p;
  p.byteToBuf(1);
  if(myPrinting) printf("ArClientRatioDrive: Sending setSafeDrive 1.\n");
  myClient->requestOnce("setSafeDrive",&p);
  if(myPrinting) printf("\nArClientRatioDrive: Sent enable safe drive.\n");
}

AREXPORT void ArClientRatioDrive::unsafeDrive()
{
  ArNetPacket p;
  p.byteToBuf(0);
  if(myPrinting) printf("ArClientRatioDrive: Sending setSafeDrive 0.\n");
  myClient->requestOnce("setSafeDrive",&p);
  if(myPrinting) printf("\nArClientRatioDrive: Sent disable safe drive command. Your robot WILL run over things if you're not careful.\n");
}


void ArClientRatioDrive::sendInput(void)
{
  /* This method is called by the main function to send a ratioDrive
   * request with our current velocity values. If the server does 
   * not support the ratioDrive request, then we abort now: */
  //if(!myClient->dataExists("ratioDrive")) return;

  /* Construct a ratioDrive request packet.  It consists
   * of three doubles: translation ratio, rotation ratio, and an overall scaling
   * factor. */
  ArNetPacket packet;
  packet.doubleToBuf(myTransRatio);
  packet.doubleToBuf(myRotRatio);
  packet.doubleToBuf(myThrottle); // this is an additional amount (percentage) that is applied to each of the trans,rot,lat velocities. 
  packet.doubleToBuf(myLatRatio);
//  if (myPrinting) printf("ArClientRatioDrive: Sending ratioDrive request\n");
  myClient->requestOnce("ratioDrive", &packet);
}

void ArClientRatioDrive::stop()
{
  if(myPrinting) puts("ArClientRatioDrive: Stopping...");
  myTransRatio = 0;
  myRotRatio = 0;
  myLatRatio = 0;
  myStop = true;
  myClient->requestOnce("stop", NULL);
  if(myPrinting) puts("ArClientRatioDrive: Sent stop request.");
}

void ArClientRatioDrive::task()
{
  //if(myPrinting) puts("ArClientRatioDrive: cycle callback");
  if(!myStop)
    sendInput();
}


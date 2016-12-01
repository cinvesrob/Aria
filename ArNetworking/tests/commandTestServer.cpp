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

void function1(void)
{
  printf("Function1 called\n");
}

void function2(void)
{
  printf("Function2 called\n");
}

void function3(void)
{
  printf("Function3 called\n");
}

void function4(ArArgumentBuilder *arg)
{
  printf("Function4 called with string '%s'\n", arg->getFullString());
}

void function5(ArArgumentBuilder *arg)
{
  printf("Function5 called with string '%s'\n", arg->getFullString());
}

void function6(ArArgumentBuilder *arg)
{
  printf("Function6 called with string '%s'\n", arg->getFullString());
}


int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;

  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  ArServerHandlerCommands commands(&server);
  commands.addCommand("Function1", "Call the function that is number 1!", 
			    new ArGlobalFunctor(&function1));
  commands.addCommand("Function2", "Second function to call", 
			    new ArGlobalFunctor(&function2));
  commands.addCommand("Function3", "Tree climb", 
			    new ArGlobalFunctor(&function3));
  commands.addStringCommand("StringFunction4", 
			"Print a string in function 4", 
			    new ArGlobalFunctor1<
			    ArArgumentBuilder *>(&function4));
  commands.addStringCommand("StringFunction5", 
			"Prints a string given (5)", 
			    new ArGlobalFunctor1<
			    ArArgumentBuilder *>(&function5));
  commands.addStringCommand("StringFunction6", 
				  "Print out a value for function 6", 
			    new ArGlobalFunctor1<
			    ArArgumentBuilder *>(&function6));

  server.run();
}

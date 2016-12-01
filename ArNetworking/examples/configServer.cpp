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

int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;
  
  ArConfig *config;
  config = Aria::getConfig();
  std::string section;
  char joy[512];
  sprintf(joy, "Joy");
  section = "section1";
  config->addParam(ArConfigArg("int", new int, "fun int", 0), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("double", new double, "fun double", 0, 1), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("bool", new bool, "fun bool"), section.c_str(), ArPriority::IMPORTANT);
  config->addParam(ArConfigArg("string", joy, "fun string", sizeof(joy)), section.c_str(), ArPriority::TRIVIAL);
  section = "section8";
  config->addParam(ArConfigArg("int", new int, "fun int", 0), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("double", new double, "fun double", 0, 1), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("doubleFOUR", (double).4, "fun double", 0.0, 1.0), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("double three", new double, "fun double", 0, 1), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("double two", new double, "fun double", 0, 1), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("double one", new double, "fun double", 0, 1), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("double", new double, "fun double", 0, 1), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("bool", new bool, "fun bool"), section.c_str(), ArPriority::IMPORTANT);
  config->addParam(ArConfigArg("string", joy, "fun string", sizeof(joy)), section.c_str(), ArPriority::TRIVIAL);
  section = "some section";
  config->setSectionComment("some section", "this is a random section with 4 ints");
  config->addParam(ArConfigArg("int1", new int, "fun int"), section.c_str(), ArPriority::TRIVIAL);
  config->addParam(ArConfigArg("int2", new int, "fun int", -1, 1200), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("int3", new int, "fun int"), section.c_str(), ArPriority::IMPORTANT);
  config->addParam(ArConfigArg("int4", new int, "fun int"), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("int4", new int, "fun int"), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("int1", new int, "fun int"), section.c_str(), ArPriority::TRIVIAL);
  section = "another section";
  config->setSectionComment("another section", "this is another section with 1 of each type");
  config->addParam(ArConfigArg("inta", new int, "fun int"), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("doublea", new double, "fun double"), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("boola", new bool, "fun bool"), section.c_str(), ArPriority::NORMAL);
  config->addParam(ArConfigArg("stringa", new char[512], "fun string", 512), section.c_str(), ArPriority::NORMAL);

  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }
  config->setBaseDirectory("./");
  config->writeFile("default.txt");
  config->parseFile("modified.txt");
  config->writeFile("modifiedModified.txt");
  ArServerHandlerConfig configHandler(&server, Aria::getConfig(), 
				      "default.txt");
  
  server.run();
  
  Aria::shutdown();

  return 0;
}

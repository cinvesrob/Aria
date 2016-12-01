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

/** @example getVideoExample.cpp
 *
 *  This example client requests video images from the server
 *  and saves them repeatedly to a file named "video.jpg", or
 *  to a series of files ("video1.jpg", "video2.jpg", etc.) if
 *  you give the -counter option. Give the image request rate
 *  in images per second with the -rate option (or use -1 to let
 *  the server decide when to send image; may not work with all servers).
 *
 *  Connect to SAVserver, ACTS, or another video server to get
 *  images. 
 *
 *  To make a movie from the images, you can use ffmpeg on Linux.
 *  First run with frame rate and counter options to save multiple images:
 *    ./getVideoExample -host robothost -port 7272 -rate 20 -counter
 *  Then use ffmpeg:
 *    ffmpeg -i video%d.jpeg -r 20 movie.mpeg
 *  See ffmpeg -h for full list of options.
 */


bool useCounter = false;
unsigned long counter = 1;

double rate = 0;
int rateAsTime = 0;

void jpegHandler(ArNetPacket *packet)
{
  unsigned int width;
  unsigned int height;
  static unsigned char jpeg[50000];
  int jpegSize;
  FILE *file;

  width = packet->bufToUByte2();
  height = packet->bufToUByte2();
  jpegSize = packet->getDataLength() - packet->getDataReadLength();
  if(jpegSize > 50000)
  {
    ArLog::log(ArLog::Normal, "Cannot save image, it's too big. (image is %d bytes, my buffer is 50000 bytes)", jpegSize);
    return;
  }
  packet->bufToData((char *)jpeg, jpegSize);
  char filename[128];
  char tmpFilename[128];
  sprintf(tmpFilename, "tmp.jpg");
  if ((file = ArUtil::fopen(tmpFilename, "wb")) != NULL)
  {
    fwrite(jpeg, jpegSize, 1, file);
    fclose(file);
    if(useCounter)
      snprintf(filename, 64, "video%lu.jpg", counter++);
    else
      strcpy(filename, "video.jpg");
#ifdef WIN32
    // On windows, rename() fails if the new file already exists
    unlink(filename);
#endif
    if (rename(tmpFilename, filename) == 0)
      ArLog::log(ArLog::Normal, "Wrote a %dx%d image, %d bytes, named %s.", width, height, jpegSize, filename);
    else
      ArLog::log(ArLog::Normal, "Wrote a %dx%d image, %d bytes, named %s (could not rename to real name).", width, height, jpegSize, tmpFilename);
  }
  else
    ArLog::log(ArLog::Normal, "Could not write temp file %s", tmpFilename);

  if (rate == 0 || rateAsTime == 0)
  {
    ArLog::log(ArLog::Normal, "Only one frame was requested, so exiting");
    Aria::exit(0);
  }
}

int main(int argc, char **argv)
{

#ifndef WIN32
  ArDaemonizer daemonizer(&argc, argv, false);
  bool isDaemonized = false;
  if (!daemonizer.daemonize())
  {
    ArLog::log(ArLog::Terse, "Could not daemonize process");
    exit(1);
  }
  if (daemonizer.isDaemonized())
    isDaemonized = true;
#endif

  Aria::init();
  ArLog::init(ArLog::File, ArLog::Normal, "getVideoLog.txt", true, true, true);
  
  ArArgumentParser argParser(&argc, argv);
  argParser.loadDefaultArguments();

  ArClientSimpleConnector clientConnector(&argParser);

  if(argParser.checkParameterArgumentDouble("-rate", &rate) && rate != 0.0)
  {
    if(rate == -1)
      rateAsTime = -1;
    else
      rateAsTime = ArMath::roundInt(1000.0 / rate);
  }

  useCounter = argParser.checkArgument("-counter");

  if(!Aria::parseArgs() || !argParser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    ArLog::log(ArLog::Terse, "\n\n%s options:\n-rate <FramesPerSecondAsDouble> (If this isn't given, then the program will take one frame then exit, note that it is a double (so you can do .5 to do one frame per 2 seconds) and that if you set it to be too fast you'll saturate the robot's bandwidth and make it useless)\n-counter (default no)\n", argv[0]);

    ArLog::log(ArLog::Terse, "\n\nNotes:\nThis program saves the images as video.jpg if you aren't using a counter, or video<counter>.jpg if you are using the counter.\nIt puts its logs into getVideoLog.txt, and overwrites it whenever it runs\n");
    
    return 1;
  }
  


  ArClientBase client;
  if (!clientConnector.connectClient(&client))
  {
    ArLog::log(ArLog::Normal, "Could not connect to server, exiting\n");
    exit(1);
  }    

  ArGlobalFunctor1<ArNetPacket *> jpegHandlerCB(&jpegHandler);

  if(client.dataExists("getPictureCam1"))
  {
    ArLog::log(ArLog::Normal, "Requesting images using \"getPictureCam1\" request.");
    client.addHandler("getPictureCam1", &jpegHandlerCB);
    if (rate != 0 && rateAsTime != 0)
      client.request("getPictureCam1", rateAsTime); 
    else
      client.requestOnce("getPictureCam1");
  } 
  else if(client.dataExists("sendVideo"))
  {
    ArLog::log(ArLog::Normal, "Server does not have \"getPictureCam1\" request, requesting images using old \"sendVideo\" request.");
    client.addHandler("sendVideo", &jpegHandlerCB);
    if (rate != 0 && rateAsTime != 0)
      client.request("sendVideo", rateAsTime); 
    else
      client.requestOnce("sendVideo"); 
  }
  else
  {
    ArLog::log(ArLog::Terse, "Error: Server does not have \"getPictureCam1\" or \"sendVideo\" request, cannot request images.");
    Aria::exit(2);
  }


  client.run();

  Aria::shutdown();
  return 0;
}




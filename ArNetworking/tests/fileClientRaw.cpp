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

ArClientBase client;

void getDirListing(ArNetPacket *packet)
{
  char name[2048];
  char directory[2048];
  time_t atime;
  time_t mtime;
  unsigned int num;
  unsigned int ret;
  unsigned int i;

  ret = packet->bufToUByte2();
  if (ret != 0)
  {
    printf("Bad return %d\n", ret);
    exit(1);
  }
  packet->bufToStr(directory, sizeof(directory));
  printf("In directory '%s'\n", directory);
  num = packet->bufToUByte2();
  printf("%d directories\n", num);
  for (i = 0; i < num; i++)
  {
    packet->bufToStr(name, sizeof(name));
    atime = packet->bufToUByte4();
    mtime = packet->bufToUByte4();
    printf("\t%s\n", name);
    printf("\t\t%s", ctime(&atime));
    printf("\t\t%s", ctime(&mtime));
  }
  num = packet->bufToUByte2();
  printf("%d files\n", num);
  for (i = 0; i < num; i++)
  {
    packet->bufToStr(name, sizeof(name));
    atime = packet->bufToUByte4();
    mtime = packet->bufToUByte4();
    printf("\t%s\n", name);
    printf("\t\t%s", ctime(&atime));
    printf("\t\t%s", ctime(&mtime));
  }
  exit(0);
}

FILE *file = NULL;

void netGetFile(ArNetPacket *packet)
{
  int ret;
  char fileName[2048];
  ret = packet->bufToUByte2();
  packet->bufToStr(fileName, sizeof(fileName));
  if (ret != 0)
  {
    printf("Bad return %d on file %s\n", ret, fileName);
    exit(1);
  }
  if (file == NULL)
  {
    printf("Getting file %s\n", fileName);
    if ((file = ArUtil::fopen("fileClientRaw.dump", "w")) == NULL)
    {
      printf("Can't open fileClientRaw.dump to dump file into\n");
      exit(2);
    }
  }
  ArTypes::UByte4 numBytes;
  char buf[32000];
  // file should be good here, so just write into it
  numBytes = packet->bufToUByte4();
  if (numBytes == 0)
  {
    printf("Got all of file %s\n", fileName);
    fclose(file);
    exit(0);
  }
  else
  {
    printf("Got %d bytes of file %s\n", numBytes, fileName);
    packet->bufToData(buf, numBytes);
    fwrite(buf, 1, numBytes, file);
  }
}

void netPutFile(ArNetPacket *packet)
{
  int ret;
  char fileName[2048];

  ret = packet->bufToUByte2();
  packet->bufToStr(fileName, sizeof(fileName));
  
  printf("Ret of '%s' is %d\n", fileName, ret);
}

void putFile(char *fileName, char *asFileName)
{
  ArNetPacket sendPacket;
  size_t ui;
  size_t len;

  FILE *file;
  if ((file = ArUtil::fopen(fileName, "r")) == NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "putFile: can't open file '%s'", fileName);
    return;
  }

  // tell the server we're sending
  
  sendPacket.empty();
  sendPacket.uByte2ToBuf(0);
  sendPacket.strToBuf(asFileName);
  client.requestOnce("putFile", &sendPacket);
  printf("Starting send of file %s\n", fileName);
  
  char buf[30000];
  size_t ret;
  // now send the file
  while ((ret = fread(buf, 1, sizeof(buf), file)) == sizeof(buf))
  {
    sendPacket.empty();
    sendPacket.uByte2ToBuf(1);
    sendPacket.strToBuf(asFileName);
    sendPacket.uByte4ToBuf(ret);
    sendPacket.dataToBuf(buf, ret);
    client.requestOnce("putFile", &sendPacket);
    printf("Sent packet with %d\n", ret);
  }
  if (ferror(file))
  {
    ArLog::log(ArLog::Normal, "ArServerFileToClient: Error sending file %s", 
	       fileName);
    sendPacket.empty();
    sendPacket.uByte2ToBuf(3);
    sendPacket.strToBuf(asFileName);
    client.requestOnce("putFile", &sendPacket);
    return;
  }

  sendPacket.empty();
  sendPacket.uByte2ToBuf(1);
  sendPacket.strToBuf(asFileName);
  sendPacket.uByte4ToBuf(ret);
  sendPacket.dataToBuf(buf, ret);
  client.requestOnce("putFile", &sendPacket);
  printf("Sent packet with %d\n", ret);


  sendPacket.empty();
  sendPacket.uByte2ToBuf(2);
  sendPacket.strToBuf(asFileName);
  client.requestOnce("putFile", &sendPacket);

  if (feof(file))
  {
    ArLog::log(ArLog::Normal, "ArServerFileToClient: Sent file %s", fileName);
  }
  
  fclose(file);
}

void netDeleteFile(ArNetPacket *packet)
{
  int ret;
  char fileName[2048];

  ret = packet->bufToUByte2();
  packet->bufToStr(fileName, sizeof(fileName));
  
  printf("Delete file ret of '%s' is %d\n", fileName, ret);
}

int main(int argc, char **argv)
{

  std::string hostname;
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  if (argc == 1)
    hostname = "localhost";
  else
    hostname = argv[1];
  if (!client.blockingConnect(hostname.c_str(), 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  } 

  
  /*
  ArGlobalFunctor1<ArNetPacket *> getDirListingCB(&getDirListing);
  client.addHandler("getDirListing", &getDirListingCB);
  //client.requestOnceWithString("getDirListing", "");

  ArGlobalFunctor1<ArNetPacket *> getFileCB(&netGetFile);
  client.addHandler("getFile", &getFileCB);
  client.requestOnceWithString("getFile", "all.bob");

  ArGlobalFunctor1<ArNetPacket *> netPutFileCB(&netPutFile);
  client.addHandler("putFile", &netPutFileCB);
  putFile("doxygen.conf", "ArGH/DoxYGEN");
  */

  ArGlobalFunctor1<ArNetPacket *> deleteFileCB(&netDeleteFile);
  client.addHandler("deleteFile", &deleteFileCB);
  client.requestOnceWithString("deleteFile", "1/all.bob");

  client.runAsync();

  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
  }
  Aria::shutdown();
  return 0;

}





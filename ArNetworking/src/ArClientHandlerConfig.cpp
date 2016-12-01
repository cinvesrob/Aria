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
#include "ArClientHandlerConfig.h"
#include "ArClientArgUtils.h"

#define ARDEBUG_CLIENTHANDLERCONFIG

#if (defined(_DEBUG) && defined(ARDEBUG_CLIENTHANDLERCONFIG))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

/**
   @param client the client base to attach to
   @param ignoreBounds whether the ArConfig we have should ignore bounds or not, this should only be used for debugging
   @param robotName a name or identifier for the robot the server is controlling, used for logging etc.
*/
AREXPORT ArClientHandlerConfig::ArClientHandlerConfig(ArClientBase *client,
						                                          bool ignoreBounds,
                                                      const char *robotName,
                                                      const char *logPrefix) :
  myRobotName((robotName != NULL) ? robotName : ""),
  myLogPrefix((logPrefix != NULL) ? logPrefix : ""),

  myGotConfigCBList(),
  mySaveConfigSucceededCBList(),
  mySaveConfigFailedCBList(),
  myGotConfigDefaultsCBList(),
  myGotLastEditablePriorityCBList(),

  myClient(client),

  myConfig(NULL, 
           false, 
           ignoreBounds),
  myDefaultConfig(NULL),
  myLastEditablePriority(ArPriority::LAST_PRIORITY),

  myDataMutex(),
  myCallbackMutex(),

  myHaveRequestedLastEditablePriority(false),
  myHaveGottenLastEditablePriority(false),
  myHaveRequestedConfig(false),
  myHaveGottenConfig(false),
  myHaveRequestedDefaults(false),
  myHaveGottenDefaults(false),
  myHaveRequestedDefaultCopy(false),

  myIsQuiet(false),

  myHandleGetConfigBySectionsV4CB
                     (this, &ArClientHandlerConfig::handleGetConfigBySectionsV4),
  myHandleGetConfigBySectionsV3CB
                     (this, &ArClientHandlerConfig::handleGetConfigBySectionsV3),
  myHandleGetConfigBySectionsV2CB
                     (this, &ArClientHandlerConfig::handleGetConfigBySectionsV2),
  myHandleGetConfigBySectionsCB
                     (this, &ArClientHandlerConfig::handleGetConfigBySections),
  myHandleGetConfigCB(this, &ArClientHandlerConfig::handleGetConfig),
  myHandleSetConfigCB(this, &ArClientHandlerConfig::handleSetConfig),
  myHandleSetConfigBySectionsCB(this, &ArClientHandlerConfig::handleSetConfigBySections),
  myHandleSetConfigBySectionsV2CB(this, &ArClientHandlerConfig::handleSetConfigBySectionsV2),
  myHandleGetConfigDefaultsCB
                     (this, &ArClientHandlerConfig::handleGetConfigDefaults),
  myHandleGetConfigSectionFlagsCB
                     (this, &ArClientHandlerConfig::handleGetConfigSectionFlags),
  myHandleGetLastEditablePriorityCB
                     (this, &ArClientHandlerConfig::handleGetLastEditablePriority)

{
  myDataMutex.setLogName("ArClientConfigHandler::myDataMutex");
  myCallbackMutex.setLogName("ArClientConfigHandler::myCallbackMutex");

  if ((logPrefix == NULL) && !myRobotName.empty()) {
    myLogPrefix = myRobotName + ": ";
  }

  myConfig.setConfigName("Server", myRobotName.c_str());

}

AREXPORT ArClientHandlerConfig::~ArClientHandlerConfig()
{
  if (myDefaultConfig != NULL)
    delete myDefaultConfig;
}

/**
   This requests the config from the server.  The handler's internal state is
   reset to indicate that the config hasn't been received.

   If the server supports editable priority levels and the client wishes to
   receive parameters of ineditable priorities, then the method 
   requestLastEditablePriority should be called (and completed) before 
   requestConfigFromServer is called.
 **/
AREXPORT void ArClientHandlerConfig::requestConfigFromServer(void)
{
  IFDEBUG(ArLog::log(ArLog::Normal,
                     "ArClientHandlerConfig::requestConfigFromServer()"));

  char *getConfigPacketName = "getConfigBySectionsV4";
  bool isInsertPriority = true;
  bool isInsertRestartLevel = true;

  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> *getConfigCB = &myHandleGetConfigBySectionsV4CB;
  
  if (!myClient->dataExists(getConfigPacketName)) {
      
    getConfigPacketName = "getConfigBySectionsV3";
    getConfigCB = &myHandleGetConfigBySectionsV3CB;

    if (!myClient->dataExists(getConfigPacketName)) {

      getConfigPacketName = "getConfigBySectionsV2";
      getConfigCB = &myHandleGetConfigBySectionsV2CB;

      if (!myClient->dataExists(getConfigPacketName)) {

        getConfigPacketName = "getConfigBySections";
        isInsertRestartLevel = false;

        getConfigCB = &myHandleGetConfigBySectionsCB;

        if (!myClient->dataExists(getConfigPacketName)) {
          getConfigPacketName = "getConfig";
          isInsertPriority = false;

          getConfigCB = &myHandleGetConfigCB;

        } // end if packet name does not exist
      } // end if  packet name V2 does not exist
    } // end if packet name V3 does not exist
  } // end if packet name V4 does not exist

  char *setConfigPacketName = "setConfigBySectionsV2";
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> *setConfigCB = &myHandleSetConfigBySectionsV2CB;
  
  if (!myClient->dataExists(setConfigPacketName)) {
 
    setConfigPacketName = "setConfigBySections";

    setConfigCB = &myHandleSetConfigBySectionsCB;

    if (!myClient->dataExists(setConfigPacketName)) {
 
      setConfigPacketName = "setConfig";

      setConfigCB = &myHandleSetConfigCB;

    } // end if packet name does not exist

  } // end if packet name does not exist

  myDataMutex.lock();
  ArLog::log(ArLog::Normal, "%sRequesting config from server (with %s, save with %s)", 
	     myLogPrefix.c_str(), getConfigPacketName, setConfigPacketName);
  myConfig.clearSections();
  myDataMutex.unlock();

  myClient->remHandler(getConfigPacketName, getConfigCB);
  myClient->addHandler(getConfigPacketName, getConfigCB);

  myClient->remHandler(setConfigPacketName, setConfigCB);
  myClient->addHandler(setConfigPacketName, setConfigCB);

  if (myClient->dataExists("getConfigDefaults")) {
    myClient->remHandler("getConfigDefaults", &myHandleGetConfigDefaultsCB);
    myClient->addHandler("getConfigDefaults", &myHandleGetConfigDefaultsCB);
  }

  if (myClient->dataExists("getConfigSectionFlags"))
  {
    myClient->remHandler("getConfigSectionFlags", 
			                   &myHandleGetConfigSectionFlagsCB);
    myClient->addHandler("getConfigSectionFlags", 
			                   &myHandleGetConfigSectionFlagsCB);
    myClient->requestOnce("getConfigSectionFlags");
  }

  bool isInsertLastEditablePriority = true;
  if (!myClient->dataExists("getLastEditablePriority")) {
    isInsertLastEditablePriority = false;
  }
  else if (!myHaveGottenLastEditablePriority) {
    isInsertLastEditablePriority = false;
    ArLog::log(ArLog::Terse,
               "%sConfig requested but last editable priority not yet received, using old protocol",
               myLogPrefix.c_str());
  } // end else if last editable priority not received


  if (isInsertPriority) {
    ArLog::log(ArLog::Verbose,
               "%sRequesting that config has last priority %s",
               myLogPrefix.c_str(),
               ArPriority::getPriorityName(ArPriority::LAST_PRIORITY));

    ArNetPacket packet;
    packet.byteToBuf(ArPriority::LAST_PRIORITY);

    if (isInsertLastEditablePriority) {
      ArLog::log(ArLog::Verbose,
                 "%sRequesting that config has last editable priority %s",
                 myLogPrefix.c_str(),
                 ArPriority::getPriorityName(myLastEditablePriority));
      packet.byteToBuf(myLastEditablePriority);
    }

    myClient->requestOnce(getConfigPacketName, &packet);
  }
  else { // don't insert priority
    myClient->requestOnce(getConfigPacketName);
  } // end else don't insert priority

  myDataMutex.lock();
  myHaveGottenConfig = false;  
  myDataMutex.unlock();
}


void ArClientHandlerConfig::handleGetConfigBySectionsV4
                                                        (ArNetPacket *packet)
{
  handleGetConfigData(packet, true, 4);
}


void ArClientHandlerConfig::handleGetConfigBySectionsV3(ArNetPacket *packet)
{
  handleGetConfigData(packet, true, 3);
}

void ArClientHandlerConfig::handleGetConfigBySectionsV2(ArNetPacket *packet)
{
  handleGetConfigData(packet, true, 2);
}

void ArClientHandlerConfig::handleGetConfigBySections(ArNetPacket *packet)
{
  handleGetConfigData(packet, true, 1);
}

void ArClientHandlerConfig::handleGetConfig(ArNetPacket *packet)
{
  handleGetConfigData(packet, false, 0);
}

void ArClientHandlerConfig::handleGetConfigData(ArNetPacket *packet,
                                                         bool isMultiplePackets,
                                                         int version)
{
  char name[32000];
  char comment[32000];
  char categoryName[512]; 

  char type;
  std::string section;
  
  // The multiple packets method also sends display hints with the parameters;
  // the old single packet method does not.
  ArClientArg clientArg(isMultiplePackets, 
                        ArPriority::LAST_PRIORITY,
                        version);
  bool isEmptyPacket = true;

  myDataMutex.lock();

  while (packet->getDataReadLength() < packet->getDataLength())
  {
    isEmptyPacket = false;

    type = packet->bufToByte();

    if (type == 'S')
    {
      packet->bufToStr(name, sizeof(name));
      packet->bufToStr(comment, sizeof(comment));

      if (version >= 2) {

        packet->bufToStr(categoryName, sizeof(categoryName));

        int index = 1;

        if (version >= 3) {

          index = packet->bufToUByte2();
        }
        if (index <= 1) {
          ArLog::log(ArLog::Verbose, "%sReceiving config section %s, %s...", 
                      myLogPrefix.c_str(), categoryName, name);
              
          myConfig.addSection(categoryName, name, comment);
        }
        else {
          ArLog::log(ArLog::Verbose, "%sAppending to config section %s, %s (packet %i)...", 
                      myLogPrefix.c_str(), categoryName, name, index);
              
        }
      }
      else {
        ArLog::log(ArLog::Verbose, "%sReceiving config section %s...", 
                   myLogPrefix.c_str(), name);
        myConfig.setSectionComment(name, comment);
      }
      //printf("%c %s %s\n", type, name, comment);

 
      
      section = name;

    }
    else if (type == 'P')
    {
		  ArConfigArg configArg;

		  bool isSuccess = clientArg.createArg(packet,
						       configArg);

		  if (isSuccess) {
		    myConfig.addParamAsIs(configArg,
					  section.c_str());
		  }
		  else
		  {
			  ArLog::log(ArLog::Terse, "ArClientHandlerConfig unknown param type");
		  }
    }
    else // unrecognized header
    {
      ArLog::log(ArLog::Terse, "ArClientHandlerConfig unknown type");
    }
  
  } // end while more to read

  if (!isMultiplePackets || isEmptyPacket) {

    ArLog::log(ArLog::Normal, "%sGot config from server.", myLogPrefix.c_str());
    
    // KMC 1/22/13 To only log some of the config, substitute the commented
    // lines below for the following config->log statement. 
    IFDEBUG(myConfig.log());
    /*** 
    std::list<std::string> sectionNameList;
    sectionNameList.push_back("Data Log Settings");
    IFDEBUG(myConfig.log(false, &sectionNameList, myLogPrefix.c_str()));
    ****/

    myHaveGottenConfig = true;
  }

  myDataMutex.unlock();


  if (myHaveGottenConfig) {

    myCallbackMutex.lock();

    for (std::list<ArFunctor *>::iterator it = myGotConfigCBList.begin(); 
        it != myGotConfigCBList.end(); 
        it++) {
      (*it)->invoke();
    }
    myCallbackMutex.unlock();

  } // end if config received

} // end method handleGetConfigData


void ArClientHandlerConfig::handleGetConfigSectionFlags(
	ArNetPacket *packet) 
{
  int numSections = packet->bufToByte4();
  
  int i;

  char section[32000];
  char flags[32000];

  myDataMutex.lock();
  for (i = 0; i < numSections; i++)
  {
    packet->bufToStr(section, sizeof(section));
    packet->bufToStr(flags, sizeof(flags));
    myConfig.addSectionFlags(section, flags);
  }
  myDataMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::saveConfigToServer(void)
{
  saveConfigToServer(&myConfig);
}

AREXPORT void ArClientHandlerConfig::saveConfigToServer(
	  ArConfig *config, 
	  const std::set<std::string, ArStrCaseCmpOp> *ignoreTheseSections)
{
  //ArConfigArg param;
  ArClientArg clientArg;

  bool isMultiplePackets = true;
  int version = 2;
  char *setConfigPacketName = "setConfigBySectionsV2";
  if (!myClient->dataExists(setConfigPacketName)) {

    setConfigPacketName = "setConfigBySections";
    version = 1;

    if (!myClient->dataExists(setConfigPacketName)) {
      version = 0;
      isMultiplePackets = false;
      setConfigPacketName = "setConfig";
    } 
  }

  ArNetPacket sending;
  ArLog::log(ArLog::Normal, 
             "%sSaving config to server (with %s)", 
             myLogPrefix.c_str(), setConfigPacketName);
  
   
  // KMC 1/22/13 To only log some of the config, substitute the commented
  // lines below for the following config->log statement. 
  IFDEBUG(config->log(false, NULL, myLogPrefix.c_str()));
  /***
  std::list<std::string> sectionNameList;
  sectionNameList.push_back("Data Log Settings");
  IFDEBUG(config->log(false, &sectionNameList, myLogPrefix.c_str()));
  ***/
   

  myDataMutex.lock();
  bool isSuccess = true;
  std::list<ArConfigSection *> *sections = config->getSections();
  for (std::list<ArConfigSection *>::iterator sIt = sections->begin(); 
       (isSuccess && (sIt != sections->end())); 
       sIt++)
  {
    ArConfigSection *section = (*sIt);
    // if we're ignoring sections and we're ignoring this one, then
    // don't send it
    if (ignoreTheseSections != NULL && 
	      (ignoreTheseSections->find(section->getName()) != 
	      ignoreTheseSections->end()))
    {
      ArLog::log(ArLog::Verbose, "Not sending section %s", 
		  section->getName());
      continue;
    }
    sending.strToBuf("Section");
    sending.strToBuf(section->getName());
    std::list<ArConfigArg> *params = section->getParams();

    for (std::list<ArConfigArg>::iterator pIt = params->begin(); 
         pIt != params->end(); 
         pIt++)
    {
      ArConfigArg &param = (*pIt);

      if (!clientArg.isSendableParamType(param)) {
        continue;
      }

      clientArg.addArgTextToPacket(param, &sending);

      //sending.strToBuf(param.getName());
      
      //clientArg.argTextToBuf(param, &sending);

    } // end for each param

    if (isMultiplePackets) {

      if (!myClient->requestOnce(setConfigPacketName, &sending)) {

        isSuccess = false;
        break;
 
      }
      // Empty the packet so that the next section can be sent    
      sending.empty();

    } // end if send multiple packets

  } // end for each section

  myDataMutex.unlock();

  if (isMultiplePackets) {
    // Send an empty packet to indicate the end of the config
    ArNetPacket emptyPacket;
    if (!myClient->requestOnce(setConfigPacketName, &emptyPacket)) {
      isSuccess = false;
    }
    else {
      ArLog::log(ArLog::Normal,
                 "ArClientHandlerConfig sent empty packet");
    }
  }
  else {
    if (!myClient->requestOnce(setConfigPacketName, &sending)) {
      isSuccess = false;
    }
  }
  
  if (!isSuccess) {
    ArLog::log(ArLog::Normal,
               "%sclient request to %s failed, invoking callbacks",
               myLogPrefix.c_str(), setConfigPacketName);
    for (std::list<ArFunctor1<const char *> *>::iterator it = mySaveConfigFailedCBList.begin(); 
	       it != mySaveConfigFailedCBList.end(); 
	       it++) {
      (*it)->invoke();
    }
  }
}

void ArClientHandlerConfig::handleSetConfig(ArNetPacket *packet)
{ 
  char buffer[1024];
  packet->bufToStr(buffer, sizeof(buffer));

  myCallbackMutex.lock();
  if (buffer[0] == '\0')
  {
    ArLog::log(ArLog::Normal, "%sSaved config to server successfully", myLogPrefix.c_str());
    std::list<ArFunctor *>::iterator it;
    for (it = mySaveConfigSucceededCBList.begin(); 
	       it != mySaveConfigSucceededCBList.end(); 
	       it++)
      (*it)->invoke();
  }
  else
  {
    ArLog::log(ArLog::Normal, "%sSaving config to server had error: %s", myLogPrefix.c_str(), buffer);
    std::list<ArFunctor1<const char *> *>::iterator it;
    for (it = mySaveConfigFailedCBList.begin(); 
	       it != mySaveConfigFailedCBList.end(); 
	       it++)
      (*it)->invoke(buffer);
  }
  myCallbackMutex.unlock();
}

/// Handles the return packet from the setConfig (saveConfigToServer)
void ArClientHandlerConfig::handleSetConfigBySections(ArNetPacket *packet)
{
  char buffer[1024];
  packet->bufToStr(buffer, sizeof(buffer));

  myCallbackMutex.lock();
  if (buffer[0] == '\0')
  {
    ArLog::log(ArLog::Normal, "%sSaved config to server successfully", myLogPrefix.c_str());
    std::list<ArFunctor *>::iterator it;
    for (it = mySaveConfigSucceededCBList.begin(); 
	       it != mySaveConfigSucceededCBList.end(); 
	       it++)
      (*it)->invoke();
  }
  else
  {
    ArLog::log(ArLog::Normal, "%sSaving config to server had error: %s", myLogPrefix.c_str(), buffer);
    std::list<ArFunctor1<const char *> *>::iterator it;
    for (it = mySaveConfigFailedCBList.begin(); 
	       it != mySaveConfigFailedCBList.end(); 
	       it++)
      (*it)->invoke(buffer);
  }
  myCallbackMutex.unlock();
} 

/// Handles the return packet from the setConfig (saveConfigToServer)
void ArClientHandlerConfig::handleSetConfigBySectionsV2(ArNetPacket *packet)
{
  IFDEBUG(ArLog::log(ArLog::Normal,
                     "ArClientHandlerConfig::handleSetConfigBySectionsV2()"));

  char sectionBuffer[1024];
  char errorBuffer[1024];

  packet->bufToStr(sectionBuffer, sizeof(sectionBuffer));
  packet->bufToStr(errorBuffer, sizeof(errorBuffer));

  if (sectionBuffer[0] == '\0') {
    myCallbackMutex.lock();
    if (errorBuffer[0] == '\0')
    {
      ArLog::log(ArLog::Normal, "%sSaved config to server successfully", myLogPrefix.c_str());
      std::list<ArFunctor *>::iterator it;
      for (it = mySaveConfigSucceededCBList.begin(); 
	         it != mySaveConfigSucceededCBList.end(); 
	         it++)
        (*it)->invoke();
    }
    else
    {
      ArLog::log(ArLog::Normal, "%sSaving config to server had error: %s", myLogPrefix.c_str(), errorBuffer);
      std::list<ArFunctor1<const char *> *>::iterator it;
      for (it = mySaveConfigFailedCBList.begin(); 
	         it != mySaveConfigFailedCBList.end(); 
	         it++)
        (*it)->invoke(errorBuffer);
    }
    myCallbackMutex.unlock();
  }
  else {
    myCallbackMutex.lock();
    if (errorBuffer[0] == '\0')
    {
      ArLog::log(ArLog::Normal, "%sSaved config section %s to server successfully", 
                 myLogPrefix.c_str(), 
                 sectionBuffer);
      /***
      std::list<ArFunctor *>::iterator it;
      for (it = mySaveConfigSucceededCBList.begin(); 
	         it != mySaveConfigSucceededCBList.end(); 
	         it++)
        (*it)->invoke();
      **/
    }
    else
    {
      ArLog::log(ArLog::Normal, "%sSaving config to server had error: %s", myLogPrefix.c_str(), errorBuffer);
      std::list<ArFunctor1<const char *> *>::iterator it;
      for (it = mySaveConfigFailedCBList.begin(); 
	         it != mySaveConfigFailedCBList.end(); 
	         it++)
        (*it)->invoke(errorBuffer);
    }
    myCallbackMutex.unlock();
  }
} 


AREXPORT void ArClientHandlerConfig::reloadConfigOnServer(void)
{
  myClient->requestOnce("reloadConfig");
}

/**
   This will get the config, note, it gets the config we're using, so
   if you change something while data comes in things will get broken
 **/
AREXPORT ArConfig *ArClientHandlerConfig::getConfig(void)
{
  return &myConfig;
}

/**
 * Returns a pointer to the robot server's default configuration, if
 * canRequestDefaults() is true.  Note that both requestConfigFromServer() 
 * and then requestDefaultConfigFromServer() must be called before a 
 * valid default configuration is available on the client.
 * If there is no default configuration, then NULL is returned.
**/
AREXPORT ArConfig *ArClientHandlerConfig::getDefaultConfig(void)
{
  return myDefaultConfig;
}

/**
   This will get a copy of the config, you can use this with the
   safeConfigToServer that takes an argument
 **/
AREXPORT ArConfig ArClientHandlerConfig::getConfigCopy(void)
{
  ArConfig copy;
  myDataMutex.lock();
  copy = myConfig;
  myDataMutex.unlock();
  return copy;
}

AREXPORT int ArClientHandlerConfig::lock(void)
{
  return myDataMutex.lock();
}

AREXPORT int ArClientHandlerConfig::tryLock(void)
{
  return myDataMutex.tryLock();
}

AREXPORT int ArClientHandlerConfig::unlock(void)
{
  return myDataMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::setQuiet(bool isQuiet)
{
  myIsQuiet = isQuiet;
  myConfig.setQuiet(isQuiet);
  if (myDefaultConfig != NULL) {
    myDefaultConfig->setQuiet(isQuiet);
  }

}

AREXPORT bool ArClientHandlerConfig::haveGottenConfig(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myHaveGottenConfig;
  myDataMutex.unlock();
  return ret;
}

AREXPORT void ArClientHandlerConfig::addGotConfigCB(ArFunctor *functor,
						    ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myGotConfigCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    myGotConfigCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addGotConfigCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remGotConfigCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  myGotConfigCBList.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::addSaveConfigSucceededCB(
	ArFunctor *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    mySaveConfigSucceededCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    mySaveConfigSucceededCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addSaveConfigSucceededCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remSaveConfigSucceededCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  mySaveConfigSucceededCBList.remove(functor);
  myCallbackMutex.unlock();
}


AREXPORT void ArClientHandlerConfig::addSaveConfigFailedCB(
	ArFunctor1<const char *> *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    mySaveConfigFailedCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    mySaveConfigFailedCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addSaveConfigFailedCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remSaveConfigFailedCB(ArFunctor1<const char *> *functor)
{
  myCallbackMutex.lock();
  mySaveConfigFailedCBList.remove(functor);
  myCallbackMutex.unlock();
}


AREXPORT void ArClientHandlerConfig::addGotConfigDefaultsCB(
	ArFunctor *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myGotConfigDefaultsCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    myGotConfigDefaultsCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "ArClientHandlerConfig::addGotConfigDefaultsCB: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientHandlerConfig::remGotConfigDefaultsCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  myGotConfigDefaultsCBList.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT bool ArClientHandlerConfig::haveRequestedDefaults(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myHaveRequestedDefaults || myHaveRequestedDefaultCopy;
  myDataMutex.unlock();
  return ret;
}

AREXPORT bool ArClientHandlerConfig::haveGottenDefaults(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myHaveGottenDefaults;
  myDataMutex.unlock();
  return ret;
}

AREXPORT bool ArClientHandlerConfig::canRequestDefaults(void)
{
  return myClient->dataExists("getConfigDefaults");
}

AREXPORT bool ArClientHandlerConfig::requestConfigDefaults(void)
{
  if (haveRequestedDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestConfigDefaults: Cannot request defaults as there are already some being requested",
         myLogPrefix.c_str());
    return false;
  }
  if (!canRequestDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestConfigDefaults: Defaults requested but not available",
         myLogPrefix.c_str());
    return false;
  }

  ArLog::log(ArLog::Normal, 
	           "%sRequesting config reset to default",
             myLogPrefix.c_str());

  myDataMutex.lock();
  myHaveRequestedDefaults = true;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myDataMutex.unlock();

  myClient->requestOnce("getConfigDefaults");
  return true;
}

AREXPORT bool ArClientHandlerConfig::requestDefaultConfigFromServer(void)
{
  if (haveRequestedDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%requestDefaultConfigFromServer: Cannot request defaults as there are already some being requested",
         myLogPrefix.c_str());
    return false;
  }
  if (!canRequestDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestConfigDefaults: Defaults requested but not available",
         myLogPrefix.c_str());
    return false;
  }

  
  myDataMutex.lock();

  myHaveRequestedDefaults = false;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = true;
  myDataMutex.unlock();

  myClient->requestOnce("getConfigDefaults");
  return true;
}

AREXPORT bool ArClientHandlerConfig::requestSectionDefaults(
	const char *section)
{
  if (haveRequestedDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "RequestSectionDefaults: Cannot request defaults as there are already some being requested");
    return false;
  }
  if (!canRequestDefaults())
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestSectionDefaults: Defaults requested but not available",
         myLogPrefix.c_str());
    return false;
  }
  myDataMutex.lock();
  if (myConfig.findSection(section) == NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequestSectionDefaults: Section '%s' requested but doesn't exist", 
         myLogPrefix.c_str(), section);
    myDataMutex.unlock();
    return false;
  }

  ArLog::log(ArLog::Normal, 
	           "%sRequesting config section %s reset to default",
             myLogPrefix.c_str(),
             section);

  myHaveRequestedDefaults = true;
  myHaveGottenDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myDataMutex.unlock();
  myClient->requestOnceWithString("getConfigDefaults", section);
  return true;
}



void ArClientHandlerConfig::handleGetConfigDefaults(
	ArNetPacket *packet)
{
  ArLog::log(ArLog::Normal, "%sreceived default config %s", 
		             myLogPrefix.c_str(), 
                 ((myHaveRequestedDefaultCopy) ? "(copy)" : "(reset)"));

  char param[1024];
  char argument[1024];
  char errorBuffer[1024];
  
  myDataMutex.lock();

  ArConfig *config = NULL;

  // If the config (or a section) is being reset to its default values,
  // then we don't want to remove any parameters that are not set -- i.e.
  // any parameters that are not contained in the default config.
  bool isClearUnsetValues = false;

  if (myHaveRequestedDefaults) {

    config = &myConfig;
  }
  else if (myHaveRequestedDefaultCopy) {

    // If we have requested a copy of the default configuration, then we
    // will want to remove any parameters that haven't been explicitly set.
    // (This is because of the next line, which copies the current config 
    // to the default config.)
    isClearUnsetValues = true;

    // The default config is transmitted in an "abbreviated" form -- just 
    // the section/param names and values.  Copy the current config to the
    // default before processing the packet so that the parameter types, etc.
    // can be preserved.
    if (myDefaultConfig == NULL) {
      myDefaultConfig = new ArConfig(myConfig);
      myDefaultConfig->setConfigName("Default", myRobotName.c_str());
      myDefaultConfig->setQuiet(myIsQuiet);
    }
    else {
      *myDefaultConfig = myConfig;
    }


    config = myDefaultConfig;
  }
  // if we didn't ask for any of these, then just return since the
  // data is for someone else
  else
  {
    myDataMutex.unlock();
    return;
  }

  if (config == NULL) {
    ArLog::log(ArLog::Normal,
               "%serror determining config to populate with default values",
               myLogPrefix.c_str());
  myDataMutex.unlock();
    return;
  }

  ArArgumentBuilder *builder = NULL;
  ArLog::log(ArLog::Normal, "%sGot defaults", myLogPrefix.c_str());
  errorBuffer[0] = '\0';
  
  //myDataMutex.lock();
  if (isClearUnsetValues) {
    config->clearAllValueSet();
  }

  while (packet->getDataReadLength() < packet->getDataLength())
  {
    packet->bufToStr(param, sizeof(param));  
    packet->bufToStr(argument, sizeof(argument));  


    builder = new ArArgumentBuilder;
    builder->setQuiet(myIsQuiet);
    builder->setExtraString(param);
    builder->add(argument);

    if ((strcasecmp(param, "Section") == 0 && 
        !config->parseSection(builder, errorBuffer, sizeof(errorBuffer))) ||
        (strcasecmp(param, "Section") != 0 &&
        !config->parseArgument(builder, errorBuffer, sizeof(errorBuffer))))
    {
      ArLog::log(ArLog::Terse, "%shandleGetConfigDefaults: Hideous problem getting defaults, couldn't parse '%s %s'", 
		             myLogPrefix.c_str(), param, argument);
    }
    else {
      IFDEBUG(if (strlen(param) > 0) {
                 ArLog::log(ArLog::Normal, "%shandleGetConfigDefaults: added default '%s %s'", 
                 myLogPrefix.c_str(), param, argument); } );
    }
    delete builder;
    builder = NULL;
  }
  myHaveRequestedDefaults = false;
  myHaveRequestedDefaultCopy = false;
  myHaveGottenDefaults = true;

  if (isClearUnsetValues) {
    config->removeAllUnsetValues();
  }

  IFDEBUG(config->log());

  myDataMutex.unlock();

  myCallbackMutex.lock();
  std::list<ArFunctor *>::iterator it;
  for (it = myGotConfigDefaultsCBList.begin(); 
       it != myGotConfigDefaultsCBList.end(); 
       it++)
    (*it)->invoke();
  myCallbackMutex.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Last Editable Priority
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AREXPORT bool ArClientHandlerConfig::requestLastEditablePriorityFromServer()
{
  if (myClient->dataExists("getLastEditablePriority")) {

    myClient->remHandler("getLastEditablePriority", &myHandleGetLastEditablePriorityCB);
    myClient->addHandler("getLastEditablePriority", &myHandleGetLastEditablePriorityCB);
    myClient->requestOnce("getLastEditablePriority");

    myDataMutex.lock();
    myHaveRequestedLastEditablePriority = true;
    myHaveGottenLastEditablePriority = false;
    myDataMutex.unlock();

    ArLog::log(ArLog::Normal, "%sRequesting last editable priority", 
		          myLogPrefix.c_str());

    return true;
  }
  else { // server does not support last editable priority

    ArLog::log(ArLog::Normal, "%sserver does not support last editable priority", 
		          myLogPrefix.c_str());
    return false;

  } // end else server does not support last editable priority

} // end method requestLastEditablePriorityFromServer
  
AREXPORT bool ArClientHandlerConfig::isLastEditablePriorityAvailable()
{
  if ((myClient != NULL) &&
      (myClient->dataExists("getLastEditablePriority"))) {
    return true;
  }
  else {
    return false;
  }
}

AREXPORT bool ArClientHandlerConfig::haveGottenLastEditablePriority()
{
  myDataMutex.lock();
  bool b = myHaveGottenLastEditablePriority;
  myDataMutex.unlock();

  return b;

} // end method haveGottenLastEditablePriority

AREXPORT ArPriority::Priority ArClientHandlerConfig::getLastEditablePriority()
{
  myDataMutex.lock();
  ArPriority::Priority p = myLastEditablePriority;
  myDataMutex.unlock();

  return p;

} // end method getLastEditablePriority

AREXPORT void ArClientHandlerConfig::addGotLastEditablePriorityCB
                                        (ArFunctor *functor, 
			                                   int position)
{
  myGotLastEditablePriorityCBList.addCallback(functor, position);

} // end method addGotLastEditablePriorityCB

AREXPORT void ArClientHandlerConfig::remGotLastEditablePriorityCB
                                        (ArFunctor *functor)
{
  myGotLastEditablePriorityCBList.remCallback(functor);

} // end method remGotLastEditablePriorityCB


void ArClientHandlerConfig::handleGetLastEditablePriority
                                                    (ArNetPacket *packet)
{

  if (packet == NULL) {
    ArLog::log(ArLog::Normal, "%sreceived null packet for getLastEditablePriority", 
		          myLogPrefix.c_str());
    return;
  }

  ArPriority::Priority lastPriority = ArPriority::LAST_PRIORITY;

  int tempPriority = packet->bufToByte();
  if ((tempPriority >= 0) && (tempPriority < ArPriority::PRIORITY_COUNT)) {
    lastPriority = (ArPriority::Priority) tempPriority;
  }
  else {
    ArLog::log(ArLog::Terse, "%sReceived invalid last editable priority %i", 
		          myLogPrefix.c_str(), 
              tempPriority);
  }

  ArLog::log(ArLog::Normal, "%sReceived last editable priority %s", 
		         myLogPrefix.c_str(), 
             ArPriority::getPriorityName(lastPriority));

  myDataMutex.lock();
  myLastEditablePriority = lastPriority;
  myHaveRequestedLastEditablePriority = false;
  myHaveGottenLastEditablePriority = true;

  myDataMutex.unlock();

  myGotLastEditablePriorityCBList.invoke();

} // end method handleGetLastEditablePriority

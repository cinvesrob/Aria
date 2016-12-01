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
#include "ArServerHandlerConfig.h"

#include "ArClientArgUtils.h"

//#define ARDEBUG_SERVERHANDLERCONFIG

//#if (defined(_DEBUG) && defined(ARDEBUG_SERVERHANDLERCONFIG))
#if (defined(ARDEBUG_SERVERHANDLERCONFIG))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

/**
@param server the server to add data to

@param config the config to serve up

@param defaultFile if this is given the config will try to copy the
config given and then load the default file into that config and
then serve data so clients can get those defaults

@param defaultFileBaseDirectory base directory for the default file
**/
AREXPORT ArServerHandlerConfig::ArServerHandlerConfig(
	  ArServerBase *server, ArConfig *config, const char *defaultFile, 
	  const char *defaultFileBaseDirectory, bool allowFactory,
	  const char *robotName, bool preventChanges, 
	  const char *preventChangesString) :
  myRobotName((robotName != NULL) ? robotName : ""),
  myLogPrefix(""),
  myServer(server),
  myConfig(config),
  myDefault(NULL),
  myPreWriteCallbacks(),
  myPostWriteCallbacks(),
  myRestartSoftwareCB(NULL),
  myRestartSoftwareCBSet(false),
  myRestartHardwareCB(NULL),
  myRestartHardwareCBSet(false),
  myGetConfigBySectionsCB(this, &ArServerHandlerConfig::getConfigBySections),
  myGetConfigBySectionsV2CB(this, &ArServerHandlerConfig::getConfigBySectionsV2),
  myGetConfigBySectionsV3CB(this, &ArServerHandlerConfig::getConfigBySectionsV3),
  myGetConfigBySectionsV4CB(this, &ArServerHandlerConfig::getConfigBySectionsV4),
  myGetConfigCB(this, &ArServerHandlerConfig::getConfig),
  mySetConfigCB(this, &ArServerHandlerConfig::setConfig),
  mySetConfigParamCB(this, &ArServerHandlerConfig::setConfigParam),
  mySetConfigBySectionsCB(this, &ArServerHandlerConfig::setConfigBySections),
  mySetConfigBySectionsV2CB(this, &ArServerHandlerConfig::setConfigBySectionsV2),
  myReloadConfigCB(this, &ArServerHandlerConfig::reloadConfig),
  myGetConfigDefaultsCB(this, &ArServerHandlerConfig::getConfigDefaults),
  myGetConfigSectionFlagsCB(this, 
			    &ArServerHandlerConfig::getConfigSectionFlags),
  myGetLastEditablePriorityCB(this, 
          &ArServerHandlerConfig::getLastEditablePriority)
{
  myPermissionAllowFactory = allowFactory;
  myPreventChanges = preventChanges;
  if (preventChangesString != NULL)
    myPreventChangesString = preventChangesString;
  else
    myPreventChangesString = "Changes prevented for unspecified reason";

  if (!myRobotName.empty()) {
    myLogPrefix = myRobotName + ": ";
  }
  myDefaultConfigMutex.setLogName(
	  "ArServerHandlerConfig::myDefaultConfigMutex");
  myConfigMutex.setLogName("ArServerHandlerConfig::myConfigMutex");
  myAddedDefaultServerCommands = false;
  
  myServer->addData("getConfigBySectionsV4", 
                    "Gets the complete configuration information from the server", 
                    &myGetConfigBySectionsV4CB, 
                    "none", 
                    "Advanced configuration retrieval, including handling of serializable.  Use ArClientHandlerConfig if desired.", 
                    "ConfigEditing", "RETURN_UNTIL_EMPTY");

  myServer->addData("getConfigBySectionsV3", 
                    "Gets the complete configuration information from the server", 
                    &myGetConfigBySectionsV3CB, 
                    "none", 
                    "Advanced configuration retrieval, including handling of very big sections.  Use ArClientHandlerConfig if desired.", 
                    "ConfigEditing", "RETURN_UNTIL_EMPTY");


  myServer->addData("getConfigBySectionsV2", 
                    "Gets the complete configuration information from the server", 
                    &myGetConfigBySectionsV2CB, 
                    "none", 
                    "Advanced configuration retrieval, including restart level and list objects.  Use ArClientHandlerConfig if desired.", 
                    "ConfigEditing", "RETURN_UNTIL_EMPTY");

  myServer->addData("getConfigBySections", 
                    "Gets the configuration information from the server", 
                    &myGetConfigBySectionsCB, 
                    "none", 
                    "A single packet is sent for each config section.  Too complex to describe here.  Use ArClientHandlerConfig if desired.", 
                    "ConfigEditing", "RETURN_UNTIL_EMPTY");

  myServer->addData("getConfig", 
    "gets the configuration information from the server", 
    &myGetConfigCB, 
    "none", 
    "deprecated (getConfigBySections is preferred).  Too complex to describe here.  Use ArClientHandlerConfig if desired.", 
    "ConfigEditing", "RETURN_SINGLE");

  myServer->addData("setConfig", 
		    "takes a config back from the client to use",
		    &mySetConfigCB,
		    "Repeating pairs of strings which are parameter name and value to parse",
		    "string: if empty setConfig worked, if the string isn't empty then it is the first error that occured (all non-error parameters are parsed, and only the first error is reported)",
		    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");
  
  myServer->addData("setConfigParam", 
		    "Sets the value of a single configuration parameter",
		    &mySetConfigParamCB,
		    "TODO",
		    "string: empty if successful, otherwise the first error that occurred",
		    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");
  
  myServer->addData("setConfigBySections", 
		    "Saves the configuration received from the client, with each section in an individual packet",
		    &mySetConfigBySectionsCB,
		    "Section; string: section name; Repeating pairs of strings which are parameter name and value to parse",
		    "string: if empty setConfig worked, if the string isn't empty then it is the first error that occured (all non-error parameters are parsed, and only the first error is reported)",
		    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");
  
  myServer->addData("setConfigBySectionsV2", 
		    "Saves the configuration received from the client, with each section in an individual packet",
		    &mySetConfigBySectionsV2CB,
		    "Section; string: section name; Repeating pairs of strings which are parameter name and value to parse",
		    "string: sectionName, or empty if end marker; string: if empty setConfig worked, if the string isn't empty then it is the first error that occured (all non-error parameters are parsed, and only the first error is reported)",
		    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");
  
  myServer->addData("reloadConfig", 
		    "reloads the configuration file last loaded",
		    &myReloadConfigCB, "none", "none",
		    "ConfigEditing", "RETURN_SINGLE|IDLE_PACKET");

  myServer->addData("configUpdated", 
    "gets sent when the config is updated",
    NULL, "none", "none",
    "ConfigEditing", "RETURN_SINGLE");

  myServer->addData("getConfigSectionFlags", 
    "gets the flags for each section of the config", 
    &myGetConfigSectionFlagsCB, 
    "none", 
    "byte4: number of sections; repeating for number of sections (string: section; string: flags (separated by |))",
    "ConfigEditing", "RETURN_SINGLE");
  
  myServer->addData("getLastEditablePriority", 
    "Returns the last priority that may be edited by the user; e.g. EXPERT or FACTORY",
    &myGetLastEditablePriorityCB, 
    "none", 
    "byte: the last ArPriority::Priority value for which parameters may be edited", 
    "ConfigEditing", "RETURN_SINGLE");

  myServer->addData("configCausingRestart", 
		    "When a config parameter that causes a restart is broadcast 1 second before the restart...",
		    NULL, "none", 
		    "byte: 1 == software restart, 2 == hardware restart",
    "RobotInfo", "RETURN_NONE");

  if (defaultFile != NULL)
    myDefaultFile = defaultFile;
  if (defaultFileBaseDirectory != NULL)
    myDefaultFileBaseDir = defaultFileBaseDirectory;
  loadDefaultsFromFile();
}

AREXPORT ArServerHandlerConfig::~ArServerHandlerConfig()
{
  if (myDefault != NULL)
    delete myDefault;
}

AREXPORT bool ArServerHandlerConfig::loadDefaultsFromFile(void)
{
  bool ret = true;

  lockConfig();
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;  
  }

  if (!myDefaultFile.empty())
  {
    ArLog::log(ArLog::Normal, "%sWill attempt to load default file '%s'",
	       myLogPrefix.c_str(), myDefaultFile.c_str());
    createDefaultConfig(myDefaultFileBaseDir.c_str());
    myDefault->clearAllValueSet();
    // now fill in that copy
    myDefault->setPermissions(true, false);
    if (myDefault->parseFile(myDefaultFile.c_str()))
    {
      addDefaultServerCommands();
    }
    else
    {
      ret = false;
      ArLog::log(ArLog::Normal, 
                 "%sDid not load default file '%s' successfully, not allowing getDefault", 
                 myLogPrefix.c_str(),
                 myDefaultFile.c_str());
      delete myDefault;
      myDefault = NULL;
    }
    if (myDefault != NULL)
      myDefault->removeAllUnsetValues();
  }
  unlockConfig();
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, "configDefaultsUpdated");
  return ret;
}

AREXPORT bool ArServerHandlerConfig::loadDefaultsFromPacket(
	ArNetPacket *packet)
{
  bool ret = true;
  
  lockConfig();
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;  
  }

  createDefaultConfig(NULL);
  myDefault->clearAllValueSet();
  // now fill in that copy
  if (internalSetConfig(NULL, packet, 0))
  {
      addDefaultServerCommands();    
  }
  else
  {
    ArLog::log(ArLog::Normal, 
               "%sDid not load default from packet successfully, not allowing getDefault",
               myLogPrefix.c_str());
    delete myDefault;
    myDefault = NULL;
  }
  if (myDefault != NULL)
    myDefault->removeAllUnsetValues();
  unlockConfig();
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, "configDefaultsUpdated");
  return ret;
}

AREXPORT void ArServerHandlerConfig::createEmptyConfigDefaults(void)
{
  lockConfig();
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;  
  }

  addDefaultServerCommands();    
  unlockConfig();
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, "configDefaultsUpdated");
}

/** 
    @internal

    doesn't delete the old one, do that if you're going to call this
    yourself and make sure you lock around all that (okay, it deletes
    it now, but the stuff that calls it should still take care of it)
**/
void ArServerHandlerConfig::createDefaultConfig(const char *defaultFileBaseDir)
{
  if (myDefault != NULL)
  {
    delete myDefault;
    myDefault = NULL;
  }
  // copy that config (basedir will be NULL if we're not loading from
  // a file)... don't have the default save unknown values
  myDefault = new ArConfig(defaultFileBaseDir, false, false, false, false);

  std::list<ArConfigSection *>::iterator sectionIt;
  std::list<ArConfigArg>::iterator paramIt;
  ArConfigSection *section = NULL;
  std::list<ArConfigArg> *params = NULL;
  ArConfigArg param;
  for (sectionIt = myConfig->getSections()->begin(); 
       sectionIt != myConfig->getSections()->end(); 
       sectionIt++)
  {
    section = (*sectionIt);
    params = section->getParams();


    for (paramIt = params->begin(); paramIt != params->end(); paramIt++)
    {
      param.copyAndDetach(*paramIt);

      myDefault->addParam(param);


 //     switch (param.getType()) {
 //     case ArConfigArg::INT:
	//myDefault->addParam(
	//	ArConfigArg(param.getName(), param.getInt(), 
	//		    param.getDescription(), 
	//		    param.getMinInt(), param.getMaxInt()), 
	//	section->getName(), 
	//	param.getConfigPriority(),
	//	param.getDisplayHint());
	//break;
 //     case ArConfigArg::DOUBLE:
	//myDefault->addParam(
	//	ArConfigArg(param.getName(), param.getDouble(), 
	//		    param.getDescription(),
	//		    param.getMinDouble(), param.getMaxDouble()), 
	//	section->getName(), 
	//	param.getConfigPriority(),
	//	param.getDisplayHint());
	//break;
	//
 //     case ArConfigArg::BOOL:
	//myDefault->addParam(
	//	ArConfigArg(param.getName(), param.getBool(), 
	//		    param.getDescription()),
	//	section->getName(), 
	//	param.getConfigPriority(),
	//	param.getDisplayHint());
	//break;
	//
 //     case ArConfigArg::STRING:
	//myDefault->addParam(
	//	ArConfigArg(param.getName(), (char *)param.getString(), 
	//		    param.getDescription(), 0),
	//	section->getName(), 
	//	param.getConfigPriority(),
	//	param.getDisplayHint());
	//break;
	//
 //     case ArConfigArg::SEPARATOR:
	//      myDefault->addParam(
	//	      ArConfigArg(ArConfigArg::SEPARATOR),
	//	      section->getName(), 
	//	      param.getConfigPriority(),
	//	      param.getDisplayHint());
	//break;
 //     default:
	//break;
 //     } // end switch param type
    } // end for each param
  } // end for each section
} // create default config

void ArServerHandlerConfig::addDefaultServerCommands(void)
{
  if (myAddedDefaultServerCommands)
    return;

  myServer->addData("getConfigDefaults", 
		    "Gets the config default values ",
		    &myGetConfigDefaultsCB, 
		    "string: section to load, empty string means get the whole thing", 
		    "repeating strings that are the parameters and arguments to parse, but use ArClientHandlerConfig to handle this if you want",
		    "ConfigEditing");
  myServer->addData("configDefaultsUpdated", 
		    "Gets sent when the config defaults are updated",
		    NULL, "none", "none",
		    "ConfigEditing", "RETURN_SINGLE");
  myAddedDefaultServerCommands = true;
}

/**
 * @param client the ArServerClient * to which to send the config
 * @param packet the ArNetPacket * which accompanied the client's request
 * @param isMultiplePackets a bool set to true if the server should send a
 * packet for each config section followed by the empty packet; false if 
 * the server should send the entire config in one packet (i.e. the old style)
 * @param lastPriority the last ArPriority::Priority that should be sent 
 * to the client (this is the greatest numerical value and the least 
 * semantic priority).
**/
AREXPORT void ArServerHandlerConfig::handleGetConfig
                                      (ArServerClient *client, 
                                       ArNetPacket *packet,
                                       bool isMultiplePackets,
                                       ArPriority::Priority lastPriority,
                                       bool isSendIneditablePriorities,
                                       int version)
{

  ArConfigArg param;

  // The multiple packets method also sends display hints with the parameters;
  // the old single packet method does not.
  ArClientArg clientArg(isMultiplePackets,
                        lastPriority,
                        version);

 
  std::set<std::string> sent;

  ArNetPacket sending;
  ArLog::log(ArLog::Normal, 
             "%sConfig requested.",
             myLogPrefix.c_str());

  std::list<ArConfigSection *> *sections = myConfig->getSections();
  for (std::list<ArConfigSection *>::iterator sIt = sections->begin(); 
       sIt != sections->end(); 
       sIt++)
  {
    // Clear the packet...
    if (isMultiplePackets) {
      sending.empty();
    }

    // clear out the sent list between sections
    sent.clear();

    ArConfigSection *section = (*sIt);
    if (section == NULL) {
      continue;
    }
    std::list<ArConfigArg> *params = section->getParams();

bool isSectionSuccess = handleGetConfigSection(sending,
                                            client, 
                                            packet,
                                            isMultiplePackets,
                                            lastPriority,
                                            isSendIneditablePriorities,
                                            version,
                                            section,
                                            0,
                                            params->size(),
                                            1,
                                            sent);


    //sending.byteToBuf('S');
    //sending.strToBuf(section->getName());
    //sending.strToBuf(section->getComment());
    //if (version >= 2) {
    //  sending.strToBuf(section->getCategoryName());
    //}

    //ArLog::log(ArLog::Verbose, 
    //           "%sSending config section %s...", 
    //           myLogPrefix.c_str(),
    //           section->getName());

    ////printf("S %s %s\n", section->getName(), section->getComment());
    //std::list<ArConfigArg> *params = section->getParams();
    //for (std::list<ArConfigArg>::iterator pIt = params->begin(); 
    //     pIt != params->end(); 
    //     pIt++)
    //{
    //  param = (*pIt);

    //  if ((!isSendIneditablePriorities) &&
    //      (param.getConfigPriority() > lastEditablePriority)) {
    //    ArLog::log(ArLog::Verbose,
    //               "%sNot sending parameter %s priority %s ineditable",
    //               myLogPrefix.c_str(),
    //               param.getName(),
    //               ArPriority::getPriorityName(param.getConfigPriority()));
    //    continue;
    //  } // end if ineditable param


    //  bool isCheckableName = 
    //          (param.getType() != ArConfigArg::DESCRIPTION_HOLDER && 
    //           param.getType() != ArConfigArg::SEPARATOR &&
    //           param.getType() != ArConfigArg::STRING_HOLDER);

    //  // if we've already sent it don't send it again
    //  if (isCheckableName &&
    //      sent.find(param.getName()) != sent.end()) {
    //    continue;
    //  }
    //  else if (isCheckableName) {
    //    sent.insert(param.getName());
    //  }

    //  if (clientArg.isSendableParamType(param))
    //  {
    //    sending.byteToBuf('P');

    //    bool isSuccess = clientArg.createPacket(param,
    //                                            &sending);

    //  }
    //} // end for each parameter

    //if (!sending.isValid()) {

    //  ArLog::log(ArLog::Terse, 
    //             "%sConfig section %s cannot be sent; packet size exceeded",
    //             myLogPrefix.c_str(),
    //             section->getName());

    //} // end if length exceeded...
    //else if (isMultiplePackets) {

    //  client->sendPacketTcp(&sending);

    //} // end else send in chunks...

  } // end for each section

  // If sending each section in individual packets, then send an empty packet 
  // to indicate the end of the config data.
  if (isMultiplePackets) {

    sending.empty();
    client->sendPacketTcp(&sending);
  }
  else { //  send the entire config in one packet

    // If the config is too big to fit in the packet, then just send an empty
    // packet (to try to prevent an older client from crashing)
    // TODO: Is there any better way to notify the user of an error....
    if (!sending.isValid()) {
      ArLog::log(ArLog::Terse, 
                 "%sError sending config; packet size exceeded",
                 myLogPrefix.c_str());
      sending.empty();
    }

    client->sendPacketTcp(&sending);

  } // end else send the entire packet

} // end method getConfigBySections


AREXPORT bool ArServerHandlerConfig::handleGetConfigSection
                                           (ArNetPacket &sending,
                                            ArServerClient *client, 
                                            ArNetPacket *packet,
                                            bool isMultiplePackets,
                                            ArPriority::Priority lastPriority,
                                            bool isSendIneditablePriorities,
                                            int version,
                                            ArConfigSection *section,
                                            int startIndex,
                                            int paramCount,
                                            int sectionIndex,
                                            std::set<std::string> &sentParams)
{
    // The multiple packets method also sends display hints with the parameters;
    // the old single packet method does not.
    ArClientArg clientArg(isMultiplePackets,
                          lastPriority,
                          version);

    ArPriority::Priority lastEditablePriority = findLastEditablePriority();

    sending.byteToBuf('S');
    sending.strToBuf(section->getName());
    sending.strToBuf(section->getComment());
    if (version >= 2) {
      sending.strToBuf(section->getCategoryName());
    }
    if (version >= 3) {
      sending.uByte2ToBuf(sectionIndex);
    }

    ArLog::log(ArLog::Verbose, 
               "%sSending config section %s...", 
               myLogPrefix.c_str(),
               section->getName());
    bool isSuccess = true;
    
    std::list<ArConfigArg> *params = section->getParams();


    int successCount = 0;
    int curIndex = 0;
    int count = 0;
    //printf("S %s %s\n", section->getName(), section->getComment());
    for (std::list<ArConfigArg>::iterator pIt = params->begin();
         ((pIt != params->end()) && (isSuccess)); 
         pIt++, curIndex++)
    {
      if (curIndex < startIndex) {
        continue;
      }
      count++;
      if (count > paramCount) {
        break;
      }
      ArConfigArg &param = (*pIt);

      if ((!isSendIneditablePriorities) &&
          (param.getConfigPriority() > lastEditablePriority)) {
        ArLog::log(ArLog::Verbose,
                   "%sNot sending parameter %s priority %s ineditable",
                   myLogPrefix.c_str(),
                   param.getName(),
                   ArPriority::getPriorityName(param.getConfigPriority()));
        continue;
      } // end if ineditable param


      bool isCheckableName = 
              (param.getType() != ArConfigArg::DESCRIPTION_HOLDER && 
               param.getType() != ArConfigArg::SEPARATOR &&
               param.getType() != ArConfigArg::STRING_HOLDER);

      // if we've already sent it don't send it again
      if (isCheckableName &&
          sentParams.find(param.getName()) != sentParams.end()) {
        continue;
      }
      else if (isCheckableName) {
        sentParams.insert(param.getName());
      }


      if (clientArg.isSendableParamType(param))
      {
        sending.byteToBuf('P');

        isSuccess = clientArg.createPacket(param,
                                           &sending);
        if (isSuccess && sending.isValid()) {
          IFDEBUG(ArLog::log(ArLog::Normal,
                             "ArServerHandlerConfig added param %s to packet",
                             param.getName()));
          successCount++; 
        }

      }
    } // end for each parameter


    if ((!sending.isValid()) || (!isSuccess)) {

      if (version >= 3) {
      
        if (successCount == 0) {
          ArLog::log(ArLog::Normal,
                     "ArServerHandlerConfig No successful param sent for section %s",
                      section->getName());
          return false;
        }

       sentParams.clear();

        
        IFDEBUG(ArLog::log(ArLog::Normal,
                           "ArServerHandlerConfig About to send front for section %s",
                           section->getName()));
    
        sending.empty();
        bool isFrontSuccess = handleGetConfigSection
                                           (sending,
                                            client, 
                                            packet,
                                            isMultiplePackets,
                                            lastPriority,
                                            isSendIneditablePriorities,
                                            version,
                                            section,
                                            startIndex,
                                            successCount,
                                            sectionIndex,
                                            sentParams);

        IFDEBUG(ArLog::log(ArLog::Normal,
                          "ArServerHandlerConfig About to send rear for section %s",
                           section->getName()));
    
        sending.empty();
        bool isRearSuccess = handleGetConfigSection
                                           (sending,
                                            client, 
                                            packet,
                                            isMultiplePackets,
                                            lastPriority,
                                            isSendIneditablePriorities,
                                            version,
                                            section,
                                            startIndex + successCount,
                                            count - successCount,
                                            sectionIndex + 1,
                                            sentParams);
        IFDEBUG(ArLog::log(ArLog::Normal,
                           "ArServerHandlerConfig Finished sending %s",
                            section->getName()));
        return (isFrontSuccess && isRearSuccess);

      }
      else {


        ArLog::log(ArLog::Terse, 
                   "%sConfig section %s cannot be sent; packet size exceeded",
                   myLogPrefix.c_str(),
                   section->getName());

        return false;
      }

    } // end if length exceeded...
    else if (isMultiplePackets) {

      IFDEBUG(ArLog::log(ArLog::Normal,
                         "ArServerHandlerConfig sending packet to client"));

      client->sendPacketTcp(&sending);

      return true;

    } // end else send in chunks...

  return true;

} // end method handleGetConfigSection

AREXPORT void ArServerHandlerConfig::getConfigBySectionsV4(ArServerClient *client, 
                                                           ArNetPacket *packet)
{
  doGetConfigBySections(client, packet, 4);

} // end method getConfigBySectionsV4

AREXPORT void ArServerHandlerConfig::getConfigBySectionsV3(ArServerClient *client, 
                                                           ArNetPacket *packet)
{
  doGetConfigBySections(client, packet, 3);

} // end method getConfigBySectionsV3

AREXPORT void ArServerHandlerConfig::getConfigBySectionsV2(ArServerClient *client, 
                                                           ArNetPacket *packet)
{
  doGetConfigBySections(client, packet, 2);

} // end method getConfigBySectionsV2

AREXPORT void ArServerHandlerConfig::getConfigBySections(ArServerClient *client, 
                                                         ArNetPacket *packet)
{
  doGetConfigBySections(client, packet, 1);

} // end method getConfigBySections

AREXPORT void ArServerHandlerConfig::doGetConfigBySections(ArServerClient *client, 
                                                           ArNetPacket *packet,
                                                           int version)
{
  // The default value of lastPriority is DETAILED because that was the 
  // original last value and some of the older clients cannot handle
  // anything greater.  
  ArPriority::Priority lastPriority = ArPriority::DETAILED;

  bool isSendIneditablePriorities = false;
  ArPriority::Priority lastEditablePriority = findLastEditablePriority();

  // Newer clients insert a priority value into the packet.  This is 
  // the last priority (i.e. greatest numerical value) that the client
  // can handle.
  if ((packet != NULL) &&
      (packet->getDataReadLength() < packet->getDataLength())) {

    char priorityVal = packet->bufToByte();
    ArLog::log(ArLog::Verbose,
               "%sSending config with maximum priority value set to %i", 
               myLogPrefix.c_str(),
               priorityVal);

    lastPriority = convertToPriority(priorityVal, lastPriority);

    /***
    if ((priorityVal >= 0) && (priorityVal <= ArPriority::LAST_PRIORITY)) {
      lastPriority = (ArPriority::Priority) priorityVal;
    }
    else if (priorityVal > ArPriority::LAST_PRIORITY) {
      // This is just to handle the unlikely case that more priorities 
      // are added.  That is, both the server and client can handle more
      // than DETAILED -- and the client can handle even more than the
      // server.
      lastPriority = ArPriority::LAST_PRIORITY;
    }
    ***/

    // If the packet contains more information, it is the last priority
    // that may be edited by the client; this means that the client
    // is relatively new and can handle editable versus display-only
    // (or hidden) priorities; send the ineditable priorities too
    if (packet->getDataReadLength() < packet->getDataLength()) {

      priorityVal = packet->bufToByte();
      lastEditablePriority = convertToPriority(priorityVal, 
                                               ArPriority::LAST_PRIORITY);

      ArLog::log(ArLog::Verbose,
                 "%sReceived last editable priority %s in config request",
                 myLogPrefix.c_str(),
                 ArPriority::getPriorityName(lastEditablePriority));
      
      if (lastEditablePriority != findLastEditablePriority()) {
        // Not sure whether this is really a critical error...
        ArLog::log(ArLog::Normal,
                   "%sReceived unexpected last editable priority %s (expected %s)",
                   myLogPrefix.c_str(),
                   ArPriority::getPriorityName(lastEditablePriority),
                   ArPriority::getPriorityName(findLastEditablePriority()));
      }

      isSendIneditablePriorities = true;

    } // end if packet contains last editable priority

  } // end if packet is not empty

  handleGetConfig(client, 
                  packet,
                  true, // multiple packets
                  lastPriority,
                  isSendIneditablePriorities,
                  version);

} // end method getConfigBySections
 



ArPriority::Priority ArServerHandlerConfig::findLastEditablePriority()
{
  // KMC 10/26/12 Changed from EXPERT to CALIBRATION
  ArPriority::Priority lastPriority = ArPriority::CALIBRATION;
  // KMC 10/26/12 ...and removed this because I'm under the impression that
  // we're not currently supporting the (not) "allow factory" permission
//  if (myPermissionAllowFactory) {
//    lastPriority = ArPriority::CALIBRATION;
//  }
  return lastPriority;
}


ArPriority::Priority ArServerHandlerConfig::convertToPriority
                                             (int priorityVal,
                                              ArPriority::Priority defaultPriority)
{
  ArPriority::Priority priority = defaultPriority;

  if ((priorityVal >= 0) && (priorityVal <= ArPriority::LAST_PRIORITY)) {
    priority = (ArPriority::Priority) priorityVal;
  }
  else if (priorityVal > ArPriority::LAST_PRIORITY) {
    // This is just to handle the unlikely case that more priorities 
    // are added.  That is, both the server and client can handle more
    // than DETAILED -- and the client can handle even more than the
    // server.
    priority = ArPriority::LAST_PRIORITY;
  }

  return priority;

} // end method converToPriority


AREXPORT void ArServerHandlerConfig::getConfig(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  handleGetConfig(client, 
                  packet,
                  false,  // single packet
                  ArPriority::DETAILED, // older clients can't handle greater priorities
                  false,  // older clients don't have ineditable priorities
                  false); // older clients also don't have restart levels
}



AREXPORT void ArServerHandlerConfig::setConfig(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  // MPL 9/13/12 added this lock when I added the setPreventChanges
  // function call
  myConfigMutex.lock();
  if (myPreventChanges)
  {
    ArNetPacket retPacket;
    retPacket.strToBuf(myPreventChangesString.c_str());
    ArLog::log(ArLog::Normal, "SetConfig prevented because '%s'",
	       myPreventChangesString.c_str());
    if (client != NULL)
    {
      client->sendPacketTcp(&retPacket);
      ArNetPacket emptyPacket;
      emptyPacket.setCommand(myServer->findCommandFromName("configUpdated"));
      client->sendPacketTcp(&emptyPacket);
    }
    return;
  }
  myConfigMutex.unlock();

  internalSetConfig(client, packet, 0);
}


AREXPORT void ArServerHandlerConfig::setConfigParam(ArServerClient *client, 
                                                    ArNetPacket *packet)
{
  // MPL 9/13/12 added this lock when I added the setPreventChanges
  // function call
  myConfigMutex.lock();
  if (myPreventChanges)
  {
    ArNetPacket retPacket;
    retPacket.strToBuf(myPreventChangesString.c_str());
    ArLog::log(ArLog::Normal, "setConfigParam prevented because '%s'",
	       myPreventChangesString.c_str());
    if (client != NULL)
    {
      client->sendPacketTcp(&retPacket);
      ArNetPacket emptyPacket;
      emptyPacket.setCommand(myServer->findCommandFromName("configUpdated"));
      client->sendPacketTcp(&emptyPacket);
    }
    return;
  }
  myConfigMutex.unlock();

  internalSetConfig(client, packet, 4);
}


AREXPORT void ArServerHandlerConfig::setConfigBySections(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  // MPL 9/13/12 added this lock when I added the setPreventChanges
  // function call
  myConfigMutex.lock();
  if (myPreventChanges)
  {
    ArNetPacket retPacket;
    retPacket.strToBuf(myPreventChangesString.c_str());
    ArLog::log(ArLog::Normal, "SetConfigBySections prevented because '%s'",
	       myPreventChangesString.c_str());
    if (client != NULL)
    {
      client->sendPacketTcp(&retPacket);
      ArNetPacket emptyPacket;
      emptyPacket.setCommand(myServer->findCommandFromName("configUpdated"));
      client->sendPacketTcp(&emptyPacket);
    }
    return;
  }
  myConfigMutex.unlock();

  internalSetConfig(client, packet, 1, true);
}

AREXPORT void ArServerHandlerConfig::setConfigBySectionsV2(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  // MPL 9/13/12 added this lock when I added the setPreventChanges
  // function call
  myConfigMutex.lock();
  if (myPreventChanges)
  {
    ArNetPacket retPacket;
    retPacket.strToBuf(myPreventChangesString.c_str());
    ArLog::log(ArLog::Normal, "SetConfigBySections prevented because '%s'",
	       myPreventChangesString.c_str());
    if (client != NULL)
    {
      client->sendPacketTcp(&retPacket);
      ArNetPacket emptyPacket;
      emptyPacket.setCommand(myServer->findCommandFromName("configUpdated"));
      client->sendPacketTcp(&emptyPacket);
    }
    return;
  }
  myConfigMutex.unlock();

  internalSetConfig(client, packet, 2, true);
}

/**
   @param client If client is NULL it means use the default config
   @param packet request packet containing config options
**/

bool ArServerHandlerConfig::internalSetConfig(ArServerClient *client, 
					                                    ArNetPacket *packet,
                                              int version,
                                              bool isMultiplePackets,
                                              bool isSingleParam)
{
  char param[1024];
  char argument[1024];
  char errorBuffer[1024];
  char firstError[1024];
  ArNetPacket retPacket;
  ArConfig *config;
  bool ret = true;

  if (client != NULL)
    config = myConfig;
  else
    config = myDefault;

  if (client != NULL)
    lockConfig();

  if (client != NULL)
    config->setPermissions(myPermissionAllowFactory, false);
  else
    config->setPermissions(true, false);

  ArArgumentBuilder *builder = NULL;
  if (client != NULL)
    ArLog::log(ArLog::Normal, "%sGot new config from client %s", 
               myLogPrefix.c_str(),
               client->getIPString());
  else
    ArLog::log(ArLog::Verbose, 
               "%sNew default config",
               myLogPrefix.c_str());
  errorBuffer[0] = '\0';
  firstError[0] = '\0';
  std::string sectionName;

  if (!isMultiplePackets) {
    config->resetRestartLevelNeeded();
  }

  bool isEmptyPacket = true;
 
  while (packet->getDataReadLength() < packet->getDataLength())
  {
    isEmptyPacket = false;

    packet->bufToStr(param, sizeof(param));  
    packet->bufToStr(argument, sizeof(argument));  

    builder = new ArArgumentBuilder;
    builder->setExtraString(param);
    builder->addPlain(argument);

    ArLog::LogLevel level = ArLog::Verbose;
    IFDEBUG(
      if (ArUtil::strcasecmp(sectionName.c_str(), "Touchscreen") == 0) {
        level = ArLog::Normal;
      }
    );
    ArLog::log(level,
               "Config: \"%s\" \"%s\"", param, argument);

    // if the param name here is "Section" we need to parse sections,
    // otherwise we parse the argument
    if (strcasecmp(param, "Section") == 0) {

      if (builder->getArgc() > 0) {
        IFDEBUG(ArLog::log(ArLog::Normal,
                           "ArServerHandlerConfig::internalSetConfig() Receiving config section %s",
                           builder->getArg(0)));
        sectionName = builder->getArg(0);
      }
      if (!config->parseSection(builder, errorBuffer, sizeof(errorBuffer))) {
        if (firstError[0] == '\0') {
          strcpy(firstError, errorBuffer); 
        } // end if no error yet
      } // end if error parsing section

    } 
    else if (strcasecmp(param, ArConfigArg::LIST_BEGIN_TAG) == 0) {
        
      if (!config->parseListBegin(builder, errorBuffer, sizeof(errorBuffer))) {
        if (firstError[0] == '\0')
          strcpy(firstError, errorBuffer);
      }

    }
    else if (strcasecmp(param, ArConfigArg::LIST_END_TAG) == 0) {
      
      if (!config->parseListEnd(builder, errorBuffer, sizeof(errorBuffer))) {
        if (firstError[0] == '\0')
          strcpy(firstError, errorBuffer);
      }

    }
    else if ((strcasecmp(param, "Section") != 0 &&
             !config->parseArgument(builder, errorBuffer, sizeof(errorBuffer))))
    {
      if (firstError[0] == '\0')
        strcpy(firstError, errorBuffer);
    }
    delete builder;
    builder = NULL;
  }
  if (version >= 2) {

 //   ArLog::log(ArLog::Normal,
 //              " ----- adding %s to retPacket",
 //              sectionName.c_str());

    retPacket.strToBuf(sectionName.c_str());
  } // end if version >= 2

  if (isMultiplePackets && !isEmptyPacket) {
  if (firstError[0] != '\0')
  {
    ret = false;
    if (client != NULL)
      ArLog::log(ArLog::Normal, 
		             "%sNew config from client %s had at least this problem: %s", 
                 myLogPrefix.c_str(),
		             client->getIPString(), firstError);
    else
      ArLog::log(ArLog::Normal, 
		             "%sNew default config had at least this problem: %s", 
                 myLogPrefix.c_str(),
		             firstError);
     
    retPacket.strToBuf(firstError);
  }
  //printf("Sending ");
  //retPacket.log();
  if (client != NULL)
    client->sendPacketTcp(&retPacket);
  if (client != NULL)
    unlockConfig();
  // MPL 12/9/2012 don't update the config since it's only partial so far
  //  if (client != NULL)
  //  configUpdated(client);

  return ret;

  }

    // KMC 1/22/13 To only log some of the config, substitute the commented
    // lines below for the following config->log statement. 

//    IFDEBUG(config->log(false, NULL, myLogPrefix.c_str()));
//    std::list<std::string> sectionNameList;
//    sectionNameList.push_back("Touchscreen");
//    IFDEBUG(config->log(false, &sectionNameList, myLogPrefix.c_str()));

    if (firstError[0] == '\0')
    {
      if (config->callProcessFileCallBacks(true, 
                                             errorBuffer, 
                                             sizeof(errorBuffer)))
      {
        if (client != NULL)
           ArLog::log(ArLog::Normal, 
                      "%sNew config from client %s was fine.",
                      myLogPrefix.c_str(),
                      client->getIPString());
        else
           ArLog::log(ArLog::Verbose, 
                      "%sNew default config was fine.",
                      myLogPrefix.c_str());
        retPacket.strToBuf("");
        writeConfig();
      }
      else // error processing config callbacks
      {
        ret = false;
        if (firstError[0] == '\0')
          strcpy(firstError, errorBuffer);
        // if its still empty it means we didn't have anything good in the errorBuffer
        if (firstError[0] == '\0')
          strcpy(firstError, "Error processing");

        if (client != NULL)
          ArLog::log(ArLog::Normal, 
                     "%sNew config from client %s had errors processing ('%s').",
                     myLogPrefix.c_str(),
                     client->getIPString(), firstError);
         
        else
          ArLog::log(ArLog::Normal, 
                     "%sNew default config had errors processing ('%s').",
                     myLogPrefix.c_str(),
                     firstError);
        retPacket.strToBuf(firstError);
      }
    }
  // }
  // KMC Not sure about this.... Seems like it would be good to return the
  // error as soon as possible -- but what affect on client?
  else // if (firstError[0] != '\0')
  {
    ret = false;
    if (client != NULL)
      ArLog::log(ArLog::Normal, 
		             "%sNew config from client %s had at least this problem: %s", 
                 myLogPrefix.c_str(),
		             client->getIPString(), firstError);
    else
      ArLog::log(ArLog::Normal, 
		             "%sNew default config had at least this problem: %s", 
                 myLogPrefix.c_str(),
		             firstError);
     
    retPacket.strToBuf(firstError);
  }
  //printf("Sending ");
  //retPacket.log();
  if (client != NULL)
    client->sendPacketTcp(&retPacket);
  if (client != NULL)
    unlockConfig();
  if (client != NULL)
    configUpdated(client);
    
  ArConfigArg::RestartLevel restartLevel = config->getRestartLevelNeeded();

  config->resetRestartLevelNeeded();

  // if we're changing main config then we need to do more
  if (config == myConfig)
  {
    if (ret)
    {
      if (restartLevel == ArConfigArg::RESTART_CLIENT)
      {
	ArLog::log(ArLog::Normal, "%sRequesting client restart because RESTART_CLIENT (%d) config param(s) changed", 
		   myLogPrefix.c_str(), restartLevel);
	ArNetPacket clientRestartPacket;
	clientRestartPacket.byteToBuf(1);
	clientRestartPacket.strToBuf(
		"Client restart requested because of config change");
	
	myServer->broadcastPacketTcp(&clientRestartPacket,
				     "requestClientRestart");
      }
      else if (restartLevel == ArConfigArg::RESTART_IO)
      {
	restartIO("RESTART_IO config param(s) changed");
      }
      else if (restartLevel == ArConfigArg::RESTART_SOFTWARE)
      {
	restartSoftware("RESTART_SOFTWARE config param(s) changed");
      }
      else if (restartLevel == ArConfigArg::RESTART_HARDWARE)
      {
	restartHardware("RESTART_HARDWARE config param(s) changed");
      }
    }
    // if we couldn't successfully load the config... then reload a new
    // config (since otherwise some values can linger when they
    // shouldn't)
    else
    {
      ArLog::log(ArLog::Normal, 
		             "%sReloading config.", 
                 myLogPrefix.c_str());
      reloadConfig(NULL, NULL);
    }
  }

  config->resetRestartLevelNeeded();
  return ret;
}

AREXPORT void ArServerHandlerConfig::reloadConfig(ArServerClient *client, 
                                                  ArNetPacket *packet)
{
  if (client != NULL)
    ArLog::log(ArLog::Normal, "%sReloading config for %s.", 
	       myLogPrefix.c_str(), client->getIPString());

  bool isNotify = true;

  lockConfig();
  myConfig->setPermissions(myPermissionAllowFactory, false);
  // this isn't going to cause any restarts ever, but that's
  // intentional since by definition if we're reloading the config
  // we're actively using we don't need a restart
  if (!myConfig->parseFile(myConfig->getFileName(), true)) {
    ArLog::log(ArLog::Normal,
               "ArServerHandlerConfig::reloadConfig() error parsing config file %s",
               myConfig->getFileName());

    // reloadConfig is called by internalSetConfig when the received configuration 
    // fails to load. In this case, the client and packet are both set to NULL.
    // If the received configuration and the file both fail to load, then the
    // configUpdated packet should be suppressed. Otherwise, the robot and the 
    // central server can enter a nasty infinite loop of perpetual config downloads.
    // (See Bug 13306.) 
    if ((client == NULL) && (packet == NULL)) {

      isNotify = false;
      ArLog::log(ArLog::Normal,
                 "ArServerHandlerConfig::reloadConfig() suppressing configUpdated() because received config also in error ");

    }

  } // end if error occurred parsing config file

  if (isNotify) {
    configUpdated();
  }

  unlockConfig();
}

AREXPORT void ArServerHandlerConfig::getConfigDefaults(ArServerClient *client, 
                                                       ArNetPacket *packet)
{
  char sectionRequested[512];
  sectionRequested[0] = '\0';
  ArNetPacket sending;

  if (myDefault == NULL)
  {
    ArLog::log(ArLog::Normal, 
               "%sArServerHandlerConfig::getConfigDefaults: No default config to get",
               myLogPrefix.c_str());
    client->sendPacketTcp(&sending);
    return;
  }
  myConfigMutex.lock();
  // if we have a section name pick it up, otherwise we send everything
  if (packet->getDataReadLength() < packet->getDataLength())
    packet->bufToStr(sectionRequested, sizeof(sectionRequested));

  //ArConfigArg param;

  ArClientArg clientArg;

  if (sectionRequested[0] == '\0')
    ArLog::log(ArLog::Normal, 
               "%sSending all defaults to client",
               myLogPrefix.c_str());
  else
    ArLog::log(ArLog::Normal, 
               "%sSending defaults for section '%s' to client",
               myLogPrefix.c_str(),
               sectionRequested);


  std::list<ArConfigSection *> *sections = myDefault->getSections();

  for (std::list<ArConfigSection *>::iterator sIt = sections->begin(); 
       sIt != sections->end(); 
       sIt++)
  {
    ArConfigSection *section = (*sIt);

    if (section == NULL) {
      continue; // Should never happen...
    }

    // if we're not sending them all and not in the right section just cont
    if (sectionRequested[0] != '\0' &&
        ArUtil::strcasecmp(sectionRequested, section->getName()) != 0) {
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

      sending.strToBuf(param.getName());
      clientArg.argTextToBuf(param, &sending);

    } // end for each param
  } // end for each section
  myConfigMutex.unlock();

  client->sendPacketTcp(&sending);
}


AREXPORT void ArServerHandlerConfig::addPreWriteCallback(
  ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPreWriteCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPreWriteCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "%sArServerHandlerConfig::addPreWriteCallback: Invalid position.",
               myLogPrefix.c_str());
}

AREXPORT void ArServerHandlerConfig::remPreWriteCallback(
  ArFunctor *functor)
{
  myPreWriteCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerConfig::addPostWriteCallback(
  ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPostWriteCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPostWriteCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "%sArServerHandlerConfig::addPostWriteCallback: Invalid position.",
               myLogPrefix.c_str());
}

AREXPORT void ArServerHandlerConfig::remPostWriteCallback(
  ArFunctor *functor)
{
  myPostWriteCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerConfig::addConfigUpdatedCallback(
  ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myConfigUpdatedCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myConfigUpdatedCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "%sArServerHandlerConfig::addConfigUpdatedCallback: Invalid position.",
               myLogPrefix.c_str());
}

AREXPORT void ArServerHandlerConfig::remConfigUpdatedCallback(
  ArFunctor *functor)
{
  myConfigUpdatedCallbacks.remove(functor);
}

AREXPORT bool ArServerHandlerConfig::writeConfig(void)
{
  bool ret;
  std::list<ArFunctor *>::iterator fit;

  bool origSaveUnknown = myConfig->getSaveUnknown();

  IFDEBUG(ArLog::log(ArLog::Normal,
                     "ArServerHandlerConfig::writeConfig() origSaveUnknown = %i",
                     origSaveUnknown));

  // KMC 12/11/13 I don't think that this has any effect.  Should perhaps
  // be removed in the future.
  myConfig->setSaveUnknown(true);

  if (myConfig->getFileName() != NULL && 
      strlen(myConfig->getFileName()) > 0)
  {
    // call our pre write callbacks
    for (fit = myPreWriteCallbacks.begin(); 
	       fit != myPreWriteCallbacks.end(); 
	       fit++) 
    {
      IFDEBUG(ArLog::log
                 (ArLog::Normal,
                  "ArServerHandlerConfig::writeConfig() invoking pre-write callback %s",
                  (*fit)->getName()));

      (*fit)->invoke();
    }
 
//ArLog::logBacktrace(ArLog::Normal);
    // write it 
    ArLog::log(ArLog::Normal, 
               "%sWriting config file %s", 
               myLogPrefix.c_str(),
	             myConfig->getFileName());
    ret = myConfig->writeFile(myConfig->getFileName());
    
    
    // call our post write callbacks
    for (fit = myPostWriteCallbacks.begin(); 
	 fit != myPostWriteCallbacks.end(); 
	 fit++) 
    {
      IFDEBUG(ArLog::log(ArLog::Normal,
                         "ArServerHandlerConfig::writeConfig() invoking post-write callback %s",
                         (*fit)->getName()));
      (*fit)->invoke();
    }
  
    myConfig->setSaveUnknown(origSaveUnknown);
  }
  else
  {
    ArLog::log(ArLog::Normal, "%sCannot write config since myConfig has no filename",
	       myLogPrefix.c_str());
  }
  return ret;
}

AREXPORT bool ArServerHandlerConfig::configUpdated(ArServerClient *client)
{
  ArNetPacket emptyPacket;

  std::list<ArFunctor *>::iterator fit;

  // call our post write callbacks
  for (fit = myConfigUpdatedCallbacks.begin(); 
       fit != myConfigUpdatedCallbacks.end(); 
       fit++) 
  {
    (*fit)->invoke();
  }
  // this one is okay to exclude, because if the central server is
  // managing the config then its handling those updates and what not
  return myServer->broadcastPacketTcpWithExclusion(&emptyPacket, 
						   "configUpdated", client);
}

AREXPORT void ArServerHandlerConfig::getConfigSectionFlags(
	ArServerClient *client, ArNetPacket *packet)
{
  ArLog::log(ArLog::Normal, 
             "%sConfig section flags requested.",
             myLogPrefix.c_str());

  ArNetPacket sending;

  std::list<ArConfigSection *> *sections = myConfig->getSections();
  std::list<ArConfigSection *>::iterator sIt;
  sending.byte4ToBuf(sections->size());

  for (sIt = sections->begin(); sIt != sections->end(); sIt++)
  {

    ArConfigSection *section = (*sIt);
    if (section == NULL) 
    {
      sending.strToBuf("");
      sending.strToBuf("");
    }
    else
    {
      sending.strToBuf(section->getName());
      sending.strToBuf(section->getFlags());
    }
  }
  client->sendPacketTcp(&sending);
}


AREXPORT void ArServerHandlerConfig::getLastEditablePriority
                                          (ArServerClient *client,
                                           ArNetPacket *packet)
{

  ArPriority::Priority lastPriority = ArPriority::EXPERT;
  if (myPermissionAllowFactory) {
    lastPriority = ArPriority::FACTORY;
  }
  ArLog::log(ArLog::Verbose,
             "%sArServerHandlerConfig::getLastEditablePriority returning %s",
             myLogPrefix.c_str(),
             ArPriority::getPriorityName(lastPriority));

  ArNetPacket sending;
  sending.byteToBuf(lastPriority);

  client->sendPacketTcp(&sending);

} // end method getLastEditablePriority

AREXPORT void ArServerHandlerConfig::setRestartSoftwareCB(
	ArFunctor *restartServerCB)
{
  myRestartSoftwareCB = restartServerCB;
  myRestartSoftwareCBSet = true;
}

AREXPORT ArFunctor *ArServerHandlerConfig::getRestartSoftwareCB(void)
{
  return myRestartSoftwareCB;
}

AREXPORT void ArServerHandlerConfig::setRestartHardwareCB(ArFunctor *restartRobotCB)
{
  myRestartHardwareCB = restartRobotCB;
  myRestartHardwareCBSet = true;
}

AREXPORT ArFunctor *ArServerHandlerConfig::getRestartHardwareCB(void)
{
  return myRestartHardwareCB;
}

/// Changes the variables that prevent changes
AREXPORT void ArServerHandlerConfig::setPreventChanges(bool preventChanges, const char *preventChangesString)
{
  myConfigMutex.lock();
  myPreventChanges = preventChanges;
  if (preventChangesString != NULL)
    myPreventChangesString = preventChangesString;
  else
    myPreventChangesString = "Changes prevented for unspecified reason";
  myConfigMutex.unlock();
}

/// The callbackes added via addRestartIOCB(), if any, are invoked.
AREXPORT void ArServerHandlerConfig::restartIO(const char *reason)
{
  ArLog::log(ArLog::Normal, "%sRequesting restartIO because '%s'", 
	     myLogPrefix.c_str(), reason);
  myRestartIOCBList.invoke();
}

/// Clients are notified via configCausingRestart packet containing value 1, then the restart-software callback set by setRestartSoftwareCB(), if any, is invoked 1 second after the server has finished sending pending packets.
AREXPORT void ArServerHandlerConfig::restartSoftware(const char *reason)
{
  ArLog::log(ArLog::Normal, "%sAbout to broadcast configCausingRestart (software)",
	       myLogPrefix.c_str());
  
  ArNetPacket sendPacket;
  sendPacket.byteToBuf(1);
    
  myServer->broadcastPacketTcp(&sendPacket,
			                         "configCausingRestart");


  myServer->sleepAfterSend(1000);
//  ArUtil::sleep(1* 1000);
  
  // if the functor was set to a value, call it
  if (myRestartSoftwareCB != NULL)
  {
    ArLog::log(ArLog::Normal, "%sRestarting software because '%s'",
	       myLogPrefix.c_str(), reason);
    myRestartSoftwareCB->invoke();
  }
  // if it was never set then just exit
  else if (!myRestartSoftwareCBSet)
  {
    ArLog::log(ArLog::Normal, "%sExiting because '%s' but no restartServer functor set",
	       myLogPrefix.c_str(), reason);
    Aria::exit(0);
  }
  // if it was explicitly set to NULL we do nothing (mostly for
  // the central server)
}

/// Clients are notified via configCausingRestart packet containing value 2, then the restart-hardware callback set by setRestartHardwareCB(), if any, is invoked 1 second after the server has finished sending pending packets.
AREXPORT void ArServerHandlerConfig::restartHardware(const char *reason)
{
  ArLog::log(ArLog::Normal, "%sAbout to broadcast configCausingRestart (hardware)",
	       myLogPrefix.c_str());
  
  ArNetPacket sendPacket;
  sendPacket.byteToBuf(2);
  myServer->broadcastPacketTcp(&sendPacket,
			                         "configCausingRestart");

  myServer->sleepAfterSend(1000);
//  ArUtil::sleep(1* 1000);
  
  // if the functor was set to a value, call it
  if (myRestartHardwareCB != NULL)
  {
    ArLog::log(ArLog::Normal, "%sRestarting entire robot because '%s'",
	       myLogPrefix.c_str(), reason);
    myRestartHardwareCB->invoke();
  }
  // if it was never set then just exit
  else if (!myRestartHardwareCBSet)
  {
    ArLog::log(ArLog::Normal, "%sExiting because '%s' but no restartRobot functor set",
	       myLogPrefix.c_str(), reason);
    Aria::exit(0);
  }
  // if it was explicitly set to NULL we do nothing (mostly for
  // the central server)
}

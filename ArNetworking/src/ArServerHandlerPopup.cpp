#include "Aria.h"
#include "ArExport.h"
#include "ArServerHandlerPopup.h"

ArServerHandlerPopup::PopupData::PopupData(
	ArServerHandlerPopupInfo *popupInfo, ArTypes::Byte4 id,
	ArFunctor2<ArTypes::Byte4, int> *callback) 
{
  myPopupInfo = new ArServerHandlerPopupInfo(*popupInfo); 
  myID = id;
  myCallback = callback; 
  myStarted.setToNow();
}

ArServerHandlerPopup::PopupData::~PopupData()
{
  delete myPopupInfo;
}

AREXPORT ArServerHandlerPopup::ArServerHandlerPopup(ArServerBase *server) :
  myNetPopupClickedCB(this, &ArServerHandlerPopup::netPopupClicked),
  myNetPopupListCB(this, &ArServerHandlerPopup::netPopupList),
  myServerCycleCB(this, &ArServerHandlerPopup::serverCycleCallback)
{
  myDataMutex.setLogName("ArServerHandlerPopup::myDataMutex");
  myServer = server;
  myServer->addData(
	  "popupCreate", 
	  "Tells the gui to create a new popup",
	  NULL, 
	  "none (you can't send this command to the server)",
	  "byte4: id of this particular popup; string: ignoreIdentifier; string: title; string: message; byte: popupType (0 = NoIcon, 1 = Information, 2 = Warning, 3 = Critical, 4 = Question); string: button0Label; string: button1Label; string: button2Label; byte: defaultButton; byte: escapeButton",
	  "PopupGui", "RETURN_SINGLE");
  myServer->addData(
	  "popupClose",
	  "Closes a popup that was created",
	  NULL,
	  "none (you can't send this command to the server)",
	  "byte4: id of the popup to close; string: string to display",
	  "PopupGui", "RETURN_SINGLE");
  myServer->addData(
	  "popupClicked",
	  "Returns the clicked data of a popup ",
	  &myNetPopupClickedCB,
	  "byte4: id of the popup that was clicked; byte: buttonClicked",
	  "no response",
	  "PopupGui", "RETURN_NONE");
  myServer->addData(
	  "popupList", 
	  "Resends all the popupCreates to this client that asked for this",
	  &myNetPopupListCB, 
	  "none",
	  "technically none, but it resends all the popupCreate events that are active",
	  "PopupGui", "RETURN_NONE");
  myServer->addCycleCallback(&myServerCycleCB);
  myLastTimeCheck.setToNow();
  myLastID = 0;

} 


AREXPORT ArServerHandlerPopup::~ArServerHandlerPopup()
{

}


/**
   Creates a popup, for information about whats in the popup see
   ArServerHandlerPopupInfo... this just describes what happens with
   the popup...  

   So you pass in a popupInfo for the popup you want and a functor to
   call when buttons in the popup are pushed, this returns an id for
   that popup, which you can use to cancel the popup and so you know
   which popup this is.... the callback (if there is one) will be
   called when the popup has a button pushed with the id and the
   number of the button or -1 if the popup timed out or -2 if it was
   canceled.  The popup has a timeout, so that if no clients are
   connected watching for popups or if no one is paying attention that
   things can move on easily.

   @param popupInfo the information that describes the popup... this
   class makes a copy of the information so you can do whatever you
   want with the information after you've called this function (ie
   change the button and call it again or whatever)

   @param callback The class will call this function when one of the
   buttons is pressed in the popup on one of the clients, the callback
   will be called with the long int being the ID, and the int being
   the button that is pushed (or -1 if the timeout happened or -2 if
   it was closed)
 **/
AREXPORT ArTypes::Byte4 ArServerHandlerPopup::createPopup(
	ArServerHandlerPopupInfo *popupInfo, 
	ArFunctor2<ArTypes::Byte4, int> *callback)
{
  PopupData *popupData;
  ArTypes::Byte4 retID;

  myDataMutex.lock();
  // first find a good id
  myLastID++;
  if (myLastID < 0)
    myLastID = 1;
  //printf("Checking if id %u is good\n", myLastID);
  while (myMap.find(myLastID) != myMap.end())
  {
    myLastID++;
    if (myLastID < 0)
      myLastID = 1;
  }
  //printf("Got id %d\n", myLastID);
  popupData = new PopupData(popupInfo, myLastID, callback);
  myMap[myLastID] = popupData;

  ArNetPacket sendingPacket;
  buildPacket(&sendingPacket, popupData);

  /*
  printf("!!! %s %s %s\n", 
	 popupData->myPopupInfo->getButton0Label(),
	 popupData->myPopupInfo->getButton1Label(),
	 popupData->myPopupInfo->getButton2Label());
  */
  retID = myLastID;
  myDataMutex.unlock();
  myServer->broadcastPacketTcp(&sendingPacket, "popupCreate");
  return retID;
}

void ArServerHandlerPopup::buildPacket(ArNetPacket *sendingPacket, 
				       PopupData *popupData)
{
  sendingPacket->byte4ToBuf(popupData->myID);
  sendingPacket->strToBuf(popupData->myPopupInfo->getIgnoreIdentifier());
  sendingPacket->strToBuf(popupData->myPopupInfo->getTitle());
  sendingPacket->strToBuf(popupData->myPopupInfo->getMessage());
  sendingPacket->byteToBuf((ArTypes::Byte) 
			  popupData->myPopupInfo->getPopupType());
  sendingPacket->strToBuf(popupData->myPopupInfo->getButton0Label());
  sendingPacket->strToBuf(popupData->myPopupInfo->getButton1Label());
  sendingPacket->strToBuf(popupData->myPopupInfo->getButton2Label());
  sendingPacket->byteToBuf(popupData->myPopupInfo->getDefaultButtonNumber());
  sendingPacket->byteToBuf(popupData->myPopupInfo->getEscapeButtonNumber());
}

AREXPORT void ArServerHandlerPopup::closePopup(ArTypes::Byte4 id, 
						const char *closeMessage)
{
  ArNetPacket sendingPacket;
  PopupData *popupData;
  std::map<ArTypes::Byte4, PopupData *>::iterator it;

  myDataMutex.lock();
  if ((it = myMap.find(id)) == myMap.end())
  {
    ArLog::log(ArLog::Verbose, 
       "Cannot close popup %u as it doesn't exist (anymore at least)", 
	       id);
    myDataMutex.unlock();
  }
  else
  {
    popupData = (*it).second;
    sendingPacket.byte4ToBuf(id);
    sendingPacket.strToBuf(closeMessage);
    myMap.erase(id);
    myDataMutex.unlock();
    delete popupData;
    if (popupData->myCallback != NULL)
      popupData->myCallback->invoke(popupData->myID, -2);
    myServer->broadcastPacketTcp(&sendingPacket, "popupClose");
  }
}

AREXPORT void ArServerHandlerPopup::netPopupClicked(ArServerClient *client,
						     ArNetPacket *packet)
{
  ArNetPacket sendingPacket;
  PopupData *popupData;
  std::map<ArTypes::Byte4, PopupData *>::iterator it;

  ArTypes::Byte4 id;
  ArTypes::Byte button;

  id = packet->bufToByte4();
  button = packet->bufToByte();

  myDataMutex.lock();
  if ((it = myMap.find(id)) == myMap.end())
  {
    ArLog::log(ArLog::Verbose, 
       "Cannot close popup %u for client %s as it doesn't exist (anymore at least)", 
	       id, client->getIPString());
    myDataMutex.unlock();
  }
  else
  {
    popupData = (*it).second;
    sendingPacket.byte4ToBuf(id);
    if (button == 0)
      sendingPacket.strToBuf(popupData->myPopupInfo->getButton0Pressed()); 
    else if (button == 1)
      sendingPacket.strToBuf(popupData->myPopupInfo->getButton1Pressed()); 
    else if (button == 2)
      sendingPacket.strToBuf(popupData->myPopupInfo->getButton2Pressed()); 
    else 
      sendingPacket.strToBuf("Popup closed because of odd click");
    myMap.erase(id);
    myDataMutex.unlock();
    if (popupData->myCallback != NULL)
      popupData->myCallback->invoke(popupData->myID, button);
    delete popupData;
    myServer->broadcastPacketTcp(&sendingPacket, "popupClose");
  }

  
}

AREXPORT void ArServerHandlerPopup::netPopupList(ArServerClient *client,
						 ArNetPacket *packet)
{
  ArLog::log(ArLog::Normal, "Sending popup list");

  std::map<ArTypes::Byte4, PopupData *>::iterator it;
  ArNetPacket sendingPacket;
  PopupData *popupData;

  myDataMutex.lock();
  for (it = myMap.begin(); it != myMap.end(); it++)
  {
    popupData = (*it).second;
    sendingPacket.empty();

    ArLog::log(ArLog::Normal, "Sending popup %d", popupData->myID);
    buildPacket(&sendingPacket, popupData);
    sendingPacket.setCommand(myServer->findCommandFromName("popupCreate"));

    client->sendPacketTcp(&sendingPacket);
  }

  sendingPacket.empty();
  client->sendPacketTcp(&sendingPacket);
  ArLog::log(ArLog::Normal, "Sent popups");
  myDataMutex.unlock();
}

AREXPORT void ArServerHandlerPopup::serverCycleCallback(void)
{
  std::map<ArTypes::Byte4, PopupData *>::iterator it;
  ArNetPacket sendingPacket;
  PopupData *popupData;
  int timeout;

  std::list<ArTypes::Byte4> doneIDs;
  std::list<PopupData *> donePopups;
  myDataMutex.lock();
  // only check it if we haven't checked it lately
  if (myLastTimeCheck.mSecSince() > 1000)
  {
    myLastTimeCheck.setToNow();
    for (it = myMap.begin(); it != myMap.end(); it++)
    {
      popupData = (*it).second;
      if ((timeout = popupData->myPopupInfo->getTimeout()) > 0 && 
	  popupData->myStarted.secSince() >= timeout)
      {
	sendingPacket.empty();
	sendingPacket.byte4ToBuf((*it).first);
	sendingPacket.strToBuf(popupData->myPopupInfo->getTimeoutString()); 
	myServer->broadcastPacketTcp(&sendingPacket, "popupClose");
	doneIDs.push_back((*it).first);
	donePopups.push_back(popupData);

      }
    }
  }
  while (doneIDs.begin() != doneIDs.end())
  {
    myMap.erase((*doneIDs.begin()));
    doneIDs.pop_front();
  } 
  myDataMutex.unlock();

  std::list<PopupData *>::iterator donePopupIt;
  while ((donePopupIt = donePopups.begin()) != donePopups.end())
  {
    popupData = (*donePopupIt);
    if (popupData->myCallback != NULL)
      popupData->myCallback->invoke(popupData->myID, -1);
    delete popupData;
    donePopups.pop_front();
  } 
}


AREXPORT ArServerHandlerPopupInfo::ArServerHandlerPopupInfo(
	const char *ignoreIdentifier, const char *title, const char *message, 
	ArServerHandlerPopup::PopupType popupType,  
	ArTypes::Byte defaultButtonNumber, ArTypes::Byte escapeButtonNumber,
	int timeoutInSeconds, const char *timeoutString,
	const char *button0Label, const char *button0Pressed,
	const char *button1Label, const char *button1Pressed,
	const char *button2Label, const char *button2Pressed)
{
  if (ignoreIdentifier != NULL)
    myIgnoreIdentifier = ignoreIdentifier;
  else
    myIgnoreIdentifier = "";
  if (title != NULL) 
    myTitle = title;
  else
    myTitle = "";
  if (message != NULL)
    myMessage = message;
  else
    myMessage = "";
  myPopupType = popupType; 
  myDefaultButtonNumber = defaultButtonNumber;
  myEscapeButtonNumber = escapeButtonNumber;
  myTimeout = timeoutInSeconds;
  if (timeoutString != NULL) 
    myTimeoutString = timeoutString;
  else
    myTimeoutString = "";

  if (button0Label != NULL)
    myButton0Label = button0Label;
  else
    myButton0Label = "";
  if (button0Pressed != NULL)
    myButton0Pressed = button0Pressed;
  else
    myButton0Pressed = "";
  if (button1Label != NULL)
    myButton1Label = button1Label;
  else
    myButton1Label = "";
  if (button1Pressed != NULL)
    myButton1Pressed = button1Pressed;
  else
    myButton1Pressed = "";
  if (button2Label != NULL)
    myButton2Label = button2Label;
  else
    myButton2Label = "";
  if (button2Pressed != NULL)
    myButton2Pressed = button2Pressed;
  else
    myButton2Pressed = "";

  /*
    printf("@@ 0l %s 0p %s 1l %s 1p %s 2l %s 2p%s\n", 
	 myButton0Label.c_str(), myButton0Pressed.c_str(), 
	 myButton1Label.c_str(), myButton1Pressed.c_str(), 
	 myButton2Label.c_str(), myButton2Pressed.c_str());
  */
}

AREXPORT ArServerHandlerPopupInfo::~ArServerHandlerPopupInfo()
{

}

AREXPORT ArServerHandlerPopupInfo::ArServerHandlerPopupInfo(
	const ArServerHandlerPopupInfo &popupInfo)
{
  myIgnoreIdentifier = popupInfo.myIgnoreIdentifier;
  myTitle = popupInfo.myTitle;
  myMessage = popupInfo.myMessage;
  myPopupType = popupInfo.myPopupType;
  myDefaultButtonNumber = popupInfo.myDefaultButtonNumber; 
  myEscapeButtonNumber = popupInfo.myEscapeButtonNumber;
  myTimeout = popupInfo.myTimeout;
  myTimeoutString = popupInfo.myTimeoutString;
  myButton0Label = popupInfo.myButton0Label;
  myButton0Pressed = popupInfo.myButton0Pressed;
  myButton1Label = popupInfo.myButton1Label;
  myButton1Pressed = popupInfo.myButton1Pressed;
  myButton2Label = popupInfo.myButton2Label;
  myButton2Pressed = popupInfo.myButton2Pressed;
  /*
  printf("## 0l %s 0p %s 1l %s 1p %s 2l %s 2p%s\n", 
	 myButton0Label.c_str(), myButton0Pressed.c_str(), 
	 myButton1Label.c_str(), myButton1Pressed.c_str(), 
	 myButton2Label.c_str(), myButton2Pressed.c_str());
  */
}


AREXPORT ArServerHandlerPopupInfo &ArServerHandlerPopupInfo::operator=(
	const ArServerHandlerPopupInfo &popupInfo)
{
  if (this != &popupInfo)
  {
    myIgnoreIdentifier = popupInfo.myIgnoreIdentifier;
    myTitle = popupInfo.myTitle;
    myMessage = popupInfo.myMessage;
    myPopupType = popupInfo.myPopupType;
    myDefaultButtonNumber = popupInfo.myDefaultButtonNumber; 
    myEscapeButtonNumber = popupInfo.myEscapeButtonNumber;
    myTimeout = popupInfo.myTimeout;
    myTimeoutString = popupInfo.myTimeoutString;
    myButton0Label = popupInfo.myButton0Label;
    myButton0Pressed = popupInfo.myButton0Pressed;
    myButton1Label = popupInfo.myButton1Label;
    myButton1Pressed = popupInfo.myButton1Pressed;
    myButton2Label = popupInfo.myButton2Label;
    myButton2Pressed = popupInfo.myButton2Pressed;
  }
  return *this;
}

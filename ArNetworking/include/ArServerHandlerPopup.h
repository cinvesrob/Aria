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
#ifndef ARSERVERHANDLERPOPUP_H
#define ARSERVERHANDLERPOPUP_H

#include "Aria.h"
#include "ArServerBase.h"


class ArServerClient;

class ArServerHandlerPopupInfo;

/// Class for having generic popups appear in MobileEyes (created on the server)
/**
   TODO make the callbacks actually happen
**/

class ArServerHandlerPopup
{
public:
  /// Constructor
  AREXPORT ArServerHandlerPopup(ArServerBase *server);
  /// Destructor
  AREXPORT virtual ~ArServerHandlerPopup();
  /// Creates a new popup
  AREXPORT ArTypes::Byte4 createPopup(
	  ArServerHandlerPopupInfo *popupInfo,
	  ArFunctor2<ArTypes::Byte4, int> *callback = NULL);
  /// Cancels a popup with the ID
  AREXPORT void closePopup(ArTypes::Byte4 id,
			    const char *closeMessage);
  /// The call from the network that the popup was clicked
  /// @internal
  AREXPORT void netPopupClicked(ArServerClient *client, 
				ArNetPacket *packet);
  /// The call from the network for getting the popup list
  /// @internal
  AREXPORT void netPopupList(ArServerClient *client, 
				ArNetPacket *packet);
  /// Our cycle callback
  /// @internal
  AREXPORT void serverCycleCallback(void);
  enum PopupType { 
    NOICON = 0, ///< No icon at all
    INFORMATION = 1, ///< Just an informational message
    WARNING = 2, ///< A warning 
    CRITICAL = 3, ///< A critical problem (program failure likely)
    QUESTION = 4 ///< A question
  };
protected:
  ArServerBase *myServer;
  
  class PopupData {
  public:
    /// Constructor (copies and owns popupInfo, leaves callback)
    PopupData(ArServerHandlerPopupInfo *popupInfo, 
	      ArTypes::Byte4 id,
	      ArFunctor2<ArTypes::Byte4, int> *callback);
    /// Destructor, deleted the popup info
    virtual ~PopupData();
    /// The popup info
    ArServerHandlerPopupInfo *myPopupInfo;
    /// The functor to call when its done
    ArFunctor2<ArTypes::Byte4, int> *myCallback;
    /// When we started this popup
    ArTime myStarted;
    /// The popup this was serving
    ArTypes::Byte4 myID;
    
  };
  void buildPacket(ArNetPacket *sendingPacket, PopupData *popupData);

  ArMutex myDataMutex;
  std::map<ArTypes::Byte4, PopupData *> myMap;
  ArTypes::Byte4 myLastID;
  ArTime myLastTimeCheck;
  ArFunctor2C<ArServerHandlerPopup, ArServerClient*, 
      ArNetPacket *> myNetPopupClickedCB;
  ArFunctor2C<ArServerHandlerPopup, ArServerClient*, 
      ArNetPacket *> myNetPopupListCB;
  ArFunctorC<ArServerHandlerPopup> myServerCycleCB;
};

/// Holds the information for a popup 
/**
   This holds the information for the popup....  So there's a message
   box with the title which has in it the message, and has between 1
   and 3 buttons (button0, button1, and button2) with labels of
   button0Label, button1Label, and button2Label (if the label is empty
   or NULL then there'll be no button), the default button of
   defaultButtonNumber (0 - 2) and the escape button number of
   escapeButtonNumber (0 - 2)... when a button is pushed the

   @param ignoreIdentifier The identifier to use for ignoring these
   boxes, this should be NULL or empty if you don't want this
   particular box to be able to be ignored (if any popup with this
   identifier is already being ignored this one will be too)

   @param title The title of the box (displayed in the titlebar)

   @param message The string that will be displayed in the message box
   (the point of the whole thing)

   @param popupType The type of popup this is, which controls the icon
   displayed, may someday affect behavior

   @param defaultButtonPressed The button that enter defaults to (This
   should be whatever is most likely)

   @param escapeButtonPressed The button that escape defaults to (this
   should be doesn't change the state, ie like cancel), this also
   should be whats returned if the X is hit.

   @param button0Label The label that is displayed on button 0, the
   leftmost button
   
   @param timeoutInSeconds the number of seconds we should give people
   to respond to the popup before timing it out, 0 means leave it up
   forever (note that no clients may be watching this or no one may be
   at the console and use this option wisely (way too many untimed out
   popups could bog down the server))
   
   @param timeoutString The string that will be displayed if a timeout
   happens
   
   @param button0Pressed The string that will be put into the box if
   button0 is pressed (this is mainly so that with multiple clients
   connected the other clients will get feedback)

   @param button1Label The label that is displayed on button 1 (the
   middle button if there are 3, right button if there are two)

   @param button1Pressed The string that will be put into the box if
   button1 is pressed (this is mainly so that with multiple clients
   connected the other clients will get feedback)

   @param button2Label The label that is displayed on button 2 (the
   right button)
   
   @param button2Pressed The string that will be put into the box if
   button2 is pressed (this is mainly so that with multiple clients
   connected the other clients will get feedback)
 **/
class ArServerHandlerPopupInfo
{
public:
  /// Constructor
  AREXPORT ArServerHandlerPopupInfo(
	  const char *ignoreIdentifier, const char *title, 
	  const char *message, ArServerHandlerPopup::PopupType popupType, 
	  ArTypes::Byte defaultButtonNumber, 
	  ArTypes::Byte escapeButtonNumber,  
	  int timeoutInSeconds, const char *timeoutString,
	  const char *button0Label, const char *button0Pressed, 
	  const char *button1Label = "", const char *button1Pressed = "", 
	  const char *button2Label = "", const char *button2Pressed = "");

  /// Destructor
  AREXPORT virtual ~ArServerHandlerPopupInfo();
  /// Copy constructor
  AREXPORT ArServerHandlerPopupInfo(const ArServerHandlerPopupInfo &popupInfo);
  /// Assignment operator
  AREXPORT ArServerHandlerPopupInfo &operator=(
	  const ArServerHandlerPopupInfo &popupInfo);

  /// Gets the popup identifer (this is used only for ignoring popups, if empty or NULL then it can't be ignored)
  const char *getIgnoreIdentifier(void) { return myIgnoreIdentifier.c_str(); }
  /// Gets the title (the title of the popup box)
  const char *getTitle(void) { return myTitle.c_str(); }
  /// Gets the message (the long string that is displayed that explains things)
  const char *getMessage(void) { return myMessage.c_str(); }
  /// Gets the type (the icon thats displayed and what type of popup it is)
  ArServerHandlerPopup::PopupType getPopupType(void) { return myPopupType; }
  /// Gets the default button number (whats pressed when enter is hit)
  ArTypes::Byte getDefaultButtonNumber(void) { return myDefaultButtonNumber; }
  /// Gets the escape button number (whats pressed when escape is hit)
  ArTypes::Byte getEscapeButtonNumber(void) { return myEscapeButtonNumber; }
  /// Gets the timeout in seconds (0 is never)
  int getTimeout(void) { return myTimeout; }
  /// Gets the timeout string (the string that is displayed on the popup if timeout occurs)
  const char * getTimeoutString(void) 
    { return myTimeoutString.c_str(); }
  /// Gets the button0Label (the label on the leftmost button, must have a label)
  const char *getButton0Label(void) { return myButton0Label.c_str(); }
  /// Gets the button0Pressed (string sent as box disppears if this button is pressed)
  const char *getButton0Pressed(void) { return myButton0Pressed.c_str(); }

  /// Gets the button1Label (the label on the middle button, empty string or NULL for no button)
  const char *getButton1Label(void) { return myButton1Label.c_str(); }
  /// Gets the button1Pressed (string sent as box disppears if this button is pressed)
  const char *getButton1Pressed(void) { return myButton1Pressed.c_str(); }
  /// Gets the button2Label (the label on the right button, empty string or NULL for no button)
  const char *getButton2Label(void) { return myButton2Label.c_str(); }
  /// Gets the button2Pressed (string sent as box disppears if this button is pressed)
  const char *getButton2Pressed(void) { return myButton2Pressed.c_str(); }

  /// Gets the popup identifer (this is used only for ignoring popups, if empty or NULL then it can't be ignored)
  void setIgnoreIdentifier(const char *identifier) 
    { if (identifier != NULL) myIgnoreIdentifier = identifier; else myIgnoreIdentifier = ""; }
  /// Sets the title (the title of the popup box)
  void setTitle(const char *title) 
    { if (title != NULL) myTitle = title; else myTitle = ""; }
  /// Sets the message (the long string that is displayed that explains things)
  void setMessage(const char *message) 
    { if (message != NULL) myMessage = message; else myMessage = ""; }
  /// Sets the type (the icon thats displayed and what type of popup it is)
  void setPopupType(ArServerHandlerPopup::PopupType popupType) 
    { myPopupType = popupType; }
  /// Sets the default button number (whats pressed when enter is hit)
  void setDefaultButtonNumber(ArTypes::Byte defaultButtonNumber) 
    { myDefaultButtonNumber = defaultButtonNumber; }
  /// Sets the escape button number (whats pressed when escape is hit)
  void setEscapeButtonNumber(ArTypes::Byte escapeButtonNumber) 
    { myEscapeButtonNumber = escapeButtonNumber; }
  /// Sets the timeout in seconds (0 is never)
  void setTimeout(int timeoutInSeconds)
    { myTimeout = timeoutInSeconds; }
  /// Sets the timeout string (the string that is displayed on the popup if timeout occurs)
  void setTimeoutString(const char *timeoutString) 
    { if (timeoutString != NULL) myTimeoutString = timeoutString; else myTimeoutString = ""; }
  /// Sets the button0Label (the label on the leftmost button, must have a label)
  void setButton0Label(const char *label) 
    { if (label != NULL) myButton0Label = label; else myButton0Label = ""; }
  /// Sets the button0Pressed (string sent as box disppears if this button is pressed)
  void setButton0Pressed(const char *pressed) 
    { if (pressed != NULL) myButton0Pressed = pressed; else myButton0Pressed = ""; }
  /// Sets the button1Label (the label on the middle button, empty string or NULL for no button)
  void setButton1Label(const char *label) 
    { if (label != NULL) myButton1Label = label; else myButton1Label = ""; }
  /// Sets the button1Pressed (string sent as box disppears if this button is pressed)
  void setButton1Pressed(const char *pressed) 
    { if (pressed != NULL) myButton1Pressed = pressed; else myButton1Pressed = ""; }
  /// Sets the button2Label (the label on the right button, empty string or NULL for no button)
  void setButton2Label(const char *label) 
    { if (label != NULL) myButton2Label = label; else myButton2Label = ""; }
  /// Sets the button2Pressed (string sent as box disppears if this button is pressed)
  void setButton2Pressed(const char *pressed) 
    { if (pressed != NULL) myButton2Pressed = pressed; else myButton2Pressed = ""; }

protected:
  std::string myIgnoreIdentifier;
  std::string myTitle;
  std::string myMessage;
  ArServerHandlerPopup::PopupType myPopupType;
  ArTypes::Byte myDefaultButtonNumber;
  ArTypes::Byte myEscapeButtonNumber;
  int myTimeout;
  std::string myTimeoutString;
  std::string myButton0Label;
  std::string myButton0Pressed;
  std::string myButton1Label;
  std::string myButton1Pressed;
  std::string myButton2Label;
  std::string myButton2Pressed;
};

#endif // ARSERVERHANDLERPOPUP_H

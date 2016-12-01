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
/*! 
 *  \brief Defines the ArClientArg class.
 *  \date 05/01/05
 *  \author K. Cunningham
 * 
**/
#ifndef ARCLIENTARGUTILS_H
#define ARCLIENTARGUTILS_H

#include "Aria.h"

class ArNetPacket;
class ArConfigArg;

/// Small helper class for sending/receiving an ArConfigArg in an ArNetPacket.
/**
 * ArClientArg defines methods for packing/unpacking an ArConfigArg into/from
 * an ArNetPacket.  (The name of the class is a slight misnomer since it may
 * be used both on the server and client sides.)
 * <p>
 * The structure of the network packet information is as follows:
 * <pre>
 *    string: arg.getName()
 *    string: arg.getDescription()
 *    byte: arg.getConfigPriority()
 *    byte: arg type ('B' == BOOL | 'I' == INT | 'D' == DOUBLE | 'S' == STRING | 'L' == LIST)
 *    &lt;arg values&gt;
 *    string: arg.getDisplayHint()      -- only if isDisplayHintParsed is set to true
 *    string: arg.getExtraExplanation() -- only if version >= 2
 *    byte: arg.getRestartLevel()       -- only if version >= 2
 *
 *    ubyte: arg.getIsSerializable()    -- only if version >= 4
 *    string: "" | arg.getParentPathName()   -- only if version >= 4 , parent included only if isSingleParam set to true 
 * 
 *    &lt;arg values&gt; varies by arg type:
 *
 *    if BOOL, then:
 *        byte: arg.getBool()
 *    if INT, then:
 *	      byte4: arg.getInt()
 *		    byte4: arg.getMinInt()
 *		    byte4: arg.getMaxInt()
 *    if DOUBLE, then:
 *	      byte4: arg.getDouble()
 *		    byte4: arg.getMinDouble()
 *		    byte4: arg.getMaxDouble()
 *        byte4: arg.getDoublePrecision()  -- only if version >= 2
 *    if STRING, then:
 *        string: arg.getString()
 *    if LIST, then:
 *        byte4: arg.getArgCount()
 *        &lt;list contents&gt;
 *        
 *
 *    
 * </pre>
 * <p>
 * ArClientArg also defines methods to send an "abbreviated" ArConfigArg 
 * (i.e. just value).  The short packet structure is as follows:
 * <pre>
 *    &lt;arg value&gt; varies by arg type:
 *    if BOOL, then:
 *        byte: arg.getBool()
 *    if INT, then:
 *	      byte4: arg.getInt()
 *    if DOUBLE, then:
 *	      byte4: arg.getDouble()
 *    if STRING, then:
 *        string: arg.getString()
 *    if LIST, then:
 *        <TODO>
 *        &lt;list contents&gt;
 * </pre>
 * Lastly, it defines a method to send an "abbreviated" ArConfigArg in a
 * text format.  
**/
class ArClientArg
{
public:

	/// Constructor
	AREXPORT ArClientArg(bool isDisplayHintParsed = false, 
                       ArPriority::Priority lastPriority = ArPriority::LAST_PRIORITY,
                       int version = 1,
                       bool isSingleParam = false); 
	
	/// Destructor
	AREXPORT virtual ~ArClientArg();

  /// Returns whether the given parameter can be sent in a network packet.
  /**
   * Currently, a parameter can only be sent if it is of type INT, DOUBLE,
   * STRING, BOOL, LIST, or a SEPARATOR.
   * @param arg the ArConfigArg to be checked for "sendability"
   * @param isIncludeSeparator a bool set to true if separators should be sent;
   * (this is currently set to false for the text socket commands)
  **/
  AREXPORT static bool isSendableParamType(const ArConfigArg &arg,
                                           bool isIncludeSeparator = true);
	
	/// Unpacks the given network packet and stores the data in the config arg.
	/**
	 * @param packet the ArNetPacket * from which data is extracted
	 * @param argOut the ArConfigArg in which to set the data
	 * @return bool set to true if the data was successfully extracted from
	 * the packet; false if an error occurred and argOut is invalid
	**/
	AREXPORT virtual bool createArg(ArNetPacket *packet, 
						                      ArConfigArg &argOut,
                                  std::string *parentPathNameOut = NULL);


	/// Stores the given config arg into the network packet. 
	/**
	 * @param arg the ArConfigArg from which to retrieve the data
	 * @param packet the ArNetPacket * to which data is added
	 * @return bool set to true if the data was successfully stored in
	 * the packet; false if an error occurred and the packet is invalid
	**/
  AREXPORT virtual bool createPacket(const ArConfigArg &arg,
                                     ArNetPacket *packet,
                                     const char *parentPathName = NULL);



	/// Unpacks the abbreviated arg value and stores the data in the config arg.
	/**
	 * @param packet the ArNetPacket * from which data is extracted
	 * @param arg the ArConfigArg in which to set the value
	 * @return bool set to true if the data was successfully extracted from
	 * the packet; false if an error occurred and arg is invalid
	**/
  AREXPORT virtual bool bufToArgValue(ArNetPacket *packet,
                                      ArConfigArg &arg);

  /// Stores the abbreviated arg value into the network packet.
  /**
   * @param arg the ArConfigArg from which to retrieve the data
 	 * @param packet the ArNetPacket * to which data is added
	 * @return bool set to true if the data was successfully stored in
	 * the packet; false if an error occurred and the packet is invalid
 **/
	AREXPORT virtual bool argValueToBuf(const ArConfigArg &arg,
                                      ArNetPacket *packet);



  /// Stores the arg value into the network packet as a text string.
  /**
   * The stored text string is suitable for parsing by an ArArgumentBuilder.
   *
   * @param arg the ArConfigArg from which to retrieve the data
 	 * @param packet the ArNetPacket * to which data is added
	 * @return bool set to true if the data was successfully stored in
	 * the packet; false if an error occurred and the packet is invalid
 **/
	AREXPORT virtual bool argTextToBuf(const ArConfigArg &arg,
                                     ArNetPacket *packet);

  AREXPORT virtual bool addArgTextToPacket(const ArConfigArg &arg,
                                           ArNetPacket *packet);

  AREXPORT virtual bool addAncestorListToPacket(const std::list<ArConfigArg*> &ancestorList,
                                                ArNetPacket *packet);

  AREXPORT virtual bool addListBeginToPacket(ArConfigArg *parentArg,
                                             ArNetPacket *packet);
  AREXPORT virtual bool addListEndToPacket(ArConfigArg *parentArg,
                                           ArNetPacket *packet);

protected:

  enum {
    BUFFER_LENGTH = 1024
  };

  bool myIsDisplayHintParsed;
  ArPriority::Priority myLastPriority;
  int myVersion;
  bool myIsSingleParam;

  char myBuffer[BUFFER_LENGTH];
  char myDisplayBuffer[BUFFER_LENGTH];
  char myExtraBuffer[BUFFER_LENGTH];
  char myParentPathNameBuffer[BUFFER_LENGTH];

}; // end class ArClientArgUtils
 
#endif //ARCLIENTARGUTILS_H

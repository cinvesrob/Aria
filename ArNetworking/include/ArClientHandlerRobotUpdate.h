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
#ifndef ARCLIENTHANDLERROBOTUPDATE_H
#define ARCLIENTHANDLERROBOTUPDATE_H

#include "Aria.h"
#include <string>

class ArNetPacket;
class ArClientBase;

/** Receives robot "update" responses from server and stores them. 
    After creating this object, call requestUpdates() to begin receiving
    updates from the server.  You can use the  accessors for robot data
    to get copies of the most recently received data. Use lock() and
    unlock() before and after using these accessors.  You can also
    register a callback function which is called as soon as robot data is 
    received, or whenever the server's mode or status strings change. 
    These callbacks will be called from the ArNetworking
    client thread (managed by ArClientBase), so these callbacks must 
    not lock any mutex used by that thread, and they must return quickly to
    avoid stalling/blocking the ArNetworking client thread. To avoid blocking
    the ArNetworking thread, monitor the state of ArClientHandlerRobotUpdate
    from your programs main thread or from another monitor thread, or if using
    ARNL, use RemoteArnlTask.

    If the server supports separate "updateNumbers" and "updateStrings"
    requests, then ArClientHandlerRobotUpdate uses these requests when
    requestUpdates() is called. Otherwise, it uses the older combined "update"
    request.  (In either case, status and mode callbacks are only called if 
    status/mode strings also change value.)

    @todo Determining whether battery data is state of charge or voltage is not
    yet implemented.  It is currently assumed to be voltage.
*/
class ArClientHandlerRobotUpdate
{
public:
#ifndef SWIG
	struct RobotData
	{
		ArPose pose;
		bool haveStateOfCharge; ///< Whether stateOfCharge is a valid value or not.
		bool haveVoltage; ///< Whether voltage is a valid value or not.
		double stateOfCharge;
		double voltage;
		double vel;
		double rotVel;
		double latVel;
		bool haveTemperature; ///< Whether temperature is a valid value or not.
		double temperature;
		RobotData() :
			haveStateOfCharge(false),
			haveVoltage(false),
			stateOfCharge(0.0),
			voltage(0.0),
			vel(0.0),
			rotVel(0.0),
			latVel(0.0),
			haveTemperature(false),
			temperature(0.0)
		{}
	};
#endif

  /** @arg client Pointer to an ArClientBase object, which will be used to
   * request and receive 'updateStrings' and 'updateNumbers'; if these requests
   * are unavailable, then the older 'update' request is used instead. 
   */
	AREXPORT ArClientHandlerRobotUpdate(ArClientBase *client);
	AREXPORT ~ArClientHandlerRobotUpdate();

  /** Request updates at the given rate.
     @arg freq Rate (miliseconds) at which to request the server send updates.  If not given, default is 100ms.
  */
	AREXPORT void requestUpdates(int freq = 100);

  /** Stop receiving updates */
	AREXPORT void stopUpdates();

  /** Add a callback function which is invoked if a new, different status
    * string is received from the server. The callback is passed the mode and
    * the status. */
	void addStatusChangedCB(ArFunctor2<const char*, const char*> *cb) { myStatusChangedCBList.addCallback(cb); }

  /** Remove status-changed callback function */
	void remStatusChangedCB(ArFunctor2<const char*, const char*> *cb) { myStatusChangedCBList.remCallback(cb); }

  /** Add a callback function which is invoked if a new, different status
    * string is received from the server. */
	void addModeChangedCB(ArFunctor1<const char*> *cb) { myModeChangedCBList.addCallback(cb); }

  /** Remove status-changed callback function */
	void remModeChangedCB(ArFunctor1<const char*> *cb) { myModeChangedCBList.remCallback(cb); }

#ifndef SWIG
  /** Add a callback function which is called with robot data received in every update
   * from the server. @see RobotData */
	void addUpdateCB(ArFunctor1<RobotData> *cb) { myUpdateCBList.addCallback(cb); }

  /** Remove data update callback function */
	void remUpdateCB(ArFunctor1<RobotData> *cb) { myUpdateCBList.remCallback(cb); }
#endif

  /** Lock mutex used to protect robot data, status and mode while new data is
   * stored from an update from the server, in the ArNetwork client thread.
   * Call lock() before accessing data using getData() or other accessor
   * functions below, and call unlock() when done.  Hold the lock for as short a
   * time as possible to avoid stalling the ArNetworking thread.  You can copy 
   * data returned by getData() to a new RobotData object, then unlock.
   */
	void lock() { myMutex.lock(); }

  /** Unlock mutex used to protect robot data, status and mode, which was locked
   * with lock(). */
	void unlock() { myMutex.unlock(); }

  /** Get a copy of robot position, status, etc. which was most recently received
   * from the server. */
  //@{
#ifndef SWIG
	RobotData getData() { return myData; }
#endif
	double getX() { return myData.pose.getX(); } 
	double getY() { return myData.pose.getY(); }
	double getTh() { return myData.pose.getTh(); }
	ArPose getPose() { return myData.pose; }
	double getVel() { return myData.vel; } 
	double getRotVel() { return myData.rotVel; }
	double getLatVel() { return myData.latVel; }
  /** Whether battery status was provided as state of charge or not. If true,
   * getStateOfCharge() provides a valid state of charge percentage value. If
   * false, getStateOfCharge() should not be used. */
	bool haveStateOfCharge() { return myData.haveStateOfCharge; }
	double getStateOfCharge() { return myData.stateOfCharge; }
  /** Whether battery status was provided as voltage or not. If true,
   * getVoltage() provides a valid voltage value. If
   * false, getVoltage() should not be used. */
	bool haveVoltage() { return myData.haveVoltage; }
	double getVoltage() { return myData.voltage; }
	double getTemperature() { return myData.temperature; }
  //@}
  
  /** Get pointer to status string most recently received from server. Use
   * lock() and unlock() to protect from changes by other threads; you must copy
   * this string if you wish to store it.
   */
	const char *getStatus() { return myStatus.c_str(); }

  /** Get pointer to mode string most recently received from server. Use
   * lock() and unlock() to protect from changes by other threads; you must copy
   * this string if you wish to store it. 
   */
	const char *getMode() { return myMode.c_str(); }

  AREXPORT bool waitForStatus(const char *status, long timeout); 
  AREXPORT bool waitForMode(const char *mode, long timeout);

protected:
	void handleUpdateOld(ArNetPacket *pkt);
	void handleUpdateNumbers(ArNetPacket *pkt);
	void handleUpdateStrings(ArNetPacket *pkt);
	void parseData(ArNetPacket *pkt);
	ArClientBase *myClient;
	ArMutex myMutex;
	RobotData myData;
	std::string myStatus;
	std::string myMode;
	ArFunctor1C<ArClientHandlerRobotUpdate,  ArNetPacket*> myHandleUpdateStringsCB;
	ArFunctor1C<ArClientHandlerRobotUpdate, ArNetPacket*> myHandleUpdateNumbersCB;
	ArFunctor1C<ArClientHandlerRobotUpdate, ArNetPacket*> myHandleUpdateOldCB;
	ArCallbackList2<const char*, const char*> myStatusChangedCBList;
	ArCallbackList1<const char*> myModeChangedCBList;
	ArCallbackList1<RobotData> myUpdateCBList;
  int myRequestFreq;
};

#endif

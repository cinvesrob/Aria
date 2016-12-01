/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2014 Adept Technology

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
/* A simple Java program that demonstrates using a SICK laser rangefinder
 * as well as sonar on the robot, and using a predefined ArAction from
 * the ARIA library to avoid obstacles.
 *
 * NOTE you currently must change the call to configureShort() below if 
 * you are using the simulator.  This is a bug in the Java wrapper.
 */

import com.mobilerobots.Aria.*;

public class laserWander {

  static {
    try {
        System.loadLibrary("AriaJava");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library libAriaJava) failed to load. Make sure that its directory is in your library path; See javaExamples/README.txt and the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {
    System.out.println("Starting Java Laser Example");
    
    Aria.init(Aria.SigHandleMethod.SIGHANDLE_THREAD, true);

    // Main robot object
    ArRobot robot = new ArRobot("robot1", true, true, true);

    // Range device
    ArSick laser = new ArSick();
    ArSonarDevice sonar = new ArSonarDevice();
    robot.addRangeDevice(sonar);
    robot.addRangeDevice(laser);

    // Connect to robot
    ArSimpleConnector conn = new ArSimpleConnector(argv);
    if(!Aria.parseArgs())
    {
      System.err.println("Error parsing arguments (defaults or command line). Exiting.");
      Aria.logOptions();
      System.exit(1);
    }
    if (!conn.connectRobot(robot))
    {
      System.err.println("Could not connect to robot, exiting.\n");
      System.exit(1);
    }

    // Configure and connect to laser. The first argument determines whether
    // to use simulated laser connection over the TCP port, or to try to connect
    // to a real SICK on the serial port.
    conn.setupLaser(laser);
    //laser.configureShort(false /* Use Simulator? */, ArSick.BaudRate.BAUD38400, ArSick.Degrees.DEGREES180, ArSick.Increment.INCREMENT_ONE);
    laser.runAsync();
    if(!laser.blockingConnect())
    {
      System.err.println("Could not connect to a laser, it won't be used.");
    }
    else
    {
    }
    
    // Add actions
    ArActionBumpers bumpers = new ArActionBumpers();
    ArActionLimiterForwards limiter = new ArActionLimiterForwards("speed limiter", 300, 600, 250, 1.1);
    ArActionTurn turn = new ArActionTurn();
    ArActionConstantVelocity constantVel = new ArActionConstantVelocity("constant velocity", 400);
    robot.addAction(bumpers, 75);
    robot.addAction(limiter, 49);
    robot.addAction(turn, 30);
    robot.addAction(constantVel, 20);

    
    // Let program run forever until cancelled
    System.out.println("Wandering...");
    robot.enableMotors();
    robot.run(true);
    System.out.println("Robot disconnected. Shutting down and exiting.");
    Aria.exit(0);
  }
}

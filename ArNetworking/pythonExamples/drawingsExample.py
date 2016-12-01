from AriaPy import *
from ArNetworkingPy import *
import sys
from math import sin


# This is an example server that shows how to draw arbitrary figures in a
# client (e.g. MobileEyes).


# These are callbacks that respond to client requests for the drawings' 
# geometry data. 


def exampleHomeDrawingNetCallback(client, requestPkt):
  print "exampleHomeDrawingNetCallback"

  reply = ArNetPacket()

  # 7 Vertices
  reply.byte4ToBuf(7)

  # Centered on 0,0.
  # X:                    Y:
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(500);   # Vertex 1
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(-500);  # Vertex 2
  reply.byte4ToBuf(500);   reply.byte4ToBuf(-500);  # Vertex 3
  reply.byte4ToBuf(500);   reply.byte4ToBuf(500);   # Vertex 4
  reply.byte4ToBuf(0);     reply.byte4ToBuf(1000);  # Vertex 5
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(500);   # Vertex 6
  reply.byte4ToBuf(500);   reply.byte4ToBuf(500);   # Vertex 7

  client.sendPacketUdp(reply)
  print "exampleHomeDrawingNetCallback Done."

def exampleDotsDrawingNetCallback(client, requestPkt):
  reply = ArNetPacket()

  tik = ArUtil_getTime() % 200
  t = tik / 5.0

  # Three dots
  reply.byte4ToBuf(3)

  # Dot 1:
  reply.byte4ToBuf(3000);  # X coordinate (mm)
  reply.byte4ToBuf((int) (sin(t) * 1000));# Y

  # Dot 2:
  reply.byte4ToBuf(3500);  # X
  reply.byte4ToBuf((int) (sin(t+500) * 1000));# Y

  # Dot 3:
  reply.byte4ToBuf(4000);  # X
  reply.byte4ToBuf((int) (sin(t+1000) * 1000));# Y

  client.sendPacketUdp(reply)

def exampleXDrawingNetCallback(client, requestPkt):
  reply = ArNetPacket()

  # X marks the spot. 2 line segments, so 4 vertices:
  reply.byte4ToBuf(4)

  # Segment 1:
  reply.byte4ToBuf(-4250); # X1
  reply.byte4ToBuf(250);   # Y1
  reply.byte4ToBuf(-3750); # X2
  reply.byte4ToBuf(-250);  # Y2

  # Segment 2:
  reply.byte4ToBuf(-4250); # X1
  reply.byte4ToBuf(-250);  # Y1
  reply.byte4ToBuf(-3750); # X2
  reply.byte4ToBuf(250);   # Y2
  
  client.sendPacketUdp(reply)

def exampleArrowsDrawingNetCallback(client, requestPkt):
  # 1 arrow that points at the robot
  reply = ArNetPacket()
  reply.byte4ToBuf(1)       # 1 arrow
  reply.byte4ToBuf(0);      # Pos. X
  reply.byte4ToBuf(700);    # Pos. Y
  client.sendPacketUdp(reply)




# Main program:

Aria_init()
robot = ArRobot()
server = ArServerBase()

parser = ArArgumentParser(sys.argv)
simpleConnector = ArSimpleConnector(parser)
simpleOpener = ArServerSimpleOpener(parser)

parser.loadDefaultArguments()

if not Aria_parseArgs() or not parser.checkHelpAndWarnUnparsed():
  Aria_logOptions()
  Aria_exit(1)


if not simpleOpener.open(server):
  if simpleOpener.wasUserFileBad():
    print "Error: Bad user/password/permissions file."
  else:
    print "Error: Could not open server port. Use -help to see options."
  Aria_exit(1)


# Devices
sonarDev = ArSonarDevice()
robot.addRangeDevice(sonarDev)

irs = ArIRs()
robot.addRangeDevice(irs)

bumpers = ArBumpers()
robot.addRangeDevice(bumpers)

sick = ArSick()
robot.addRangeDevice(sick);  


# attach services to the server
serverInfoRobot = ArServerInfoRobot(server, robot)
serverInfoSensor = ArServerInfoSensor(server, robot)

# This is the service that provides drawing data to the client.
drawings = ArServerInfoDrawings(server)

# Convenience function that sets up drawings for all the robot's current
# range devices (using default shape and color info)
drawings.addRobotsRangeDevices(robot)

# Add our custom drawings

linedd = ArDrawingData("polyLine", ArColor(255, 0, 0), 2,      49) # shape name, color, size, layer
drawings.addDrawing( linedd, "exampleDrawing_Home", exampleHomeDrawingNetCallback)

dotsdd = ArDrawingData("polyDots", ArColor(0, 255, 0), 250, 48)
drawings.addDrawing(dotsdd, "exampleDrawing_Dots", exampleDotsDrawingNetCallback)

segdd =  ArDrawingData("polySegments", ArColor(0, 0, 0), 4, 52)
drawings.addDrawing( segdd, "exampleDrawing_XMarksTheSpot", exampleXDrawingNetCallback)

ardd =  ArDrawingData("polyArrows", ArColor(255, 0, 255), 500, 100)
drawings.addDrawing( ardd, "exampleDrawing_Arrows", exampleArrowsDrawingNetCallback)

# modes for moving the robot
modeStop = ArServerModeStop(server, robot)
modeDrive = ArServerModeDrive(server, robot)
modeRatioDrive = ArServerModeRatioDrive(server, robot)
modeWander = ArServerModeWander(server, robot)
modeStop.addAsDefaultMode()
modeStop.activate()

# set up some simple commands ("custom commands")
commands = ArServerHandlerCommands(server)
uCCommands = ArServerSimpleComUC(commands, robot)
loggingCommands = ArServerSimpleComMovementLogging(commands, robot)
configCommands = ArServerSimpleComLogRobotConfig(commands, robot)

# add the commands to enable and disable safe driving to the simple commands
modeDrive.addControlCommands(commands)



# Connect to the robot.
if not simpleConnector.connectRobot(robot):
  print "Error: Could not connect to robot... exiting"
  Aria_exit(1)

# set up the laser before handing it to the laser mode
simpleConnector.setupLaser(sick)

robot.enableMotors()

# start the robot cycle running in a background thread
robot.runAsync(True)

# start the laser processing cycle in a background thread
sick.runAsync()

# connect the laser if it was requested
if not simpleConnector.connectLaser(sick):
  print "Error: Could not connect to laser... exiting"
  Aria_exit(2)

# log whatever we wanted to before the runAsync
simpleOpener.checkAndLog()

# run the server thread in the background
server.runAsync()

print "Server is now running on port %d..." % (simpleOpener.getPort())

robot.waitForRunExit()
Aria_exit(0)


Example programs that show how to use the ArNetworking library to write 
either a server or client program.  ARNL also includes examples that combine
ArNetworking services with ARNL.  MobileEyes can be used as a client
to connect to any ArNetworking server.   The ArVideo library includes examples
that use ArVideo and ArNetworking together to provide video camera images
and configuration.

simpleServerExample - Simplest possible ArNetworking server. The basic server
  is created but it does not connect to a robot or other devices and provides
  no servicel.

simpleRobotServerExample - Basic example of an ArNetworking server that connects
  to the robot and provides a bit of information about it.

simpleClientExample - Simplest possible ArNetworking client. The client connects
  to a server but makes no requests.

serverDemo - A more complete server that will show the setting up of a
  server with robot sensor visualization, robot control, and more... 
  connect with MobileEyes. 

clientDemo - A more complete client that can be used to teleoperate a 
  robot if it provides the ratioDrive interface. (E.g. use with serverDemo,
  arnlServer, etc.)

clientCommandLister - A tool that connects to the server then lists the
  data requests that the server has available. 

configClient - A client that will get the ArConfig information from the server

configClientToServer - A client that will send ArConfig information from
  a config file to the server

configServer - A server that takes the ArConfig information and
  makes it available to the client, use in conjunction with configClient

configServerRobot - Serves up a robot parameter file to edit (example)

drawingsExample - Shows how to display custom graphics (drawings) in 
  a client

drawingsExampleWithRobot - Shows how to display custom graphics (drawings)
  in a client, along with the robot and its sensors.

popupExample - Server that shows how to send information to the client that
  should be shown to the user in some kind of popup or dialog box, with 
  a response sent back to the server.

robotUpdateExample - Client that receives robot state updates (position,
  status, mode, etc.)

clientRatioDRiveExample - Client that drives the robot forward, back, left
  and right in turn, showing how to use the ArClientRatioDrive class to
  teleoperate the robot.

getVideoExample - Client that requests camera images

ptzCameraClientExample - Client that requests PTZ or PTU movement



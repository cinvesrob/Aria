#include "Aria.h"
#include "ArNetworking.h"
#include <math.h>


/** @example drawingsExample.cpp Example showing how to draw custom graphics in
 * clients such as MobileEyes
 *
 * This is an example server that shows how to draw arbitrary figures in a
 * client (e.g. MobileEyes) via ArServerInfoDrawings.  It draws some lines, "arrows", and moving dots
 * with various sizes and colors.   You can use these drawings for debugging
 * or visualization, for example, to represent sensor readings. In fact,
 * specific support for ArRobot, ArSick, ArSonarDevice, ArIRs and ArBumpers
 * are built in to ArServerInfoDrawings: see drawingsExampleWithRobot.cpp
 * or serverDemo.cpp.
 *
 * @sa drawingsExampleWithRobot.cpp
 */


/* These are some callbacks that respond to client requests for the drawings' 
 * geometry data. */
void exampleHomeDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);
void exampleDotsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);
void exampleXDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);
void exampleArrowsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);


/* This class shows how to encapsulate the various aspects of a drawing into one
 * object that other parts of the program can easily access in a more general
 * (and thread-safe) way. It lets you change position, radius, resolution
 * (number of dots), and dot drawing properties.
 * The default radius is 1 meter and the default number of points is 360.
 *
 * In this example program, the radius is updated periodically by a loop in the
 * main thread (see main() function), while ArNetworking requests are handled
 * in the separate ArNetworking server thread, calling drawingServerCB() callback (this is 
 * registered with the server when we register the drawing with the
 * ArServerInfoDrawings object).  So note the use of an ArMutex object to
 * control this asyrchronous access to the reply packet.
 */
class Circle
{
private:
  ArNetPacket myReply;
  ArMutex myMutex; // mutex around myReply
  ArPose myPos;
  double myRadius;
  unsigned int myNumPoints;
  void drawingServerCB(ArServerClient *client, ArNetPacket *pkt);
  ArFunctor2C<Circle, ArServerClient*, ArNetPacket*> callback;
  void regenerate();
public:
  Circle(ArServerInfoDrawings *drawingsServer, const std::string& name, ArDrawingData *drawData);
  ~Circle();

  void setPos(const ArPose& p) 
  {
    myPos = p;
    regenerate();
  }

  void setRadius(double r)
  {
    myRadius = r;
    regenerate();
  }

  void setNumPoints(unsigned int r)
  {
    myNumPoints = r;
    regenerate();
  }

};

Circle::Circle(ArServerInfoDrawings *drawingsServer, const std::string& name, ArDrawingData* drawData) :
  myRadius(1000.0),
  myNumPoints(360),
  callback(this, &Circle::drawingServerCB)
{
  drawingsServer->addDrawing(drawData, name.c_str(), &callback);
}

Circle::~Circle() 
{
  // TODO remove drawing from the server, but not implemented yet
}

void Circle::drawingServerCB(ArServerClient *client, ArNetPacket *pkt)
{
  myMutex.lock();
  client->sendPacketUdp(&myReply);
  myMutex.unlock();
}

// Method called by accessor methods when properties changed. This reconstructs
// the myReply packet sent in response to requests from clients
void Circle::regenerate()
{
  myMutex.lock();
  myReply.empty();
  myReply.byte4ToBuf(myNumPoints);
  double a = 360.0/myNumPoints;
  for(unsigned int i = 0; i < myNumPoints; ++i)
  {
    myReply.byte4ToBuf(ArMath::roundInt(myPos.getX()+ArMath::cos(i*a)*myRadius)); // X
    myReply.byte4ToBuf(ArMath::roundInt(myPos.getY()+ArMath::sin(i*a)*myRadius)); // Y
  }
  myMutex.unlock();
}


int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;

  ArArgumentParser parser(&argc, argv);
  ArServerSimpleOpener simpleOpener(&parser);

  // parse the command line... fail and print the help if the parsing fails
  // or if help was requested
  parser.loadDefaultArguments();
  if (!simpleOpener.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    simpleOpener.logOptions();
    exit(1);
  }


  // first open the server up
  if (!simpleOpener.open(&server))
  {
    if (simpleOpener.wasUserFileBad())
      printf("Error: Bad user/password/permissions file.\n");
    else
      printf("Error: Could not open server port. Use -help to see options.\n");
    exit(1);
  }


  // This is the service that provides drawing data to the client.
  ArServerInfoDrawings drawings(&server);

  // Add our custom drawings
  drawings.addDrawing(
      //                shape:      color:               size:   layer:
      new ArDrawingData("polyLine", ArColor(255, 0, 0),  2,      49),
      "exampleDrawing_Home", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleHomeDrawingNetCallback)
  );
  drawings.addDrawing(                                    
      new ArDrawingData("polyDots", ArColor(0, 255, 0), 250, 48),
      "exampleDrawing_Dots", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleDotsDrawingNetCallback)
  );
  drawings.addDrawing(
      new ArDrawingData("polySegments", ArColor(0, 0, 0), 4, 52),
      "exampleDrawing_XMarksTheSpot", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleXDrawingNetCallback)
  );
  drawings.addDrawing(
      new ArDrawingData("polyArrows", ArColor(255, 0, 255), 500, 100),
      "exampleDrawing_Arrows", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleArrowsDrawingNetCallback)
  );

  Circle circle(&drawings, "exampleDrawing_circle", 
    new ArDrawingData("polySegments", ArColor(255, 150, 0), 3, 120));
  circle.setPos(ArPose(0, -5000));
  circle.setRadius(1000);
  circle.setNumPoints(360);

  // log whatever we wanted to before the runAsync
  simpleOpener.checkAndLog();

  // run the server thread in the background
  server.runAsync();

  printf("Server is now running...\n");


  // Add a key handler mostly that windows can exit by pressing
  // escape, note that the key handler prevents you from running this program
  // in the background on Linux.
  ArKeyHandler *keyHandler;
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    keyHandler = new ArKeyHandler;
    Aria::setKeyHandler(keyHandler);
    printf("To exit, press escape.\n");
  }

 
  double circleRadius = 1000;
  while(true) 
  {
    ArUtil::sleep(100);
    circleRadius += 50;
    if(circleRadius > 5000)
      circleRadius = 0;
    circle.setRadius(circleRadius);
  }

  Aria::shutdown();
  exit(0);  
}




// Network callbacks for drawings' current geometry data:

void exampleHomeDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  ArNetPacket reply;

  // 7 Vertices
  reply.byte4ToBuf(7);

  // Centered on 0,0.
  // X:                    Y:
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(500);   // Vertex 1
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(-500);  // Vertex 2
  reply.byte4ToBuf(500);   reply.byte4ToBuf(-500);  // Vertex 3
  reply.byte4ToBuf(500);   reply.byte4ToBuf(500);   // Vertex 4
  reply.byte4ToBuf(0);     reply.byte4ToBuf(1000);  // Vertex 5
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(500);   // Vertex 6
  reply.byte4ToBuf(500);   reply.byte4ToBuf(500);   // Vertex 7

  client->sendPacketUdp(&reply);
}

void exampleDotsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  ArNetPacket reply;

  unsigned int tik = ArUtil::getTime() % 200;
  double t = tik / 5.0;

  // Three dots
  reply.byte4ToBuf(3);

  // Dot 1:
  reply.byte4ToBuf(3000);  // X coordinate (mm)
  reply.byte4ToBuf((int) (sin(t) * 1000));// Y

  // Dot 2:
  reply.byte4ToBuf(3500);  // X
  reply.byte4ToBuf((int) (sin(t+500) * 1000));// Y

  // Dot 3:
  reply.byte4ToBuf(4000);  // X
  reply.byte4ToBuf((int) (sin(t+1000) * 1000));// Y

  client->sendPacketUdp(&reply);
}

void exampleXDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  ArNetPacket reply;

  // X marks the spot. 2 line segments, so 4 vertices:
  reply.byte4ToBuf(4);

  // Segment 1:
  reply.byte4ToBuf(-4250); // X1
  reply.byte4ToBuf(250);   // Y1
  reply.byte4ToBuf(-3750); // X2
  reply.byte4ToBuf(-250);  // Y2

  // Segment 2:
  reply.byte4ToBuf(-4250); // X1
  reply.byte4ToBuf(-250);  // Y1
  reply.byte4ToBuf(-3750); // X2
  reply.byte4ToBuf(250);   // Y2
  
  client->sendPacketUdp(&reply);
}

void exampleArrowsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  // 1 Arrow that points at the robot
  ArNetPacket reply;
  reply.byte4ToBuf(1);
  reply.byte4ToBuf(0);      // Pos. X
  reply.byte4ToBuf(700);   // Pos. Y
  client->sendPacketUdp(&reply);
}


#include "Aria.h"
#include "ArNetworking.h"

class InputHandler
{
public: 
  InputHandler(ArClientBase *client, ArKeyHandler *keyHandler);
  virtual ~InputHandler(void);
  void up(void);
  void down(void);
  void left(void);
  void right(void);
  void space(void);
protected:
  ArClientBase *myClient;
  ArKeyHandler *myKeyHandler;
  ArFunctorC<InputHandler> myUpCB;
  ArFunctorC<InputHandler> myDownCB;
  ArFunctorC<InputHandler> myLeftCB;
  ArFunctorC<InputHandler> myRightCB;
  ArFunctorC<InputHandler> mySpaceCB;
};

InputHandler::InputHandler(ArClientBase *client, ArKeyHandler *keyHandler) : 
  myUpCB(this, &InputHandler::up),
  myDownCB(this, &InputHandler::down),
  myLeftCB(this, &InputHandler::left),
  myRightCB(this, &InputHandler::right),
  mySpaceCB(this, &InputHandler::space)
{
  myClient = client;
  myKeyHandler = keyHandler;
  myKeyHandler->addKeyHandler(ArKeyHandler::UP, &myUpCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::DOWN, &myDownCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::LEFT, &myLeftCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::RIGHT, &myRightCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::SPACE, &mySpaceCB);
}

InputHandler::~InputHandler(void)
{

}

void InputHandler::up(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(25);
  myClient->requestOnce("deltaVel", &packet);
}

void InputHandler::down(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(-25);
  myClient->requestOnce("deltaVel", &packet);
}

void InputHandler::left(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(3.2);
  myClient->requestOnce("deltaHeading", &packet);
}

void InputHandler::right(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(-3.2);
  myClient->requestOnce("deltaHeading", &packet);
}

void InputHandler::space(void)
{
  ArNetPacket packet;
  packet.doubleToBuf(0.00001);
  myClient->requestOnce("setVel", &packet);
  myClient->requestOnce("deltaHeading", &packet);
}

class OutputHandler
{
public:
  OutputHandler(ArClientBase *client);
  virtual ~OutputHandler(void);
  void handleOutput(ArNetPacket *packet);
protected:
  double myX;
  double myY;
  double myTh;
  double myVel;
  double myRotVel;
  double myVoltage;
  ArClientBase *myClient;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleOutputCB;

};

OutputHandler::OutputHandler(ArClientBase *client) :
  myHandleOutputCB(this, &OutputHandler::handleOutput)
{
  myClient = client;
  myClient->addHandler("output", &myHandleOutputCB);
  myClient->request("output", 100);
}

OutputHandler::~OutputHandler(void)
{
  myClient->requestStop("output");
}

void OutputHandler::handleOutput(ArNetPacket *packet)
{
  myX = packet->bufToByte4();
  myY = packet->bufToByte4();
  myTh = packet->bufToByte2()/10.0;
  myVel = packet->bufToByte2();
  myRotVel = packet->bufToByte2();
  myVoltage = packet->bufToByte2()/10.0;
  printf("\rx: %6.0f y: %6.0f th: %5.1f vel: %4.0f rotVel: %3.0f voltage: %4.1f",
	 myX, myY, myTh, myVel, myRotVel, myVoltage);
  fflush(stdout);
}


void escape(void)
{
  Aria::shutdown();
}

int main(int argc, char **argv)
{
  char* host = "localhost";
  if(argc > 1)
    host = argv[1];
  Aria::init();
  ArClientBase client;
  ArGlobalFunctor escapeCB(&escape);
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);


  printf("Connecting to standaloneServerDemo at %s:%d...\n", host, 7272);
  if (!client.blockingConnect(host, 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  } 
  InputHandler inputHandler(&client, &keyHandler);
  OutputHandler outputHandler(&client);
  keyHandler.addKeyHandler(ArKeyHandler::ESCAPE, &escapeCB);
  client.runAsync();
  while (client.getRunningWithLock())
  {
    keyHandler.checkKeys();
    ArUtil::sleep(1);
  }
  keyHandler.restore();
  Aria::shutdown();
  return 0;
}

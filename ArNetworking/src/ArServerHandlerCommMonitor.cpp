#include "Aria.h"
#include "ArExport.h"
#include "ArServerHandlerCommMonitor.h"

const char *ArServerHandlerCommMonitor::HEARTBEAT_TCP_PACKET_NAME = "heartbeatTcp";
const char *ArServerHandlerCommMonitor::HEARTBEAT_UDP_PACKET_NAME = "heartbeatUdp";

const char *ArServerHandlerCommMonitor::GET_HEARTBEAT_INTERVAL_PACKET_NAME = "getHeartbeatInterval";
const char *ArServerHandlerCommMonitor::ACK_COMM_TCP_PACKET_NAME = "ackCommTcp";
const char *ArServerHandlerCommMonitor::ACK_COMM_UDP_PACKET_NAME = "ackCommUdp";
const char *ArServerHandlerCommMonitor::COMMAND_GROUP = "RobotInfo";
const char *ArServerHandlerCommMonitor::NO_ARGS = "None";


ArServerHandlerCommMonitor::ArServerHandlerCommMonitor
                                            (ArServerBase *server,
                                             int heartbeatInterval) :
  myServer(server),
  myHeartbeatInterval(heartbeatInterval),
  myLastHeartbeatTime(),
  myGetHeartbeatIntervalCB(this, &ArServerHandlerCommMonitor::handleGetHeartbeatInterval),
  myAckCommUdpCB(this, &ArServerHandlerCommMonitor::handleAckCommUdp),
  myAckCommTcpCB(this, &ArServerHandlerCommMonitor::handleAckCommTcp),
  myCycleCB(this, &ArServerHandlerCommMonitor::cycleCallback)
{
  // Do not allow intervals too small...
  if (myHeartbeatInterval < MIN_HEARTBEAT_INTERVAL) {
    myHeartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
  }

  myServer->addData(HEARTBEAT_TCP_PACKET_NAME, 
		                "Packet is broadcast (TCP) at regular intervals to confirm that server is alive.",
		                NULL, 
 		                NO_ARGS, 
                    NO_ARGS,
		                COMMAND_GROUP, "RETURN_SINGLE");

  myServer->addData(HEARTBEAT_UDP_PACKET_NAME, 
		                "Packet is broadcast (UDP) at regular intervals to confirm that server is alive.",
		                NULL, 
 		                NO_ARGS, 
                    NO_ARGS,
		                COMMAND_GROUP, "RETURN_SINGLE");

  myServer->addData(GET_HEARTBEAT_INTERVAL_PACKET_NAME, 
		                "Gets the expected heartbeat interval (msecs); this is an approximate value depending on the server cycle time.",
		                &myGetHeartbeatIntervalCB, 
		                NO_ARGS, 
                    "uByte4: expected heartbeat interval (msecs)",
		                COMMAND_GROUP, "RETURN_SINGLE"); 

  myServer->addData(ACK_COMM_TCP_PACKET_NAME, 
		                "Acknowledges two-way communication over TCP",
		                &myAckCommTcpCB, 
		                NO_ARGS, 
                    NO_ARGS,
		                COMMAND_GROUP, "RETURN_SINGLE"); 

  myServer->addData(ACK_COMM_UDP_PACKET_NAME, 
		                "Acknowledges two-way communication over UDP",
		                &myAckCommUdpCB, 
		                NO_ARGS, 
                    NO_ARGS,
		                COMMAND_GROUP, "RETURN_SINGLE"); 


  myServer->addCycleCallback(&myCycleCB);

} // end ctor


ArServerHandlerCommMonitor::~ArServerHandlerCommMonitor()
{
  // TODO Remove CBs from server?
}


AREXPORT void ArServerHandlerCommMonitor::handleGetHeartbeatInterval
                                              (ArServerClient *client, 
                                               ArNetPacket *packet)
{
  if (client == NULL) {
    return; // Something very bad has happened...
  }

  ArNetPacket sendPacket;
  sendPacket.uByte4ToBuf(myHeartbeatInterval);

  client->sendPacketTcp(&sendPacket);

} // end method handleGetHeartbeatInterval


AREXPORT void ArServerHandlerCommMonitor::handleAckCommTcp(ArServerClient *client, 
                                                           ArNetPacket *packet)
{
  if (client == NULL) {
    return; // Something very bad has happened...
  }

  ArNetPacket sendPacket;

  client->sendPacketTcp(&sendPacket);

} // end method handleAckCommTcp


AREXPORT void ArServerHandlerCommMonitor::handleAckCommUdp(ArServerClient *client, 
                                                           ArNetPacket *packet)
{
  if (client == NULL) {
    return; // Something very bad has happened...
  }

  ArNetPacket sendPacket;

  client->sendPacketUdp(&sendPacket);

} // end method handleAckCommUdp


AREXPORT void ArServerHandlerCommMonitor::cycleCallback()
{
  if (myLastHeartbeatTime.mSecSince() < myHeartbeatInterval) {
    return;
  }

  ArNetPacket sendPacket;

  myServer->broadcastPacketTcp(&sendPacket, HEARTBEAT_TCP_PACKET_NAME);
  myServer->broadcastPacketUdp(&sendPacket, HEARTBEAT_UDP_PACKET_NAME);

  myLastHeartbeatTime.setToNow();

} // end method cycleCallback




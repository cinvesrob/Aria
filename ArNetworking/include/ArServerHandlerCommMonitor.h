#ifndef ARSERVERHANDLERCOMMMONITOR_H
#define ARSERVERHANDLERCOMMMONITOR_H

#include "Aria.h"

#include "ArNetworking.h"

/// Handler that enables the client to monitor communication to the robot server.
/** 
 * ArServerHandlerCommMonitor defines network packet handlers that simply verify
 * the communication between the robot and server.  
 * 
 * This class handles the following requests:
 *  <ul>
 *    <li>heartbeatTcp:  Packet broadcast (TCP) periodically to confirm that
 *                       the server is alive.</li>
 *    <li>heartbeatUdp:  Packet broadcast (UDP) periodically to confirm that 
 *                       the server is alive.</li>
 *    <li>getHeartbeatInterval: Command to return the expected heartbeat interval.
 *                              Packet contains:
 *                              <ol>
 *                              <li>uByte4 : Interval (msecs)</li>
 *                              </ol>
 *    <li>ackCommTcp:    Command to acknowledge two-way communication over TCP.
 *                       An empty packet is returned.</li>
 *    <li>ackCommUdp:    Command to acknowledge two-way communication over UDP.
 *                       An empty packet is returned.</li>
 *  </ul>
**/
class ArServerHandlerCommMonitor 
{
public:

  enum {
    MIN_HEARTBEAT_INTERVAL = 100,
    DEFAULT_HEARTBEAT_INTERVAL = 500
  };

protected:
  /// Name of the network packet broadcast (TCP) to confirm the server is alive.
  static const char *HEARTBEAT_TCP_PACKET_NAME;
  /// Name of the network packet broadcast (UDP) to confirm the server is alive.
  static const char *HEARTBEAT_UDP_PACKET_NAME;

  /// Name of the network packet to get the heartbeat interval.
  static const char *GET_HEARTBEAT_INTERVAL_PACKET_NAME;
  /// Name of the network packet to confirm TCP communication.
  static const char *ACK_COMM_TCP_PACKET_NAME;
  /// Name of the network packet to confirm UDP communication.
  static const char *ACK_COMM_UDP_PACKET_NAME;


  static const char *COMMAND_GROUP;
  static const char *NO_ARGS;

public:

  /// Constructs a new handler to monitor communication between the robot server and client.
  /**
   * @param server the ArServerBase used to send/receive requests
   * @param heartBeatInterval How often to send a "heartbeat" packet to the client
  **/
  AREXPORT ArServerHandlerCommMonitor(ArServerBase *server,
                                      int heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL);


  /// Destructor
  AREXPORT virtual ~ArServerHandlerCommMonitor();

 
  // -----------------------------------------------------------------------------
  // Packet Handlers:
  // -----------------------------------------------------------------------------

  /// Handles the getHeartbeatInterval network packet.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleGetHeartbeatInterval(ArServerClient *client, ArNetPacket *packet);

  /// Handles the ackCommTcp network packet.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleAckCommTcp(ArServerClient *client, ArNetPacket *packet);


  /// Handles the ackCommUdp network packet.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleAckCommUdp(ArServerClient *client, ArNetPacket *packet);


  /// Callback for the server cycle; broadcasts heartbeat packets when interval has elapsed.
  AREXPORT void cycleCallback();

protected:

  /// Server from which requests are received
  ArServerBase *myServer;

  /// Number of msecs between heartbeat broadcasts
  int myHeartbeatInterval;
  /// Time that the last heartbeat packets were broadcast
  ArTime myLastHeartbeatTime;

  /// Callback for getting the heartbeat interval
  ArFunctor2C<ArServerHandlerCommMonitor, 
              ArServerClient *, ArNetPacket *> myGetHeartbeatIntervalCB;
  /// Callback for replying to the ack request (UDP)
  ArFunctor2C<ArServerHandlerCommMonitor, 
              ArServerClient *, ArNetPacket *> myAckCommUdpCB;
  /// Callback for replying to the ack request (TCP)
  ArFunctor2C<ArServerHandlerCommMonitor, 
              ArServerClient *, ArNetPacket *> myAckCommTcpCB;
  /// Cycle callback executed to broadcast the heartbeat packets
  ArFunctorC<ArServerHandlerCommMonitor>       myCycleCB;
  
}; // end class ArServerHandlerCommMonitor

#endif // ARSERVERHANDLERCOMMMONITOR_H

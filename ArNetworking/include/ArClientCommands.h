#ifndef NLCLIENTCOMMANDS_H
#define NLCLIENTCOMMANDS_H

/**
   The commands from the client to the server
**/

class ArClientCommands
{
public:
  enum ClientCommands {
    SHUTDOWN = 1, ///< Closes the connection
    INTRODUCTION = 2, ///< Introduces the client to the server
    UDP_INTRODUCTION = 3, ///< Udp introduction of the client to the server
    UDP_CONFIRMATION = 4, ///< Confirmation Udp was received from server
    TCP_ONLY = 5, ///< Client tells server to only send TCP
    LIST = 128, ///< Lists the types that can be handled
    REQUEST = 129, ///< Requests packet of a certain type
    REQUESTSTOP = 130 ///< Requests that the server stop sending the given type
    /// You can request any other command once if you know its number just by sending that number
  };
};

#endif // NLCLIENTCOMMANDS_H

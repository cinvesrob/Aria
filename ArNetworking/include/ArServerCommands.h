#ifndef NLSERVERCOMMANDS_H
#define NLSERVERCOMMANDS_H

/**
   The commands from the server to the client
**/

class ArServerCommands
{
public:
  enum ServerCommands {
    SHUTDOWN = 1, ///< Closes the connection
    INTRODUCTION = 2, ///< Introduces the server to the client
    UDP_INTRODUCTION = 3, ///< Udp introduction of the server to the client
    UDP_CONFIRMATION = 4, ///< Confirmation Udp was received from client
    CONNECTED = 5, ///< Server accepts clients connection
    REJECTED = 6, ///< Server rejects clients connection, has a byte2, then a string.... these reasons (1 = bad username password, string then is empty, 2 = rejecting connection because using central server, string then is central server IP, 3 = client rejecting server because server has wrong protocol, 4 = server rejected client because client has wrong protocol, 5 = maxClients exceeded, 6 = type of client is specified, but doesn't match)
    TCP_ONLY = 7, ///< Server tells client to only send TCP
    LIST = 129, ///< Map of the string names for a type to a number along with a long description of the data type
    LISTSINGLE = 130, ///< Map of a single type to a number (for late additions to server) along with its description
    LISTARGRET = 131, ///< Map of the number to their arguments and returns descriptions
    LISTARGRETSINGLE = 132, ///< Map of a single type to a number (for late additions to server) along with its argument and return descriptions
    LISTGROUPANDFLAGS = 133, ///< Map of the number to their command groups and data flags
    LISTGROUPANDFLAGSSINGLE = 134 ///< Map of a single type to a number (for late additions to server) along with its command group and data flags
  };

  enum Type
  {
    TYPE_UNSPECIFIED = 0, /// Unspecified (anything can connect)
    TYPE_REAL = 1, ///< Only real robots permitted to connect
    TYPE_SIMULATED = 2, ///< Only simulated robots permitted to connect
    TYPE_NONE = 3 ///< Nothing is permitted to connect
  };
  static const char *toString(Type type) 
    { 
      if (type == TYPE_UNSPECIFIED)
	return "unspecified";
      else if (type == TYPE_REAL)
	return "real";
      else if (type == TYPE_SIMULATED)
	return "simulated";
      else if (type == TYPE_NONE)
	return "none";
      else
	return "unknown";
    }
};

#endif // NLSERVERCOMMANDS_H

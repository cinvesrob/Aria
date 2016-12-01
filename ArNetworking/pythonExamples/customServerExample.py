from AriaPy import *
from ArNetworkingPy import *
import sys

def requestCallback(client, packet):
  replyPacket = ArNetPacket()
  replyPacket.strToBuf("Reply");
  print "requestCallback received a packet with command #%d. Sending a reply...\n" % (packet.getCommand())
  client.sendPacketTcp(replyPacket)

Aria_init()
server = ArServerBase()
packet = ArNetPacket()

server.addData("test", "some wierd test", requestCallback, "none", "none")
server.addData("test2", "another wierd test", requestCallback, "none", "none")
server.addData("test3", "yet another wierd test", requestCallback, "none", "none")
if not server.open(7273):
  print "Error: Could not open server port 7273"
  Aria_exit(1)
print "Opened server on port 7273. Connect with customClientExample."
server.runAsync()
while (server.getRunningWithLock()):
  ArUtil_sleep(1000)
  server.broadcastPacketTcp(packet, "test3")
Aria_exit(0)


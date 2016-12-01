from AriaPy import *
from ArNetworkingPy import *
import sys

def testCB(packet):
  print "client: received reply packet with command %d\n" % (packet.getCommand())

Aria_init()

client = ArClientBase()

startTime = ArTime()
startTime.setToNow()
if not client.blockingConnect("localhost", 7273):
  print "Could not connect to server at localhost port 7273, exiting"
  Aria_exit(1);
print "client: Took %ld msec to connect\n" % (startTime.mSecSince())

client.runAsync()

client.lock()

print "Client detected the following data requests on the server:"
client.logDataList()

print "Adding callback for data requests \"test\", \"test2\", \"test3\"..."
client.addHandler("test", testCB)
client.addHandler("test2", testCB)
client.addHandler("test3", testCB)


print "Requesting \"test\" once..."
client.requestOnce("test")

print "Requesting \"test2\" every 100ms..."
client.request("test2", 100)

print "Requesting \"test3\" to be sent at server's discrecion..."
client.request("test3", -1)
client.unlock()

print "Waiting 2 sec..."
ArUtil_sleep(2000)

print "Changing request frequency of \"test2\" to 300ms..."
client.lock()
client.request("test2", 300)
client.unlock()

print "Watiing 2 sec..."
ArUtil_sleep(1000)

print "Stopping request for \"test2\"..."
client.lock()
client.requestStop("test2")
client.unlock()

print "Waiting 2 sec..."
ArUtil_sleep(2000)

print "Disconnecting and exiting."
client.lock()
client.disconnect()
client.unlock()
ArUtil_sleep(50)


#include "Aria.h"
#include "ArNetworking.h"

//typedef ArRobotPacket TestingPacketType;
typedef ArNetPacket TestingPacketType;

int main(void)
{


  for (int ii = 0; ii < 8; ii++)
  {
    ArTypes::Byte before = (long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::Byte after;
    
    packet.byteToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToByte();
   
    printf(" byte  %d %d %d\n", ii+1, before, after);
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    

  for (int ii = 0; ii < 8; ii++)
  {
    ArTypes::UByte before = (unsigned long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::UByte after;
    
    packet.uByteToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToUByte();
   
    printf("uByte  %d %u %u\n", ii+1, before, after);
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    

  for (int ii = 0; ii < 16; ii++)
  {
    ArTypes::Byte2 before = (long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::Byte2 after;
    
    packet.byte2ToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToByte2();
   
    printf(" byte2 %d %d %d\n", ii+1, before, after);
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    

  for (int ii = 0; ii < 16; ii++)
  {
    ArTypes::UByte2 before = (unsigned long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::UByte2 after;
    
    packet.uByte2ToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToUByte2();
   
    printf("uByte2 %d %u %u\n", ii+1, before, after);
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    
  

  for (int ii = 0; ii < 32; ii++)
  {
    ArTypes::Byte4 before = (long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::Byte4 after;
    
    packet.byte4ToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToByte4();
   
    printf(" byte4 %d %d %d\n", ii+1, before, after);
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    

  for (int ii = 0; ii < 32; ii++)
  {
    ArTypes::UByte4 before = (unsigned long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::UByte4 after;
    
    packet.uByte4ToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToUByte4();
   
    printf("uByte4 %d %u %u\n", ii+1, before, after);
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    
  
  for (int ii = 0; ii < 64; ii++)
  {
    ArTypes::Byte8 before = (long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::Byte8 after;
    
    packet.byte8ToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToByte8();
    printf(" byte8 %d %lld %lld\n", ii+1, before, after);
   
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    

  for (int ii = 0; ii < 64; ii++)
  {
    ArTypes::UByte8 before = (unsigned long long) 1 << ii;
    TestingPacketType packet;
    
    ArTypes::UByte8 after;
    
    packet.uByte8ToBuf(before);
    packet.finalizePacket();
    packet.resetRead();
    
    after = packet.bufToUByte8();
    printf("uByte8 %d %llu %llu\n", ii+1, before, after);
   
    if (before != after) 
    {
      printf("FAILED\n");
      exit(-1);
    }
  }    

  printf("PASSED\n");
  exit(0);
}

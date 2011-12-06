// Copyright 2011 by Timo Furrer
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

// Some needed constants
#define BUF_MAX         17*6
#define MAC_ADDR_MAX    6
#define REMOTE_ADDR     "255.255.255.255"
#define REMOTE_PORT     9

// Test MAC Addr: 00:0B:CD:39:2D:E9

/**
* @brief Sends the WOL magic packet to the given mac address
*
* @param mac The mac address to which the magic packet has to be sent.
*
* @return integer
*/
int sendWOL( const unsigned char *mac );

/**
* @brief Converts a ma address string to a byte array.
*
* @param mac       The mac address to convert
* @param byteArray The byte array to write in
*
* @return integer
*/
int macAddrToByteArray( char *mac, unsigned char *byteArray );

int main( int argc, char **argv ) {
  unsigned char macAddr[MAC_ADDR_MAX];

  if( argc < 2 ) {
    printf( "Usage: %s mac\n", *argv );
    exit( EXIT_FAILURE );
  }

  if( macAddrToByteArray( argv[1], macAddr ) < 0 ){
    printf( "MAC Address is not valid %s ...!\n", argv[1] );
    exit( EXIT_FAILURE );
  }

  sendWOL( macAddr );
  
  return EXIT_SUCCESS;
}

int sendWOL( const unsigned char *mac ) {
  int udpSocket;
  struct sockaddr_in addr;
  unsigned char packet[BUF_MAX];
  int i, j, optval = 1;

  udpSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

  if( udpSocket < 0 ) {
    printf( "Cannot open socket: %s ...!\n", strerror( errno ) );
    return -1;
  }

  if( setsockopt( udpSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof( optval ) ) < 0 ) {
    printf( "Cannot set socket options: %s ...!\n", strerror( errno ) );
    return -1;
  }

  addr.sin_family = AF_INET;
  addr.sin_port   = htons( REMOTE_PORT );
  inet_aton( REMOTE_ADDR, &addr.sin_addr );
  
  for( i = 0; i < 6; i++ ) {
    packet[i] = 0xFF;
  }

  for( i = 1; i <= 16; i++ ) {
    for( j = 0; j < 6; j++ ) {
      packet[i*6+j] = mac[j];
    }
  }

  if( sendto( udpSocket, packet, sizeof( packet ), 0, (struct sockaddr *) &addr, sizeof( addr ) ) < 0 ) {
    printf( "Cannot send data: %s ...!\n", strerror( errno ) );
    return -1;
  }

  printf( "Successful sent WOL magic packet to %s ...!\n", mac );
  
  return 0;
}

int macAddrToByteArray( char *mac, unsigned char *byteArray ) {
  char *delimiter = ":";
  char *tok;
  int i;

  tok = strtok( mac, delimiter );
  for( i = 0; i < MAC_ADDR_MAX; i++ ) {
    byteArray[i] = (unsigned char)strtol( tok, NULL, 16 );
    tok = strtok( NULL, delimiter );
  }

  return 0;
}

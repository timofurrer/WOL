/* Copyright 2011 by Timo Furrer */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

/* Some needed constants */
#define BUF_MAX            17*6
#define MAC_ADDR_MAX       6
#define MAC_ADDR_INPUT_MAX 64

#define CONVERT_BASE       16

#define REMOTE_ADDR        "255.255.255.255"
#define REMOTE_PORT        9

#define ARGS_BUF_MAX       128

#define USAGE              "Usage: %s [-f filename|mac1 ...]\n" 

/* Test MAC Addr: 00:0B:CD:39:2D:E9 */

/**
* @brief Sends the WOL magic packet to the given mac address
*
* @param mac       The mac address to which the magic packet has to be sent.
* @param macString The mac address as string for output.
*
* @return integer
*/
int sendWOL( const unsigned char *mac, const char *macString );

/**
* @brief Gets the next mac address from the terminal arguments
*
* @param argument The arguments
* @param length   The length of the arguments.
*
* @return 
*/
char *nextAddrFromArg( char **argument, int length );

/**
* @brief Gets the next mac address from a file.
*
* @param filenames The files with the mac addresses on each line.
* @param length    The length of the file array
*
* @return char * 
*/
char *nextAddrFromFile( char **filenames, int length );

/**
* @brief Converts a ma address string to a byte array.
*
* @param mac       The mac address to convert
* @param byteArray The byte array to write in
*
* @return integer
*/
int packMacAddr( char *mac, unsigned char *byteArray );

int main( int argc, char **argv ) {
  char *( *funcp )( char **args, int length );
  unsigned char currentMacAddr[MAC_ADDR_MAX];
  char currentMacAddrStrTmp[MAC_ADDR_INPUT_MAX];
  char *currentMacAddrStr = NULL;
  char **args             = (char **)malloc( argc * ARGS_BUF_MAX * sizeof( char ) ); 
  int length              = 0;

  if( argc < 2 ) {
    printf( USAGE, *argv );
    exit( EXIT_FAILURE );
  }

  if( !strncmp( argv[1], "-f", 2 ) ) {
    if( argc < 3 ) {
      printf( USAGE, *argv );
      exit( EXIT_FAILURE );
    }

    funcp = nextAddrFromFile;
    args[0] = argv[2];
    length = 1;
  }
  else {
    funcp = nextAddrFromArg;
    args = argv;
    length = argc;
  }

  while( (currentMacAddrStr = funcp( args, length ) ) != NULL ) {
    strncpy( currentMacAddrStrTmp, currentMacAddrStr, strlen( currentMacAddrStr ) );
    currentMacAddrStrTmp[strlen( currentMacAddrStr )] = '\0';

    if( packMacAddr( currentMacAddrStrTmp, currentMacAddr ) < 0 ) {
      printf( "MAC Address ist not valid: %s ...!\n", currentMacAddrStr );
    }
    else {
      sendWOL( currentMacAddr, currentMacAddrStr );
    }
  }

  return EXIT_SUCCESS;
}

char *nextAddrFromArg( char **argument, int length ) {
  static int i = 0;

  while( i < length ) {
    i++;
    return argument[i];
  }

  return NULL;
}

char *nextAddrFromFile( char **filenames, int length ) {
  static FILE *fp = NULL;
  char *currentMacAddr = (char *)malloc( MAC_ADDR_INPUT_MAX * sizeof( char ) );
  
  if( fp == NULL ) {
    if( ( fp = fopen( filenames[0], "r" ) ) == NULL ) {
      printf( "Cannot open file %s: %s ...!\n", filenames[0], strerror( errno ) );
      exit( EXIT_FAILURE );
    }
  }

  if( fgets( currentMacAddr, MAC_ADDR_INPUT_MAX, fp ) != NULL ) {
    currentMacAddr[strlen( currentMacAddr )-1] = '\0';
    return currentMacAddr;
  }
  else {
    fclose( fp );
    return NULL;
  }
}

int packMacAddr( char *mac, unsigned char *byteArray ) {
  char *delimiter = ":";
  char *tok;
  int i;

  tok = strtok( mac, delimiter );
  for( i = 0; i < MAC_ADDR_MAX; i++ ) {
    if( tok == NULL ) {
      return -1;
    }

    byteArray[i] = (unsigned char)strtol( tok, NULL, CONVERT_BASE );
    tok = strtok( NULL, delimiter );
  }

  return 0;
}

int sendWOL( const unsigned char *mac, const char *macString ) {
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

  printf( "Successful sent WOL magic packet to %s ...!\n", macString );
  
  return 0;
}

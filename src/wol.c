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
#define MAC_ADDR_BUF       32
#define MAC_ADDR_INPUT_MAX 64
#define MAC_ADDR_STR_MAX   18

#define CONVERT_BASE       16

#define REMOTE_ADDR        "255.255.255.255"
#define REMOTE_PORT        9

/* Test MAC Addr: 00:0B:CD:39:2D:E9 */

/**
* @brief Executes a WOL command for each mac address in the given array. 
*
* @param macAddresses The mac address array
* @param length       The length of the mac address array
*
* @return integer
*/
int executeWOL( char **macAddresses, int length ); 

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
* @brief Converts a ma address string to a byte array.
*
* @param mac       The mac address to convert
* @param byteArray The byte array to write in
*
* @return integer
*/
int macAddrToByteArray( char *mac, unsigned char *byteArray );

/**
* @brief Reads a file with mac addresses on each line.
*
* @param filename The filename to the mac address file.
* @param readAddr The read mac addresses from the file.
*
* @return char ** 
*/
char **readMacFile( char *filename, int *readAddr );

int main( int argc, char **argv ) {
  char **macAddresses = NULL;
  int readAddr        = 0;
  int i;

  if( argc < 2 ) {
    printf( "Usage: %s [-f filename|mac1 ...] \n", *argv );
    exit( EXIT_FAILURE );
  }

  if( !strncmp( argv[1], "-f", 2 ) ) {
    if( argc < 3 ) {
      printf( "Usage: %s [-f filename|mac1 ...] \n", *argv );
      exit( EXIT_FAILURE );
    }

    macAddresses = readMacFile( argv[2], &readAddr );
    if( macAddresses == NULL ) {
      printf( "An error occured during reading the mac address file: %s ...!\n", strerror( errno ) );
      exit( EXIT_FAILURE );
    }

    /*executeWOL( macAddresses, readAddr );*/

    for( i = 0; i < readAddr; i++ ) {
      free( macAddresses[i] );
    }
    free( macAddresses );
  }
  else {
    executeWOL( argv, argc );
  }

  return EXIT_SUCCESS;
}

int executeWOL( char **macAddresses, int length ) {
  unsigned char macAddr[MAC_ADDR_MAX];
  char currentInputMac[MAC_ADDR_INPUT_MAX];
  int i;

  for( i = 1; i < length; i++ ) {
    strncpy( currentInputMac, macAddresses[i], strlen( macAddresses[i] ) );
    currentInputMac[strlen( macAddresses[i] )] = '\0';

    if( macAddrToByteArray( currentInputMac, macAddr ) < 0 ){
      printf( "MAC Address is not valid %s ...!\n", macAddresses[i] );
    }
    else {
      sendWOL( macAddr, macAddresses[i] );
    }
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

int macAddrToByteArray( char *mac, unsigned char *byteArray ) {
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

char **readMacFile( char *filename, int *readAddr ) {
  char **macAddresses = NULL;
  char currentMacAddr[MAC_ADDR_BUF];
  int lines = 0;
  FILE *fp;
  int i;

  *readAddr = 0;
  fp = fopen( filename, "r" );

  if( fp == NULL ) {
    printf( "Cannot open file %s: %s ...!\n", filename, strerror( errno ) );
    exit( EXIT_FAILURE );
  }
  
  while( fgets( currentMacAddr, MAC_ADDR_BUF, fp ) != NULL ) lines++; 
  *readAddr = lines + 1;
  rewind( fp );

  macAddresses = (char **)malloc( *readAddr * sizeof( char ) );

  if( macAddresses == NULL ) {
    printf( "Cannot allocate memory for addresses in file: %s ...!\n", strerror( errno ) );
    exit( EXIT_FAILURE );
  }

  macAddresses[0] = (char *)malloc( MAC_ADDR_STR_MAX * sizeof( char ) );

  if( macAddresses[0] == NULL ) {
    printf( "Cannot allocate memory for address: %s ...!\n", strerror( errno ) );
    exit( EXIT_FAILURE );
  }

  macAddresses[0] = "fromfile";

  printf( "Lines: %d\n", lines );

  i = 1;
  while( fgets( currentMacAddr, MAC_ADDR_BUF, fp ) != NULL ) {
    currentMacAddr[strlen( currentMacAddr ) - 1] = '\0';
    printf( "Read MAC address: %s ...\n", currentMacAddr );
    
    macAddresses[i] = (char *)malloc( MAC_ADDR_STR_MAX * sizeof( char ) );

    if( macAddresses[i] == NULL ) {
      printf( "Cannot allocate memory for address: %s ...!\n", strerror( errno ) );
      exit( EXIT_FAILURE );
    }

    strncpy( macAddresses[i], currentMacAddr, MAC_ADDR_STR_MAX );
    macAddresses[strlen( currentMacAddr )] = '\0';

    i++;
  }

  fclose( fp );
  return macAddresses;
}

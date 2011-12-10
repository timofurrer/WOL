/*
*
* @author    Timo Furrer
* @co-author Mogria
* 
* @version   0.01.05
* @copyright GNU General Public License 
*
*/

#include <ctype.h>
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

#define USAGE              "Usage: %s [-f filename1, ...|mac1, ...]\n" 

/* Test MAC Addr: 00:0B:CD:39:2D:E9 */

/**
* @brief Structure for mac address
*/
struct mac_addr {
  unsigned char mac_addr[MAC_ADDR_MAX];
};


/**
* @brief Sends the WOL magic packet to the given mac address
*
* @param mac       The mac address to which the magic packet has to be sent.
* @param macString The mac address as string for output.
*
* @return integer
*/
int sendWOL( const struct mac_addr *mac, const char *macString );

/**
* @brief Gets the next mac address from the terminal arguments
*
* @param argument The arguments
* @param length   The length of the arguments.
*
* @return char * 
*/
char *nextAddrFromArg( char **argument, int length );

/**
* @brief Gets the next mac address from a file.
*
* @param filenames The filenames with the mac addresses on each line.
* @param length    The length of the filename array
*
* @return char * 
*/
char *nextAddrFromFile( char **filenames, int length );

/**
* @brief Converts a mac address string to a packet mac address.
*
* @param mac       The mac address to convert
* @param packedMac The byte array to write in
*
* @return integer
*/
int packMacAddr( char *mac, struct mac_addr *packedMac );

int main( int argc, char **argv ) {
  char *( *funcp )( char **args, int length ) = nextAddrFromArg;
  struct mac_addr *currentMacAddr = (struct mac_addr *)malloc( sizeof( struct mac_addr ) );
  char currentMacAddrStrTmp[MAC_ADDR_INPUT_MAX];
  char *currentMacAddrStr = NULL;
  char **args             = (char **)malloc( argc * ARGS_BUF_MAX * sizeof( char ) ); 
  int length              = argc;
  char argument;

  while(( argument = getopt( argc, argv, "f" )) != -1 ) {
    if( argument == 'f' ) {
      funcp = nextAddrFromFile;
    } else if( argument == '?' ) {
      if( isprint( optopt ) ) {
        fprintf( stderr, "Unknown option: %c ...!\n", optopt);
      }
    } else {
      printf( USAGE, *argv );
    }
  }

  if( argc < 2 ) {
    printf( USAGE, *argv );
    exit( EXIT_FAILURE );
  }

  args = &argv[optind];
  length = argc - optind;

  while( (currentMacAddrStr = funcp( args, length ) ) != NULL ) {
    strncpy( currentMacAddrStrTmp, currentMacAddrStr, strlen( currentMacAddrStr ) );
    currentMacAddrStrTmp[strlen( currentMacAddrStr )] = '\0';

    if( packMacAddr( currentMacAddrStrTmp, currentMacAddr ) < 0 ) {
      printf( "MAC Address ist not valid: %s ...!\n", currentMacAddrStr );
    }
    else {
      if( sendWOL( currentMacAddr, currentMacAddrStr ) < 0 ) {
        printf( "Error occured during sending the WOL magic packet for mac address: %s ...!\n", currentMacAddrStr );
      }
    }
  }

  return EXIT_SUCCESS;
}

char *nextAddrFromArg( char **argument, int length ) {
  static int i = 0;

  while( i < length ) {
    return argument[i++];
  }

  return NULL;
}

char *nextAddrFromFile( char **filenames, int length ) {
  static FILE *fp = NULL;
  static int fileNr = 0;
  char *currentMacAddr = (char *)malloc( MAC_ADDR_INPUT_MAX * sizeof( char ) );
  
  while( fileNr < length ) {
    if( fp == NULL ) {
      if( ( fp = fopen( filenames[fileNr], "r" ) ) == NULL ) {
        printf( "Cannot open file %s: %s ...!\n", filenames[fileNr], strerror( errno ) );
        exit( EXIT_FAILURE );
      }
      printf( "Read from file %s:\n", filenames[fileNr] );
    }

    if( fgets( currentMacAddr, MAC_ADDR_INPUT_MAX, fp ) != NULL ) {
      currentMacAddr[strlen( currentMacAddr ) - 1] = '\0';
      return currentMacAddr;
    }
    else {
      fclose( fp );
      fp = NULL;
      fileNr++;
      puts( "" );
    }
  }
  
  return NULL;
}

int packMacAddr( char *mac, struct mac_addr *packedMac ) {
  char *delimiter = ":";
  char *tok;
  int i;

  tok = strtok( mac, delimiter );
  for( i = 0; i < MAC_ADDR_MAX; i++ ) {
    if( tok == NULL ) {
      return -1;
    }

    packedMac->mac_addr[i] = (unsigned char)strtol( tok, NULL, CONVERT_BASE );
    tok = strtok( NULL, delimiter );
  }

  return 0;
}

int sendWOL( const struct mac_addr *mac, const char *macString ) {
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
      packet[i*6+j] = mac->mac_addr[j];
    }
  }

  if( sendto( udpSocket, packet, sizeof( packet ), 0, (struct sockaddr *) &addr, sizeof( addr ) ) < 0 ) {
    printf( "Cannot send data: %s ...!\n", strerror( errno ) );
    return -1;
  }

  printf( "Successful sent WOL magic packet to %s ...!\n", macString );
  
  return 0;
}

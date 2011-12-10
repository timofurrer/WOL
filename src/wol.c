/*
*
* @author    Timo Furrer
* @co-author Mogria
* 
* @version   0.01.05
* @copyright GNU General Public License 
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

/* Some needed constants */
#define PACKET_BUF         17*6
#define MAC_ADDR_MAX       6
#define MAC_ADDR_STR_MAX   64

#define CONVERT_BASE       16

#define REMOTE_ADDR        "255.255.255.255"
#define REMOTE_PORT        9

#define ARGS_BUF_MAX       128

#define USAGE              "Usage: %s [-f filename1, ...|mac1, ...]\n" 

/* Test MAC Addr: 00:0B:CD:39:2D:E9 */

/**
* @brief Structure for mac address
*/
typedef struct {
  unsigned char mac_addr[MAC_ADDR_MAX];
  char mac_addr_str[MAC_ADDR_STR_MAX];
} mac_addr_t;

/**
* @brief Sends the WOL magic packet to the given mac address
*
* @param mac       The mac address to which the magic packet has to be sent.
*
* @return integer
*/
int sendWOL( const mac_addr_t *mac );

/**
* @brief Gets the next mac address from the terminal arguments
*
* @param argument The arguments
* @param length   The length of the arguments.
*
* @return mac_addr_t * 
*/
mac_addr_t *nextAddrFromArg( char **argument, int length );

/**
* @brief Gets the next mac address from a file.
*
* @param filenames The filenames with the mac addresses on each line.
* @param length    The length of the filename array
*
* @return mac_addr_t * 
*/
mac_addr_t *nextAddrFromFile( char **filenames, int length );

/**
* @brief Converts a mac address string to a mac address struct of type mac_addr_t
*
* @param mac       The mac address to convert
* @param packedMac The mac address struct of type mac_addr_t
*
* @return integer
*/
int packMacAddr( const char *mac, mac_addr_t *packedMac );

int main( int argc, char **argv ) {
  mac_addr_t *( *funcp )( char **args, int length ) = nextAddrFromArg;
  mac_addr_t *currentMacAddr = (mac_addr_t *)malloc( sizeof( mac_addr_t ) );
  char **args                = (char **)malloc( argc * ARGS_BUF_MAX * sizeof( char ) ); 
  int length                 = argc;
  char argument;

  while( ( argument = getopt( argc, argv, "f" ) ) != -1 ) {
    if( argument == 'f' ) {
      funcp = nextAddrFromFile;
    } else if( argument == '?' ) {
      if( isprint( optopt ) ) {
        fprintf( stderr, "Unknown option: %c ...!\n", optopt );
      }
    } else {
      fprintf( stderr, USAGE, *argv );
    }
  }

  if( argc < 2 ) {
    fprintf( stderr, USAGE, *argv );
    exit( EXIT_FAILURE );
  }

  args   = &argv[optind];
  length = argc - optind;

  while( ( currentMacAddr = funcp( args, length ) ) != NULL ) {
    if( sendWOL( currentMacAddr ) < 0 ) {
      fprintf( stderr, "Error occured during sending the WOL magic packet for mac address: %s ...!\n", currentMacAddr->mac_addr_str );
    }
  }

  return EXIT_SUCCESS;
}

mac_addr_t *nextAddrFromArg( char **argument, int length ) {
  static int i = 0;
  mac_addr_t *currentMacAddr = (mac_addr_t *)malloc( sizeof( mac_addr_t ) );

  while( i < length ) {
    if( packMacAddr( argument[i], currentMacAddr ) < 0 ) {
      fprintf( stderr, "MAC Address ist not valid: %s ...!\n", argument[i] );
      i++;
      continue;
    }
    i++;
    return currentMacAddr;
  }

  return NULL;
}

mac_addr_t *nextAddrFromFile( char **filenames, int length ) {
  static FILE *fp            = NULL;
  static int fileNr          = 0;
  mac_addr_t *currentMacAddr = (mac_addr_t *)malloc( sizeof( mac_addr_t ) ); 
  char *currentInputMacAddr  = (char *)malloc( MAC_ADDR_STR_MAX * sizeof( char ) );
  
  while( fileNr < length ) {
    if( fp == NULL ) {
      if( ( fp = fopen( filenames[fileNr], "r" ) ) == NULL ) {
        fprintf( stderr, "Cannot open file %s: %s ...!\n", filenames[fileNr], strerror( errno ) );
        exit( EXIT_FAILURE );
      }
      printf( "Read from file %s:\n", filenames[fileNr] );
    }

    if( fgets( currentInputMacAddr, MAC_ADDR_STR_MAX, fp ) != NULL ) {
      currentInputMacAddr[strlen( currentInputMacAddr ) - 1] = '\0';
      if( packMacAddr( currentInputMacAddr, currentMacAddr ) < 0 ) {
        fprintf( stderr, "MAC Address ist not valid: %s ...!\n", currentInputMacAddr );
        continue;
      }
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

int packMacAddr( const char *mac, mac_addr_t *packedMac ) {
  char *tmpMac    = (char *)malloc( strlen( mac ) * sizeof( char ) );
  char *delimiter = ":";
  char *tok;
  int i;

  if( tmpMac == NULL ) {
    fprintf( stderr, "Cannot allocate memory for mac address: %s ...!\n", strerror( errno ) );
    return -1;
  }
  
  strncpy( tmpMac, mac, strlen( mac ) );
  tok = strtok( tmpMac, delimiter );
  for( i = 0; i < MAC_ADDR_MAX; i++ ) {
    if( tok == NULL ) {
      return -1;
    }

    packedMac->mac_addr[i] = (unsigned char)strtol( tok, NULL, CONVERT_BASE );
    tok = strtok( NULL, delimiter );
  }

  strncpy( packedMac->mac_addr_str, mac, MAC_ADDR_STR_MAX );
  return 0;
}

int sendWOL( const mac_addr_t *mac ) {
  int udpSocket;
  struct sockaddr_in addr;
  unsigned char packet[PACKET_BUF];
  int i, j, optval = 1;

  udpSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

  if( udpSocket < 0 ) {
    fprintf( stderr, "Cannot open socket: %s ...!\n", strerror( errno ) );
    return -1;
  }

  if( setsockopt( udpSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof( optval ) ) < 0 ) {
    fprintf( stderr, "Cannot set socket options: %s ...!\n", strerror( errno ) );
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
    fprintf( stderr, "Cannot send data: %s ...!\n", strerror( errno ) );
    return -1;
  }

  printf( "Successful sent WOL magic packet to %s ...!\n", mac->mac_addr_str );
  return 0;
}

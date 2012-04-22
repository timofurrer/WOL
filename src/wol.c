/*
*
* @author     Timo Furrer
* @co-author  Mogria
*
* @version    0.01.06
* @copyright  GNU General Public License
*
* @reopsitory https://github.com/timofurrer/WOL
*
*/

#include "wol.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

int main( int argc, char **argv )
{
  mac_addr_t *( *funcp )( char **args, int length ) = nextAddrFromArg;
  wol_header_t *currentWOLHeader = (wol_header_t *)malloc( sizeof( wol_header_t ));
  char **args                    = (char **)malloc( argc * ARGS_BUF_MAX * sizeof( char ));
  int length                     = argc;
  char argument;

  strncpy( currentWOLHeader->remote_addr, REMOTE_ADDR, ADDR_LEN );

  while(( argument = getopt( argc, argv, "r:f" )) != -1 )
  {
    if( argument == 'f' )
    {
      funcp = nextAddrFromFile;
    }
    else if( argument == 'r' )
    {
      strncpy( currentWOLHeader->remote_addr, optarg, ADDR_LEN );
    }
    else if( argument == '?' )
    {
      if( isprint( optopt ))
      {
        fprintf( stderr, "Unknown option: %c ...!\n", optopt );
      }
    }
    else
    {
      fprintf( stderr, USAGE, *argv );
    }
  }

  if( argc < 2 )
  {
    fprintf( stderr, USAGE, *argv );
    exit( EXIT_FAILURE );
  }

  args   = &argv[optind];
  length = argc - optind;

  while(( currentWOLHeader->mac_addr = funcp( args, length )) != NULL )
  {
    if( sendWOL( currentWOLHeader ) < 0 )
    {
      fprintf( stderr, "Error occured during sending the WOL magic packet for mac address: %s ...!\n", currentWOLHeader->mac_addr->mac_addr_str );
    }
  }

  return EXIT_SUCCESS;
}

mac_addr_t *nextAddrFromArg( char **argument, int length )
{
  static int i = 0;
  mac_addr_t *currentMacAddr = (mac_addr_t *)malloc( sizeof( mac_addr_t ));

  if( currentMacAddr == NULL )
  {
    fprintf( stderr, "Cannot allocate memory: %s ...!\n", strerror( errno ));
    exit( EXIT_FAILURE );
  }

  while( i < length )
  {
    if( packMacAddr( argument[i], currentMacAddr ) < 0 )
    {
      fprintf( stderr, "MAC Address ist not valid: %s ...!\n", argument[i] );
      i++;
      continue;
    }
    i++;
    return currentMacAddr;
  }

  return NULL;
}

mac_addr_t *nextAddrFromFile( char **filenames, int length )
{
  static FILE *fp            = NULL;
  static int fileNr          = 0;
  mac_addr_t *currentMacAddr = (mac_addr_t *)malloc( sizeof( mac_addr_t ));
  char *currentInputMacAddr  = (char *)malloc( MAC_ADDR_STR_MAX * sizeof( char ));

  if( currentMacAddr == NULL || currentInputMacAddr == NULL )
  {
    fprintf( stderr, "Cannot allocate memory: %s ...!\n", strerror( errno ));
    exit( EXIT_FAILURE );
  }

  while( fileNr < length )
  {
    if( fp == NULL )
    {
      if(( fp = fopen( filenames[fileNr], "r" )) == NULL )
      {
        fprintf( stderr, "Cannot open file %s: %s ...!\n", filenames[fileNr], strerror( errno ));
        exit( EXIT_FAILURE );
      }
      printf( "Read from file %s:\n", filenames[fileNr] );
    }

    if( fgets( currentInputMacAddr, MAC_ADDR_STR_MAX, fp ) != NULL )
    {
      currentInputMacAddr[strlen( currentInputMacAddr ) - 1] = '\0';
      if( packMacAddr( currentInputMacAddr, currentMacAddr ) < 0 )
      {
        fprintf( stderr, "MAC Address ist not valid: %s ...!\n", currentInputMacAddr );
        continue;
      }
      return currentMacAddr;
    }
    else
    {
      fclose( fp );
      fp = NULL;
      fileNr++;
      puts( "" );
    }
  }

  return NULL;
}

int packMacAddr( const char *mac, mac_addr_t *packedMac )
{
  char *tmpMac    = (char *)malloc( strlen( mac ) * sizeof( char ));
  char *delimiter = ":";
  char *tok;
  int i;

  if( tmpMac == NULL )
  {
    fprintf( stderr, "Cannot allocate memory for mac address: %s ...!\n", strerror( errno ));
    return -1;
  }

  strncpy( tmpMac, mac, strlen( mac ));
  tok = strtok( tmpMac, delimiter );
  for( i = 0; i < MAC_ADDR_MAX; i++ )
  {
    if( tok == NULL )
    {
      return -1;
    }

    packedMac->mac_addr[i] = (unsigned char)strtol( tok, NULL, CONVERT_BASE );
    tok = strtok( NULL, delimiter );
  }

  strncpy( packedMac->mac_addr_str, mac, MAC_ADDR_STR_MAX );
  return 0;
}

int sendWOL( const wol_header_t *wol_header )
{
  int udpSocket;
  struct sockaddr_in addr;
  unsigned char packet[PACKET_BUF];
  int i, j, optval = 1;

  if(( udpSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0 )
  {
    fprintf( stderr, "Cannot open socket: %s ...!\n", strerror( errno ));
    return -1;
  }

  if( setsockopt( udpSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof( optval )) < 0 )
  {
    fprintf( stderr, "Cannot set socket options: %s ...!\n", strerror( errno ));
    return -1;
  }

  addr.sin_family = AF_INET;
  addr.sin_port   = htons( REMOTE_PORT );
  if( inet_aton( wol_header->remote_addr, &addr.sin_addr ) == 0 )
  {
    fprintf( stderr, "Invalid remote ip address given: %s ...!\n", wol_header->remote_addr );
    return -1;
  }

  for( i = 0; i < 6; i++ )
  {
    packet[i] = 0xFF;
  }

  for( i = 1; i <= 16; i++ )
  {
    for( j = 0; j < 6; j++ )
    {
      packet[i*6+j] = wol_header->mac_addr->mac_addr[j];
    }
  }

  if( sendto( udpSocket, packet, sizeof( packet ), 0, (struct sockaddr *) &addr, sizeof( addr )) < 0 )
  {
    fprintf( stderr, "Cannot send data: %s ...!\n", strerror( errno ));
    return -1;
  }

  printf( "Successful sent WOL magic packet to %s ...!\n", wol_header->mac_addr->mac_addr_str );
  return 0;
}

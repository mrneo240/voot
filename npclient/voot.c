/*  voot.c

    VOOT Netplay protocol parser and constructor. It uses file handles and memory structures.
*/

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "voot.h"

static bool voot_check_packet(voot_packet *packet, uint32 size)
{
    /* See if there is as much data as advertised. */
    return ntohs(packet->header.size) == size - sizeof(voot_packet_header);
}

voot_packet* voot_parse_buffer(unsigned char *buffer, unsigned int buffer_size)
{
    voot_packet *packet;

    if (!voot_check_packet((voot_packet *) buffer, buffer_size))
        return NULL;

    packet = malloc(sizeof(voot_packet));
    memcpy(packet, buffer, buffer_size);

    packet->header.size = ntohs(packet->header.size);

    return packet;
}

voot_packet* voot_parse_socket(int32 socket)
{
    unsigned char data[BIUDP_SEGMENT_SIZE];
    int rx;

    rx = recvfrom(socket, data, sizeof(data), 0, NULL, NULL);

    return voot_parse_buffer(data, rx);
}    

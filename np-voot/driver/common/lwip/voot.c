/*  voot.c

    $Id: voot.c,v 1.8 2003/03/09 13:01:12 quad Exp $

DESCRIPTION

    VOOT netplay protocol implementation.

TODO

    Write a better packet handler chain function. Something more general and
    portable to other handlers.

    Figure out why first version packet off refreshing PCB is either ignored or simply not responded to. 

*/

#include <vars.h>
#include <util.h>
#include <anim.h>
#include <malloc.h>
#include <printf.h>
#include <init.h>

#include "net.h"

#include "lwip/udp.h"

#include "voot.h"

static anim_render_chain_f      old_anim_chain;
static voot_packet_handler_f    voot_packet_handler_chain;
static np_reconf_handler_f      my_reconfigure_handler;
static struct udp_pcb          *voot_pcb;
static uint32                   last_packet_time;

static void voot_handle_packet (void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, uint16 port)
{
    voot_packet *packet;

    packet = p->payload;

    /* STAGE: Ensure the packet size is enough for a basic header. */

    if (p->len < sizeof (packet->header))
    {
        pbuf_free (p);
        return;
    }

    /* STAGE: Fix the size's byte order. */

    packet->header.size = ntohs (packet->header.size);

    /* STAGE: Ensure the packet size is correct (or at least manageable.) */

    if (p->len < (sizeof (packet->header) + packet->header.size))
    {
        struct pbuf    *new;
        struct pbuf    *q;
        uint32          index;

        /* STAGE: Try to allocate a new pbuf, exclusively for the packet. */

        new = pbuf_alloc (PBUF_RAW, p->tot_len, PBUF_RAM);

        if (!new)
        {
            pbuf_free (p);
            return;
        }

        /* STAGE: Copy over the contents from the old pbuf to the new one. */

        index = 0;

        for (q = p; q != NULL; q = q->next)
        {
            memcpy (((uint8 *) new->payload) + index, q->payload, q->len);
            index += q->len;
        }

        /* STAGE: Free the original pbuf and switch the new one out for it. */

        pbuf_free (p);
        p = new;
        packet = p->payload;
    }

    /* STAGE: Focus on whoever is talking and update timeout timer. */

    udp_connect (upcb, addr, port);
    last_packet_time = time();

    /* STAGE: Pass on to the first packet handler. */

    /*
        STAGE: If someone in the chain wants to retain the packet, they
        simply return TRUE.
    */

    if (!voot_packet_handler_chain (packet, p))
        pbuf_free (p);
}

/* NOTE: This is guaranteed to be last in its chain. */

static bool voot_packet_handle_default (voot_packet *packet, void *ref)
{
    switch (packet->header.type)
    {
        case VOOT_PACKET_TYPE_COMMAND :
        {
            /* STAGE: Ensure there is actually a command. */

            if (!(packet->header.size))
                break;

            /* STAGE: Handle the version command. */

            if (packet->buffer[0] == VOOT_COMMAND_TYPE_VERSION)
            {
                uint32  freesize;
                uint32  max_freesize;

                malloc_stat (&freesize, &max_freesize); 

                voot_printf (VOOT_PACKET_TYPE_DEBUG, "VOX common, PRE-RELEASE [mem: %u block: %u]", freesize, max_freesize);
            }

            break;
        }

        default :
            break;
    }

    return FALSE;
}

static void voot_handle_reconfigure ()
{
    /* STAGE: Clear out reinitialized data... */

    voot_pcb = NULL;

    return my_reconfigure_handler ();
}

static void my_anim_chain (uint16 anim_code_a, uint16 anim_code_b)
{
    /* STAGE: Timeout if we haven't received packets after a certain amount of time. */

    if (time () > (last_packet_time + VOOT_CONNECT_TIMEOUT))
    {
        udp_connect (voot_pcb, IP_ADDR_BROADCAST, VOOT_UDP_PORT);
        udp_disconnect (voot_pcb);
    }

    if (old_anim_chain)
        return old_anim_chain (anim_code_a, anim_code_b);
}

void* voot_add_packet_chain (voot_packet_handler_f function)
{
    voot_packet_handler_f old_function;

    /* STAGE: Switch out the functions. */

    old_function = voot_packet_handler_chain;
    voot_packet_handler_chain = function;

    /* STAGE: Give them the old one so they can properly handle it. */

    return old_function;
}

bool voot_send_packet (uint8 type, const uint8 *data, uint32 data_size)
{
    voot_packet_header *netout;
    struct pbuf        *p;
    bool                retval;

    if (!voot_pcb)
        return FALSE;

    /*
        STAGE: Make sure the dimensions are legit and we have enough space
        for data_size + NULL.
    */

    if (data_size > VOOT_PACKET_BUFFER_SIZE)
        return FALSE;

    /* STAGE: Malloc the full-sized voot_packet. */

    p = pbuf_alloc (PBUF_TRANSPORT, sizeof (voot_packet_header) + data_size + 1, PBUF_RAM);

    if (!p)
        return FALSE;

    /* STAGE: Set the packet header information, including the NULL. */

    netout          = p->payload;
    netout->type    = type;
    netout->size    = htons (data_size + 1);

    /* STAGE: Copy over the input buffer data and append NULL. */

    memcpy (p->payload + sizeof (voot_packet_header), data, data_size);
    ((uint8 *) p->payload)[sizeof (voot_packet_header) + data_size] = 0x0;

    /* STAGE: Transmit the packet. */

    retval = !udp_send (voot_pcb, p);

    /* STAGE: Free the buffer and return. */

    pbuf_free (p);

    return retval;
}

int32 voot_aprintf (uint8 type, const char *fmt, va_list args)
{
    int32           i;
    char           *printf_buffer;

    /* STAGE: Allocate the largest possible buffer for the printf. */

    printf_buffer = malloc (VOOT_PACKET_BUFFER_SIZE);

    if (!printf_buffer)
        return 0;

    /* STAGE: Actually perform the printf. */

    i = vsnprintf (printf_buffer, VOOT_PACKET_BUFFER_SIZE, fmt, args);

    /* STAGE: Send the packet, if we need to, and maintain correctness. */

    if (i && !voot_send_packet (type, printf_buffer, i))
        i = 0;

    /* STAGE: Keep our memory clean. */

    free (printf_buffer);

    return i;  
}

int32 voot_printf (uint8 type, const char *fmt, ...)
{
    va_list args;
    int32   i;

    va_start (args, fmt);
    i = voot_aprintf (type, fmt, args);
    va_end (args);

    return i;
}

bool voot_send_command (uint8 type)
{
    return voot_send_packet (VOOT_PACKET_TYPE_COMMAND, &type, sizeof (type));
}

void voot_init (void)
{
    /* STAGE: Configure lwIP for UDP reception, if necessary. */

    if (!voot_pcb)
    {
        voot_pcb = udp_new ();

        udp_bind (voot_pcb, IP_ADDR_ANY, VOOT_UDP_PORT);
        udp_recv (voot_pcb, voot_handle_packet, NULL);
        udp_connect (voot_pcb, IP_ADDR_BROADCAST, VOOT_UDP_PORT);
        udp_disconnect (voot_pcb);
    }

    /* STAGE: Setup animation callback for LAN Mode timeout. */

    if (!old_anim_chain)
    {
        anim_init ();

        anim_add_render_chain (my_anim_chain, &old_anim_chain);
    }

    /* STAGE: Default packet handler chain... */

    if (!voot_packet_handler_chain)
        voot_packet_handler_chain = voot_packet_handle_default;

    /* STAGE: Because we malloc, we need to handle situations in which it
        disappears from under us. */

    if (!my_reconfigure_handler)
        my_reconfigure_handler = np_add_reconfigure_chain (voot_handle_reconfigure);
}

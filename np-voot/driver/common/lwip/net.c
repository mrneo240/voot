/*  net.c

    $Id: net.c,v 1.5 2002/11/14 20:56:08 quad Exp $

DESCRIPTION

    Network interface layer between the common library and lwIP

TODO

    Implement TCP.

    Add support for LAN Adapter.

    Add support for modem via SLIP.

    Handle cases of soft-reset flushing our malloc-able memory. (maybe
    switch lwIP back into static memory mode)

*/

#include <vars.h>
#include <anim.h>
#include <util.h>
#include <timer.h>

#include "bbaif.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"

#include "net.h"

static anim_render_chain_f  old_anim_chain;

#ifdef DO_DHCP
static struct dhcp_state   *client;
#endif

static void net_etharp_timer (void)
{
    static uint32   last_tick = 0;

    if (last_tick && ((time () - last_tick) >= (ARP_TMR_INTERVAL / 1000)))
    {
        etharp_tmr ();

        last_tick = time ();
    }
    else
        last_tick = time ();
}

#ifdef DO_DHCP
static void net_dhcp_coarse_timer (void)
{
    static uint32   last_tick = 0;

    if (last_tick && ((time () - last_tick) >= DHCP_COARSE_TIMER_SECS))
    {
        dhcp_coarse_tmr ();

        last_tick = time ();
    }
    else
        last_tick = time ();
}

static void net_dhcp_fine_timer (void)
{
    static uint32   last_tick = 0;

    if (last_tick && (timer_gen_micro(timer_gen_count() - last_tick) >= (DHCP_FINE_TIMER_MSECS * 1000)))
    {
        dhcp_fine_tmr ();

        last_tick = timer_gen_count ();
    }
    else
        last_tick = timer_gen_count ();
}
#endif

static void my_anim_chain (uint16 anim_code_a, uint16 anim_code_b)
{
    net_etharp_timer ();

#ifdef DO_DHCP
    net_dhcp_coarse_timer ();
    net_dhcp_fine_timer ();
#endif

    //net_tcp_timer ();

    if (old_anim_chain)
        return old_anim_chain (anim_code_a, anim_code_b);
}

void net_handle_tx (void *owner)
{
    struct netif   *netif;

    netif = (struct netif *) owner;

    /* NOTE: Dummy function, for the moment. */
}

void net_handle_rx (void *owner)
{
    struct netif   *netif;

    netif = (struct netif *) owner;

    /* NOTE: Dummy function, simply calls bbaif_input. */

    bbaif_input (netif);
}

void net_init (void)
{
    struct ip_addr  ipaddr;
    struct ip_addr  netmask;
    struct ip_addr  gw;
    struct netif   *netif;

    /* STAGE: Initialize the various timers. */

    timer_init ();
    anim_init ();

    anim_add_render_chain (my_anim_chain, &old_anim_chain);

    /* STAGE: Initialize lwIP. */

    mem_init ();
    memp_init ();
    pbuf_init (); 
    netif_init ();
    ip_init ();
    udp_init ();
    //tcp_init();

    /* STAGE: Intial IP configuration. */

#ifdef DO_DHCP
    IP4_ADDR(&gw, 0,0,0,0);
    IP4_ADDR(&ipaddr, 0,0,0,0);
    IP4_ADDR(&netmask, 0,0,0,0);
#else
#ifdef KIRK
    IP4_ADDR(&gw, 192,168,0,1);
    IP4_ADDR(&ipaddr, 192,168,0,11);
    IP4_ADDR(&netmask, 255,255,255,0);
#else
    IP4_ADDR(&gw, 10,1,1,254);
    IP4_ADDR(&ipaddr, 10,1,1,70);
    IP4_ADDR(&netmask, 255,0,0,0);
#endif
#endif

    /* STAGE: Enable the BBA network interface. */

    netif = netif_add (&ipaddr, &netmask, &gw, bbaif_init, ip_input);
  
    netif_set_default (netif);

#ifdef DO_DHCP
    /* STAGE: Enable address resolution via DHCP. */

    dhcp_init ();
    client = dhcp_start (netif);
#endif
}
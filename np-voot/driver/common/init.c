/*  main.c

    $Id: init.c,v 1.6 2002/06/29 12:57:04 quad Exp $

DESCRIPTION

    This is the C initialization core for the np-voot common library.

    The "np_init" function is the receiver of the loader's second stage function call.

    The "np_configure" function is the handler of main initializationm,
    after VOOT has been loaded.

*/

#include "vars.h"
#include "exception.h"
#include "exception-lowlevel.h"
#include "util.h"
#include "biosfont.h"
#include "malloc.h"
#include "assert.h"
#include "callbacks.h"

#include "init.h"

static void handle_bios_vector (void)
{
    /* STAGE: Make sure the BIOS hasn't done anything funny. */

    assert_x (dbr () == ubc_handler_lowlevel, dbr ());

    /* STAGE: It's nice to not have a corrupted BSS segment. */
 
    assert_x ((uint8 *) vbr () >= end, vbr ());

    /*
        STAGE: Give the module configuration core a chance to do something
        wonky.
    */

    module_bios_vector ();
}

/*
    NOTE: We can only support four arguments. Any further would require
    stack management.
*/

void np_initialize (void *arg1, void *arg2, void *arg3, void *arg4)
{
    /* STAGE: Initialize the exception handling chain. */

    exception_init ();

    /* STAGE: Give the module initialization core a chance for overrides. */

    module_initialize ();

    /* STAGE: Patch the c000 handler so the BIOS won't crash. */

    bios_patch_handler = handle_bios_vector;    /* NOTE: This must occur before the copy. */
    memcpy ((uint32 *) 0x8c00c000, bios_patch_base, bios_patch_end - bios_patch_base);

    /*
        STAGE: Make sure the cache is invalidated before jumping to a
        changed future.
    */

    flush_cache ();

    /* STAGE: Special BIOS reboot. Doesn't kill the DBR. */

    (*(uint32 (*)()) (*(uint32 *) 0x8c0000e0)) (-3);
}

void np_configure (void)
{
    /* STAGE: Cache the biosfont address. (REQUIRED) */

    bfont_init (); 

    /* STAGE: Locate and assign syMalloc functionality. (REQUIRED) */

    malloc_init ();

    /* STAGE: Give the module configuration core a chance to set itself up. */

    module_configure ();
}

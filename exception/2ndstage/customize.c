/*  customize.c

DESCRIPTION

    Customization reverse engineering helper code.

TODO

    VMU Loading.

*/

#include "vars.h"
#include "exception.h"
#include "exception-lowlevel.h"
#include "util.h"
#include "voot.h"
#include "gamedata.h"

#include "customize.h"

static const uint8 custom_func_key[] = { 0xe6, 0x2f, 0x53, 0x6e, 0xd6, 0x2f, 0xef, 0x63, 0xc6, 0x2f, 0x38, 0x23 };
static uint32 custom_func;
static char gamebin_c[20];
static customize_check_mode custom_status;

uint32 do_osd = 0;

#define HARD_BREAK

static void customize_locate_func(void)
{
#ifdef HARD_BREAK
    return;
#endif

    /* STAGE: Locate the customization function beginning. */
    custom_func = (uint32) search_sysmem_at(custom_func_key, sizeof(custom_func_key), GAME_MEM_START, SYS_MEM_END);

    /* STAGE: If we found the customization function, configure UBC Channel
        A to break on it. */
    if (custom_func)
    {
        *UBC_R_BARA = custom_func;
        *UBC_R_BAMRA = UBC_BAMR_NOASID;
        *UBC_R_BBRA = UBC_BBR_READ | UBC_BBR_INSTRUCT;
    }
    /* STAGE: Otherwise set our break on Temjin's customize data. */
    else
    {
        *UBC_R_BARA = 0x8ccf9f15;
        *UBC_R_BAMRA = UBC_BAMR_NOASID;
        *UBC_R_BBRA = UBC_BBR_READ | UBC_BBR_OPERAND;
    }

    ubc_wait();
}

bool customize_reinit(void)
{
#ifdef HARD_BREAK
    *UBC_R_BARA = 0x8c397f62;
    *UBC_R_BAMRA = UBC_BAMR_NOASID;
    *UBC_R_BBRA = UBC_BBR_READ | UBC_BBR_INSTRUCT;

    ubc_wait();

    return TRUE;
#endif

    /* STAGE: Is the current vector still valid? */
    if (custom_status == RUN && !memcmp((uint8 *) custom_func, custom_func_key, sizeof(custom_func_key)))
        return TRUE;
    /* STAGE: Is the current vector still known broken? (with no chance of having changed. */
    else if (custom_status == LOAD && !strcmp((uint8 *) VOOT_MEM_START, gamebin_c))
        return FALSE;
    /* STAGE: Ignore any references from outside the game module. */
    else if (spc() < 0x8c270000)
        return FALSE;

    /* STAGE: Function reference is either invalid or we're allowed to search again. */
    customize_locate_func();

    /* STAGE: If the function was located, set our status to run but DO NOT
        allow the handler to process this iteration. */
    if (custom_func)
    {
        replace_game_text("GATE FIELD INTERVENTION",
                          "A shark on whiskey");
        replace_game_text("STATUS CRITICAL    ",
                          "is mighty risky.");
        replace_game_text("VR EYE CAMERA 1P",
                          "PUNK BITCH CAM");
        replace_game_text("(pause)",
                          "(stop)");

        custom_status = RUN;
    }
    /* STAGE: No customization function was located. Continue in LOAD mode. */
    else
    {
        custom_status = LOAD;
        strncpy(gamebin_c, (uint8 *) VOOT_MEM_START, sizeof(gamebin_c));
    }

    /* STAGE: This iteration is invalid. Don't let the handler touch it. */
    return FALSE;
}

void customize_init(void)
{
    exception_table_entry new;

    /* STAGE: Notify the module we're in the LOAD process. */
    custom_status = LOAD;
    custom_func = 0x0;

    /* STAGE: Make a (futile) search for the customization function. */
    customize_reinit();

    /* STAGE: Add exception handler for serial access. */
    new.type = EXP_TYPE_GEN;
    new.code = EXP_CODE_UBC;
    new.handler = customize_handler;

    add_exception_handler(&new);
}

static void* my_customize_handler(register_stack *stack, void *current_vector)
{
    /* STAGE: Make a vague attempt at a reinitialization. */
    if (!customize_reinit())
        return current_vector;

    // 8c389468
    //customize (VR %u, PLR %u, CUST %x, CMAP %x)
    // 8c39bc04
    //osd_midboss (COL %u, ROW %u, TEXT '%s', FONT %x)
    // 8c3970e2
    //osd_game (COL %u, ROW %u, TEXT '%s', COLOR b%u, ARROWS b%u)

    //voot_printf(VOOT_PACKET_TYPE_DEBUG, "(%x, %x, %x, %x) from %x", stack->r4, stack->r5, stack->r6, stack->r7, stack->pr);

    //(*(void (*)()) 0x8c3970e2)(0x100, 0x100, "test123", 0, 0, do_osd);

    return current_vector;
}

void* customize_handler(register_stack *stack, void *current_vector)
{
    /* STAGE: We only break on the serial (channel A) exception. */
    if (*UBC_R_BRCR & UBC_BRCR_CMFA)
    {
        /* STAGE: Be sure to clear the proper bit. */
        *UBC_R_BRCR &= ~(UBC_BRCR_CMFA);

        /* STAGE: Pass control to the actual code base. */
        return my_customize_handler(stack, current_vector);
    }
    else
        return current_vector;
}
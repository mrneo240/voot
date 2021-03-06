#ifndef __ASIC_H__
#define __ASIC_H__

#include "vars.h"
#include "system.h"

#define ASIC_BASE           (0xa05f6900)
#define ASIC_IRQ_STATUS     (REGISTER(uint32) (ASIC_BASE + 0x00))
#define ASIC_IRQ13_MASK     (REGISTER(uint32) (ASIC_BASE + 0x10))
#define ASIC_IRQ11_MASK     (REGISTER(uint32) (ASIC_BASE + 0x20))
#define ASIC_IRQ9_MASK      (REGISTER(uint32) (ASIC_BASE + 0x30))

/* Masks taken from an e-mail from Marcus Comstedt on dcdev */

#define ASIC_MASK0_TADONE           0x04    /* Rendering complete */
#define ASIC_MASK0_RASTER_BOTTOM    0x08    /* Bottom raster event */
#define ASIC_MASK0_RASTER_TOP       0x10    /* Top raster event */
#define ASIC_MASK0_VSYNC            0x20    /* Vsync event */

#define ASIC_MASK0_OPAQUE_POLY      0x080   /* Opaque polygon binning complete */
#define ASIC_MASK0_OPAQUE_MOD       0x100   /* Opaque modifier binning complete */
#define ASIC_MASK0_TRANS_POLY       0x200   /* Transparent polygon binning complete */
#define ASIC_MASK0_TRANS_MOD        0x400   /* Transparent modifier binning complete */

#define ASIC_MASK0_MAPLE_DMA        0x01000     /* Maple DMA complete */
#define ASIC_MASK0_MAPLE_ERROR      0x02000     /* Maple error */
#define ASIC_MASK0_GDROM_DMA        0x04000     /* GD-ROM DMA complete */
#define ASIC_MASK0_AICA_DMA         0x08000     /* AICA DMA complete */
#define ASIC_MASK0_EXT1_DMA         0x10000     /* External DMA 1 complete */
#define ASIC_MASK0_EXT2_DMA         0x20000     /* External DMA 2 complete */
#define ASIC_MASK0_MYSTERY_DMA      0x40000     /* Mystery DMA complete */

#define ASIC_MASK0_PUNCHPOLY        0x200000    /* Punchthrough polygon binning complete */

#define ASIC_MASK1_GDROM            0x01    /* GD-ROM command status */
#define ASIC_MASK1_AICA             0x02    /* AICA */
#define ASIC_MASK1_MODEM            0x04    /* Modem ? */
#define ASIC_MASK1_PCI              0x08    /* Expansion port (PCI Bridge) */

#define ASIC_TABLE_SIZE  10

typedef struct
{
    uint32  mask0, mask1;
    uint32  irq;

    bool    (*handler)(const void *, register_stack *);
} asic_lookup_table_entry;

typedef struct
{
    /* Function hooks for various interrupts */
    asic_lookup_table_entry   table[ASIC_TABLE_SIZE];
} asic_lookup_table;

void dump_asic(void);
uint32 add_asic_handler(asic_lookup_table *asic_table, const asic_lookup_table_entry *new_entry);
bool handle_asic_exception(const asic_lookup_table *asic_table, register_stack *stack);
void init_asic_handler(exception_table *exp_table);

#endif

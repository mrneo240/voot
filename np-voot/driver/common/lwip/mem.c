/*  mem.c

    $Id: mem.c,v 1.2 2002/11/12 02:00:55 quad Exp $

DESCRIPTION

    Interface module emulating lwIP's bss space memory manager. This
    translates the functions over to the Katana heap.

TODO

    Finish integrating the debugging statistics functions.

*/

#include <lwip/debug.h>

#include <lwip/arch.h>
#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/mem.h>

#include <lwip/sys.h>

#include <lwip/stats.h>

#include <vars.h>
#include <malloc.h>

void mem_init (void)
{
    uint32  freesize;
    uint32  max_freesize;

    malloc_init ();
    malloc_stat (&freesize, &max_freesize);

#ifdef MEM_STATS
    stats.mem.avail = max_freesize;
#endif /* MEM_STATS */
}

void* mem_malloc (mem_size_t size)
{
    return malloc (size);
}

void mem_free (void *mem)
{
    return free (mem);
}

void* mem_realloc (void *mem, mem_size_t size)
{
    return realloc (mem, size);
}

void* mem_reallocm (void *rmem, mem_size_t newsize)
{
    void   *nmem;

    nmem = malloc (newsize);

    if(nmem == NULL)
    {
        return realloc (rmem, newsize);
    }

    bcopy (rmem, nmem, newsize);
    free (rmem);

    return nmem;
}

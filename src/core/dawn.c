#include <stdlib.h>
#include <stdio.h>

#include "dawn.h"

int32_t dawn_init_context(dawn_context_t *ctx, dawn_read read, dawn_write write)
{
    int32_t ret = 0;

    if (ctx == NULL) {
        ctx = (dawn_context_t *)dawn_alloc(sizeof(dawn_context_t));
        memset(ctx , 0x00, sizeof(dawn_context_t));
    }
    
    if (NULL==ctx || NULL == read || NULL = write) {
        ret = -1;
        goto out;
    }

    ctx->read = read;
    ctx->write = write;
    out:
    return ret;
}
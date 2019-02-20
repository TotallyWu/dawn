#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dawn.h"

int32_t _dawn_wait_signal(dawn_context_t *ctx, uint8_t signal)
{
    int32_t ret = 0;
    uint8_t sig = 0;
    uint8_t count = 0;

loop:
    ret = ctx->read(&sig, (uint16_t)(1), ctx->timeout_ms);

    if (1 != ret) {
        ret = DAWN_ERR_RECV_FAILED;
        goto out;
    }

    if (sig != signal) {
        if (count++ > ctx->retry_times) {
            ret = DAWN_ERR_RECV_FAILED;
            goto out;
        }

        goto loop;
    }

    ret = 0;
    out:
    return ret;
}

#define _dawn_wait_ack(ctx) _dawn_wait_signal(ctx, ACK)
#define _dawn_wait_syn(ctx) _dawn_wait_signal(ctx, SYN)
#define _dawn_wait_hang_up(ctx) _dawn_wait_signal(ctx, HANG_UP)

int32_t dawn_init_context(dawn_context_t *ctx, dawn_read read, dawn_write write)
{
    int32_t ret = 0;

    if (ctx == NULL) {
        ctx = (dawn_context_t *)dawn_alloc(sizeof(dawn_context_t));
        memset(ctx , 0x00, sizeof(dawn_context_t));
    }

    if (NULL==ctx || NULL == read || NULL == write) {
        ret = -1;
        goto out;
    }

    ctx->read = read;
    ctx->write = write;
    out:
    return ret;
}



int32_t _dawn_send_mtu(dawn_context_t *ctx){
    int32_t ret = 0;
    uint8_t *data;

    if (NULL==ctx ||ctx->user_data.buf == NULL || ctx->user_data.len == 0 || ctx->write == NULL) {
        ret = -1;
        goto out;
    }

    data = dawn_alloc(ctx->user_data.len+2+2);
    if (NULL == data) {
        ret = DAWN_ERR_OOM;
        goto out;
    }

    data[0] = ctx->user_data.remain / 256;
    data[1] = ctx->user_data.remain % 256;
    data[2] = ctx->user_data.len / 256;
    data[3] = ctx->user_data.len % 256;
    memcpy(data+2, ctx->user_data.buf, ctx->user_data.len);
    ret = ctx->write(data, (uint16_t)ctx->user_data.len+4);

    if (ret != ctx->user_data.len+4) {
        ret = DAWN_ERR_SEND_FAILED;
    }

    out:
    dawn_free(data);
    return ret;
}

int32_t _dawn_handshake(dawn_context_t *ctx)
{
    int32_t ret = 0;
    uint8_t syn = SYN;

    ret = ctx->write(&syn, (uint16_t)1);

    if (1!=ret) {
        ret = DAWN_ERR_SEND_FAILED;
        goto out;
    }

    ret = _dawn_wait_ack(ctx);
    out:
    return ret;
}

int32_t _dawn_ack(dawn_context_t *ctx)
{
    int32_t ret = 0;
    uint8_t ack = ACK;

    ret = ctx->write(&ack, (uint16_t)1);

    if (1!=ret) {
        ret = DAWN_ERR_SEND_FAILED;
        goto out;
    }

    ret = 0;
    out:
    return ret;
}

int32_t _dawn_hang_up(dawn_context_t *ctx)
{
    int32_t ret = 0;
    uint8_t signal = HANG_UP;

    ret = ctx->write(&signal, (uint16_t)(1));

    if (1!=ret) {
        ret = DAWN_ERR_SEND_FAILED;
        goto out;
    }

    ret = 0;
    out:
    return ret;
}

int32_t dawn_transfer(dawn_context_t *ctx, void *data, uint16_t len){
    int32_t ret = 0;
    uint16_t count = 0;
    uint8_t *pData = data;
    if (NULL==ctx ||NULL == data || len == 0 || ctx->write == NULL ||ctx->read == NULL) {
        ret = -1;
        goto out;
    }

    ret = _dawn_handshake(ctx);
    if (ret != 0) {
        goto out;
    }

    count = (len + (ctx->mtu-1)) / ctx->mtu;

    for(uint16_t i = 0; i < count; i++)
    {
        ctx->user_data.remain = count-i-1;
        ctx->user_data.buf = pData;

        if (i == (count - 1))
        {
            ctx->user_data.len = len;
        }else
        {
            ctx->user_data.len = ctx->mtu;
        }

        len -= ctx->mtu;
        pData += ctx->mtu;

        ret = _dawn_send_mtu(ctx);

        if (0!= ret) {
            goto out;
        }

        ret = _dawn_wait_ack(ctx);

        if (0!= ret) {
            goto out;
        }
    }

    ret = _dawn_hang_up(ctx);

    out:
    return ret;
}

int32_t _dawn_receive_mtu(dawn_context_t *ctx)
{
    int32_t ret = 0;
    uint8_t *pData = (uint8_t *)ctx->user_data.buf;
    uint16_t exp_len ;

    if (NULL==ctx ||NULL == ctx->user_data.buf || ctx->user_data.len == 0 ||ctx->read == NULL) {
        ret = -1;
        goto out;
    }

    exp_len = 4;

    ret = ctx->read(ctx->user_data.buf, exp_len, ctx->timeout_ms);

    if (ret != exp_len) {
        ret = DAWN_ERR_RECV_FAILED;
        goto out;
    }

    exp_len = (uint16_t)(pData[2]*256 + pData[3]);
    ret = ctx->read(ctx->user_data.buf, exp_len, ctx->timeout_ms);

    if (ret != exp_len) {
        ret = DAWN_ERR_RECV_FAILED;
        goto out;
    }

    ret = 0;
    out:
    return ret;
}

int32_t dawn_receive(dawn_context_t *ctx){
    int32_t ret = 0;
    uint8_t *pData = ctx->user_data.buf;
    uint16_t len = ctx->user_data.len;
    uint8_t *tmp=NULL;
    uint16_t tmp_len = 0;

    if (NULL==ctx ||NULL == ctx->user_data.buf || ctx->user_data.len == 0 || ctx->write == NULL ||ctx->read == NULL) {
        ret = -1;
        goto out;
    }

    ret = _dawn_wait_syn(ctx);

    if (0!=ret) {
        goto out;
    }

    ret = _dawn_ack(ctx);

    if (0!=ret) {
        goto out;
    }

    tmp = (uint8_t *)dawn_alloc(ctx->mtu+4);
    ctx->user_data.buf = tmp;

    do
    {
        ret = _dawn_receive_mtu(ctx);

        if (0 != ret) {
            goto out;
        }

        ctx->user_data.len = tmp[2] * 256 + tmp[3];
        if (len < (tmp_len+ctx->user_data.len)) {
            ret = DAWN_ERR_OOM;
            goto out;
        }

        memcpy(pData+tmp_len, ctx->user_data.buf, ctx->user_data.len);
        tmp_len += ctx->user_data.len;

        ret = _dawn_ack(ctx);
        ctx->user_data.remain = tmp[0] * 256 + tmp[1];

        if (ctx->user_data.remain == 0) {
            break;
        }

    } while (1);

    ret = _dawn_wait_hang_up(ctx);

    ctx->user_data.buf=pData;
    ctx->user_data.len = tmp_len;

    out:
    dawn_free(tmp);
    return ret;
}

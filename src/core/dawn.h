#ifndef _DAWN_H_
#define _DAWN_H_


#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#ifndef int32_t
typedef int int32_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

#ifndef uint8_t
typedef unsigned char uint8_t;
#endif

typedef enum _dawn_state_t{
    DAWN_STATE_IDLE,
    DAWN_STATE_READY,
    DAWN_STATE_TRANSFER,
    DAWN_STATE_TBD,
}dawn_state_t;


typedef int32_t (*dawn_read)(void *buf, uint16_t len, uint16_t timeout_ms);
typedef int32_t (*dawn_write)(void *buf, uint16_t len);

typedef struct _dawn_context_t{
    dawn_state_t state;
    uint16_t mtu;
    uint16_t index;

    struct {
        void *buf;
        uint16_t len;
        uint16_t remain;
    }user_data;

    dawn_read read;
    dawn_write write;
    uint16_t timeout_ms;
    uint8_t retry_times;
}dawn_context_t;


#define dawn_alloc(size) malloc(size)
#define dawn_free(p) do{ \
                        free(p);\
                    }while(0)

#define SYN 0x22
#define ACK 0x06
#define HANG_UP  0x04


enum{
    DAWN_ERR_SEND_FAILED = -2019,
    DAWN_ERR_RECV_FAILED ,
    DAWN_ERR_OOM ,
};

int32_t dawn_init_context(dawn_context_t *ctx, dawn_read read, dawn_write write);

int32_t dawn_transfer(dawn_context_t *ctx, void *data, uint16_t len);

int32_t dawn_receive(dawn_context_t *ctx);
#endif
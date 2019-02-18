#ifndef _DAWN_H_
#define _DAWN_H_

#include <stdint.h>

typedef enum _dawn_state_t{
    DAWN_STATE_IDLE,
    DAWN_STATE_READY,
    DAWN_STATE_TRANSFER,
    DAWN_STATE_TBD,
}dawn_state_t;


typedef int32_t (*dawn_read)(void *buf, uint16_t *len, uint16_t timeout_ms);
typedef int32_t (*dawn_write)(void *buf, uint16_t *len, uint16_t timeout_ms);

typedef struct _dawn_context_t{
    dawn_state_t state;
    uint16_t mtu;
    uint16_t index;
    
    struct {
        void *buf;
        uint16_t len;
    }*user_data;

    dawn_read read;
    dawn_write write;
}dawn_context_t;


#define dawn_alloc(size) malloc(size)
#define dawn_free(p) do{ \
                        free(p);\
                    }while(0)


#endif
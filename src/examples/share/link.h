#ifndef _LINK_H_
#define _LINK_H_

#include <stdint.h>

 int32_t dawn_example_link_read(void *buf, uint16_t len, uint16_t timeout_ms);

 int32_t dawn_example_link_write(void *buf, uint16_t len);

#endif


#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdio.h>
#include <stdarg.h>

#define dawn_printf(fmt, ...) do{\
                            printf("[%s-%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);\
                            printf("\r\n");\
                        } while (0)

#define dawn_print_hex(hint, va, l) do{\
                                        uint8_t *p = (uint8_t *)(va);\
                                        dawn_printf("%s\r\n", hint);\
                                        for(size_t k = 0; k < (l); k++)\
                                        {\
                                            printf("%02X ", p[k]);\
                                        }\
                                        printf("\r\n");\
                                    } while (0)


#endif


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include "dawn.h"
#include "platform.h"

 #define MAXLINE 1024
#define ERR_RANDOM    -100
#define ERR_OOM    -101
#define ERR_TRANSFER    -102
#define ERR_RECV    -103
#define ERR_MISMATCH    -104

 void print_usage(void)
 {
     fprintf(stdout, "slave usage\r\n");
     fprintf(stdout, "slave [option]\r\n");
     fprintf(stdout, "-h host, default is 127.0.0.1 \r\n");
     fprintf(stdout, "-p port, default is 6666 \r\n");
 }

int socketfd = -1;

int32_t dawn_example_slave_read(void *buf, uint16_t len, uint16_t timeout_ms)
 {
	 int ret = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;
    uint8_t *p_buf = (uint8_t *)buf;
    uint16_t offset = 0;

    if (socketfd <= 2) {
        return -1;
    }

    FD_ZERO(&rfds);
    FD_SET(socketfd, &rfds);

    /* Wait up to five seconds. */
    do{
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000)*1000;

        retval = select(socketfd+1, &rfds, NULL, NULL, &tv);
        /* Don't rely on the value of tv now! */

        if (retval == -1){
            perror("select()");
            break;
        }
        else if (retval){
            ret = recv(socketfd, p_buf+offset, len-offset,0);
            if (ret > 0) {
                offset += ret;
            }
        }
        else{
            dawn_printf("No data within five seconds.\n");
            break;
        }
    }while(offset < len);

    ret = offset;
	 return ret;
 }

 int32_t dawn_example_slave_write(void *buf, uint16_t len)
 {
	 int ret = 0;
     uint16_t offset = 0;


    if (socketfd <= 2) {
        return -1;
    }

     do
     {
         ret = send(socketfd, buf+offset, len-offset, MSG_CONFIRM);

         if (ret > 0) {
             offset += ret;
         }

     } while (offset<len);

	 return ret;
 }


int random_gen(void *p, uint16_t len){
    int ret = 0;
    int fd = 0;
    int offset = 0;
    uint8_t *pa = (uint8_t *)p;

    fd = open("/dev/random", O_RDONLY);

    if (fd <= 2) {
        printf("open device[/dev/random] failed:%d %s", fd, strerror(errno));
        return -1;
    }

    // while(offset<len){
        ret = read(fd, pa+offset, len);
        // if (ret<0) {
        //     break;
        // }

        // offset +=  ret;
        // len -= ret;
    // }

    close(fd);

    return ret;
}

int stress_test(dawn_context_t *ctx){
    int ret = 0;
    uint8_t *data;
    uint16_t len = 0;
    uint8_t *recv_data;

    if (ctx == NULL) {
        exit(0);
    }

    ret = random_gen(&len, sizeof(uint16_t));

    if (ret != sizeof(uint16_t)) {
        ret =  ERR_RANDOM;
        goto out;
    }

    len%=256;
    printf("random len:%d\r\n", len);
    data = (uint8_t *)malloc(len);
    ret = random_gen(data, len);

    if (ret != len) {
        printf("return:%d vs exp:%d\r\n", ret, len);
    }

    len = ret;
    usleep(1000*100);
    ret = dawn_transfer(ctx, data, len);

    if (ret != 0) {
        ret = ERR_TRANSFER;
        goto out;
    }

    dawn_print_hex("data transfered!\n", data, len);
    recv_data = (uint8_t *)malloc(len);
    ctx->user_data.buf = recv_data;
    ctx->user_data.len = len;
    ret = dawn_receive(ctx);
    if (ret!=0) {
        ret = ERR_RECV;
        goto out;
    }

    if (ctx->user_data.len != len) {
        ret = ERR_MISMATCH;
        goto out;
    }

    if (memcmp(data, ctx->user_data.buf,len)) {
        dawn_print_hex("recv data:\n", ctx->user_data.buf, ctx->user_data.len);
        ret = ERR_MISMATCH;
        goto out;
    }

out:
    dawn_free(data);
    dawn_free(recv_data);

    return ret;
}
typedef struct _st{
                    int total;
                    int suc;
                    int fail;
                    int fail_random;
                    int fail_oom;
                    int fail_transfer;
                    int fail_recv;
                    int fail_mismatch;
                }statistic_t;

void show_statistic(statistic_t *st){
    if (st ==NULL) {
        return;
    }

    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
    printf("        Total:%d\r\n", st->total);
    printf("        Success:%d  [%f\%]\r\n", st->suc, (st->suc*1.0 / st->total)*100);
    printf("        Failed:%d  [%f\%]\r\n", st->fail, (st->fail*1.0 / st->total)*100);
    printf("        Random Failed:%d  [%f\%]\r\n", st->fail_random, (st->fail_random*1.0 / st->total)*100);
    printf("        OOM Failed:%d  [%f\%]\r\n", st->fail_oom, (st->fail_oom*1.0 / st->total)*100);
    printf("        Transfer Failed:%d  [%f\%]\r\n", st->fail_transfer, (st->fail_transfer*1.0 / st->total)*100);
    printf("        Recv Failed:%d  [%f\%]\r\n", st->fail_recv, (st->fail_recv*1.0 / st->total)*100);
    printf("        Mismatch Failed:%d  [%f\%]\r\n", st->fail_mismatch, (st->fail_mismatch*1.0 / st->total)*100);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
}

 int main(int argc,char **argv)
 {
    char *servInetAddr = (char *)"127.0.0.1";
    struct sockaddr_in sockaddr;
    int opt = 1;
    int port = 1111;
    int test_type = 0;
    int on = 1;

    // if(argc != 5)
    // {
    //     print_usage();
    //     exit(0);
    // }
    print_usage();

    do
    {
        if (argv[opt] ==NULL ||
        argv[opt][0] != '-'||
        argv[opt][2] != 0) {
            // print_usage();
            // exit(0);
            break;
        }

        switch (argv[opt][1])
        {
            case 'h':
                opt++;
                if (opt>argc) {
                    print_usage();
                    exit(0);
                }

                servInetAddr = argv[opt];
                break;
            case 'p':
                opt++;
                 if (opt>argc) {
                    print_usage();
                    exit(0);
                }
                port = atoi(argv[opt]);
                break;
            case 't':
                opt++;
                 if (opt>argc) {
                    print_usage();
                    exit(0);
                }
                test_type = atoi(argv[opt]);
                break;
            default:
                break;
        }

        opt++;
    } while (opt<argc);


    socketfd = socket(AF_INET,SOCK_STREAM,0);
    setsockopt( socketfd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));
    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    inet_pton(AF_INET,servInetAddr,&sockaddr.sin_addr);
    printf("try to connect to %s:%d\r\n", servInetAddr, port);
    if((connect(socketfd,(struct sockaddr*)&sockaddr,sizeof(sockaddr))) < 0 )
    {
        printf("connect error %s errno: %d\n",strerror(errno),errno);
        exit(0);
    }else
    {
        dawn_context_t dawn_slave;
        char *str;
        int ret = 0;
        statistic_t st;

        dawn_slave.mtu = 10;
        dawn_slave.retry_times = 3;
        dawn_slave.timeout_ms = 1000*10;
        dawn_slave.state = 0;
        ret = dawn_init_context(&dawn_slave, dawn_example_slave_read, dawn_example_slave_write);
        if (0!=ret) {
            printf("dawn_init_context failed:%d\r\n", ret);
            return 0;
        }

        switch(test_type) {
            default:
            case 0: //normal mode

            break;
            case 1://stress test mode
                memset(&st, 0x00, sizeof(st));
                do
                {
                    st.total++;
                    printf("#######################################################################\r\n");
                    printf("                    Round#%d\r\n", st.total);
                    printf("#######################################################################\r\n");
                    ret = stress_test(&dawn_slave);

                    switch (ret)
                    {
                        case 0:
                            st.suc++;
                            break;
                        case ERR_OOM:
                            st.fail_oom++;
                            break;
                        case ERR_RANDOM:
                            st.fail_random++;
                            break;
                        case ERR_TRANSFER:
                            st.fail_transfer++;
                            break;
                        case ERR_RECV:
                            st.fail_recv++;
                            break;
                        case ERR_MISMATCH:
                            st.fail_mismatch++;
                            break;
                        default:
                            dawn_printf("uknow code:%d\n", ret);
                            st.fail++;
                            break;
                    }

                    show_statistic(&st);
                } while (1);

            break;
        }

        uint8_t buf[MAXLINE];

        usleep(200*1000);
        str = (char *)"Hello Iris! \nAre you okay? \nI have lots of things to talk with you.\r\n";
        ret = dawn_transfer(&dawn_slave, str, strlen(str));
        if (0!=ret) {
            printf("dawn_transfer failed:%d\r\n", ret);
            return 0;
        }

        dawn_slave.user_data.buf = buf;
        dawn_slave.user_data.len = MAXLINE;
        ret = dawn_receive(&dawn_slave);

        if (0!=ret) {
            printf("dawn_receive failed:%d\r\n", ret);
            return 0;
        }

        printf("slave receive:%s\r\n", (char *)dawn_slave.user_data.buf);
        close(socketfd);
    }
 }
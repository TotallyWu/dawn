#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

 #include "dawn.h"
 #include "platform.h"


int listenfd,connfd = -1;

 #define MAXLINE 1000

 int32_t dawn_example_master_read(void *buf, uint16_t len, uint16_t timeout_ms)
 {
	 int ret = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;
    uint8_t *p_buf = (uint8_t *)buf;
    uint16_t offset = 0;

    if (connfd <= 2) {
        return -1;
    }

    FD_ZERO(&rfds);
    FD_SET(connfd, &rfds);

    /* Wait up to five seconds. */
    do{
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000)*1000;

        retval = select(connfd+1, &rfds, NULL, NULL, &tv);
        /* Don't rely on the value of tv now! */

        if (retval == -1){
            dawn_printf("select()");
            break;
        }
        else if (retval){
            ret = recv(connfd, p_buf+offset, len-offset, 0);
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

 int32_t dawn_example_master_write(void *buf, uint16_t len)
 {
	 int ret = 0;
     uint16_t offset = 0;


    if (connfd <= 2) {
        return -1;
    }

     do
     {
         ret = send(connfd, buf+offset, len-offset, MSG_CONFIRM);

         if (ret > 0) {
             offset += ret;
         }
     } while (offset<len);

	 return ret;
 }

 void print_usage(void)
 {
     fprintf(stdout, "master usage\r\n");
     fprintf(stdout, "master [option]\r\n");
     fprintf(stdout, "-p port, default is 6666 \r\n");
     fprintf(stdout, "-t test type, default is 0 1 for stress test \r\n");
 }

 typedef struct statistics_t{
     uint32_t total;
     uint32_t failed;
     uint32_t suc;
     uint32_t recv_failed;
     uint32_t transfer_failed;
 }statistics_t;

 void print_stics(statistics_t *st){
     if (NULL == st) {
         return;
     }

    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
    printf("                    staticts\r\n");
    printf("        Total:%d\r\n", st->total);
    printf("        Success:%d      [%f\%]\r\n", st->suc, ((st->suc*1.0)/st->total) *100);
    printf("        Recv failed:%d      [%f\%]\r\n", st->recv_failed, ((st->recv_failed*1.0)/st->total) *100);
    printf("        Transfer failed:%d      [%f\%]\r\n", st->transfer_failed, ((st->transfer_failed*1.0)/st->total) *100);
    printf("        Failed:%d       [%f\%]\r\n", st->failed, ((st->failed*1.0)/st->total) *100);
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
 }

 void *stress_test(void *argv)
 {
    dawn_context_t dawn_master;
    int ret = 0;
    uint8_t buf[MAXLINE];
    statistics_t st;

    dawn_master.mtu = 128;
    dawn_master.retry_times = 3;
    dawn_master.timeout_ms = 1000*10;
    dawn_master.state = 0;
    ret = dawn_init_context(&dawn_master, dawn_example_master_read, dawn_example_master_write);
    if (0!=ret) {
        printf("dawn_init_context failed:%d\r\n", ret);
        return 0;
    }

    dawn_master.user_data.buf = buf;
    dawn_master.user_data.len = MAXLINE;
    memset(&st, 0x00, sizeof(st));
    do
    {
        st.total++;
        printf("#######################################################################\r\n");
        printf("                    Round#%d\r\n", st.total);
        printf("#######################################################################\r\n");
        ret = dawn_receive(&dawn_master);

        if (0!=ret) {
            st.failed++;
            st.recv_failed++;
            printf("dawn_receive failed:%d\r\n", ret);
            continue;
        }

        dawn_print_hex("master recv:", dawn_master.user_data.buf, dawn_master.user_data.len);
        usleep(100*1000);

        ret = dawn_transfer(&dawn_master, buf, dawn_master.user_data.len);
        if (0!=ret) {
            st.transfer_failed++;
            st.failed;
            printf("dawn_transfer failed:%d\r\n", ret);
        }else
        {
            st.suc++;
        }

        print_stics(&st);
    } while (1);

 }

 int main(int argc, char **argv)
 {
    struct sockaddr_in sockaddr;
    int opt = 1;
    int port = 1111;
    int test_type = 0;

    #if 1
    do
    {
        printf("argv[%d]:%s\r\n", opt, argv[opt]);
        if (argv[opt] ==NULL ||
        argv[opt][0] != '-'||
        argv[opt][2] != 0) {
            print_usage();
            break;
        }
        switch (argv[opt][1])
        {
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
                print_usage();
                break;
        }

        opt++;
    } while (opt<argc);

    printf("set up host with port:%d\r\n", port);
    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(port);
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    bind(listenfd,(struct sockaddr *) &sockaddr,sizeof(sockaddr));
    listen(listenfd,1024);

    for(;;)
    {
        printf("Please wait for the client information\n");
        if((connfd = accept(listenfd,(struct sockaddr*)NULL,NULL))==-1)
        {
            printf("accpet socket error: %s errno :%d\n",strerror(errno),errno);
            continue;
        }else{
            switch (test_type)
            {
                case 0:
                {
                    dawn_context_t dawn_master;
                    char *str;
                    int ret = 0;
                    uint8_t buf[MAXLINE];

                    dawn_master.mtu = 128;
                    dawn_master.retry_times = 3;
                    dawn_master.timeout_ms = 1000*10;
                    dawn_master.state = 0;
                    ret = dawn_init_context(&dawn_master, dawn_example_master_read, dawn_example_master_write);
                    if (0!=ret) {
                        printf("dawn_init_context failed:%d\r\n", ret);
                        return 0;
                    }

                    printf("new connection comming......\r\n");

                    dawn_master.user_data.buf = buf;
                    dawn_master.user_data.len = MAXLINE;
                    printf("user data len:%d\r\n", dawn_master.user_data.len);
                    ret = dawn_receive(&dawn_master);

                    if (0!=ret) {
                        printf("dawn_receive failed:%d\r\n", ret);
                        return 0;
                    }

                    dawn_print_hex("master recv:", dawn_master.user_data.buf, dawn_master.user_data.len);
                    printf("master receive:%s\r\n", (char *)dawn_master.user_data.buf);

                    str = (char *)"Hello ALex! \nI am fine and you\nI would like to hear you with pleasure\r\n";
                    ret = dawn_transfer(&dawn_master, str, strlen(str));
                    if (0!=ret) {
                        printf("dawn_transfer failed:%d\r\n", ret);
                        return 0;
                    }

                    close(connfd);
                }
                    break;
                case 1:
                {
                    stress_test(NULL);
                }
                    break;
                default:
                    break;
            }
        }


    }
    close(listenfd);
    #endif
 }
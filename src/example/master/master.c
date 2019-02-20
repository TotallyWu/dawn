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

 #include "../../core/dawn.h"


int listenfd,connfd = -1;

 #define MAXLINE 1024

 int32_t dawn_example_master_read(void *buf, uint16_t len, uint16_t timeout_ms)
 {
	 int ret = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;
    uint8_t p_buf = (uint8_t *)buf;
    uint16_t offset = 0;

    if (connfd <= 2) {
        return -1;
    }

    /* Watch stdin (fd 0) to see when it has input. */

    FD_ZERO(&rfds);
    FD_SET(connfd, &rfds);

    /* Wait up to five seconds. */
    do{
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000)*1000;

        retval = select(1, &rfds, NULL, NULL, &tv);
        /* Don't rely on the value of tv now! */

        if (retval == -1){
            perror("select()");
            break;
        }
        else if (retval){
            printf("Data is available now.\n");
            ret = read(connfd, p_buf+offset, len-offset);

            if (ret > 0) {
                offset += ret;
            }
        }
            /* FD_ISSET(0, &rfds) will be true. */
        else{
            printf("No data within five seconds.\n");
            break;
        }
    }while(offset < len);

    ret = offset;
	 out:
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
         ret = send(connfd, buf+offset, len-offset, 0);

         if (ret > 0) {
             offset += ret;
         }
     } while (offset<len);

	 out:
	 return ret;
 }


 int main(int argc,char **argv)
 {
    struct sockaddr_in sockaddr;
    char buff[MAXLINE];
    int n;

    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(argv[1]);
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    bind(listenfd,(struct sockaddr *) &sockaddr,sizeof(sockaddr));
    listen(listenfd,1024);

    printf("Please wait for the client information\n");

    for(;;)
    {
        if((connfd = accept(listenfd,(struct sockaddr*)NULL,NULL))==-1)
        {
            printf("accpet socket error: %s errno :%d\n",strerror(errno),errno);
            continue;
        }else{
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

            str = "Hello ALex!\r\n";
            ret = dawn_transfer(&dawn_master, str, strlen(str));
            if (0!=ret) {
                printf("dawn_transfer failed:%d\r\n", ret);
                return 0;
            }

            dawn_master.user_data.buf = buf;
            dawn_master.user_data.len = MAXLINE;
            ret = dawn_receive(&dawn_master);

            if (0>=ret) {
                printf("dawn_receive failed:%d\r\n", ret);
                return 0;
            }

            printf("master receive:%s\r\n", dawn_master.user_data.buf);
            close(connfd);
        }


    }
    close(listenfd);
 }
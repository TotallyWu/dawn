

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

#include "dawn.h"

 #define MAXLINE 1024

 void print_usage(void)
 {
     fprintf(stdout, "slave usage\r\n");
     fprintf(stdout, "slave [option]\r\n");
     fprintf(stdout, "-h host, default is 127.0.0.1 \r\n");
     fprintf(stdout, "-p port, default is 1111 \r\n");
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

    /* Watch stdin (fd 0) to see when it has input. */

    FD_ZERO(&rfds);
    FD_SET(socketfd, &rfds);

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
            ret = read(socketfd, p_buf+offset, len-offset);

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
         ret = send(socketfd, buf+offset, len-offset, 0);

         if (ret > 0) {
             offset += ret;
         }
     } while (offset<len);

	 return ret;
 }


 int main(int argc,char **argv)
 {
    char *servInetAddr = (char *)"127.0.0.1";
    struct sockaddr_in sockaddr;
    int opt = 1;
    int port = 1111;

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
            default:
                break;
        }
    } while (opt<argc);


    socketfd = socket(AF_INET,SOCK_STREAM,0);
    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    inet_pton(AF_INET,servInetAddr,&sockaddr.sin_addr);

    if((connect(socketfd,(struct sockaddr*)&sockaddr,sizeof(sockaddr))) < 0 )
    {
        printf("connect error %s errno: %d\n",strerror(errno),errno);
        exit(0);
    }else
    {
        dawn_context_t dawn_slave;
        char *str;
        int ret = 0;
        uint8_t buf[MAXLINE];

        dawn_slave.mtu = 128;
        dawn_slave.retry_times = 3;
        dawn_slave.timeout_ms = 1000*10;
        dawn_slave.state = 0;
        ret = dawn_init_context(&dawn_slave, dawn_example_slave_read, dawn_example_slave_write);
        if (0!=ret) {
            printf("dawn_init_context failed:%d\r\n", ret);
            return 0;
        }

        str = (char *)"Hello Iris!\r\n";
        ret = dawn_transfer(&dawn_slave, str, strlen(str));
        if (0!=ret) {
            printf("dawn_transfer failed:%d\r\n", ret);
            return 0;
        }

        dawn_slave.user_data.buf = buf;
        dawn_slave.user_data.len = MAXLINE;
        ret = dawn_receive(&dawn_slave);

        if (0>=ret) {
            printf("dawn_receive failed:%d\r\n", ret);
            return 0;
        }

        printf("master receive:%s\r\n", (char *)dawn_slave.user_data.buf);
        close(socketfd);
    }
 }
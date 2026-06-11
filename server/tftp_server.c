
#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include<errno.h>

int main() 
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len ;
    tftp_packet packet;
    // Create UDP socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    // Set socket timeout option
    skip:
    printf("Waiting for a client to connect.....\n");
    
    recvfrom(sockfd,&packet.body.request.mode,4,0,(struct sockaddr *)&client_addr,&client_len);
    printf("mode-> %d\n",packet.body.request.mode);
    fflush(stdout);
    if(packet.body.request.mode == WRQ)
    {
            recvfrom(sockfd,&packet.body.request.mode_flag,4,0,(struct sockaddr *)&client_addr,&client_len);
            printf("mode flag > %d\n",packet.body.request.mode_flag);
            fflush(stdout);
            recvfrom(sockfd,packet.body.request.filename,sizeof(packet.body.request.filename),0,(struct sockaddr *)&client_addr,&client_len);  
            printf("file_name-> %s\n",packet.body.request.filename);
            int fd = open(packet.body.request.filename,O_CREAT|O_EXCL|O_WRONLY,0666);
            if(fd == -1)
            {
                if(errno == EEXIST)
                {
                    fd = open(packet.body.request.filename,O_TRUNC|O_WRONLY,0666);
                }
            }

            //acknowlegment
            char ack[50]="SUCCESS";
            sendto(sockfd,ack,strlen(ack)+1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
            printf("TFTP Server listening on port %d...\n", PORT);
            fflush(stdout);
            // Main loop to handle incoming requests
            int pack_no;
            if(packet.body.request.mode_flag == 0)
            {
                while (1) 
                {
                    int n = recvfrom(sockfd,packet.body.data_packet.data,BUFFER_SIZE , 0, (struct sockaddr *)&client_addr, &client_len);
                    if(strcmp(packet.body.data_packet.data,"sending_over") == 0)
                    {
                        break;
                    }
                    recvfrom(sockfd,&packet.body.data_packet.data_size, 4, 0, (struct sockaddr *)&client_addr, &client_len);
                    //printf("data size send from client: %d\n",packet.body.data_packet.data_size);
                    write(fd,packet.body.data_packet.data,packet.body.data_packet.data_size);
                    memset(packet.body.data_packet.data,0,sizeof(packet.body.data_packet.data));
                    sendto(sockfd,&n,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
                }
                goto skip;
            }
            else if(packet.body.request.mode_flag == 2)
            {
                int data_ack;
                while(1)
                {
                    recvfrom(sockfd,&data_ack,4,0,(struct sockaddr *)&client_addr,&client_len);
                    if(data_ack == DATA)
                    {
                        recvfrom(sockfd,&packet.body.data_packet.ch,1,0,(struct sockaddr *)&client_addr,&client_len);
                        write(fd,&packet.body.data_packet.ch,1);
                    }
                    else if(data_ack == ACK)
                    {
                        break;
                    }
                }
                goto skip;
            }
            else if(packet.body.request.mode_flag == 3)
            {
                while (1) 
                {
                    int n = recvfrom(sockfd,packet.body.data_packet.data,BUFFER_SIZE , 0, (struct sockaddr *)&client_addr, &client_len);
                    if(strcmp(packet.body.data_packet.data,"sending_over") == 0)
                    {
                        break;
                    }
                    recvfrom(sockfd,&packet.body.data_packet.data_size, 4, 0, (struct sockaddr *)&client_addr, &client_len);
                    for(int i=0;i<strlen(packet.body.data_packet.data);i++)
                    { 
                        char ch = packet.body.data_packet.data[i];
                        if(ch != '\n')
                        {
                            write(fd,&ch,1);
                        }
                        else
                        {
                            ch = '\r';
                            write(fd,&ch,1);
                            ch = '\n';
                            write(fd,&ch,1);
                        }
                    }
                        memset(packet.body.data_packet.data,0,sizeof(packet.body.data_packet.data));
                        sendto(sockfd,&n,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
                }
                goto skip;
            }
    }

    else if(packet.body.request.mode == RRQ)
    {
        recvfrom(sockfd,&packet.body.request.mode_flag,4,0,(struct sockaddr *)&client_addr,&client_len);
        printf("mode flag > %d\n",packet.body.request.mode_flag);
        fflush(stdout);

        recvfrom(sockfd,packet.body.request.filename,sizeof(packet.body.request.filename),0,(struct sockaddr *)&client_addr,&client_len);
        printf("file_name->%s\n",packet.body.request.filename);
        int fd = open(packet.body.request.filename,O_RDONLY);
        char ack[50];
        if(fd != -1)
        {
            strcpy(ack,"SUCCESS");
            sendto(sockfd,ack,strlen(ack)+1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
        }
        else
        {
            strcpy(ack,"FAILURE");
            sendto(sockfd,ack,strlen(ack)+1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
            goto skip;
        }
        char ack_file[50];
        recvfrom(sockfd,ack_file,sizeof(ack_file),0,(struct sockaddr *)&client_addr,&client_len);

        if(strcmp(ack_file,"SUCCESS") == 0)
        {
            int n;
            if(packet.body.request.mode_flag == 0 || packet.body.request.mode_flag == 3)
            {
                while((n =(read(fd,packet.body.data_packet.data,BUFFER_SIZE))) >= 0)
                {
                    packet.body.data_packet.data_size = n;
                    send_file(sockfd,client_addr,client_len,&packet);
                    if(n == 0)
                    {
                        break;
                    }
                    int size;
                    recvfrom(sockfd,&size,4,0,(struct sockaddr *)&client_addr,&client_len);

                    if(size != n)
                    {
                        send_file(sockfd,client_addr,client_len,&packet);
                    }
                    memset(packet.body.data_packet.data,0,sizeof(packet.body.data_packet.data));
                }
                goto skip;
            }
            else if(packet.body.request.mode_flag == 2)
            {
                int data_ack = DATA;
                while((n =(read(fd,&packet.body.data_packet.ch,1))) > 0)
                {
                    sendto(sockfd,&data_ack,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
                    sendto(sockfd,&packet.body.data_packet.ch,1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
                }
                data_ack = ACK;
                sendto(sockfd,&data_ack,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
                goto skip;
            }
        }
        else if(strcmp(ack_file,"FAILURE") == 0)
        {
            return 0;
        }
    }
}






/*

Name : Namrata Subhash Ketgale   
Start Date :26/03/2026
End Date   :1/04/2026
Description: Trivial File Transfer Protocol (TFTP) 

*/
#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<errno.h>


int main() 
{
    tftp_client_t client;
    tftp_packet packet;
    memset(&client, 0, sizeof(client));  // Initialize client structure
    packet.body.request.mode_flag=0;
    int option;
    // Main loop for command-line interface
    while (1) 
    {
        printf("tftp> ");
        printf("Enter the option\n");
        printf("1. Connect\n2. Put\n3. Get\n4. Mode\n5. Exit\n");
        scanf("%d",&option);
        switch(option)
        {
           case 1:
           {
                connect_to_server(&client);
                break;
           }
           case 2:
           {
                put_file(&client,&packet);
                break;
           }
           case 3:
           {
                get_file(&client,&packet);
                break;
           }
           case 4:
           {
                int choice;
                printf("1. Default\n2. Octet\n3. Net Ascii\n");
                scanf("%d",&choice);
                if(choice == 1)
                {
                    packet.body.request.mode_flag = 0;

                }
                else if(choice == 2)
                {
                    packet.body.request.mode_flag = 2;
                }
                else if(choice == 3)
                {
                    packet.body.request.mode_flag = 3;
                }

                break;
           }
           case 5:
           {    
                disconnect(&client);
                exit(0);
                break;
           }
           default:
           {
                printf("Invalid option\n"); 
           }
        }
    }
    return 0;
}

// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client) 
{
    // Create UDP socket
    client->sockfd = socket(AF_INET,SOCK_DGRAM,0);
    // read the server address and port no
    printf("Enter the ip address :");
    getchar();
    scanf(" %[^\n]",client->server_ip);
    //validate the information

    for(int i=0;i<strlen(client->server_ip);)
    {
        if((client->server_ip[i] >=48 && client->server_ip[i] <=57) || client->server_ip[i] == '.') 
        {
            if(client->server_ip[i] == '.')
            {
                int flag=0;
                if(client->server_ip[i+1] >=48 && client->server_ip[i+1] <=57)
                {
                    flag=1;
                }
                if(flag == 0)
                {
                    printf("Invalid IP\n");
                    return;
                }
            }
            i++;
        }
        else
        {
            printf("Invalid IP\n");
            return;
        }
    }
    printf("Enter the port number :");
    scanf(" %d",&client->port);
    int flag=0;
    if(client->port >=1024 && client->port <= 49151)
    {
        flag = 1;
    }
    if(flag == 0)
    {
        printf("Invalid Port number\n");
        return;
    }

    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(client->port);
    client->server_addr.sin_addr.s_addr = inet_addr(client->server_ip);

}

void put_file(tftp_client_t *client,tftp_packet *packet) 
{
    //read the file name from the user
    char temp_file[100];
    printf("Enter the filename :");
    getchar();
    scanf("%[^\n]",temp_file);
    strcpy(packet->body.request.filename,temp_file);

    //validate the file exist or not
    int fd = open(packet->body.request.filename,O_RDONLY);
    if(fd == -1)
    {
        printf("File not exist\n");
        return;
    }
    else
    {        
        packet->opcode = WRQ;
        send_request(client,packet);
        sendto(client->sockfd,&packet->body.request.mode_flag,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        sendto(client->sockfd,packet->body.request.filename,strlen(packet->body.request.filename)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        //collect acknowledgemnt
        char ack[50];
        recvfrom(client->sockfd,ack,50,0,(struct sockaddr*)&client->server_addr,&client->server_len);
        if(strcmp(ack,"SUCCESS") != 0)
        {
            printf("Error in sending the file and mode\n");
            return;
        }
        else if(strcmp(ack,"SUCCESS") == 0)
        {
            int n;
            int pack_no = 1;
            if(packet->body.request.mode_flag == 0 || packet->body.request.mode_flag == 3)
            {
                while((n =(read(fd,packet->body.data_packet.data,BUFFER_SIZE))) >= 0)
                {
                    if(n == 0)
                    {
                        char buf[100] = "sending_over";
                        sendto(client->sockfd,&buf,strlen(buf)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                        break;
                    }

                    packet->body.data_packet.data_size = n;
                    send_file(client,packet);
                    int temp_data_size;
                    recvfrom(client->sockfd,&temp_data_size,4,0,(struct sockaddr*)&client->server_addr,&client->server_len);

                    if(n != temp_data_size)
                    {
                        send_file(client,packet);                 
                    }
                    memset(packet->body.data_packet.data, 0, sizeof(packet->body.data_packet.data));  
                }
            }
            else if(packet->body.request.mode_flag == 2)
            {
                int data_ack = DATA;
                while((n =(read(fd,&packet->body.data_packet.ch,1))) > 0)
                {
                    sendto(client->sockfd,&data_ack,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                    sendto(client->sockfd,&packet->body.data_packet.ch,1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                }
                data_ack = ACK;
                sendto(client->sockfd,&data_ack,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                return;
            }
        }
    }
}

void get_file(tftp_client_t *client,tftp_packet *packet) 
{
   // Send RRQ and recive file
    packet->opcode = RRQ;
    send_request(client,packet);
    sendto(client->sockfd,&packet->body.request.mode_flag,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    char temp_file_buff[50];
    printf("Enter the file name :");
    getchar();
    scanf("%[^\n]",temp_file_buff);
    strcpy(packet->body.request.filename,temp_file_buff);
    sendto(client->sockfd,packet->body.request.filename,strlen(packet->body.request.filename)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    int fd;
    //acknowlegement
    char ack[50];
    recvfrom(client->sockfd,ack,50,0,(struct sockaddr*)&client->server_addr,&client->server_len);
    if(strcmp(ack,"SUCCESS") == 0)
    {
        fd = open(packet->body.request.filename,O_TRUNC | O_WRONLY);
        if(fd == -1)
        {
            if(errno == EACCES)
            {
                printf("permission for write denied\n");
                return;
            }
            else if(errno == ENOENT)
            {
                fd = open(packet->body.request.filename,O_CREAT | O_WRONLY,0666);
            }
        }
    }
    else if(strcmp(ack,"FAILURE") == 0)
    {
        printf("File Not available in the server\n");
        return;
    }
    char ack_file[50];
    if(fd != -1)
    {
        strcpy(ack_file,"SUCCESS");
        sendto(client->sockfd,ack_file,strlen(ack_file)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));    
    }
    else if(fd == -1)
    {
        strcpy(ack_file,"FAILURE");
        sendto(client->sockfd,ack_file,strlen(ack_file)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    }
    int n;
    if(packet->body.request.mode_flag == 0)
    {
        while(1)
        {
            recvfrom(client->sockfd,&packet->body.data_packet.data_size, 4, 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            fflush(stdout);
            n = recvfrom(client->sockfd,packet->body.data_packet.data,packet->body.data_packet.data_size , 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            if(n == 0)
            {
                break;
            }
            write(fd,packet->body.data_packet.data,packet->body.data_packet.data_size);
            memset(packet->body.data_packet.data,0,sizeof(packet->body.data_packet.data));
            sendto(client->sockfd,&n,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        }
        return;
    }
    else if(packet->body.request.mode_flag == 2)
    {
        int data_ack;
        while(1)
        {
            recvfrom(client->sockfd,&data_ack,4,0,(struct sockaddr*)&client->server_addr,&client->server_len);
            if(data_ack == DATA)
            {
                recvfrom(client->sockfd,&packet->body.data_packet.ch,1,0,(struct sockaddr*)&client->server_addr,&client->server_len);
                write(fd,&packet->body.data_packet.ch,1);
            }
            else if(data_ack == ACK)
            {
                break;
            }
        }
        return;              
    }
    if(packet->body.request.mode_flag == 3)
    {
        while(1)
        {
            recvfrom(client->sockfd,&packet->body.data_packet.data_size, 4, 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            //printf("client data size:%d\n",packet->body.data_packet.data_size);
            fflush(stdout);
            n = recvfrom(client->sockfd,packet->body.data_packet.data,packet->body.data_packet.data_size , 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            if(n == 0)
            {
                break;
            }
            for(int i=0;i<strlen(packet->body.data_packet.data);i++)
            {
                char ch;
                ch = packet->body.data_packet.data[i];
                if(ch != '\r')
                {
                    write(fd,&ch,1);
                }
                else
                {
                    continue;
                }
            }
            memset(packet->body.data_packet.data,0,sizeof(packet->body.data_packet.data));
            sendto(client->sockfd,&n,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        }
        return;
    }
}

void disconnect(tftp_client_t *client) 
{
    // close fd
    close(client->sockfd);
   
}
void send_request(tftp_client_t *client,tftp_packet *packet)
{
    if(packet->opcode == WRQ)
    {
        packet->body.request.mode = WRQ;
        sendto(client->sockfd,&packet->body.request.mode,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    }
    else if(packet->opcode == RRQ)
    {
        packet->body.request.mode = RRQ;
        sendto(client->sockfd,&packet->body.request.mode,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    }

}

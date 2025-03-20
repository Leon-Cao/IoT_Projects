// linkJapan_iotGateway.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

/* Macro defination */
#define UDP_PORT 8889
#define BUF_SIZE 64

typedef enum{
    VENDOR_NONE_ENUM = 0,						/* Start of Vendor enum */
    CANDY_HOUSE_ENUM,                           /* Candy house, sesame serials*/
	VENDOR_END_ENUM,							/* End of Vendor enum */
}vendor_e;

typedef enum {
    ACTION_NONE_ENUM = 0,						/* Start of device action enum */
    SESAME_5_ACTION_LOCK_ENUM,                  /*<! Lock Sesame5 */
    SESAME_5_ACTION_UNLOCK_ENUM,                /*<! Unlock Sesame5 */
	DEVICE_ACTION_END_ENUM,						/* End of device action enum */
}candy_house_actions_e;


/* Data Structure Define */
typedef struct {
    unsigned char vendor_info[4];
    unsigned char iot_device_actions[4];
    unsigned char device_uuid[16];
    unsigned char addr[6];
    unsigned char conn_id;
    unsigned char reserve[1]; 
}iot_operation_message_t;    

void print_buffer(char * buffer, int len)
{
	int i = 0;
	printf("\nbuffer len= %d \nbuffer= ", len);
	while(1)
	{
		if(i > sizeof(iot_operation_message_t))
			break;
		printf("%02x ", buffer[i]);
		//printf("i=%d",i);
		i++;
	}
	return;
}

void print_action(iot_operation_message_t * p_iot_opt_msg)
{
	printf("IoT Operation: ");
	printf("\nVendor = ");
	if((p_iot_opt_msg->vendor_info[0] != VENDOR_NONE_ENUM) && 
		(p_iot_opt_msg->vendor_info[0] < VENDOR_END_ENUM))
	{
		if(p_iot_opt_msg->vendor_info[0] == CANDY_HOUSE_ENUM)
			printf("Candy House,");
	}
	else
	{
		printf("None");
		return;
	}
	
	printf("\nAction = ");
	if((p_iot_opt_msg->iot_device_actions[0] != ACTION_NONE_ENUM) && \
		(p_iot_opt_msg->iot_device_actions[0] < DEVICE_ACTION_END_ENUM))
	{
		switch(p_iot_opt_msg->iot_device_actions[0]){
			case SESAME_5_ACTION_LOCK_ENUM:
				printf("Sesame5 Lock");
				break;
			case SESAME_5_ACTION_UNLOCK_ENUM:
				printf("Sesame5 unlock");
				break;
			default:
				printf("None");
		}
	}
	else
	{
		printf("None");
	}
	return;
}

int main(int argc, char * argv[]) 
{

	//套接字信息结构
	WSADATA wsadata;
	//设置版本号
	WORD sockVersion = MAKEWORD(2, 2);
	//建立一个客户端套接字；
	SOCKET sClient;

	if(argc != 4)
    {
        printf("Usage: %s <IP> <Vendor> <Action>\n", argv[0]);
		printf("\nVendor: 1 Candy House.");
		printf("\nAction: 1 lock SSM5, 2 unlock SSM5");
        exit(1);
    }
	//printf("\nExecute command %s, IP=%s, Vendor=%s, cmd=%s",argv[0], argv[1], argv[2], argv[3]);
	//启动构建，将“为ws2_32.lib”加载到内存中，做一些初始化工作
	if (WSAStartup(sockVersion, &wsadata) != 0) {
		//判断是否构建成功，若失败，则客户端打印一句提示话。
		printf("WSAStartup failed \n");
		return 0;
	}

	//创建客户端udp套接字
    sClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SOCKET_ERROR == sClient) {
		printf("socket failed !\n");
		return 0;
	}

	//创建服务器端地址
	sockaddr_in serverAddr;
	//创建服务器端地址
	//sockaddr_in clientAddr;
	//设置服务器端地址，端口号，协议族
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(UDP_PORT);
	serverAddr.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	//获取服务器地址和客户端地址构造体的长度
	int slen = sizeof(serverAddr);
	//int clen = sizeof(clientAddr);
	//设置接受数据缓冲区大小
	char buffer[64];
	memset(buffer, 0, sizeof(buffer));

	//action
	iot_operation_message_t *p_operation_message = (iot_operation_message_t*) buffer; 
	p_operation_message->vendor_info[0] = (unsigned char) atoi(argv[2]);
	p_operation_message->iot_device_actions[0] = (unsigned char) atoi(argv[3]);
	print_action(p_operation_message);
	//print_buffer(buffer, sizeof(buffer));
	//用于记录发送函数和接受函数的返回值
	int iSend = 0;
	//int iRcv = 0;
	//string str;
	//cout << "开始主动与服务器建立通信：" << endl;

	//从控制台获取数据
	//cout << "Client send: "<<endl;
	//cout << buffer << endl;

	//发送信息给客户端
	iSend=sendto(sClient, buffer, strlen(buffer), 0, (SOCKADDR*)&serverAddr, slen);
	if (iSend== SOCKET_ERROR) {
		cout<<"sendto failed "<<endl;
		cout << "1s后关闭控制台。" << endl;
		Sleep(1000);
	}

	closesocket(sClient);
	WSACleanup();
	return 0;
}
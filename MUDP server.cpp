#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include <PROCESS.h>
#pragma comment(lib,"ws2_32.lib")

typedef struct _UserDATA {
	byte type;
	char buf[500];
}UserData;

void HandleError(char *func)
{
#ifdef _DEBUG
	int errCode = WSAGetLastError();

	char info[65] = { 0 };
	_snprintf(info, 64, "%s:       %d\n", func, errCode);
	printf(info);
#endif
}

void ServerThread(LPVOID param) {
	time_t tm;
	time(&tm);
	printf("Server Thread %d started.\n", tm);
	UserData data;
	struct sockaddr_in *fromaddr = (struct sockaddr_in *) param;

	SOCKET sockSvr = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockSvr == INVALID_SOCKET) {
		HandleError("socket");
		return;
	}

	if (connect(sockSvr, (struct sockaddr *)fromaddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		HandleError("connect");
		closesocket(sockSvr);
		return;
	}
	HeapFree(GetProcessHeap(), 0, fromaddr);//释放HeapAlloc分配的的内存块，因为fromaddr指针已经没有利用价值
	memset(&data, 0, sizeof(data));
	data.type = 1;
	if (send(sockSvr, (char *)&data, sizeof(data), 0) == SOCKET_ERROR) {
		HandleError("send");
		closesocket(sockSvr);
		return;
	}

	timeval tv;
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 5;
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(sockSvr, &readSet);

	int ret = select(0, &readSet, NULL, NULL, &tv);

	if (ret == SOCKET_ERROR) {
		HandleError("select");
		closesocket(sockSvr);
		return;
	}

	if (ret == 0) {
		closesocket(sockSvr);
		printf("Time over, server socket closed & thread %d finished!\n", tm);
		return;
	}

	while (1) {

		if (FD_ISSET(sockSvr, &readSet)) {
			memset(&data, 0, sizeof(data));
			ret = recv(sockSvr, (char *)&data, sizeof(data), 0);
			if (ret == SOCKET_ERROR) {
				HandleError("recv");
				closesocket(sockSvr);
				return;
			}
			if (data.type == 2) {

				printf("Succeed receive from: %d\n", tm);
				printf("Succeed receive data: %s\n", data.buf);
				
			}
		}
		else {
			closesocket(sockSvr);
			return;
		}

		if (data.type != 2) {
			printf("Wrong Packet, Server socket closed & thread %d finished! \n", tm);
			closesocket(sockSvr);
			return;
		}

		data.type = 3;
		send(sockSvr, (char *)&data, sizeof(data), 0);
		if ((data.buf[0] == 's')&(data.buf[1] == 't')&(data.buf[2] == 'o')&(data.buf[3] == 'p'))
		{
			printf("This thread has finished!\n");
			printf("-----------------------------\n");
			break;
		}

	}

	closesocket(sockSvr);

	printf("Server Thread %d finished.\n", tm);
}
int main(int argc, char* argv[])
{
	WSAData wsaData;
	WSAStartup(WINSOCK_VERSION, &wsaData);
	SOCKET sockListen = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockListen == INVALID_SOCKET){
		printf("socket() Failed:%d\n",WSAGetLastError());
		WSACleanup();
		return -1;
	}
	struct sockaddr_in listenaddr;

	memset(&listenaddr, 0, sizeof(listenaddr));
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(5060);
	listenaddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockListen, (struct sockaddr *)&listenaddr, sizeof(listenaddr)) == SOCKET_ERROR) {
		HandleError("bind");
		return -1;
	}

	UserData initReq;
	struct sockaddr_in *from;
	int fromlen, ret;

	while (1) {

		from = (struct sockaddr_in *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct sockaddr_in));
		memset(&initReq, 0, sizeof(struct sockaddr_in));//为from分配空间，传入线程后完成connect后就被释放
		memset(from, 0, sizeof(struct sockaddr_in));
		fromlen = sizeof(struct sockaddr_in);

		ret = recvfrom(sockListen, (char *)&initReq, sizeof(initReq), 0, (struct sockaddr *)from, &fromlen);
		if (ret == SOCKET_ERROR) {
			HandleError("recvfrom");
			break;
		}
		if (ret != sizeof(initReq)) {
#ifdef _DEBUG
			printf("Wrong package ! Discarded!\n");
#endif
			continue; 
		}

		if (initReq.type != 0) {
#ifdef _DEBUG
			printf("Wrong package ! Discarded!\n");
#endif

			continue; 
		}
		_beginthread(ServerThread, 0, from);
	}
	return 0;
}
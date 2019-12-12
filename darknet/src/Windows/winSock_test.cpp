#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <time.h>
#include "opencv2/opencv.hpp"

#define PORT_NUM	4000
#define SEND_SIZE	1024 //58368
#define DEBUG

int main()
{
	SOCKET server_socket, client_socket;
	WSADATA wsa_data;
	SOCKADDR_IN server_addr, client_addr;
	int client_addr_size;

	int recv_info[3];
	char send_buff[3];
	unsigned char* recv_data;
	int ret, total_size, loc = 0;
	int h, w, c;
	int i, x, y, z;
	int count = 0;

	// winsock 초기화, 버전 2.2
	if (0 < WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
		std::cerr << "WSAStartup() error\n";
		return -1;
	}
	
	// server socket 생성
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == server_socket) {
		std::cerr << "socket() error\n";
		return -1;
	}

	// server addr 초기화
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// server socket 및 addr 정보를 커널에 등록
	if (SOCKET_ERROR == bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr))) {
		std::cerr << "bind() error\n";
		return -1;
	}
#ifdef DEBUG
	std::cerr << "bind ok\n";
#endif
	// client 의 connect 대기
	if (SOCKET_ERROR == listen(server_socket, 5)) {
		std::cerr << "listen() error\n";
		return -1;
	}
	std::cerr << "wait for client connection...\n";
#ifdef DEBUG
	std::cerr << "listen ok\n";
#endif
	// client connect 수락
	client_addr_size = sizeof(client_addr);
	client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &client_addr_size);

	if (INVALID_SOCKET == client_socket) {
		std::cerr << "accept() error\n";
		return -1;
	}
	std::cerr << "connected!\n\n";
#ifdef DEBUG
	std::cerr << "accept ok\n";
#endif

	// client 에 보낼 send buff 초기화
	memset(send_buff, 0, sizeof(send_buff));
	
	while (1)
	{
#ifdef DEBUG
		clock_t start, end;
		double result;

		start = clock();	// 시간 측정
#endif
		// image 정보 수신
		ret = 0;
		while (ret != sizeof(recv_info)) { // socket 에서 데이터 읽을 때 끊어서 읽을 수도 있으므로
			ret += recv(client_socket, (char*)recv_info + ret, sizeof(recv_info) - ret, 0);
			if (ret < 0) {
				std::cerr << "image info receive fail\n";
				return -1;
			}
		}
		total_size = recv_info[0];	// 보낼 image의 크기

		// image data 담을 buff 생성
		recv_data = (unsigned char*)calloc(total_size, sizeof(unsigned char));
		if (recv_data == NULL) {
			std::cerr << "recv_data malloc() fail\n";
			return -1;
		}

		// image 데이터 수신
		ret = 0;
		while (ret != total_size) {	// socket 에서 데이터 읽을 때 끊어서 읽을 수도 있으므로
			ret += recv(client_socket, (char*)recv_data + loc + ret, total_size - ret, 0);
			if (ret < 0) {
				std::cerr << "image data receive fail\n";
				return -1;
			}
		}
		
#ifdef DEBUG
		std::cout << "read ret : " << ret << std::endl;
		std::cerr << "[receive img]\n";

#endif
		// 수신받은 image 저장할 Mat 객체 생성 및 값 저장
		std::vector<uchar> decoding(recv_data, recv_data + total_size);
		cv::Mat img = cv::imdecode(decoding, cv::IMREAD_COLOR);

		cv::namedWindow("recv", cv::WINDOW_AUTOSIZE);
		cv::imshow("recv", img);
		int c = cv::waitKey(1);
		if (c == 27) {
			// ESC 입력 시 종료
			send_buff[0] = 'c';
			ret = send(client_socket, send_buff, sizeof(send_buff), 0);
			if (ret < 0) {
				std::cerr << "[tcpSocket.c] message write fail\n";
				return -1;
			}
			break;
		}
		send_buff[0] = 'o';
		ret = send(client_socket, send_buff, sizeof(send_buff), 0);
		if (ret < 0) {
			std::cerr << "[tcpSocket.c] message write fail\n";
			return -1;
		}

		free(recv_data);

#ifdef DEBUG
		std::cout << "write ret : " << ret << std::endl;
		std::cout << "loop count : " << ++count << std::endl;

		end = clock();
		result = (double)(end - start) / 1000;	// 소요시간 측정
		std::cout << "time: " << result << std::endl;
#endif
	}
	std::cerr << "Quit the program!\n";

	cv::destroyWindow("recv");
	//free(recv_data);
	closesocket(client_socket);
	closesocket(server_socket);
	WSACleanup();

	return 0;
}

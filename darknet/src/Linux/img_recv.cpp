#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <opencv2/opencv.hpp>

#define SEND_SIZE	1024
#define DEBUG

using namespace cv;
using namespace std;

int main( int argc, char **argv)
{
	int server_socket, client_socket;
	unsigned int client_addr_size;
	struct sockaddr_in server_addr, client_addr;

	int recv_info[3];
	char send_buff[3];
	unsigned char *recv_data;
	int h, w, c;
	int ret, total_size, loc = 0;
	int i, x, y, z;
	int count = 0;

	// 소켓 생성
	server_socket  = socket( PF_INET, SOCK_STREAM, 0);
	if( -1 == server_socket)
	{
		printf( "socket 생성 실패n");
    	exit( 1);
	}

	// 주소 정의
	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family     = AF_INET;
	server_addr.sin_port       = htons( 4000);
	server_addr.sin_addr.s_addr= htonl(INADDR_ANY);

	// 커널에 소켓 및 주소 등록
	if(-1 == bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
		cerr << "bind() error\n";
		return -1;
	}
#ifdef DEBUG
	cout << "bind ok" << endl;
#endif
	// 클라이언트 접속 대기
	if( -1 == listen(server_socket, 5))
	{
		printf("listen fail\n");
		return -1;
	}
	cerr << "wait for client connection...\n";
#ifdef DEBUG
	cout << "listen ok" << endl;
#endif

	client_addr_size  = sizeof(client_addr);
	// 클라이언트 접속 수락
	client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);

	if( -1 == client_socket) {
		printf("client accept fail\n");
		return -1;
	}
	cerr << "connected!\n\n";
#ifdef DEBUG
	cout << "accept ok" << endl;
#endif

	memset(send_buff, 0, sizeof(send_buff));

	while(1)
	{
#ifdef DEBUG
		struct timeval time;
		gettimeofday(&time, NULL);
		double current_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
#endif
		//image 정보 수신
		ret = 0;
		while(ret != sizeof(recv_info)){ // 받아야하는 값 만큼 실제 받을때까지,
			ret += recv(client_socket, recv_info + ret, sizeof(recv_info) - ret, 0);
			if(ret < 0) {
				cerr << "image info receive fail\n";
				return -1;
			}
		}
		total_size = recv_info[0];
		recv_data = (unsigned char*)calloc(total_size, sizeof(unsigned char));

		//image 데이터 수신
		ret = 0;
		while(ret != total_size){
			ret += recv(client_socket, recv_data + ret, total_size - ret, 0);
			if(ret < 0) {
				cerr << "image data receive fail\n ";
				return -1;
			}
		}

		// jpeg 압축포맷 해제
		vector<uchar> decoding(recv_data, recv_data + total_size);
        Mat img = imdecode(decoding, IMREAD_COLOR);

		namedWindow("recv", WINDOW_AUTOSIZE);
		imshow("recv", img);
		int c = waitKey(1);
		if(c == 27) {
			//ESC 입력 시 종료
			send_buff[0] = 'c';
			ret = send(client_socket, send_buff, sizeof(send_buff), 0);
			if(ret < 0) {
				cerr << "[img_recv.cpp] message write fail\n";
				return -1;
			}
			break;
		}
		send_buff[0] = 'o';
		ret = send(client_socket, send_buff, sizeof(send_buff), 0);
		if(ret < 0) {
			cerr << "[img_recv.cpp] message write fail\n";
			return -1;
		}

		free(recv_data);
#ifdef DEBUG
		cerr << "write ret : " << ret << endl;
		cerr << count++ << endl;

		gettimeofday(&time, NULL);
		double recv_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
		recv_time -= current_time;
		cout << "time: " << recv_time << endl;
#endif
	}
	cerr << "Quit the program!\n";

	destroyWindow("recv");
	free(recv_data);
	close(server_socket);

	return 0;
}

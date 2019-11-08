#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <opencv2/opencv.hpp>

#define SEND_SIZE	1024
//#define DEBUG

using namespace cv;
using namespace std;

int main( int argc, char **argv)
{
	int server_socket, client_socket;
	unsigned int client_addr_size;
	struct sockaddr_in server_addr, client_addr;

	char temp[1024];
	int recv_info[3];
	char send_buff[3];
	unsigned char *recv_data;
	int h, w, c;
	int ret, total_size, loc = 0;
	int i, x, y, z;
	int count = 0;

	server_socket  = socket( PF_INET, SOCK_STREAM, 0);

	if( -1 == server_socket)
	{
		printf( "socket 생성 실패n");
    	exit( 1);
	}

	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family     = AF_INET;
	server_addr.sin_port       = htons( 4000);
	server_addr.sin_addr.s_addr= htonl(INADDR_ANY);

	if(-1 == bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
		cerr << "bind() error\n";
		return -1;
	}
#ifdef DEBUG
	cout << "bind ok" << endl;
#endif

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
	client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);

	if( -1 == client_socket) {
		printf("client accept fail\n");
		return -1;
	}
	cerr << "connected!\n\n";
#ifdef DEBUG
	cout << "accept ok" << endl;
#endif
	// image 정보 수신
	ret = recv(client_socket, recv_info, sizeof(recv_info), 0);
	//ret = read(client_socket, recv_info, sizeof(recv_info));

	if(ret < 0) {
		cerr << "image info receive fail\n";
		return -1;
	}

	h = recv_info[0];
	w = recv_info[1];
	c = recv_info[2];
	total_size = h*w*c;
#ifdef DEBUG
	printf("[recv info] rows: %d, cols: %d, channels: %d\n", h, w, c);
#endif
	// image data 담을 buff 생성
	recv_data = (unsigned char*)calloc(total_size, sizeof(unsigned char));
	memset(send_buff, 0, sizeof(send_buff));

	while(1)
	{
#ifdef DEBUG
		struct timeval time;
		gettimeofday(&time, NULL);
		double current_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
#endif
		
		for(i = 0; loc < total_size; i++) {
			ret = 0;
			while(ret != SEND_SIZE){
				ret += recv(client_socket, recv_data + loc + ret, SEND_SIZE - ret, 0);
				if(ret < 0) {
					cerr << "image data receive fail\n ";
					return -1;
				}
			}
			loc += SEND_SIZE;
		}
		loc = 0;

#ifdef DEBUG
		cerr << "i : " << i << endl;
		cerr << "read ret : " << ret << endl;
		cerr << "[receive img]\n";

#endif

		Mat img(h, w, CV_MAKETYPE(CV_8U, c));
		for(y = 0; y < h; ++y){
			for(x = 0; x < w; ++x){
				for(z = 0; z < c; ++z){
					img.data[y*w*c + x*c + z] = recv_data[z*h*w + y*w + x];
				}
			}
		}

		namedWindow("recv", WINDOW_AUTOSIZE);
		imshow("recv", img);
		int c = waitKey(1);
		if(c == 27) {
			// ESC 입력 시 종료
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

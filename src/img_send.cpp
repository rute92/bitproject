#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <opencv2/opencv.hpp>

#define SEND_SIZE	1024
//#define DEBUG

using namespace cv;
using namespace std;

int main()
{
	int client_socket;
	struct sockaddr_in server_addr;

	int h, w, c, step;
	int send_info[3];	// row, col, channel 정보
	unsigned char *send_data;	// mat data 정보
	char recv_buff[3];
	int ret, total_size, data_loc = 0;
	int i, k, j;
	int count = 0;


	// TCP로 client_socket 생성
	client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(-1 == client_socket) {
		cerr << "server client_socket error\n";
		return -1;
	}
	
	// socketaddr_in 구조체 초기화 및 정의(서버 addr 정의)
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	cout << "client_socketaddr ok" << endl;
	
	// image 읽기위한 정의
	Mat img;

	const char* gst = "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)608, height=(int)608, format=(string)NV12, framerate=(fraction)60/1 ! nvvidconv flip-method=4 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";

	VideoCapture cap(gst);
	if(!cap.isOpened()) {
		cerr << "VideoCapture error\n";
	}

	cap.read(img);
	if(img.empty()){
		cerr << "image is empty\n";
		return -1;
	}

	h = img.rows;
	w = img.cols;
	c = img.channels();
	step = img.step;

	send_info[0] = h;
	send_info[1] = w;
	send_info[2] = c;
	total_size = h*w*c;
#ifdef DEBUG
	printf("[img] rows: %d, cols: %d, channels: %d, step: %d\n", h, w, c, step);
#endif

	// 서버에 connect 요청
	if( -1 == connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
		cerr << "connect fail\n";
		return -1;
	}
#ifdef DEBUG
	printf("connect ok\n");
#endif

	// image info 전송 (h, w, c)
	ret = send(client_socket, send_info, sizeof(send_info), 0);
	if(ret < 0) {
		cerr << "image info send fail\n";
		return -1;
	}

	// Mat data 담을 buff 생성
	send_data = (unsigned char*)calloc(total_size, sizeof(unsigned char));
	
	while(1)
	{
#ifdef DEBUG
		struct timeval time;
		gettimeofday(&time, NULL);
		double current_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
#endif
		cap.read(img);
		if(img.empty()){
			cerr << "image is empty\n";
			return -1;
		}

		// buff에 Mat data 복사
		for(i = 0; i < h; ++i){
			for(k = 0; k < c; ++k){
				for(j = 0; j < w; ++j){
					send_data[k*w*h + i*w + j] = (unsigned char)img.data[i*step + j*c + k];
				}
			}
		}

		for(i = 0; data_loc < total_size; i++) {
			ret = send(client_socket, send_data + data_loc, SEND_SIZE, 0);
			if(ret < 0) {
				cerr << "image data send fail\n";
				return -1;
			}
			data_loc += SEND_SIZE;
		}
		data_loc = 0;

#ifdef DEBUG
		cerr << "i : " << i << endl;
		cerr << "write ret : " << ret << endl;
		cerr << "[send img] size: " << total_size << endl;
		namedWindow("cam", WINDOW_AUTOSIZE);
		imshow("cam", img);
		waitKey(1);

#endif
		// 서버 신호 대기
		ret = recv(client_socket, recv_buff, sizeof(recv_buff), 0);
		if(ret < 0) {
			cerr << "[img_send.cpp] message read fail\n";
			return -1;
		}

		if(recv_buff[0] == 'c') break;
		else if(recv_buff[0] == 'o') {
#ifdef DEBUG
			cerr << "read ret : " << ret << endl;
#endif
		}

#ifdef DEBUG
		cerr << count++ << endl;
		
		gettimeofday(&time, NULL);
		double recv_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
		recv_time -= current_time;
		cout << "time: " << recv_time << endl;
#endif
	}
	cerr << "Quit the program!\n";
#ifdef DEBUG
	destroyWindow("cam");
#endif
	free(send_data);
	close(client_socket);

	return 0;
}

#include <iostream>
#include <cstring>
#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <algorithm>

#include <opencv2/opencv.hpp>

#define PORT_NUM	4000
//#define SERVER_IP	"192.168.0.40" // server pc
//#define SERVER_IP	"192.168.0.38" // my pc
//#define SERVER_IP	"127.0.0.1"
#define SEND_SIZE	1024 //58368
#define MODE_FILE_NAME    "~/web/mode.txt"
#define MODE110_FILE_NAME    "~/mode110.txt"
#define MODE310_FILE_NAME    "~/mode310.txt"
#define DEBUG

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{

    FILE* mode_fp, mode110_fp, mode310_fp;
	int client_socket;
	struct sockaddr_in server_addr;

	int h, w, c, step;
	int send_info[3];	// row, col, channel 정보
	char mode_info[3];
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
	server_addr.sin_port = htons(PORT_NUM);
	//server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	cout << "client_socketaddr ok" << endl;

	// 서버에 connect 요청
	if( -1 == connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
		cerr << "connect fail\n";
		return -1;
	}
#ifdef DEBUG
	printf("connect ok\n");
#endif

	// image 읽기위한 정의
	Mat img;

	const char* gst = "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)608, height=(int)608, format=(string)NV12, framerate=(fraction)60/1 ! nvvidconv flip-method=4 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";

	vector<uchar> buff;
	vector<int> param_jpeg = vector<int>(2);
	param_jpeg[0] = IMWRITE_JPEG_QUALITY;
	param_jpeg[1] = 50;

	VideoCapture cap(gst);
	if(!cap.isOpened()) {
		cerr << "VideoCapture error\n";
	}

    // 파일 열기
    mode_fp = fopen(MODE_FILE_NAME, "r");
    
	while(1)
	{
        int mode;
		struct timeval time;
#ifdef DEBUG
 		gettimeofday(&time, NULL);
 		double current_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
#endif
		cap.read(img);
		if(img.empty()){
			cerr << "image is empty\n";
			return -1;
		}
		
		// 이미지 압축
		imencode(".jpg", img, buff, param_jpeg);
#ifdef DEBUG
		namedWindow("img", WINDOW_AUTOSIZE);
		imshow("img", img);
		waitKey(1);
#endif
		// 압축된 이미지 크기 전송 (할때마다 크기 다름)
		send_info[0] = buff.size();
		ret = send(client_socket, send_info, sizeof(send_info), 0);
		if(ret < 0) {
			cerr << "image info send fail\n";
			return -1;
		}
#ifdef DEBUG
		cerr << "send info: " << send_info[0] << endl;
#endif
		// 압축된 이미지 데이터 전송
		send_data = &buff[0];
		ret = send(client_socket, send_data, buff.size(), 0);
		if(ret < 0) {
			cerr << "image data send fail\n";
			return -1;
		}

		// mode 읽기
        fread(mode_info, 1, sizeof(mode_info), mode_fp);
        fseek(mode_fp, 0, SEEK_SET);
        mode = atoi(mode_info);
        
        // mode 정보 전송
		ret = send(client_socket, mode_info, sizeof(mode_info), 0);
		if(ret < 0) {
			cerr << "mode info send fail\n";
			return -1;
		}

		// server 작업 끝날때까지 대기
		ret = recv(client_socket, recv_buff, sizeof(recv_buff), 0);

  		gettimeofday(&time, NULL);
#ifdef DEBUG
  		double recv_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
  		recv_time -= current_time;
  		cout << "time: " << recv_time << endl;
#endif


#ifdef DEBUG
		cout << "recv: " << recv_buff[0] << recv_buff[1] << recv_buff[2] << endl;
#endif
		struct tm tm = *localtime(&time.tv_sec);
		char filename[256];

        if (mode == 110) { // 감지모드 110
            ////////// 박스 안친 이미지 저장 ////////////////
            // if (recv_buff[2] == '1') { // person img save
            // 	sprintf(filename, "~/capture/person/%d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            // 	imwrite(filename, img, param_jpeg);
            // }
            // else if (recv_buff[2] == '2') { // fire img save
            // 	sprintf(filename, "~/capture/fire/%d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            // 	imwrite(filename, img, param_jpeg);
            // }
		
            ////////// 박스 친 이미지 저장 ////////////////
            if (recv_buff[2] == '1' || recv_buff[2] == '2') {
                int jpg_size = 0;
                unsigned char* jpg_data = nullptr;

                if (recv_buff[2] == '1') { // person img save name
                    sprintf(filename, "~/capture/person/%d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                }
                else if (recv_buff[2] == '2') { // fire img save name
                    sprintf(filename, "~/capture/fire/%d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                }

                // jpg size recv
                ret = 0;
                while (ret != sizeof(int)) {
                    ret += recv(client_socket, &jpg_size + ret, sizeof(int) - ret, 0);
                    if(ret < 0) {
                        cerr << "jpg info receive fail\n";
                        return -1;
                    }
                }
                jpg_data = (unsigned char*)calloc(jpg_size, sizeof(unsigned char));

                // jpg data recv
                ret = 0;
                while(ret != jpg_size){ // 받아야하는 값 만큼 실제 받을때까지,
                    ret += recv(client_socket, jpg_data + ret, jpg_size - ret, 0);
                    if(ret < 0) {
                        cerr << "jpg data receive fail\n";
                        return -1;
                    }
                }

                // jpg decode & jpg save
                vector<uchar> decoding(jpg_data, jpg_data + jpg_size);
                Mat jpg_img = imdecode(decoding, IMREAD_COLOR);
                imwrite(filename, jpg_img, param_jpeg);
            } // 이미지 저장 끝

            //////////// 정지 명령 쓰기 /////////////
            if (recv_buff[1] == '5') { // 정지
                // 파일에 쓰기 또는 publish
                mode110_fp = fopen(MODE110_FILE_NAME, "w");
                // cerr << "mode310 write: " << recv_buff[1] << endl;
                fwrite(recv_buff + 1, 1, 1, mode110_fp);
                fseek(mode110_fp, 0, SEEK_SET);
                fclose(mode110_fp);
            }
            else {// 정지아님
                // 파일에 쓰기 또는 publish
                mode110_fp = fopen(MODE110_FILE_NAME, "w");
                // cerr << "mode310 write: " << recv_buff[1] << endl;
                fwrite(recv_buff + 1, 1, 1, mode110_fp);
                fseek(mode110_fp, 0, SEEK_SET);
                fclose(mode110_fp);
            }
        } // 감지모드 110 끝
        
        if(mode == 310) { // 추적모드 310
            // 파일에 recv_buff[1] 쓰기
            mode310_fp = fopen(MODE310_FILE_NAME, "w");
            // cerr << "mode310 write: " << recv_buff[1] << endl;
            fwrite(recv_buff + 1, 1, 1, mode310_fp);
            fseek(mode310_fp, 0, SEEK_SET);
            fclose(mode310_fp);
        } // 추적모드 310 끝
        else { // 추적모드 310 이 아닌 경우
            // 파일에 다른 값 쓰기
            mode310_fp = fopen(MODE310_FILE_NAME, "w");
            recv_buff[1] = '0';
            // cerr << "mode310 write: " << recv_buff[1] << endl;
            fwrite(recv_buff + 1, 1, 1, mode310_fp);
            fseek(mode310_fp, 0, SEEK_SET);
            fclose(mode310_fp);
        }
	}
	
	cerr << "Quit the program!\n";
#ifdef DEBUG
	destroyWindow("img");
#endif
	//free(send_data);
	close(client_socket);
    fclose(mode_fp)
	return 0;
}

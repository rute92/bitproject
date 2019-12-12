#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

int main()
{
    Mat img;

	const char* gst = "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)608, height=(int)608, format=(string)NV12, framerate=(fraction)60/1 ! nvvidconv flip-method=4 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";

	vector<uchar> buff; // 압축된 데이터 담을 벡터
	vector<int> param_jpeg = vector<int>(2); // 압축 설정 담을 벡터
	param_jpeg[0] = IMWRITE_JPEG_QUALITY; // 압축형태
	param_jpeg[1] = 50; // 압축률 (Quality, 낮을수록 많이 압축)

	VideoCapture cap(gst);
	if(!cap.isOpened()) {
		cerr << "VideoCapture error\n";
	}

    while(1)
    {
        cap.read(img);
	    if(img.empty()){
    		cerr << "image is empty\n";
	    	return -1;
	    }
        // 압축
        imencode(".jpg", img, buff, param_jpeg);
        cout << "buff size: " << buff.size() << endl;
        unsigned char* send_data;

        // 압축된 size만큼 버퍼 생성
        send_data = (unsigned char*)calloc(buff.size(), sizeof(unsigned char));
        
        // 버퍼로 압축된 데이터 복사 또는 참조
        //copy(buff.begin(), buff.end(), send_data);
        send_data = &buff[0];

        // vector 버퍼 참조하여 생성
        vector<uchar> dst_buff(send_data, send_data + buff.size());
        
        // 압축 해제
        Mat dst=imdecode(dst_buff, 1);

        cout << "[src] size : " << img.total() << ", " << img.rows << ", " << img.cols << ", " << img.channels() << endl;
        cout << "[dst] size : " << dst.total() << endl;

	    namedWindow("img", WINDOW_AUTOSIZE);
	    imshow("img", img);
	    imshow("dst", dst);
	    int c = waitKey(1);
        if (c == 27)
            break;
    }
	
}
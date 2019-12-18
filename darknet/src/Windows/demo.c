#include "network.h"
#include "detection_layer.h"
#include "region_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "image.h"
#include "demo.h"
#ifdef WIN32
#include <time.h>
#include "gettimeofday.h"
#else
#include <sys/time.h>
#endif

// add code -->
#include <stdlib.h>
#define STREAM_FROM_JETBOT
#define FROM_YOLO_PATH  "./from_yolo"
#define TO_YOLO_PATH    "./to_yolo"

#ifndef STREAM_FROM_JETBOT
#else
#include <WinSock2.h>

#define PORT_NUM    4000


SOCKET server_socket, client_socket;
static int total_size;
unsigned char* recv_data;
#endif

extern int per_chk, fire_chk;
double per_chk_time, fire_chk_time;
double per_time, fire_time;
FILE* from_yolo_fp, *to_yolo_fp;
int mode;
char mode_info[3];
char send_buff[3];
// end code <--

#ifdef OPENCV

#include "http_stream.h"

static char **demo_names;
static image **demo_alphabet;
static int demo_classes;

static int nboxes = 0;
static detection *dets = NULL;

static network net;
static image in_s;
static image det_s;

static cap_cv *cap;
static float fps = 0;
static float demo_thresh = 0;
static int demo_ext_output = 0;
static long long int frame_id = 0;
static int demo_json_port = -1;

#define NFRAMES 3

static float* predictions[NFRAMES];
static int demo_index = 0;
static image images[NFRAMES];
static mat_cv* cv_images[NFRAMES];
static float *avg;

mat_cv* in_img;
mat_cv* det_img;
mat_cv* show_img;

static volatile int flag_exit;
static int letter_box = 0;

void *fetch_in_thread(void *ptr)
{
	int dont_close_stream = 0;    // set 1 if your IP-camera periodically turns off and turns on video-stream

								  // add code -->
#ifndef STREAM_FROM_JETBOT
								  // origin code
	if (letter_box)
		in_s = get_image_from_stream_letterbox(cap, net.w, net.h, net.c, &in_img, dont_close_stream);
	else
		in_s = get_image_from_stream_resize(cap, net.w, net.h, net.c, &in_img, dont_close_stream);
#else

	int recv_info[3];
	int ret;

	// image 정보 수신
	ret = 0;
	while (ret != sizeof(recv_info)) { // socket 에서 데이터 읽을 때 끊어서 읽을 수도 있으므로
		ret += recv(client_socket, (char*)recv_info + ret, sizeof(recv_info) - ret, 0);
		if (ret < 0) {
			perror("[recv socket] image info receive fail\n");
			return 0;
		}
	}
	total_size = recv_info[0];
	printf("[recv socket] image size : %d\n", total_size);

	// image data 담을 buff 생성
	recv_data = (unsigned char*)calloc(total_size, sizeof(unsigned char));
	if (recv_data == NULL) {
		perror("recv_data malloc() fail\n");
		return 0;
	}

	// image 데이터 수신
	ret = 0;
	while (ret != total_size) {	// socket 에서 데이터 읽을 때 끊어서 읽을 수도 있으므로
		ret += recv(client_socket, (char*)recv_data + ret, total_size - ret, 0);
		if (ret < 0) {
			perror("[recv socket] image data receive fail\n");
			return 0;
		}
	}

	// mode 정보 수신??
	ret = 0;
	while (ret != sizeof(mode_info)) {
		ret += recv(client_socket, mode_info + ret, sizeof(mode_info) - ret, 0);
		if (ret < 0) {
			perror("[recv socket] mode info receive fail\n");
			return 0;
		}
	}


	// opencv 함수로 넘기기
	in_s = get_image_from_socket(recv_data, net.w, net.h, net.c, total_size, &in_img);

#endif
	// end code <--

	if (!in_s.data) {
		printf("Stream closed.\n");
		flag_exit = 1;
		//exit(EXIT_FAILURE);
		return 0;
	}
	//in_s = resize_image(in, net.w, net.h);

	return 0;
}

void *detect_in_thread(void *ptr)
{
	layer l = net.layers[net.n - 1];
	float *X = det_s.data;
	float *prediction = network_predict(net, X);

	memcpy(predictions[demo_index], prediction, l.outputs * sizeof(float));
	mean_arrays(predictions, NFRAMES, l.outputs, avg);
	l.output = avg;

	free_image(det_s);

	cv_images[demo_index] = det_img;
	det_img = cv_images[(demo_index + NFRAMES / 2 + 1) % NFRAMES];
	demo_index = (demo_index + 1) % NFRAMES;

	if (letter_box)
		dets = get_network_boxes(&net, get_width_mat(in_img), get_height_mat(in_img), demo_thresh, demo_thresh, 0, 1, &nboxes, 1); // letter box
	else
		dets = get_network_boxes(&net, net.w, net.h, demo_thresh, demo_thresh, 0, 1, &nboxes, 0); // resized

	return 0;
}

double get_wall_time()
{
	struct timeval walltime;
	if (gettimeofday(&walltime, NULL)) {
		return 0;
	}
	return (double)walltime.tv_sec + (double)walltime.tv_usec * .000001;
}

// add code -->
/* time check 해서 해당 class 가 일정 시간이상 잡히면 사진저장 */
void webcam_capture()
{

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	//char from_buff = '0'; // 정지 명령 위한 변수
	send_buff[1] = '0';	// 명령 초기화
	send_buff[2] = '0';

	per_time = get_wall_time() - per_chk_time;
	if (per_chk == 1)
	{
		if (per_time > 2.)
		{
			send_buff[1] = '5';	// 일정 시간이상 발견 시 정지 명령
		}
		if (per_time > 5.)
		{
			char filename[256];
			sprintf(filename, "capture/person/%d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec); //시간 동기화에 따라 tm.tm_hour 부분 수정해주기
			save_cv_jpg(show_img, filename);
			send_buff[1] = '0'; // 정지 명령 해제
			send_buff[2] = '1'; // person 캡처 flag
			per_chk_time = get_wall_time();
		}
	}
	else { //per_chk==0
		per_chk_time = get_wall_time();
		//send_buff[1] = '0'; // 정지 명령 해제
	}

	fire_time = get_wall_time() - fire_chk_time;
	if (fire_chk == 1)
	{
		if (fire_time > 2.)
		{
			send_buff[1] = '5'; // 일정 시간이상 발견 시 정지 명령
		}
		if (fire_time > 5.)
		{
			char filename[256];
			sprintf(filename, "capture/fire/%d%02d%02d_%02d%02d%02d.jpg", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
			save_cv_jpg(show_img, filename);
			send_buff[1] = '0';
			send_buff[2] = '2'; // fire 캡처 flag
			fire_chk_time = get_wall_time();
		}
	}
	else {
		fire_chk_time = get_wall_time();
		//send_buff[1] = '0'; // 정지 명령 해제
	}

}
// end code <--

void demo(char *cfgfile, char *weightfile, float thresh, float hier_thresh, int cam_index, const char *filename, char **names, int classes,
	int frame_skip, char *prefix, char *out_filename, int mjpeg_port, int json_port, int dont_show, int ext_output, int letter_box_in)
{
	letter_box = letter_box_in;
	in_img = det_img = show_img = NULL;
	//skip = frame_skip;
	image **alphabet = load_alphabet();
	int delay = frame_skip;
	demo_names = names;
	demo_alphabet = alphabet;
	demo_classes = classes;
	demo_thresh = thresh;
	demo_ext_output = ext_output;
	demo_json_port = json_port;
	printf("Demo\n");
	net = parse_network_cfg_custom(cfgfile, 1, 1);    // set batch=1
	if (weightfile) {
		load_weights(&net, weightfile);
	}
	fuse_conv_batchnorm(net);
	calculate_binary_weights(net);
	srand(2222222);

	// add code -->
#ifndef STREAM_FROM_JETBOT
	// origin code
	if (filename) {
		printf("video file: %s\n", filename);
		cap = get_capture_video_stream(filename);
	}
	else {
		printf("Webcam index: %d\n", cam_index);
		cap = get_capture_webcam(cam_index);
	}

	if (!cap) {
#ifdef WIN32
		printf("Check that you have copied file opencv_ffmpeg340_64.dll to the same directory where is darknet.exe \n");
#endif
		error("Couldn't connect to webcam.\n");
	}
#else
	// socket define
	WSADATA wsa_data;
	SOCKADDR_IN server_addr, client_addr;
	int client_addr_size;

	// winsock 초기화, 버전 2.2
	if (0 < WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
		perror("WSAStartup() error\n");
		return 0;
	}

	// server socket 생성
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == server_socket) {
		perror("socket() error\n");
		return 0;
	}

	// server addr 초기화
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// server socket 및 addr 정보를 커널에 등록
	if (SOCKET_ERROR == bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr))) {
		perror("bind() error\n");
		return 0;
	}

	// client 의 connect 대기
	if (SOCKET_ERROR == listen(server_socket, 5)) {
		perror("listen() error\n");
		return 0;
	}
	printf("wait for client connection...\n");

	// client connect 수락
	client_addr_size = sizeof(client_addr);
	client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &client_addr_size);

	if (INVALID_SOCKET == client_socket) {
		perror("accept() error\n");
		return 0;
	}
	printf("connected!\n\n");

#endif

	int ret, tracking, manual;
	unsigned char* jpg_data = NULL;
	int jpg_size = 0;

	float target_xval = .0f;
	float target_wval = .0f;
	float target_hval = .0f;
	float distance_val = .0f;

	// end code <--

	layer l = net.layers[net.n - 1];
	int j;

	avg = (float *)calloc(l.outputs, sizeof(float));
	for (j = 0; j < NFRAMES; ++j) predictions[j] = (float *)calloc(l.outputs, sizeof(float));
	for (j = 0; j < NFRAMES; ++j) images[j] = make_image(1, 1, 3);

	if (l.classes != demo_classes) {
		printf("Parameters don't match: in cfg-file classes=%d, in data-file classes=%d \n", l.classes, demo_classes);
		getchar();
		exit(0);
	}


	flag_exit = 0;

	pthread_t fetch_thread;
	pthread_t detect_thread;

	fetch_in_thread(0);
	det_img = in_img;
	det_s = in_s;

	// add code -->
#ifndef STREAM_FROM_JETBOT
#else
	send_buff[0] = 'o';
	ret = send(client_socket, send_buff, sizeof(send_buff), 0);
	if (ret < 0) {
		perror("[send socket] message write fail\n");
		return 0;
	}
#endif
	// end code <--

	fetch_in_thread(0);
	detect_in_thread(0);
	det_img = in_img;
	det_s = in_s;

	// add code -->
#ifndef STREAM_FROM_JETBOT
#else
	send_buff[0] = 'o';
	ret = send(client_socket, send_buff, sizeof(send_buff), 0);
	if (ret < 0) {
		perror("[send socket] message write fail\n");
		return 0;
	}
#endif
	// end code <--

	for (j = 0; j < NFRAMES / 2; ++j) {
		fetch_in_thread(0);
		detect_in_thread(0);
		det_img = in_img;
		det_s = in_s;

		// add code -->
#ifndef STREAM_FROM_JETBOT
#else
		send_buff[0] = 'o';
		ret = send(client_socket, send_buff, sizeof(send_buff), 0);
		if (ret < 0) {
			perror("[send socket] message write fail\n");
			return 0;
		}
#endif
		// end code <--

	}

	int count = 0;
	if (!prefix && !dont_show) {
		int full_screen = 0;
		create_window_cv("Demo", full_screen, 1352, 1013);
	}


	write_cv* output_video_writer = NULL;
	if (out_filename && !flag_exit)
	{
		int src_fps = 25;
		src_fps = get_stream_fps_cpp_cv(cap);
		output_video_writer =
			create_video_writer(out_filename, 'D', 'I', 'V', 'X', src_fps, get_width_mat(det_img), get_height_mat(det_img), 1);

		//'H', '2', '6', '4'
		//'D', 'I', 'V', 'X'
		//'M', 'J', 'P', 'G'
		//'M', 'P', '4', 'V'
		//'M', 'P', '4', '2'
		//'X', 'V', 'I', 'D'
		//'W', 'M', 'V', '2'
	}

	double before = get_wall_time();
	// add code -->
	per_chk_time = get_wall_time();
	fire_chk_time = get_wall_time();
	// end code <--

	while (1) {
		++count;
		{
			if (pthread_create(&fetch_thread, 0, fetch_in_thread, 0)) error("Thread creation failed");
			if (pthread_create(&detect_thread, 0, detect_in_thread, 0)) error("Thread creation failed");

			float nms = .45;    // 0.4F
			int local_nboxes = nboxes;
			detection *local_dets = dets;

			//if (nms) do_nms_obj(local_dets, local_nboxes, l.classes, nms);    // bad results
			if (nms) do_nms_sort(local_dets, local_nboxes, l.classes, nms);

			//printf("\033[2J");
			//printf("\033[1;1H");
			//printf("\nFPS:%.1f\n", fps);
			printf("Objects:\n\n");

			++frame_id;
			if (demo_json_port > 0) {
				int timeout = 400000;
				send_json(local_dets, local_nboxes, l.classes, demo_names, frame_id, demo_json_port, timeout);
			}

			// add code -->

			mode = atoi(mode_info);
			//mode = mode_info[0] - 48;
			//manual = mode_info[1] - 48;
			printf("[mode]: %d\n", mode);
			//printf("[manual]: %d\n", manual);
			// origin
			//draw_detections_cv_v3(show_img, local_dets, local_nboxes, demo_thresh, demo_names, demo_alphabet, demo_classes, demo_ext_output);
			if (mode == 110) { // 외출 청소 모드
				draw_detections_cv_v3(show_img, local_dets, local_nboxes, demo_thresh, demo_names, demo_alphabet, demo_classes, demo_ext_output, mode, -1, NULL, NULL, NULL);
			}
			else if (mode == 310) { // tracking 모드
				tracking = draw_detections_cv_v3(show_img, local_dets, local_nboxes, demo_thresh, demo_names, demo_alphabet, demo_classes, demo_ext_output, mode, 1, &target_xval, &target_wval, &target_hval);

				// 추적 목표 감지 되면
				if (tracking) {
					distance_val = target_wval * target_hval;
					printf("[darknet] dog tracking");
					printf("dist: %f\n", distance_val);
					if (target_xval > 0.6) {
						printf("우측으로 치우침");
						// 좌우 제어하게 동작명령
						send_buff[1] = '3';
					}
					else if (target_xval < 0.4) {
						printf("좌측으로 치우침");
						// 좌우 제어하게 동작명령
						send_buff[1] = '2';
					}
					else {
						printf("타겟 중앙!");
						// 직진
						send_buff[1] = '1';
					}
					if (distance_val > 0.5) {
						printf("정지");
						// 타겟 가까움
						send_buff[1] = '5';
					}
				}
				else // 추적 목표 감지 안되면
					send_buff[1] = '5';
			}
			else
				send_buff[1] = '0';
			// end code <--
			free_detections(local_dets, local_nboxes);

			printf("\nFPS:%.1f\n", fps);

			if (!prefix) {
				if (!dont_show) {
					// add code -->
					//origin
					//show_image_mat(show_img, "Demo");
					if (mode == 110 || mode == 310)
						show_image_mat(show_img, "Demo");
					else
						show_image_mat(in_img, "Demo");
					// end code <--
					int c = wait_key_cv(1);
					if (c == 10) {
						if (frame_skip == 0) frame_skip = 60;
						else if (frame_skip == 4) frame_skip = 0;
						else if (frame_skip == 60) frame_skip = 4;
						else frame_skip = 0;
					}
					else if (c == 27 || c == 1048603) // ESC - exit (OpenCV 2.x / 3.x)
					{
						flag_exit = 1;

						// add code -->
#ifndef STREAM_FROM_JETBOT
#else
						send_buff[0] = 'c';
						ret = send(client_socket, send_buff, sizeof(send_buff), 0);
						if (ret < 0) {
							perror("[send socket] message write fail\n");
							return 0;
						}
#endif
						// end code <--
					}
				}
			}
			else {
				char buff[256];
				sprintf(buff, "%s_%08d.jpg", prefix, count);
				if (show_img) save_cv_jpg(show_img, buff);
			}

			// if you run it with param -mjpeg_port 8090  then open URL in your web-browser: http://localhost:8090
			if (mjpeg_port > 0 && show_img) {
				int port = mjpeg_port;
				int timeout = 400000;
				int jpeg_quality = 40;    // 1 - 100
				send_mjpeg(show_img, port, timeout, jpeg_quality);
			}

			// save video file
			if (output_video_writer && show_img) {
				write_frame_cv(output_video_writer, show_img);
				printf("\n cvWriteFrame \n");
			}

			// add code -->
			// capture image
			if (mode == 110)
				webcam_capture();
			else {
				send_buff[1] = '0';
				send_buff[2] = '0';
			}

			// 사진 캡처했으면
			if (send_buff[2] != '0') {
				// img 를 jpg로 반환
				if (!(jpg_data = mat_to_jpg_cv(&show_img, jpg_data, 50, &jpg_size)))
					printf("jpg convert fail!\n");
			}
			// end code <--

			release_mat(&show_img);

			pthread_join(fetch_thread, 0);
			pthread_join(detect_thread, 0);

			// add code -->
#ifndef STREAM_FROM_JETBOT
#else
			free(recv_data);
#endif
			// end code <--

			if (flag_exit == 1) break;

			if (delay == 0) {
				show_img = det_img;
			}
			det_img = in_img;
			det_s = in_s;
		}
		--delay;
		if (delay < 0) {
			delay = frame_skip;

			//double after = get_wall_time();
			//float curr = 1./(after - before);
			double after = get_time_point();    // more accurate time measurements
			float curr = 1000000. / (after - before);
			fps = curr;
			before = after;
		}


		// add code -->
#ifndef STREAM_FROM_JETBOT
#else
		send_buff[0] = mode_info[0];

		printf("[send_buff] %c, %c, %c\n", send_buff[0], send_buff[1], send_buff[2]);
		ret = send(client_socket, send_buff, sizeof(send_buff), 0);
		if (ret < 0) {
			perror("[send socket] message write fail\n");
			return 0;
		}

		// 사진 캡처했으면 캡처한 사진 전송
		if (send_buff[2] != '0') {
			// jpg size 송신
			if (send(client_socket, (char*)(&jpg_size), sizeof(int), 0) < 0) {
				perror("[send socket] jpg_size write fail\n");
				return 0;
			}

			//printf("size: %d, jpg_data: %p\n", jpg_size, jpg_data);
			// jpg data 송신
			if (send(client_socket, jpg_data, jpg_size, 0) < 0) {
				perror("[send socket] jpg_data write fail\n");
				return 0;
			}
			free(jpg_data);
		}
#endif
		// end code <--
	}
	printf("input video stream closed. \n");
	if (output_video_writer) {
		release_video_writer(&output_video_writer);
		printf("output_video_writer closed. \n");
	}

	// free memory
	release_mat(&show_img);
	release_mat(&in_img);
	free_image(in_s);

	// add code -->
#ifndef STREAM_FROM_JETBOT
#else
	free(recv_data);
#endif
	// end code <--

	free(avg);
	for (j = 0; j < NFRAMES; ++j) free(predictions[j]);
	for (j = 0; j < NFRAMES; ++j) free_image(images[j]);

	free_ptrs((void **)names, net.layers[net.n - 1].classes);

	int i;
	const int nsize = 8;
	for (j = 0; j < nsize; ++j) {
		for (i = 32; i < 127; ++i) {
			free_image(alphabet[j][i]);
		}
		free(alphabet[j]);
	}
	free(alphabet);
	free_network(net);
	//cudaProfilerStop();
}
#else
void demo(char *cfgfile, char *weightfile, float thresh, float hier_thresh, int cam_index, const char *filename, char **names, int classes,
	int frame_skip, char *prefix, char *out_filename, int mjpeg_port, int json_port, int dont_show, int ext_output, int letter_box_in)
{
	fprintf(stderr, "Demo needs OpenCV for webcam images.\n");
}
#endif

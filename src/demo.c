#include "network.h"
#include "detection_layer.h"
#include "region_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "image.h"
#include "demo.h"
#include <sys/time.h>

// add code -->
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
// 동작모드 관련, 0 -> 카메라 스트리밍, 1 -> capture mode, 2 -> tracking mode
// end code <--

#define DEMO 1

#ifdef OPENCV

static char **demo_names;
static image **demo_alphabet;
static int demo_classes;

static network *net;
static image buff [3];
static image buff_letter[3];
static int buff_index = 0;
static void * cap;
static float fps = 0;
static float demo_thresh = 0;
static float demo_hier = .5;
static int running = 0;

static int demo_frame = 3;
static int demo_index = 0;
static float **predictions;
static float *avg;
static int demo_done = 0;
static int demo_total = 0;
double demo_time;

// add code -->
extern int per_chk;
double test_chk_time;
double per_chk_time;
int fd;
int mode = 2;
// end code <--

detection *get_network_boxes(network *net, int w, int h, float thresh, float hier, int *map, int relative, int *num);

int size_network(network *net)
{
    int i;
    int count = 0;
    for(i = 0; i < net->n; ++i){
        layer l = net->layers[i];
        if(l.type == YOLO || l.type == REGION || l.type == DETECTION){
            count += l.outputs;
        }
    }
    return count;
}

void remember_network(network *net)
{
    int i;
    int count = 0;
    for(i = 0; i < net->n; ++i){
        layer l = net->layers[i];
        if(l.type == YOLO || l.type == REGION || l.type == DETECTION){
            memcpy(predictions[demo_index] + count, net->layers[i].output, sizeof(float) * l.outputs);
            count += l.outputs;
        }
    }
}

detection *avg_predictions(network *net, int *nboxes)
{
    int i, j;
    int count = 0;
    fill_cpu(demo_total, 0, avg, 1); // demo_total 만큼 반복, avg[]에 0 값 채움. (초기화)
    for(j = 0; j < demo_frame; ++j){ // demo_frame = 3, 
        axpy_cpu(demo_total, 1./demo_frame, predictions[j], 1, avg, 1);
    } // demo_total만큼 반복, avg에 1/3 * predictions 함 (demo_total이 3개니까 평균낸거)
    for(i = 0; i < net->n; ++i){
        layer l = net->layers[i];
        if(l.type == YOLO || l.type == REGION || l.type == DETECTION){
            memcpy(l.output, avg + count, sizeof(float) * l.outputs);
            count += l.outputs;
        }
    }
    detection *dets = get_network_boxes(net, buff[0].w, buff[0].h, demo_thresh, demo_hier, 0, 1, nboxes);
    return dets;
}

void *detect_in_thread(void *ptr) // net을 통해 경계박스 및 class 를 예측하는 부분. 
{
	// code add -->
	int mode = *((int*)ptr);
	printf("[detect_thread] mode : %d\n", mode); // mode값 확인
	
	if (mode == 0) return 0;

	float target_xval = .0f;
	float target_wval = .0f;
	float target_hval = .0f;
	float distance_val = .0f;
	// code end <--

    running = 1; // network 예측하고있는 상태 의미? 어디 쓰는진 못찾겠음.
    float nms = .4; // non max Suppression 어쩌구 값 0.4

    layer l = net->layers[net->n-1]; // 최종레이어?????
    float *X = buff_letter[(buff_index+2)%3].data; // 경계박스 데이터
    network_predict(net, X); // net을 통해 경계박스 예측(network forward) (아마 예측텐서만 생성?)

    /*
       if(l.type == DETECTION){
       get_detection_boxes(l, 1, 1, demo_thresh, probs, boxes, 0);
       } else */
    remember_network(net); // 예측된 layer output값 predictions 변수에 저장 (예측텐서 값을 predictions[] 에 저장)
    detection *dets = 0;
    int nboxes = 0;
    dets = avg_predictions(net, &nboxes); // network를 통해 예측된 box에 대한 정보


    /*
       int i,j;
       box zero = {0};
       int classes = l.classes;
       for(i = 0; i < demo_detections; ++i){
       avg[i].objectness = 0;
       avg[i].bbox = zero;
       memset(avg[i].prob, 0, classes*sizeof(float));
       for(j = 0; j < demo_frame; ++j){
       axpy_cpu(classes, 1./demo_frame, dets[j][i].prob, 1, avg[i].prob, 1);
       avg[i].objectness += dets[j][i].objectness * 1./demo_frame;
       avg[i].bbox.x += dets[j][i].bbox.x * 1./demo_frame;
       avg[i].bbox.y += dets[j][i].bbox.y * 1./demo_frame;
       avg[i].bbox.w += dets[j][i].bbox.w * 1./demo_frame;
       avg[i].bbox.h += dets[j][i].bbox.h * 1./demo_frame;
       }
    //copy_cpu(classes, dets[0][i].prob, 1, avg[i].prob, 1);
    //avg[i].objectness = dets[0][i].objectness;
    }
     */

    if (nms > 0) do_nms_obj(dets, nboxes, l.classes, nms); // 예측된 box에 대해 nms 시행

    printf("\033[2J");	// 콘솔 화면 clear
    printf("\033[1;1H"); // 
    printf("\nFPS:%.1f\n",fps);
    printf("Objects:\n\n");
    image display = buff[(buff_index+2) % 3]; // 박스 그릴 전체 이미지

    // draw_detections(display, dets, nboxes, demo_thresh, demo_names, demo_alphabet, demo_classes); // 예측 확률 넘긴것들 클래스 찾아내서 박스 그리기
    // code add -->
	if (mode == 1)
		draw_detections(display, dets, nboxes, demo_thresh, demo_names, demo_alphabet, demo_classes, -1, NULL, NULL, NULL);
	else if (mode == 2) {
		if (draw_detections(display, dets, nboxes, demo_thresh, demo_names, demo_alphabet, demo_classes, 16, &target_xval, &target_wval, &target_hval)) {
			distance_val = target_wval * target_hval;
			printf("[detect_thread] dog tracking\n");

			if (target_xval > 0.6) {
				printf("우측으로 치우침\n");
				// 좌우 제어하게 동작명령
			}
			else if (target_xval < 0.4) {
				printf("좌측으로 치우침\n");
				// 좌우 제어하게 동작명령
			}
			else {
				printf("타겟 중앙!\n");
				// 정지
			}

			if (distance_val < 0.2) {
				printf("속도 올림\n");
				// 타겟 멀어짐, 속도 업
			}
			else if (mode == 3) { // 조건 : 위에 좌우 제어 중일 때
				// 방향 제어중일 땐 정지
			}
			else {
				printf("속도 내림");
				// 타겟 가까움, 속도 다운
			}
		}
	}
	// code end <--
	free_detections(dets, nboxes);

    demo_index = (demo_index + 1)%demo_frame; // demo_index 증가인데 뭐하는거지?
    running = 0;
    return 0;
}

void *fetch_in_thread(void *ptr) 
{  // 이미지를 저장하는 buff를 3개로이루어진 배열로 두고 fetch를 하고 바로 보여주는게 아니라 2 frame 전 이미지를 보여줌
    free_image(buff[buff_index]);
    buff[buff_index] = get_image_from_stream(cap); // 새로운 영상 받아옴
    if(buff[buff_index].data == 0) { // 영상 못읽어오면 끝
        demo_done = 1;
        return 0;
    }
    letterbox_image_into(buff[buff_index], net->w, net->h, buff_letter[buff_index]); // 경계박스를 다시 그림(network의 예측 w h 값으로)
    return 0;
}

void *display_in_thread(void *ptr)
{
    int c = show_image(buff[(buff_index + 1)%3], "Demo", 1);
    if (c != -1) c = c%256;
    if (c == 27) { // ESC
        demo_done = 1;
        return 0;
    } else if (c == 82) { // r
        demo_thresh += .02;
    } else if (c == 84) { // t
        demo_thresh -= .02;
        if(demo_thresh <= .02) demo_thresh = .02;
    } else if (c == 83) { // s
        demo_hier += .02;
    } else if (c == 81) { // q
        demo_hier -= .02;
        if(demo_hier <= .0) demo_hier = .0;
    }
    return 0;
}

void *display_loop(void *ptr)
{
    while(1){
        display_in_thread(0);
    }
}

void *detect_loop(void *ptr)
{
    while(1){
        detect_in_thread(0);
    }
}

// add code -->
/* time check 해서 해당 class 가 일정 시간이상 잡히면 사진저장 */
void webcam_capture()
{

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char from_buff[10];

	if (per_chk == 1)
	{
		test_chk_time = what_time_is_it_now() - per_chk_time;
		if (what_time_is_it_now() - per_chk_time > 1.)
		{
			from_buff[0] = '1';
			write(fd, from_buff, 1);
			//printf("demo.c : %d\n", per_chk);
		}
		if (what_time_is_it_now() - per_chk_time > 5.)
		{
			char filename[256];
			sprintf(filename, "%d%02d%02d_%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec); //시간 동기화에 따라 tm.tm_hour 부분 수정해주기
			save_image(buff[(buff_index + 1) % 3], filename);
			per_chk_time = what_time_is_it_now();
			from_buff[0] = '0';
			write(fd, from_buff, 1);

		}
	}
	else { //per_chk==0
		per_chk_time = what_time_is_it_now();

	}
}
// end code <--

void demo(char *cfgfile, char *weightfile, float thresh, int cam_index, const char *filename, char **names, int classes, int delay, char *prefix, int avg_frames, float hier, int w, int h, int frames, int fullscreen)
{
    //demo_frame = avg_frames;
    image **alphabet = load_alphabet(); // 알파벳 image파일 불러옴
    demo_names = names;
    demo_alphabet = alphabet;
    demo_classes = classes;
    demo_thresh = thresh;
    demo_hier = hier;
    printf("Demo\n");
    net = load_network(cfgfile, weightfile, 0); // cfg 파일과 weight 파일을 기반으로 network 생성
    set_batch_network(net, 1); // batch를 1로 
    pthread_t detect_thread;
    pthread_t fetch_thread;

    srand(2222222);

    int i;
    demo_total = size_network(net);	// network layer 갯수?????? 
    predictions = calloc(demo_frame, sizeof(float*));	// network layer 갯수 크기만큼의 인자 변수가 demo_frame갯수만큼 있음. 
    for (i = 0; i < demo_frame; ++i){
        predictions[i] = calloc(demo_total, sizeof(float));
    }
    avg = calloc(demo_total, sizeof(float)); // demo_frame 들의 평균을 내기위해

    if(filename){	// 카메라 filename으로 켰을 때
        printf("video file: %s\n", filename);
        cap = open_video_stream(filename, 0, 0, 0, 0); // opencv함수를 VideoCapture 객체 만들고 크기 지정 (c에선 없으므로 void * 로 사용)
    }else{
        cap = open_video_stream(0, cam_index, w, h, frames);
    }

    if(!cap) error("Couldn't connect to webcam.\n");

    buff[0] = get_image_from_stream(cap); // 이미지 얻어옴
    buff[1] = copy_image(buff[0]); // 얻어온 이미지 복사
    buff[2] = copy_image(buff[0]);
    buff_letter[0] = letterbox_image(buff[0], net->w, net->h); // 네모 칠 이미지 생성
    buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
    buff_letter[2] = letterbox_image(buff[0], net->w, net->h);

    int count = 0;
    if(!prefix){
        make_window("Demo", 1352, 1013, fullscreen);	// window 생성(fullscreen 옵션 참이면 풀로, 아니면 2 3번째 인자값 크기로)
    }

    demo_time = what_time_is_it_now(); // usec 단위로 현재 초 얻어옴

	// add code -->
	if (-1 == (fd = open("/tmp/from_yolo", O_WRONLY, 0666)))
	{
		perror("open() error");
		exit(1);
	}
	per_chk_time = what_time_is_it_now(); //추가 ###
	// end code <--

    while(!demo_done){ // demo_done 값은 display_in_thread 에서 ESC 눌리거나 fetch_in_thread에서 영상 못읽어오면 1로 됨(종료)
        buff_index = (buff_index + 1) %3; // buff_index는 0 1 2 값만 가짐
/*
		if(pthread_create(&fetch_thread, 0, fetch_in_thread, 0)) error("Thread creation failed"); // fetch_in_thread 생성
        if(pthread_create(&detect_thread, 0, detect_in_thread, 0)) error("Thread creation failed"); // detec_in_thread 생성
        if(!prefix){
            fps = 1./(what_time_is_it_now() - demo_time); // FPS : 1 / 1 장 처리하는데 걸리는 시간
            demo_time = what_time_is_it_now();
            display_in_thread(0); // 이미지 띄우는 함수 실행
        }else{
            char name[256];
            sprintf(name, "%s_%08d", prefix, count);
            save_image(buff[(buff_index + 1)%3], name);
        }
        pthread_join(fetch_thread, 0); // thread들 종료까지 기다림
        pthread_join(detect_thread, 0);
        ++count;
*/
		// code add -->
		if (pthread_create(&fetch_thread, 0, fetch_in_thread, &mode)) error("Thread creation failed"); // fetch_in_thread 생성
		if (pthread_create(&detect_thread, 0, detect_in_thread, &mode)) error("Thread creation failed"); // detec_in_thread 생성

		fps = 1. / (what_time_is_it_now() - demo_time);
		demo_time = what_time_is_it_now();
		display_in_thread(0);
		
		if (mode == 1 || mode == 2) webcam_capture();

		pthread_join(fetch_thread, 0);
		pthread_join(detect_thread, 0);
		// code end <--
    }
}

/*
   void demo_compare(char *cfg1, char *weight1, char *cfg2, char *weight2, float thresh, int cam_index, const char *filename, char **names, int classes, int delay, char *prefix, int avg_frames, float hier, int w, int h, int frames, int fullscreen)
   {
   demo_frame = avg_frames;
   predictions = calloc(demo_frame, sizeof(float*));
   image **alphabet = load_alphabet();
   demo_names = names;
   demo_alphabet = alphabet;
   demo_classes = classes;
   demo_thresh = thresh;
   demo_hier = hier;
   printf("Demo\n");
   net = load_network(cfg1, weight1, 0);
   set_batch_network(net, 1);
   pthread_t detect_thread;
   pthread_t fetch_thread;

   srand(2222222);

   if(filename){
   printf("video file: %s\n", filename);
   cap = cvCaptureFromFile(filename);
   }else{
   cap = cvCaptureFromCAM(cam_index);

   if(w){
   cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, w);
   }
   if(h){
   cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, h);
   }
   if(frames){
   cvSetCaptureProperty(cap, CV_CAP_PROP_FPS, frames);
   }
   }

   if(!cap) error("Couldn't connect to webcam.\n");

   layer l = net->layers[net->n-1];
   demo_detections = l.n*l.w*l.h;
   int j;

   avg = (float *) calloc(l.outputs, sizeof(float));
   for(j = 0; j < demo_frame; ++j) predictions[j] = (float *) calloc(l.outputs, sizeof(float));

   boxes = (box *)calloc(l.w*l.h*l.n, sizeof(box));
   probs = (float **)calloc(l.w*l.h*l.n, sizeof(float *));
   for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = (float *)calloc(l.classes+1, sizeof(float));

   buff[0] = get_image_from_stream(cap);
   buff[1] = copy_image(buff[0]);
   buff[2] = copy_image(buff[0]);
   buff_letter[0] = letterbox_image(buff[0], net->w, net->h);
   buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
   buff_letter[2] = letterbox_image(buff[0], net->w, net->h);
   ipl = cvCreateImage(cvSize(buff[0].w,buff[0].h), IPL_DEPTH_8U, buff[0].c);

   int count = 0;
   if(!prefix){
   cvNamedWindow("Demo", CV_WINDOW_NORMAL); 
   if(fullscreen){
   cvSetWindowProperty("Demo", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
   } else {
   cvMoveWindow("Demo", 0, 0);
   cvResizeWindow("Demo", 1352, 1013);
   }
   }

   demo_time = what_time_is_it_now();

   while(!demo_done){
buff_index = (buff_index + 1) %3;
if(pthread_create(&fetch_thread, 0, fetch_in_thread, 0)) error("Thread creation failed");
if(pthread_create(&detect_thread, 0, detect_in_thread, 0)) error("Thread creation failed");
if(!prefix){
    fps = 1./(what_time_is_it_now() - demo_time);
    demo_time = what_time_is_it_now();
    display_in_thread(0);
}else{
    char name[256];
    sprintf(name, "%s_%08d", prefix, count);
    save_image(buff[(buff_index + 1)%3], name);
}
pthread_join(fetch_thread, 0);
pthread_join(detect_thread, 0);
++count;
}
}
*/
#else
void demo(char *cfgfile, char *weightfile, float thresh, int cam_index, const char *filename, char **names, int classes, int delay, char *prefix, int avg, float hier, int w, int h, int frames, int fullscreen)
{
    fprintf(stderr, "Demo needs OpenCV for webcam images.\n");
}
#endif


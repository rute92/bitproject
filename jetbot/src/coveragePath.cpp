#include <iostream>
#include <algorithm>
#include "opencv2/opencv.hpp"
#include "astar.h"

#define CLEANED	128
#define RESIZEFACTOR	0.5

/* Position 클래스 선언 및 정의 */
class Position {
public:
	int x;
	int y;

	Position();
	Position(int _x, int _y);
	bool operator<(const Position& pos) const;
};

Position::Position()
	: x(0), y(0) 
{

}

Position::Position(int _x, int _y)
	: x(_x), y(_y)
{

}
bool Position::operator<(const Position& pos) const {
	if (x == pos.x)
		return (y < pos.y);
	else
		return (x < pos.x);
}


int doIBMotion(myMap& map, Position& now);
Position findNearestPosition(std::set<Position>& backTrackingList, const Position& now);
Position convertToResize(Position& origin, double resizeFactor);
Position convertToOrigin(Position& resize, double resizeFactor);


int main()
{
	// 1. astar
	//unsigned char* mapData = new unsigned char[100];
	//for (int i = 0; i < 100; ++i) {
	//	memset(mapData, LOAD, sizeof(unsigned char) * 100);
	//}
	//myMap mapp(10, 10, mapData); // 10x10 맵 생성
	//mapp.setObject(4, 3);
	//mapp.setObject(4, 4);
	//mapp.setObject(4, 5);
	//mapp.setObject(4, 6);
	//mapp.setObject(4, 7);
	//mapp.setObject(2, 5);
	//mapp.setObject(3, 5);
	//mapp.setObject(5, 5);

	//mapp.printMap();

	//Astar as;
	//as.setMap(mapp);

	//myNode* fin;
	//fin = as.findRoute(0, 0, 9, 9);
	//as.setPathToMap(fin);
	//as.printMapAll();

	//2. Coverage Path Planning
	cv::Mat test = cv::imread("matrix.bmp", cv::IMREAD_GRAYSCALE);
	std::cout << "자르기 전 OriginMap 사이즈: " << test.cols << ", " <<test.rows << std::endl;

	cv::Mat temp2;
	test(cv::Rect(50, 50, 120, 120)).copyTo(temp2);	// rect(x, y, width, height)
	std::cout << "자르고 난 후 OriginMap 사이즈: " << temp2.cols << ", " << temp2.rows << std::endl;

	cv::Mat temp3;
	cv::resize(temp2, temp3, cv::Size(0, 0), RESIZEFACTOR, RESIZEFACTOR, cv::INTER_AREA);
	std::cout << "자르고 난 후 resizeMap(testMap) 사이즈: " << temp3.cols << ", " << temp3.rows << std::endl;

	//myMap testMap(test.cols, test.rows, test.data);
	myMap originMap(temp2.cols, temp2.rows, temp2.data);
	myMap testMap(temp3.cols, temp3.rows, temp3.data);

	cv::Mat resultOrigin(temp2.rows, temp2.cols, CV_8UC1, originMap.getMapAddr());
	cv::Mat resultTest(temp3.rows, temp3.cols, CV_8UC1, testMap.getMapAddr());
	
	cv::namedWindow("origin", cv::WINDOW_NORMAL);
	cv::namedWindow("test", cv::WINDOW_NORMAL);
	cv::namedWindow("resultOrigin", cv::WINDOW_NORMAL);
	cv::namedWindow("resultTest", cv::WINDOW_NORMAL);

	cv::imshow("origin", temp2);
	cv::imshow("test", temp3);

	Astar astar;
	myNode* fin;
	astar.setMap(testMap);

	bool astarFlag = false;
	std::vector<Position> astarRoute;
	std::set<Position> backTrackingList;
	for (int j = 0; j != testMap.getMapWidth(); ++j) {
		for (int i = 0; i != testMap.getMapHeight(); ++i) {
			if (testMap.getMapData(i, j) == LOAD) {
				backTrackingList.insert(Position(i, j));
			}
		}
	}
	Position nowOrigin(80, 80);	// 시작좌표, 원래 catographer에서 localization 통해 얻음
	Position nowResized;
	Position Nearest;

	nowResized = convertToResize(nowOrigin, RESIZEFACTOR);
	

	int ret;
	std::set<Position>::iterator findPos;

	printf("\n청소 시작\n\n");
	while (!backTrackingList.empty()) {
		// darknet으로 부터 쓰여진 파일 읽기

		cv::imshow("resultOrigin", resultOrigin);
		cv::imshow("resultTest", resultTest);
		cv::waitKey(50);
		if (!astarFlag) {
			testMap.setMapData(nowResized.x, nowResized.y, CLEANED);	// 지나간 경로 체크, 지도에 CLEANED값 표시
			findPos = backTrackingList.find(nowResized); // 현재 좌표가 backTrackingList에 있는지 찾기
			if (findPos != backTrackingList.end()) { // 있다면
				backTrackingList.erase(findPos); // backTrackingList에서 삭제
			}

			// 사방에 대해 갈수있는지 체크, 갈수있으면 x y 업데이트해줌
			ret = doIBMotion(testMap, nowResized);
			switch (ret) {
			case 0:	// 갈곳 없으면
				// 가장 가까운 점 찾기
				Nearest = findNearestPosition(backTrackingList, nowResized);

				printf("시작 testMap 좌표: [%d, %d]\n", nowResized.x, nowResized.y); // 원래 좌표
				// A* 써서 그 지점 갈 경로 찾기
				fin = astar.findRoute(nowResized.x, nowResized.y, Nearest.x, Nearest.y); // 경로 찾기
				while (fin->parent != nullptr) { // 저장된 노드로부터 경로 추출
					Position tmp(fin->xPos, fin->yPos);
					astarRoute.push_back(tmp);	// vector에 추출된 경로들 저장.
					fin = fin->parent;
				}
				
				// astarRoute에 저장된 길 따라 실제 이동하게 끔 명령하는 곳으로 넘어가는 flag
				astarFlag = true;

				//nowResized.x = Nearest.x;
				//nowResized.y = Nearest.y;
				break;
			case 1: // 동쪽
				// (int)round(x + 1 / RESIZEFACTOR) 값 넘겨주기
				break;
			case 2: // 서쪽
				// (int)round(x - 1 / RESIZEFACTOR) 값 넘겨주기
				break;
			case 3: // 남쪽
				// (int)round(y - 1 / RESIZEFACTOR) 값 넘겨주기
				break;
			case 4: // 북쪽
				// (int)round(y + 1 / RESIZEFACTOR) 값 넘겨주기
				break;
			default:
				break;
			}

			nowOrigin = convertToOrigin(nowResized, RESIZEFACTOR);
			originMap.setMapData(nowOrigin.x, nowOrigin.y, CLEANED);
		}
		else { // astar 길찾기 일 때
			nowOrigin = convertToOrigin(nowResized, RESIZEFACTOR);
			originMap.setMapData(nowOrigin.x, nowOrigin.y, 200);
			if (!astarRoute.empty()) { // 안비었으면
				Position tmp = astarRoute.back();
				// (int)round((tmp.x - nowResized.x) / RESIZEFACTOR) x 값 변화량 넘겨주기
				// (int)round((tmp.y - nowResized.y) / RESIZEFACTOR) y 값 변화량 넘겨주기
				astarRoute.pop_back();
				printf("testMap 좌표 : [%d, %d]\n", tmp.x, tmp.y);
				nowResized = tmp;
			}
			else { // 비었으면
				astarFlag = false;
				printf("가까운길 찾기 완료 \n\n");
			}
		}
	}
	printf("청소 끝! \n");
	cv::imshow("resultOrigin", resultOrigin);
	cv::imshow("resultTest", resultTest);
	cv::waitKey(0);

	cv::destroyAllWindows();
}

/* Origin 좌표로 부터 Resize된 좌표로 변환 */
Position convertToResize(Position& origin, double resizeFactor) {
	Position temp;

	temp.x = (int)round(origin.x * resizeFactor);
	temp.y = (int)round(origin.y * resizeFactor);

	return temp;
}

/* Resize된 좌표로 부터 Origin 좌표로 변환 */
Position convertToOrigin(Position& resize, double resizeFactor) {
	Position temp;

	temp.x = (int)round(resize.x / resizeFactor);
	temp.y = (int)round(resize.y / resizeFactor);

	return temp;
}

/* 
IB(Intellectual Boustrophedon) 동작
	동 = x++, return 1
	서 = x--, return 2
	남 = y++, return 3
	북 = y--, return 4
	이동불가 시, return 0
	'ㄹ' 자 형태로 남/북 동작하며 서쪽에 공간이 있으면 서쪽 우선 동작
*/
int doIBMotion(myMap& map, Position& now)
{
	int ret;
	if (map.getMapData(now.x, now.y + 1) == LOAD && map.getMapData(now.x - 1, now.y) == LOAD) { // 남 , 서
		now.x--;	// 서
		ret = 2;
	}
	else if (map.getMapData(now.x, now.y - 1) == LOAD && map.getMapData(now.x - 1, now.y) == LOAD) { // 북, 서
		now.x--;	// 서
		ret = 2;
	}
	else if (map.getMapData(now.x, now.y - 1) == LOAD && map.getMapData(now.x, now.y + 1) == LOAD) { // 북, 남
		now.y++;	// 남
		ret = 3;
	}
	else if (map.getMapData(now.x, now.y - 1) == LOAD) { // 북
		now.y--;	// 북
		ret = 4;
	}
	else if (map.getMapData(now.x, now.y + 1) == LOAD) { // 남
		now.y++;	// 남
		ret = 3;
	}
	else if (map.getMapData(now.x - 1, now.y) == LOAD) { // 서
		now.x--;	// 서
		ret = 2;
	}
	else if (map.getMapData(now.x + 1, now.y) == LOAD) { // 동
		now.x++;	// 동
		ret = 1;
	}
	else {
		ret = 0;
	}

	return ret;
}


/* backTrackingList 에 있는 좌표들 중 now로부터 가장 가까운 점 찾기 */
Position findNearestPosition(std::set<Position>& backTrackingList, const Position& now)
{
	double minDistance = 10000;
	Position Nearest(now.x, now.y);	// 가장 가까운점 저장할 변수

	for (auto iter = backTrackingList.begin(); iter != backTrackingList.end(); ++iter) {
		int dist = abs((int)(now.x - (*iter).x)) + abs((int)(now.y - (*iter).y));
		if (dist < minDistance) { // 새로 계산한 거리가 기존꺼보다 작으면
			minDistance = dist; // minDist 업데이트
			Nearest.x = (*iter).x;
			Nearest.y = (*iter).y;
		}
	}

	return Nearest;
}

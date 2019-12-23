#include <iostream>
#include <algorithm>
#include "pathFinder.h"

#define RESIZEFACTOR	0.2

/*
   ###################################################################################
   ##############################  Position 클래스 정의  ##############################
   ###################################################################################
*/
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

/*
   ###################################################################################
   #############################  pathFinder 클래스 정의  #############################
   ###################################################################################
*/
void PathFinder::setMoveInterval(int _move) {
	move = _move;
}

/* 주어진 맵을 전부 탐색하는 좌표 List 반환 */
std::list<Position> PathFinder::findCoveragePath(Position start, cv::Mat mapImage, int show){

	map.setMap(mapImage.cols, mapImage.rows, mapImage.data);
	cv::Mat resultOrigin(map.getMapHeight(), map.getMapWidth(), CV_8UC1, map.getMapAddr());

	if (show) {
		//cv::namedWindow("resultOrigin", cv::WINDOW_NORMAL);
		cv::imshow("origin", mapImage);
	}
	
	for (int j = start.y % move; j < map.getMapHeight(); j += move) {
		for (int i = start.x % move; i < map.getMapWidth(); i += move) {
			if (map.getMapData(i, j) == LOAD) {
				backTrackingList.insert(Position(i, j));
			}
		}
	}
	
	Astar astar;
	myNode* fin;
	astar.setMap(map);

	int ret;
	Position Nearest;
	std::set<Position>::iterator findPos;
	std::list<Position> pathList;

	if (map.getMapData(start.x, start.y) != LOAD) {
		printf("start position is object!\n");
		Nearest = findNearestPosition();
		now = Nearest;
	}
	now = start;

	printf("\n경로탐색 시작\n\n");
	pathList.push_back(now);

	while (!backTrackingList.empty()) {
		if (show) {
			cv::imshow("resultOrigin", resultOrigin);
			cv::waitKey(1);
		}
		
		map.setMapData(now.x, now.y, CLEANED);

		findPos = backTrackingList.find(now);
		if (findPos != backTrackingList.end()) { // 있다면
			backTrackingList.erase(findPos); // backTrackingList에서 삭제
		}

		// 사방에 대해 갈수있는지 체크, 갈수있으면 x y 업데이트해줌
		ret = doIBMotion(now, move);
		if (ret == 0) { // 갈곳 없으면
			// 가장 가까운 점 찾기
			Nearest = findNearestPosition();

			// A* 써서 그 지점 갈 경로 찾기
			fin = astar.findRoute(Nearest.x, Nearest.y, now.x, now.y); // 경로 찾기
			//printf("%d\n", astar.hasFindRoute());
			if (astar.hasFindRoute()) {	// 길 찾으면
				while (fin->parent != nullptr) { // 저장된 노드로부터 경로 추출
					fin = fin->parent;
					Position tmp(fin->xPos, fin->yPos);
					pathList.push_back(tmp);	// pathList에 저장
					astarRoute.push_back(tmp);	// vector에 추출된 경로들 저장.
				}
				pathList.push_back(Nearest);

				if (show) {
					printf("A* 시작 : [%d, %d]\n", now.x, now.y);
					printf("A* 목표 : [%d, %d]\n", Nearest.x, Nearest.y);
					for (auto it = astarRoute.begin(); it != astarRoute.end(); ++it) {
						Position tmp = (*it);
						map.setMapData(tmp.x, tmp.y, 200);
						printf("A* 경로 좌표 : [%d, %d]\n", tmp.x, tmp.y);
					}
					printf("가까운길 찾기 완료 \n\n");
				}
				// astarRoute 초기화 및 현재 좌표 갱신
				astarRoute.clear();
				now = Nearest;
			}
			else {
				//printf("dfdf\n");
				findPos = backTrackingList.find(Nearest);
				if (findPos != backTrackingList.end()) { // 있다면
					backTrackingList.erase(findPos); // backTrackingList에서 삭제
				}
			}
		}
		else {
			pathList.push_back(now);
		}
	}
	printf("경로탐색 끝! \n");
	if (show) {
		cv::imshow("resultOrigin", resultOrigin);
		cv::waitKey(0);
		cv::destroyAllWindows();
	}

	return pathList;
}


/*
IB(Intellectual Boustrophedon) 동작
	동 = x++, 
	서 = x--, 
	남 = y++, 
	북 = y--, 
	이동불가 시, return 0
	'ㄹ' 자 형태로 남/북 동작하며 서쪽에 공간이 있으면 서쪽 우선 동작
*/
int PathFinder::doIBMotion(Position& now, int move)
{
	int ret = 1;
	if (map.getMapData(now.x, now.y + move) == LOAD && map.getMapData(now.x - move, now.y) == LOAD) { // 남 , 서
		//now.x--;	// 서
		now.x -= move;
		
	}
	else if (map.getMapData(now.x, now.y - move) == LOAD && map.getMapData(now.x - move, now.y) == LOAD) { // 북, 서
		//now.x--;	// 서
		now.x -= move;
	}
	else if (map.getMapData(now.x, now.y - move) == LOAD && map.getMapData(now.x, now.y + move) == LOAD) { // 북, 남
		//now.y++;	// 남
		now.y += move;
	}
	else if (map.getMapData(now.x, now.y - move) == LOAD) { // 북
		//now.y--;	// 북
		now.y -= move;
	}
	else if (map.getMapData(now.x, now.y + move) == LOAD) { // 남
		//now.y++;	// 남
		now.y += move;
	}
	else if (map.getMapData(now.x - move, now.y) == LOAD) { // 서
		//now.x--;	// 서
		now.x -= move;
	}
	else if (map.getMapData(now.x + move, now.y) == LOAD) { // 동
		//now.x++;	// 동
		now.x += move;
	}
	else {
		ret = 0;
	}

	return ret;
}


/* backTrackingList 에 있는 좌표들 중 now로부터 가장 가까운 점 찾기 */
Position PathFinder::findNearestPosition()
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

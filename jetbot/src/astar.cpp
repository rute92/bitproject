#include <iostream>
#include <algorithm>
#include "astar.h"


/* 
   ###################################################################################
   ################################  Node 클래스 정의  ################################
   ###################################################################################
*/

myNode::myNode() 
	: xPos(0), yPos(0), fScore(0), gScore(0), hScore(0), parent(nullptr)
{

}

myNode::myNode(unsigned _x, unsigned _y) 
	: xPos(_x), yPos(_y), fScore(0), gScore(0), hScore(0), parent(nullptr)
{

}

myNode::myNode(unsigned _x, unsigned _y, myNode* _parent) 
	: xPos(_x), yPos(_y), fScore(0), gScore(0), hScore(0), parent(_parent)
{

}

bool myNode::operator<(const myNode& node) const {
	if (fScore == node.fScore) {
		if (xPos == node.xPos)
			return yPos < node.yPos;
		else
			return xPos < node.xPos;
	}
	else
		return fScore > node.fScore;
}

// Node 좌표값 비교, 같으면 1 아니면 0
bool myNode::compNode(myNode* node) {
	return (this->xPos == node->xPos && this->yPos == node->yPos);
}

// Node 초기화
void myNode::initNode() {
	xPos = 0;
	yPos = 0;
	fScore = 0;
	gScore = 0;
	hScore = 0;
	parent = nullptr;
}


/*
   ###################################################################################
   ################################   myMap 클래스 정의  ################################
   ###################################################################################
*/
myMap::myMap()
	: width(0), height(0), data(nullptr)
{

}

myMap::myMap(int _width, int _height)
	: width(_width), height(_height), data(nullptr)
{

}

// 복사 생성자 (이거 안하면 Map 클래스를 인자로 받아서 처리할 때 얕은 복사 일어나서 나중에 소멸자 2번 부름)
myMap::myMap(const myMap& map)
	: width(map.width), height(map.height)
{
	int size = width * height;
	data = new unsigned char[size];

	for (int i = 0; i != size; ++i)
		data[i] = map.data[i];
}

myMap::myMap(int _width, int _height, unsigned char* _mapData)
	: width(_width), height(_height)
{
	int size = width * height;
	data = new unsigned char[size];

	for (int i = 0; i != size; ++i)
		data[i] = _mapData[i];
}

myMap::~myMap() {
	delete data;
}

 //복사 대입 연산자 재정의
myMap& myMap::operator=(const myMap& map) {
	width = map.width;
	height = map.height;
	int size = width * height;
	data = new unsigned char[size];

	for (int i = 0; i != size; ++i)
		data[i] = map.data[i];

	return *this;
}


// 외부 메모리 참고해서 Map 생성.
void myMap::setMap(int _width, int _height, unsigned char* _mapData) {
	width = _width;
	height = _height;
	int size = width * height;
	data = new unsigned char[size];
	for (int i = 0; i != size; ++i)
		data[i] = _mapData[i];
}

int myMap::getMapWidth() {
	return width;
}
int myMap::getMapHeight() {
	return height;
}
unsigned char* myMap::getMapAddr() {
	return data;
}

// 맵에 해당하는 좌표 값 반환
int myMap::getMapData(int _x, int _y) {
	if (_x < 0 || _y < 0 || _x >= width || _y >= height)
		return -1;
	return data[_y * width + _x];
}

// 맵에 해당하는 좌표 값에 val 값 설정
void myMap::setMapData(int _x, int _y, unsigned char val) {
	if (_x < 0 || _y < 0 || _x >= width || _y >= height)
		std::cerr << "setMapData fail!\n";
	else
		data[_y * width + _x] = val;
}



// 맵에 장애물 설치
void myMap::setObject(int _x, int _y) {
	if (_x < 0 || _y < 0 || _x >= width || _y >= height)
		std::cerr << "setObject fail!\n";
	else
		data[_y * width + _x] = OBJECT;
}


// 전체 맵 출력
void myMap::printMap() {
	printf("전체 맵 출력: %d x %d\n\n", width, height);
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			printf("%3d ", data[j * width + i]);
		}
		puts("");
	}
}

// 갈 수 있는 좌표인지 아닌지. 갈 수 있으면 1, 아니면 0
bool myMap::isWalkable(int _x, int _y) {
	if (_x < 0 || _y < 0 || _x >= width || _y >= height)
		return false;

	return (data[_y * width + _x] != OBJECT) ? true : false;
}


// 인자로 받은 Node 의 parent를 추적해서 PATH 그려줌.
void myMap::setPath(myNode* fin) {
	while (fin->parent != nullptr) {
		//printf("좌표: [%d, %d], F: %d, G: %d, H: %d\n", fin->xPos, fin->yPos, fin->fScore, fin->gScore, fin->hScore);
		setMapData(fin->xPos, fin->yPos, PATH);
		fin = fin->parent;
	}
	//printf("좌표: [%d, %d], F: %d, G: %d, H: %d\n", fin->xPos, fin->yPos, fin->fScore, fin->gScore, fin->hScore);
	setMapData(fin->xPos, fin->yPos, PATH);
}


/*
   ###################################################################################
   ###############################  pqueue 클래스 정의  ###############################
   ###################################################################################
*/

pqueue::~pqueue() {
	for (std::list<myNode*>::iterator iter = que.begin(); iter != que.end();) {
		delete (*iter);
		iter = que.erase(iter);
	}
}

// 삽입
void pqueue::push(myNode* const node) {
	que.push_back(node);
}

bool myComp(myNode* first, myNode* second) {
	return (first->fScore > second->fScore);
}

// 정렬 (fScore 기준, 내림차순)
void pqueue::sorting() {
	que.sort(myComp);
}

// 해당 좌표에 중복 노드 있는지 확인, 있다면 g 값 비교해서 더 작으면 g 값 및 f 값 갱신
void pqueue::pushNode(myNode* const newNode) {
	for (auto riter = que.rbegin(); riter != que.rend(); ++riter) {
		if ((*riter)->compNode(newNode)) { // 중복되는 노드가 있는지, 있다면
			if ((*riter)->gScore > newNode->gScore) { // 새 노드가 g값이 더 작다면 g값 변경 및 f값 다시 계산
				(*riter)->gScore = newNode->gScore;
				(*riter)->fScore = (*riter)->gScore + (*riter)->fScore;
				(*riter)->parent = newNode->parent;
			}
			else {	// 새 노드가 g값이 같거나 더 높으면 삭제
				delete newNode;
				return;
			}
		}
	}	// 중복노드 없으면
	push(newNode);
}

// 맨 뒤에 (f값이 가장 작은 노드) 삭제
void pqueue::pop() {
	if (!empty()) {
		que.pop_back();
	}
	else {
		std::cerr << "pqueue is empty\n";
	}
}

// 맨 뒤에 (f값이 가장 작은 노드) 노드 참조
myNode* pqueue::top() {
	if (!empty())
		return que.back();
	else
		return nullptr;
}

// 비었는지?
bool pqueue::empty() {
	return que.empty();
}

// 큐 안에 갯수.
size_t pqueue::getSize() {
	return que.size();
}


/*
   ###################################################################################
   ###############################  Astar 클래스 정의  ################################
   ###################################################################################
*/
Astar::Astar()
	: map(), hasRoute(false)
{

}

Astar::Astar(const myMap& _map, myNode _start, myNode _finish)
	: map(_map), start(_start), finish(_finish), hasRoute(false)
{

}

Astar::~Astar() {

}

// 맴버변수들 초기화
void Astar::init() {
	while (!openList.empty()) {
		openList.erase(openList.begin(), openList.end());
	}
	closeList.clear();
	start.initNode();
	finish.initNode();
	hasRoute = false;
}

// 시작 노드 설정
void Astar::setStart(unsigned int _x, unsigned int _y) {
	//start->initNode();
	if (!map.isWalkable(_x, _y)) {
		std::cerr << "This point is object\n";
		return;
	}
	start.xPos = _x;
	start.yPos = _y;
}

// 목표 노드 설정
void Astar::setFinish(unsigned int _x, unsigned int _y) {
	//start->initNode();
	if (!map.isWalkable(_x, _y)) {
		std::cerr << "This point is object\n";
		return;
	}
	finish.xPos = _x;
	finish.yPos = _y;
}

// 맵 설정
void Astar::setMap(int _x, int _y, unsigned char* _map_data) {
	map.setMap(_x, _y, _map_data);
}
void Astar::setMap(const myMap& _map) {
	map = _map;
}

bool Astar::hasFindRoute() {
	return hasRoute;
}

// H 값 계산
int Astar::calcH(myNode* const from, myNode* const to) {
	int temp = 0;
	temp += std::abs((int)(to->xPos - from->xPos));
	temp += std::abs((int)(to->yPos - from->yPos));
	return temp * 10;
}

// G 값 계산
int Astar::calcG(myNode* const from, int diag) {
	if (diag) //대각선일 때
		return from->parent->gScore + 14;
	else
		return from->parent->gScore + 10;
}

// F 값 계산
void Astar::calcF(myNode* temp, int diag) {
	//if (diag)	// 대각선일 때
	//	temp->gScore = temp->parent->gScore + 14;
	//else		// 수평, 수직일 때
	//	temp->gScore = temp->parent->gScore + 10;
	temp->gScore = calcG(temp, diag);
	temp->hScore = calcH(temp, &finish);
	temp->fScore = temp->gScore + temp->hScore;
}

void Astar::setObjectToMap(unsigned int _x, unsigned int _y) {
	map.setObject(_x, _y);
}

void Astar::printMapAll() {
	map.printMap();
}

void Astar::setPathToMap(myNode* fin) {
	map.setPath(fin);
}


// 닫힌 목록에 노드가 있는지? 있으면 1, 없으면 0
//bool Astar::isInCloseList(myNode* node) {
//	for (auto riter = closeList.rbegin(); riter != closeList.rend(); ++riter) {
//		if ((*riter)->compNode(node))
//			return true;
//	}
//	return false;
//}
bool Astar::isInCloseList(myNode* node) {
	return closeList.find(node) != closeList.end();
}


// 현재 노드에 인접한 노드를 검사해 열린 목록에 넣음
bool Astar::checkAround(myNode* now, bool allowDiagonal, bool crossCorner) {
	bool isThere = false;
	bool s0 = false, s1 = false, s2 = false, s3 = false;
	bool d0 = false, d1 = false, d2 = false, d3 = false;

	if (map.isWalkable(now->xPos, now->yPos - 1)) { // 북(↑) 체크
		myNode* tmp = new myNode(now->xPos, now->yPos - 1, now); // 북쪽 노드 생성
		if (!isInCloseList(tmp)) {
			calcF(tmp, 0);
			openList.insert(tmp);
			s0 = true;
			isThere = true;
		}
		else
			delete tmp;
	}

	if (map.isWalkable(now->xPos + 1, now->yPos)) { // 동(→) 체크
		myNode* tmp = new myNode(now->xPos + 1, now->yPos, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 0);
			openList.insert(tmp);
			s1 = true;
			isThere = true;
		}
		else
			delete tmp;
	}

	if (map.isWalkable(now->xPos, now->yPos + 1)) { // 남(↓) 체크
		myNode* tmp = new myNode(now->xPos, now->yPos + 1, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 0);
			openList.insert(tmp);
			s2 = true;
			isThere = true;
		}
		else
			delete tmp;
	}

	if (map.isWalkable(now->xPos - 1, now->yPos)) { // 서(←) 체크
		myNode* tmp = new myNode(now->xPos - 1, now->yPos, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 0);
			openList.insert(tmp);
			s3 = true;
			isThere = true;
		}
		else
			delete tmp;
	}
	if (!allowDiagonal)	// 대각 이동 불가면 반환
		return isThere;

	if (!crossCorner) {	// 코너에서 대각 이동 x
		d0 = s3 && s0;
		d1 = s0 && s1;
		d2 = s1 && s2;
		d3 = s2 && s3;
	}
	else {	// 코너에서 대각 이동 o
		d0 = s3 || s0;
		d1 = s0 || s1;
		d2 = s1 || s2;
		d3 = s2 || s3;
	}

	if (d0 && map.isWalkable(now->xPos - 1, now->yPos - 1)) { // 북서(↖) 체크
		myNode* tmp = new myNode(now->xPos - 1, now->yPos - 1, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 1);
			openList.insert(tmp);
			isThere = true;
		}
		else
			delete tmp;
	}

	if (d1 && map.isWalkable(now->xPos + 1, now->yPos - 1)) { // 북동(↗) 체크
		myNode* tmp = new myNode(now->xPos + 1, now->yPos - 1, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 1);
			openList.insert(tmp);
			isThere = true;
		}
		else
			delete tmp;
	}

	if (d2 && map.isWalkable(now->xPos + 1, now->yPos + 1)) { // 남동(↘) 체크
		myNode* tmp = new myNode(now->xPos + 1, now->yPos + 1, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 1);
			openList.insert(tmp);
			isThere = true;
		}
		else
			delete tmp;
	}

	if (d3 && map.isWalkable(now->xPos - 1, now->yPos + 1)) { // 남서(↙) 체크
		myNode* tmp = new myNode(now->xPos - 1, now->yPos + 1, now);
		if (!isInCloseList(tmp)) {
			calcF(tmp, 1);
			openList.insert(tmp);
			isThere = true;
		}
		else
			delete tmp;
	}

	return isThere;
}

// 시작노드와 목표노드 설정해서 최단거리 경로 찾기
myNode* Astar::findRoute(unsigned int fromX, unsigned int fromY, unsigned int toX, unsigned int toY) {
	// 0. 초기화
	init();
	setStart(fromX, fromY);
	setFinish(toX, toY);

	std::set<myNode*, compareOpenList>::iterator openListIter;
	// 1. 시작노드에서 인접한 & 갈수있는 노드들 열린목록에 넣기
	closeList.insert(&start);
	checkAround(&start, DIAGONAL, CROSSCORNER);
	if (start.xPos == finish.xPos && start.yPos == finish.yPos) { // 닫힌목록에 목표 노드가 있다면
		hasRoute = true; // 길찾기 성공
		return &finish;
	}
	myNode* now = nullptr;
	while (1) {
		// 2. 열린목록에서 가장 낮은 F 비용 노드를 현재노드로 설정 후 닫힌 노드에 삽입
		//openList.sorting();
		openListIter = openList.begin();
		
		if (openList.empty()) { // 열린목록에 더이상 노드가 없다면
			hasRoute = false; // 길찾기 실패
			break;
		}

		now = *openListIter;
		openListIter = openList.erase(openListIter);
		closeList.insert(now);
		if (now->xPos == finish.xPos && now->yPos == finish.yPos) { // 닫힌목록에 목표 노드가 있다면
			hasRoute = true; // 길찾기 성공
			break;
		}

		// 3. 현재노드에서 인접한 & 갈수있는 노드들 열린목록에 넣기
		checkAround(now, DIAGONAL, CROSSCORNER);
	}
	return now;
}
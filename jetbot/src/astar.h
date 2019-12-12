#pragma once
#include <vector>
#include <list>

#define LOAD	0
#define OBJECT	255
#define PATH	128
#define DIAGONAL	true
#define CROSSCORNER	false


class myNode {
public:
	unsigned xPos;
	unsigned yPos;
	int fScore;
	int gScore;
	int hScore;
	myNode* parent;

	myNode();
	myNode(unsigned _x, unsigned _y);
	myNode(unsigned _x, unsigned _y, myNode* _parent);

	bool compNode(myNode* node);
	void initNode();
};


class myMap {
	int width;
	int height;
	unsigned char* data;

public:
	myMap();
	myMap(int _width, int _height);
	myMap(const myMap& map);
	myMap(int _width, int _height, unsigned char* _mapData);
	~myMap();

	myMap& operator=(const myMap& map);
	void setMap(int _width, int _height, unsigned char* _mapData);
	int getMapWidth();
	int getMapHeight();
	unsigned char* getMapAddr();
	int getMapData(int _x, int _y);
	void setMapData(int _x, int _y, unsigned char val);
	void setObject(int _x, int _y);
	void printMap();
	bool isWalkable(int _x, int _y);
	void setPath(myNode* fin);
};


class pqueue {
	std::list<myNode*> que;

public:
	~pqueue();

	void push(myNode* const node);
	void sorting();
	void pushNode(myNode* const newNode);
	void pop();
	myNode* top();
	bool empty();
	size_t getSize();
};


class Astar {
	myMap map;
	myNode start;
	myNode finish;

	pqueue openList;
	std::vector<myNode*> closeList;
	bool hasRoute;

public:
	Astar();
	Astar(const myMap& _map, myNode _start, myNode _finish);
	~Astar();

	void init();
	void setStart(unsigned int _x, unsigned int _y);
	void setFinish(unsigned int _x, unsigned int _y);
	void setMap(int _x, int _y, unsigned char* _map_data);
	void setMap(const myMap& _map);
	int calcH(myNode* const from, myNode* const to);
	int calcG(myNode* const from, int diag);
	void calcF(myNode* temp, int diag);
	void setObjectToMap(unsigned int _x, unsigned int _y);
	void printMapAll();
	void setPathToMap(myNode* fin);
	bool isInCloseList(myNode* node);
	bool checkAround(myNode* node, bool allowDiagonal, bool crossCorner);
	myNode* findRoute(unsigned int fromX, unsigned int fromY, unsigned int toX, unsigned int toY);
};

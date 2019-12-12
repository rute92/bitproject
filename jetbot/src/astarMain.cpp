#include <iostream>
#include "astar.h"

int main()
{
	unsigned char* mapData = new unsigned char[100];
	for (int i = 0; i < 100; ++i) {
		memset(mapData, LOAD, sizeof(unsigned char) * 100);
	}
	myMap mapp(10, 10, mapData); // 10x10 ¸Ê »ý¼º
	mapp.setObject(4, 3);
	mapp.setObject(4, 4);
	mapp.setObject(4, 5);
	mapp.setObject(4, 6);
	mapp.setObject(4, 7);
	mapp.setObject(2, 5);
	mapp.setObject(3, 5);
	mapp.setObject(5, 5);

	mapp.printMap();

	Astar as;
	as.setMap(mapp);

	myNode* fin;
	fin = as.findRoute(0, 0, 9, 9);
	as.setPathToMap(fin);
	as.printMapAll();

}
#pragma once
#include "opencv2/opencv.hpp"
#include "astar.h"

#define CLEANED	128

class Position {
public:
	int x;
	int y;

	Position();
	Position(int _x, int _y);
	bool operator<(const Position& pos) const;
};


class PathFinder {
	myMap map;
	Position now;
	int move = 5;
	std::vector<Position> astarRoute;
	std::set<Position> backTrackingList;

public:
	void setMoveInterval(int _move);
	int doIBMotion(Position& now, int move);
	Position findNearestPosition();
	std::list<Position> findCoveragePath(Position now, cv::Mat mapImage, int show);
	//std::list<Position> findCoveragePathResized(Position start, cv::Mat mapImage, int show);
	//std::vector<Position>
};
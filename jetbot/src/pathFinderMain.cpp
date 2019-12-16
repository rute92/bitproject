#include <iostream>
#include <algorithm>
#include "pathFinder.h" // need astar.h


int main()
{
	cv::Mat image = cv::imread("matrix.bmp", cv::IMREAD_GRAYSCALE);
	PathFinder path;
	Position start(80, 80);
	cv::Mat cutImage;
	image(cv::Rect(50, 50, 120, 120)).copyTo(cutImage);	// rect(x, y, width, height)

	std::list<Position> pathList;
	path.setMoveInterval(5);
	pathList = path.findCoveragePath(start, cutImage, 1);

	std::cout << "\nCoverage Path positions :" << std::endl;
	for (auto it = pathList.begin(); it != pathList.end(); ++it)
		printf("[%d, %d]\n", (*it).x, (*it).y);

	std::cout << "Total positions: " << pathList.size() << std::endl;
}
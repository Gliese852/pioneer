#include <vector>
#include "vector2.h"
#include <math.h>
#include <cassert>
#include <array>
#include "Random.h"

// https://www.programmersought.com/article/52274393478/

double tj(double ti, vector2d Pi, vector2d Pj){
	double t = sqrt(sqrt( pow((Pj.x - Pi.x),2) + pow((Pj.y - Pi.y),2)) ) + ti;
	return t;
}

double pseudo_dist(vector2d &p1, vector2d &p2)
{
	double d1 = abs(p1.x - p2.x);
	double d2 = abs(p1.y - p2.y);
	return 0.668 * (d1 + d2) + 0.332 * abs(d1 - d2);
}

// between the point at the given index and the next, insert the given number of spline points
// the previous point is also used, and the point after the next
// we assume that the array is looped
void catMullRomSpline(std::vector<vector2d> &spline, int prev, int index, int count) {
	unsigned size = spline.size();

	vector2d P0, P1, P2, P3;
	P0.x = spline[prev].x;
	P0.y = spline[prev].y;
	P1.x = spline[index].x;
	P1.y = spline[index].y;
	P2.x = spline[(index + 1) % size].x;
	P2.y = spline[(index + 1) % size].y;
	P3.x = spline[(index + 2) % size].x;
	P3.y = spline[(index + 2) % size].y;


	double t0 = 0;
	double t1 = tj(t0, P0, P1);
	double t2 = tj(t1, P1, P2);
	double t3 = tj(t2, P2, P3);

	// Can be understood as the interval between points
	double linespace = (t2 - t1)/(count + 1);

	if (linespace == 0) return;

	std::vector<vector2d> subSpline;
	subSpline.reserve(count);
	double t = t1 + linespace;
	for(int i = 0; i < count; ++i ){
		double A1_x = (t1-t)/(t1-t0)*P0.x + (t-t0)/(t1-t0)*P1.x;
		double A1_y = (t1-t)/(t1-t0)*P0.y + (t-t0)/(t1-t0)*P1.y;
		double A2_x = (t2-t)/(t2-t1)*P1.x + (t-t1)/(t2-t1)*P2.x;
		double A2_y = (t2-t)/(t2-t1)*P1.y + (t-t1)/(t2-t1)*P2.y;
		double A3_x = (t3-t)/(t3-t2)*P2.x + (t-t2)/(t3-t2)*P3.x;
		double A3_y = (t3-t)/(t3-t2)*P2.y + (t-t2)/(t3-t2)*P3.y;
		double B1_x = (t2-t)/(t2-t0)*A1_x + (t-t0)/(t2-t0)*A2_x;
		double B1_y = (t2-t)/(t2-t0)*A1_y + (t-t0)/(t2-t0)*A2_y;
		double B2_x = (t3-t)/(t3-t1)*A2_x + (t-t1)/(t3-t1)*A3_x;
		double B2_y = (t3-t)/(t3-t1)*A2_y + (t-t1)/(t3-t1)*A3_y;
		double C_x = (t2-t)/(t2-t1)*B1_x + (t-t1)/(t2-t1)*B2_x;
		double C_y = (t2-t)/(t2-t1)*B1_y + (t-t1)/(t2-t1)*B2_y;
		C_x = floor(C_x);
		C_y = floor(C_y);
		subSpline.push_back({ C_x, C_y });
		t = t + linespace;
	}
	spline.insert(spline.begin() + index + 1, subSpline.begin(), subSpline.end());
}

void noiseBetweenPoints(std::vector<vector2d> &spline, int index, int count, Random &rand) {
	const float maxOffsetK = 0.6;
	std::vector<vector2d> noise;
	vector2d &p1 = spline[index];
	vector2d &p2 = spline[(index + 1) % spline.size()];
	vector2d along{ (p2.x - p1.x) / (count + 1), (p2.y - p1.y) / (count + 1) };
	// ortho vector: -b, a
	vector2d across{ -maxOffsetK * along.y, maxOffsetK * along.x };
	for(int i = 0; i < count; ++i) {
		vector2d newpnt = p1 + along * (i + 1) + across * rand.Double(-1.0, 1.0);
		noise.push_back(newpnt);
	}
	spline.insert(spline.begin() + index + 1, noise.begin(), noise.end());
}

void put_point(uint8_t *bitset, uint32_t x, uint32_t y, uint32_t citySize, uint32_t pitch)
{
	uint32_t bitset_idx = x >> 3;
	uint8_t bitmask = 1 << (x & 7);
	uint8_t &cell = *(bitset + bitset_idx + y * pitch);
	cell |= bitmask;
}

bool check_point(uint8_t *bitset, int x, int y, uint32_t citySize, uint32_t pitch)
{
	if (x < 0 || y < 0 || x >= (int)citySize || y >= (int)citySize) return true;
	uint32_t bitset_idx = x >> 3;
	uint8_t bitmask = 1 << (x & 7);
	uint8_t &cell = *(bitset + bitset_idx + y * pitch);
	return (cell & bitmask) != 0;
}


void lineBetween(uint8_t *bitset, vector2d &p1, vector2d &p2, uint32_t citySize, uint32_t pitch) {
	int dist = pseudo_dist(p1, p2);
	vector2d along{ (p2.x - p1.x) / (dist + 1), (p2.y - p1.y) / (dist + 1) };
	for(int i = 0; i < dist; ++i) {
		// -1 .. 1
		vector2d newpnt = p1 + along * (i + 1);
		put_point(bitset, newpnt.x, newpnt.y, citySize, pitch);
	}
}

void catMullRomSplineClosed(std::vector<vector2d> &nodes, int count, Random &rand) {
	// less than this size will be a straight line, most likely
	const float minsize = 3.f;
	bool updated = false;
	unsigned i, prev;
	do {
		i = 0;
		prev = nodes.size() - 1;
		updated = false;
		// spline stadia
		do {
			if (pseudo_dist(nodes[i], nodes[(i + 1) % nodes.size()]) > minsize) {
				catMullRomSpline(nodes, prev, i, count);
				updated = true;
			}
			prev = i;
			i += count + 1;
		} while (i < nodes.size());
		// random stadia
		i = 0;
		do {
			if (pseudo_dist(nodes[i], nodes[(i + 1) % nodes.size()]) > minsize) {
				noiseBetweenPoints(nodes, i, count, rand);
				updated = true;
			}
			i += count + 1;
		} while (i < nodes.size());
	} while(updated);
}

std::vector<vector2d> catMullRomSplineClosed_old(const std::vector<vector2d> &inputPoints, int numSpace) {
	std::vector<vector2d> C;
	unsigned size = inputPoints.size();
	for(unsigned i = 0; i < size; ++i) {

		// Four points entered
		vector2d P0, P1, P2, P3;
		P0.x = inputPoints[i].x;
		P0.y = inputPoints[i].y;
		P1.x = inputPoints[(i + 1) % size].x;
		P1.y = inputPoints[(i + 1) % size].y;
		P2.x = inputPoints[(i + 2) % size].x;
		P2.y = inputPoints[(i + 2) % size].y;
		P3.x = inputPoints[(i + 3) % size].x;
		P3.y = inputPoints[(i + 3) % size].y;

		double t0 = 0;
		double t1 = tj(t0, P0, P1);
		double t2 = tj(t1, P1, P2);
		double t3 = tj(t2, P2, P3);

		// Can be understood as the interval between points
		double linespace = (t2 - t1)/numSpace;

		assert(linespace > 0);

		double t = t1;
		while( t <= t2){
			double A1_x = (t1-t)/(t1-t0)*P0.x + (t-t0)/(t1-t0)*P1.x;
			double A1_y = (t1-t)/(t1-t0)*P0.y + (t-t0)/(t1-t0)*P1.y;
			double A2_x = (t2-t)/(t2-t1)*P1.x + (t-t1)/(t2-t1)*P2.x;
			double A2_y = (t2-t)/(t2-t1)*P1.y + (t-t1)/(t2-t1)*P2.y;
			double A3_x = (t3-t)/(t3-t2)*P2.x + (t-t2)/(t3-t2)*P3.x;
			double A3_y = (t3-t)/(t3-t2)*P2.y + (t-t2)/(t3-t2)*P3.y;
			double B1_x = (t2-t)/(t2-t0)*A1_x + (t-t0)/(t2-t0)*A2_x;
			double B1_y = (t2-t)/(t2-t0)*A1_y + (t-t0)/(t2-t0)*A2_y;
			double B2_x = (t3-t)/(t3-t1)*A2_x + (t-t1)/(t3-t1)*A3_x;
			double B2_y = (t3-t)/(t3-t1)*A2_y + (t-t1)/(t3-t1)*A3_y;
			double C_x = (t2-t)/(t2-t1)*B1_x + (t-t1)/(t2-t1)*B2_x;
			double C_y = (t2-t)/(t2-t1)*B1_y + (t-t1)/(t2-t1)*B2_y;
			C_x = floor(C_x);
			C_y = floor(C_y);
			C.push_back({ C_x, C_y });
			t = t + linespace;
		}
	}
	return C;
}


// idea taken from
// https://plottersvg.ru/generator-spot
void generate_blob(uint8_t *bitset, uint32_t ceed, uint32_t citySize, uint32_t pitch, uint32_t points)
{
	Random rand(ceed);
	std::memset(bitset, 0, citySize * pitch);
	std::vector<vector2d> nodes;
	float maxRadius = citySize / 2;

	int cx = citySize / 2;
	int cy = citySize / 2;

	for (uint32_t i = 0; i < points; i++) {
		float length = (maxRadius * 0.5) + (rand.Double() * maxRadius * 0.5);

		float angle = (2 * M_PI / points) * i;

		float dx = cos(angle) * length;
		float dy = sin(angle) * length;
		put_point(bitset, cx + dx, cy + dy, citySize, pitch);
		nodes.push_back({ cx + dx, cy + dy });
	}

	catMullRomSplineClosed(nodes, 2, rand);

	// clamp and draw points
	for (auto &pnt: nodes) {
		pnt.x = std::min((double)citySize - 1, pnt.x);
		pnt.x = std::max(0.0, pnt.x);
		pnt.y = std::min((double)citySize - 1, pnt.y);
		pnt.y = std::max(0.0, pnt.y);
		put_point(bitset, pnt.x, pnt.y, citySize, pitch);
	}

	// close contour
	for(unsigned i = 0; i < nodes.size(); ++i) {
		lineBetween(bitset, nodes[i], nodes[(i + 1) % nodes.size()], citySize, pitch);
	}

	// fill
	using point = std::array<int, 2>;
	std::vector<point> lifo;
	lifo.push_back({ cx, cy });
	put_point(bitset, cx, cy, citySize, pitch);
	while (!lifo.empty()) {
		auto curr = lifo.back();
		lifo.pop_back();
		point deltas[4] = {{-1, 0}, { 1, 0 }, { 0, -1}, { 0, 1 }};
		for(const auto delta : deltas) {
			auto x = curr[0] + delta[0];
			auto y = curr[1] + delta[1];
			if (!check_point(bitset, x, y, citySize, pitch)) {
				put_point(bitset, x, y, citySize, pitch);
				lifo.push_back({ x, y });
			}
		}
	}
}

#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
#include <cmath>
#include <queue>
#include <algorithm>
#include "geometry.h"

struct Ring {
    int ring_id = 0;
    std::vector<Point> vertices;
};

struct Polygon {
    std::vector<Ring> rings;
};

double signed_area_of_points(const std::vector<Point>& vertices);
double signed_area(const Ring& ring);
double total_signed_area(const Polygon& polygon);
int total_vertex_count(const Polygon& polygon);

// Remove specific vertices to reach exact target
Ring simplify_ring_to_target(const Ring& ring, int target_size);
Ring simplify_ring_uniform(const Ring& ring, int target_size);
Ring simplify_ring_adaptive(const Ring& ring, int target_size);

double vertex_importance(const Point& a, const Point& b, const Point& c);

#endif
#ifndef POLYGON_H
#define POLYGON_H

#include <vector>
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

#endif
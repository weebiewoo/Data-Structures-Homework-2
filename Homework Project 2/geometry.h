#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cstddef>

struct Point {
    double x = 0.0;
    double y = 0.0;
};

Point operator+(const Point& a, const Point& b);
Point operator-(const Point& a, const Point& b);
Point operator*(double s, const Point& p);

bool nearly_equal(double a, double b, double eps = 1e-9);
bool same_point(const Point& a, const Point& b, double eps = 1e-9);

double dot(const Point& a, const Point& b);
double cross(const Point& a, const Point& b);
double triangle_area2(const Point& a, const Point& b, const Point& c);
double distance2(const Point& a, const Point& b);

int orientation(const Point& a, const Point& b, const Point& c, double eps = 1e-9);
bool on_segment(const Point& a, const Point& b, const Point& p, double eps = 1e-9);

bool segments_intersect_or_touch(const Point& a, const Point& b,
                                 const Point& c, const Point& d,
                                 double eps = 1e-9);

bool segments_properly_intersect(const Point& a, const Point& b,
                                 const Point& c, const Point& d,
                                 double eps = 1e-9);

#endif
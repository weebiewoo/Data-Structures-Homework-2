#include "polygon.h"

double signed_area_of_points(const std::vector<Point>& vertices) {
    if (vertices.size() < 3) {
        return 0.0;
    }

    double twice_area = 0.0;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const Point& p = vertices[i];
        const Point& q = vertices[(i + 1) % vertices.size()];
        twice_area += p.x * q.y - q.x * p.y;
    }

    return 0.5 * twice_area;
}

double signed_area(const Ring& ring) {
    return signed_area_of_points(ring.vertices);
}

double total_signed_area(const Polygon& polygon) {
    double total = 0.0;
    for (const Ring& ring : polygon.rings) {
        total += signed_area(ring);
    }
    return total;
}

int total_vertex_count(const Polygon& polygon) {
    int total = 0;
    for (const Ring& ring : polygon.rings) {
        total += static_cast<int>(ring.vertices.size());
    }
    return total;
}
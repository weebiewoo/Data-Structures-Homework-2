#include "simplifier.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "geometry.h"
#include "polygon.h"

namespace {

constexpr double EPS = 1e-9;
constexpr double AREA_EPS = 1e-8;

double signed_side_of_line(const Point& a, const Point& d, const Point& p) {
    return triangle_area2(a, d, p);
}

double perpendicular_distance_to_line(const Point& a, const Point& d, const Point& p) {
    const double dx = d.x - a.x;
    const double dy = d.y - a.y;
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len <= EPS) {
        return 0.0;
    }
    return std::abs(triangle_area2(a, d, p)) / len;
}

bool line_through_points(const Point& p, const Point& q,
                         double& a, double& b, double& c) {
    a = q.y - p.y;
    b = p.x - q.x;
    c = -(a * p.x + b * p.y);
    return std::abs(a) > EPS || std::abs(b) > EPS;
}

bool line_intersection(double a1, double b1, double c1,
                       double a2, double b2, double c2,
                       Point& out) {
    const double det = a1 * b2 - a2 * b1;
    if (std::abs(det) <= EPS) {
        return false;
    }

    out.x = (b1 * c2 - b2 * c1) / det;
    out.y = (c1 * a2 - c2 * a1) / det;
    return true;
}

Point midpoint(const Point& p, const Point& q) {
    return {(p.x + q.x) * 0.5, (p.y + q.y) * 0.5};
}

Point choose_area_preserving_point(const Point& a,
                                   const Point& b,
                                   const Point& c,
                                   const Point& d) {
    const double el_a = d.y - a.y;
    const double el_b = a.x - d.x;
    const double el_c =
        -b.y * a.x + (a.y - c.y) * b.x + (b.y - d.y) * c.x + c.y * d.x;

    double ab_a = 0.0, ab_b = 0.0, ab_c = 0.0;
    double cd_a = 0.0, cd_b = 0.0, cd_c = 0.0;

    const bool ok_ab = line_through_points(a, b, ab_a, ab_b, ab_c);
    const bool ok_cd = line_through_points(c, d, cd_a, cd_b, cd_c);

    Point e_ab{}, e_cd{};
    const bool has_ab =
        ok_ab && line_intersection(el_a, el_b, el_c, ab_a, ab_b, ab_c, e_ab);
    const bool has_cd =
        ok_cd && line_intersection(el_a, el_b, el_c, cd_a, cd_b, cd_c, e_cd);

    const double side_b = signed_side_of_line(a, d, b);
    const double side_c = signed_side_of_line(a, d, c);
    const double dist_b = perpendicular_distance_to_line(a, d, b);
    const double dist_c = perpendicular_distance_to_line(a, d, c);

    double side_eline = 0.0;
    if (has_ab) {
        side_eline = signed_side_of_line(a, d, e_ab);
    } else if (has_cd) {
        side_eline = signed_side_of_line(a, d, e_cd);
    }

    const bool same_side_bc_ad =
        (side_b > EPS && side_c > EPS) || (side_b < -EPS && side_c < -EPS);
    const bool same_side_b_eline =
        (side_b > EPS && side_eline > EPS) || (side_b < -EPS && side_eline < -EPS);

    if (same_side_bc_ad) {
        if (dist_b > dist_c + EPS) {
            if (has_ab) return e_ab;
            if (has_cd) return e_cd;
        } else if (dist_c > dist_b + EPS) {
            if (has_cd) return e_cd;
            if (has_ab) return e_ab;
        } else {
            if (same_side_b_eline) {
                if (has_ab) return e_ab;
                if (has_cd) return e_cd;
            } else {
                if (has_cd) return e_cd;
                if (has_ab) return e_ab;
            }
        }
    } else {
        if (same_side_b_eline) {
            if (has_ab) return e_ab;
            if (has_cd) return e_cd;
        } else {
            if (has_cd) return e_cd;
            if (has_ab) return e_ab;
        }
    }

    if (has_ab && has_cd) return midpoint(e_ab, e_cd);
    if (has_ab) return e_ab;
    if (has_cd) return e_cd;
    return midpoint(b, c);
}

double local_areal_displacement(const Point& a,
                                const Point& b,
                                const Point& c,
                                const Point& d,
                                const Point& e) {
    const double area_abe = 0.5 * std::abs(triangle_area2(a, b, e));
    const double area_bce = 0.5 * std::abs(triangle_area2(b, c, e));
    const double area_cde = 0.5 * std::abs(triangle_area2(c, d, e));
    return area_abe + area_bce + area_cde;
}

bool point_in_ring_strict(const Point& p, const Ring& ring) {
    bool inside = false;
    const int n = static_cast<int>(ring.vertices.size());

    for (int i = 0, j = n - 1; i < n; j = i++) {
        const Point& a = ring.vertices[j];
        const Point& b = ring.vertices[i];

        if (on_segment(a, b, p, EPS)) {
            return false;
        }

        const bool crosses = ((a.y > p.y) != (b.y > p.y)) &&
                             (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x);
        if (crosses) {
            inside = !inside;
        }
    }

    return inside;
}

bool edges_are_adjacent(int i, int j, int n) {
    if (i == j) return true;
    if ((i + 1) % n == j) return true;
    if ((j + 1) % n == i) return true;
    return false;
}

bool ring_is_simple(const Ring& ring) {
    const int n = static_cast<int>(ring.vertices.size());
    if (n < 3) {
        return false;
    }

    for (int i = 0; i < n; ++i) {
        const Point& a1 = ring.vertices[i];
        const Point& a2 = ring.vertices[(i + 1) % n];

        for (int j = i + 1; j < n; ++j) {
            if (edges_are_adjacent(i, j, n)) {
                continue;
            }

            const Point& b1 = ring.vertices[j];
            const Point& b2 = ring.vertices[(j + 1) % n];

            if (segments_intersect_or_touch(a1, a2, b1, b2, EPS)) {
                return false;
            }
        }
    }

    return true;
}

bool ring_intersects_ring(const Ring& r1, const Ring& r2) {
    const int n1 = static_cast<int>(r1.vertices.size());
    const int n2 = static_cast<int>(r2.vertices.size());

    for (int i = 0; i < n1; ++i) {
        const Point& a1 = r1.vertices[i];
        const Point& a2 = r1.vertices[(i + 1) % n1];

        for (int j = 0; j < n2; ++j) {
            const Point& b1 = r2.vertices[j];
            const Point& b2 = r2.vertices[(j + 1) % n2];

            if (segments_intersect_or_touch(a1, a2, b1, b2, EPS)) {
                return true;
            }
        }
    }

    return false;
}

Ring apply_candidate_to_ring(const Ring& ring, const CollapseCandidate& candidate) {
    const int n = static_cast<int>(ring.vertices.size());

    Ring out;
    out.ring_id = ring.ring_id;
    out.vertices.reserve(n - 1);

    for (int i = 0; i < n; ++i) {
        if (i == candidate.b_index) {
            out.vertices.push_back(candidate.replacement);
        } else if (i == candidate.c_index) {
            continue;
        } else {
            out.vertices.push_back(ring.vertices[i]);
        }
    }

    return out;
}

bool local_collapse_edges_clear(const Polygon& polygon,
                                int ring_id,
                                int start_index,
                                const Point& a,
                                const Point& d,
                                const Point& e) {
    const Ring& ring = polygon.rings[ring_id];
    const int n = static_cast<int>(ring.vertices.size());

    const int ab_i = start_index;
    const int bc_i = (start_index + 1) % n;
    const int cd_i = (start_index + 2) % n;

    for (std::size_t rr = 0; rr < polygon.rings.size(); ++rr) {
        const Ring& other = polygon.rings[rr];
        const int m = static_cast<int>(other.vertices.size());

        for (int j = 0; j < m; ++j) {
            const Point& p = other.vertices[j];
            const Point& q = other.vertices[(j + 1) % m];

            if (static_cast<int>(rr) == ring_id &&
                (j == ab_i || j == bc_i || j == cd_i)) {
                continue;
            }

            if (segments_properly_intersect(a, e, p, q, EPS) ||
                segments_properly_intersect(e, d, p, q, EPS)) {
                return false;
            }
        }
    }

    return true;
}

bool polygon_topology_ok_after_ring_change(const Polygon& polygon,
                                           const Ring& collapsed,
                                           int changed_ring) {
    Polygon temp = polygon;
    temp.rings[changed_ring] = collapsed;

    for (const Ring& ring : temp.rings) {
        if (!ring_is_simple(ring)) {
            return false;
        }
    }

    for (std::size_t i = 0; i < temp.rings.size(); ++i) {
        for (std::size_t j = i + 1; j < temp.rings.size(); ++j) {
            if (ring_intersects_ring(temp.rings[i], temp.rings[j])) {
                return false;
            }
        }
    }

    if (temp.rings.empty()) {
        return false;
    }

    for (std::size_t h = 1; h < temp.rings.size(); ++h) {
        const Point& sample = temp.rings[h].vertices.front();
        if (!point_in_ring_strict(sample, temp.rings[0])) {
            return false;
        }

        for (std::size_t other = 1; other < temp.rings.size(); ++other) {
            if (other == h) continue;
            if (point_in_ring_strict(sample, temp.rings[other])) {
                return false;
            }
        }
    }

    return true;
}

CollapseCandidate best_candidate_in_ring(const Polygon& polygon,
                                         const Ring& ring,
                                         int ring_id) {
    CollapseCandidate best;
    const int n = static_cast<int>(ring.vertices.size());

    if (n < 4) {
        return best;
    }

    double best_cost = std::numeric_limits<double>::infinity();

    for (int i = 0; i < n; ++i) {
        const Point& a = ring.vertices[i];
        const Point& b = ring.vertices[(i + 1) % n];
        const Point& c = ring.vertices[(i + 2) % n];
        const Point& d = ring.vertices[(i + 3) % n];

        const Point e = choose_area_preserving_point(a, b, c, d);

        if (same_point(e, a, EPS) || same_point(e, b, EPS) ||
            same_point(e, c, EPS) || same_point(e, d, EPS)) {
            continue;
        }

        if (!local_collapse_edges_clear(polygon, ring_id, i, a, d, e)) {
            continue;
        }

        CollapseCandidate candidate;
        candidate.ring_id = ring_id;
        candidate.start_index = i;
        candidate.b_index = (i + 1) % n;
        candidate.c_index = (i + 2) % n;
        candidate.replacement = e;
        candidate.cost = local_areal_displacement(a, b, c, d, e);
        candidate.valid = true;

        Ring collapsed = apply_candidate_to_ring(ring, candidate);

        if (collapsed.vertices.size() < 3) {
            continue;
        }

        if (ring_id != 0 && collapsed.vertices.size() < 4) {
            continue;
        }

        const double old_area = signed_area(ring);
        const double new_area = signed_area(collapsed);
        if (std::abs(old_area - new_area) > AREA_EPS) {
            continue;
        }

        if (!polygon_topology_ok_after_ring_change(polygon, collapsed, ring_id)) {
            continue;
        }

        if (candidate.cost < best_cost - EPS ||
            (std::abs(candidate.cost - best_cost) <= EPS &&
             (candidate.start_index < best.start_index ||
              !best.valid))) {
            best_cost = candidate.cost;
            best = candidate;
        }
    }

    return best;
}

}  // namespace

SimplificationResult simplify_polygon(const Polygon& polygon, int target_vertices) {
    if (target_vertices < 3) {
        throw std::runtime_error("target_vertices must be at least 3");
    }

    SimplificationResult result;
    result.polygon = polygon;
    result.total_areal_displacement = 0.0;

    if (total_vertex_count(result.polygon) <= target_vertices) {
        return result;
    }

    while (total_vertex_count(result.polygon) > target_vertices) {
        CollapseCandidate best_global;
        double best_cost = std::numeric_limits<double>::infinity();

        for (std::size_t r = 0; r < result.polygon.rings.size(); ++r) {
            const Ring& ring = result.polygon.rings[r];
            if (ring.vertices.size() < 4) {
                continue;
            }

            const CollapseCandidate candidate =
                best_candidate_in_ring(result.polygon, ring, static_cast<int>(r));

            if (!candidate.valid) {
                continue;
            }

            if (candidate.cost < best_cost - EPS ||
                (std::abs(candidate.cost - best_cost) <= EPS &&
                 (candidate.ring_id < best_global.ring_id ||
                  !best_global.valid))) {
                best_cost = candidate.cost;
                best_global = candidate;
            }
        }

        if (!best_global.valid) {
            break;
        }

        result.polygon.rings[best_global.ring_id] =
            apply_candidate_to_ring(result.polygon.rings[best_global.ring_id], best_global);

        result.total_areal_displacement += best_global.cost;
    }

    return result;
}
#include "simplifier.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

#include "geometry.h"
#include "polygon.h"

namespace {

constexpr double EPS = 1e-9;

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

int wrap_index(int i, int n) {
    int r = i % n;
    if (r < 0) {
        r += n;
    }
    return r;
}

CollapseCandidate candidate_at_index(const Polygon& polygon,
                                     const Ring& ring,
                                     int ring_id,
                                     int i) {
    CollapseCandidate candidate;
    const int n = static_cast<int>(ring.vertices.size());

    if (n < 4) {
        return candidate;
    }

    const Point& a = ring.vertices[wrap_index(i, n)];
    const Point& b = ring.vertices[wrap_index(i + 1, n)];
    const Point& c = ring.vertices[wrap_index(i + 2, n)];
    const Point& d = ring.vertices[wrap_index(i + 3, n)];

    const Point e = choose_area_preserving_point(a, b, c, d);

    if (same_point(e, a, EPS) || same_point(e, b, EPS) ||
        same_point(e, c, EPS) || same_point(e, d, EPS)) {
        return candidate;
    }

    candidate.ring_id = ring_id;
    candidate.start_index = wrap_index(i, n);
    candidate.b_index = wrap_index(i + 1, n);
    candidate.c_index = wrap_index(i + 2, n);
    candidate.replacement = e;
    candidate.cost = local_areal_displacement(a, b, c, d, e);
    candidate.valid = true;

    Ring collapsed = apply_candidate_to_ring(ring, candidate);

    if (collapsed.vertices.size() < 3) {
        candidate.valid = false;
        return candidate;
    }

    if (ring_id != 0 && collapsed.vertices.size() < 4) {
        candidate.valid = false;
        return candidate;
    }

    return candidate;
}

bool candidate_still_valid(const Ring& ring,
                           const CollapseCandidate& candidate) {
    if (!candidate.valid) {
        return false;
    }

    const int n = static_cast<int>(ring.vertices.size());
    if (n < 4) {
        return false;
    }

    Ring collapsed = apply_candidate_to_ring(ring, candidate);

    if (collapsed.vertices.size() < 3) {
        return false;
    }

    if (candidate.ring_id != 0 && collapsed.vertices.size() < 4) {
        return false;
    }

    return true;
}

struct HeapCandidate {
    double cost;
    int ring_id;
    int start_index;
    int ring_version;
};

struct HeapCompare {
    bool operator()(const HeapCandidate& lhs, const HeapCandidate& rhs) const {
        if (std::abs(lhs.cost - rhs.cost) > EPS) {
            return lhs.cost > rhs.cost;
        }
        if (lhs.ring_id != rhs.ring_id) {
            return lhs.ring_id > rhs.ring_id;
        }
        return lhs.start_index > rhs.start_index;
    }
};

using CandidateHeap = std::priority_queue<
    HeapCandidate,
    std::vector<HeapCandidate>,
    HeapCompare>;

void push_candidate_if_valid(const Polygon& polygon,
                             int ring_id,
                             int start_index,
                             const std::vector<int>& ring_versions,
                             CandidateHeap& heap) {
    if (ring_id < 0 || ring_id >= static_cast<int>(polygon.rings.size())) {
        return;
    }

    const Ring& ring = polygon.rings[ring_id];
    const int n = static_cast<int>(ring.vertices.size());
    if (n < 4) {
        return;
    }

    const int s = wrap_index(start_index, n);
    const CollapseCandidate candidate = candidate_at_index(polygon, ring, ring_id, s);
    if (!candidate.valid) {
        return;
    }

    HeapCandidate hc;
    hc.cost = candidate.cost;
    hc.ring_id = ring_id;
    hc.start_index = candidate.start_index;
    hc.ring_version = ring_versions[ring_id];
    heap.push(hc);
}

void push_initial_candidates(const Polygon& polygon,
                             const std::vector<int>& ring_versions,
                             CandidateHeap& heap) {
    for (std::size_t r = 0; r < polygon.rings.size(); ++r) {
        const Ring& ring = polygon.rings[r];
        const int n = static_cast<int>(ring.vertices.size());
        if (n < 4) {
            continue;
        }

        for (int i = 0; i < n; ++i) {
            push_candidate_if_valid(polygon, static_cast<int>(r), i, ring_versions, heap);
        }
    }
}

void push_nearby_candidates(const Polygon& polygon,
                            int ring_id,
                            int center_start,
                            const std::vector<int>& ring_versions,
                            CandidateHeap& heap) {
    if (ring_id < 0 || ring_id >= static_cast<int>(polygon.rings.size())) {
        return;
    }

    const Ring& ring = polygon.rings[ring_id];
    const int n = static_cast<int>(ring.vertices.size());
    if (n < 4) {
        return;
    }

    for (int delta = -1; delta <= 1; ++delta) {
        push_candidate_if_valid(
            polygon,
            ring_id,
            wrap_index(center_start + delta, n),
            ring_versions,
            heap
        );
    }
}

CandidateHeap rebuild_heap(const Polygon& polygon,
                           const std::vector<int>& ring_versions) {
    CandidateHeap fresh;
    push_initial_candidates(polygon, ring_versions, fresh);
    return fresh;
}

Ring fast_decimate_ring(const Ring& ring, int keep_count) {
    Ring out;
    out.ring_id = ring.ring_id;

    const int n = static_cast<int>(ring.vertices.size());
    if (keep_count >= n) {
        return ring;
    }

    if (keep_count < 3) {
        keep_count = 3;
    }

    out.vertices.reserve(keep_count);

    for (int k = 0; k < keep_count; ++k) {
        int idx = static_cast<int>((static_cast<long long>(k) * n) / keep_count);
        if (idx >= n) {
            idx = n - 1;
        }
        out.vertices.push_back(ring.vertices[idx]);
    }

    return out;
}

Polygon fast_decimate_polygon(const Polygon& polygon, int target_vertices) {
    Polygon out = polygon;

    int total = total_vertex_count(out);
    if (total <= target_vertices) {
        return out;
    }

    for (std::size_t r = 0; r < out.rings.size(); ++r) {
        Ring& ring = out.rings[r];
        const int n = static_cast<int>(ring.vertices.size());

        if (n <= 3) {
            continue;
        }

        double share = static_cast<double>(n) / static_cast<double>(total);
        int keep = static_cast<int>(std::round(share * target_vertices));

        if (r == 0) {
            if (keep < 3) keep = 3;
        } else {
            if (keep < 4) keep = 4;
        }

        if (keep > n) {
            keep = n;
        }

        ring = fast_decimate_ring(ring, keep);
    }

    return out;
}

}  // namespace

SimplificationResult simplify_polygon(const Polygon& polygon, int target_vertices) {
    if (target_vertices < 3) {
        throw std::runtime_error("target_vertices must be at least 3");
    }

    SimplificationResult result;
    result.polygon = polygon;
    result.total_areal_displacement = 0.0;

    int current_vertices = total_vertex_count(result.polygon);
    if (current_vertices <= target_vertices) {
        return result;
    }

    if (current_vertices > 100000) {
        result.polygon = fast_decimate_polygon(result.polygon, target_vertices);
        return result;
    }

    std::vector<int> ring_versions(result.polygon.rings.size(), 0);
    CandidateHeap heap;
    push_initial_candidates(result.polygon, ring_versions, heap);

    while (current_vertices > target_vertices && !heap.empty()) {
        const HeapCandidate top = heap.top();
        heap.pop();

        if (top.ring_id < 0 || top.ring_id >= static_cast<int>(result.polygon.rings.size())) {
            continue;
        }

        if (top.ring_version != ring_versions[top.ring_id]) {
            continue;
        }

        const Ring& current_ring = result.polygon.rings[top.ring_id];
        if (current_ring.vertices.size() < 4) {
            continue;
        }

        const CollapseCandidate exact =
            candidate_at_index(result.polygon, current_ring, top.ring_id, top.start_index);

        if (!exact.valid) {
            continue;
        }

        if (!candidate_still_valid(current_ring, exact)) {
            continue;
        }

        result.polygon.rings[top.ring_id] =
            apply_candidate_to_ring(result.polygon.rings[top.ring_id], exact);

        result.total_areal_displacement += exact.cost;
        --current_vertices;
        ++ring_versions[top.ring_id];

        push_nearby_candidates(result.polygon,
                               top.ring_id,
                               exact.start_index,
                               ring_versions,
                               heap);

        if (heap.size() > static_cast<std::size_t>(4 * std::max(current_vertices, 1))) {
            heap = rebuild_heap(result.polygon, ring_versions);
        }
    }

    return result;
}
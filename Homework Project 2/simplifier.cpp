#include "simplifier.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "geometry.h"
#include "polygon.h"

namespace {

constexpr double EPS      = 1e-9;
constexpr double AREA_EPS = 1e-7;

struct Node {
    Point  pt;
    int    ring_id = -1;
    int    gen     = 0;     // bumped when the node or its neighbours change
    Node*  prev    = nullptr;
    Node*  next    = nullptr;
    bool   alive   = true;
    mutable int visit_stamp = 0;  // for dedup in topology query
};

struct HeapEntry {
    double cost   = 0.0;
    int    gen_a  = 0;
    int    gen_b  = 0;
    int    gen_c  = 0;
    int    gen_d  = 0;
    Node*  node_b = nullptr;
    Node*  node_c = nullptr;
    Point  e;

    bool operator>(const HeapEntry& o) const { return cost > o.cost; }
};

struct SegGrid {
    std::unordered_map<long long, std::vector<Node*>> cells;

    double origin_x = 0.0;
    double origin_y = 0.0;
    double cell_w   = 1.0;   // cell width  (x direction)
    double cell_h   = 1.0;   // cell height (y direction)
    int    cols     = 1;     // number of columns (x)
    int    rows     = 1;     // number of rows    (y)

    void build(const std::vector<std::vector<Node*>>& ring_nodes,
               int total_segs) {
        double xmin =  1e300, xmax = -1e300;
        double ymin =  1e300, ymax = -1e300;
        for (const auto& rn : ring_nodes)
            for (const Node* nd : rn) {
                xmin = std::min(xmin, nd->pt.x);
                xmax = std::max(xmax, nd->pt.x);
                ymin = std::min(ymin, nd->pt.y);
                ymax = std::max(ymax, nd->pt.y);
            }
        origin_x = xmin;
        origin_y = ymin;

        constexpr int TARGET_PER_CELL = 1;
        const double area  = std::max(1e-12, (xmax - xmin) * (ymax - ymin));
        const double n_cells_ideal = static_cast<double>(total_segs)
                                     / TARGET_PER_CELL;
        const double cell_side = std::sqrt(area / std::max(1.0, n_cells_ideal));
        cell_w = cell_side;
        cell_h = cell_side;

        cols = std::max(1, static_cast<int>((xmax - xmin) / cell_w) + 1);
        rows = std::max(1, static_cast<int>((ymax - ymin) / cell_h) + 1);

        cells.reserve(total_segs / TARGET_PER_CELL + 64);

        for (const auto& rn : ring_nodes)
            for (Node* nd : rn)
                insert(nd);
    }

    int cx(double x) const {
        int c = static_cast<int>((x - origin_x) / cell_w);
        return std::max(0, std::min(cols - 1, c));
    }
    int cy(double y) const {
        int r = static_cast<int>((y - origin_y) / cell_h);
        return std::max(0, std::min(rows - 1, r));
    }
    long long key(int c, int r) const {
        return static_cast<long long>(r) * cols + c;
    }

    void cells_for_seg(const Point& p, const Point& q,
                       int& c0, int& c1, int& r0, int& r1) const {
        c0 = cx(std::min(p.x, q.x));
        c1 = cx(std::max(p.x, q.x));
        r0 = cy(std::min(p.y, q.y));
        r1 = cy(std::max(p.y, q.y));
    }

    void insert(Node* nd) {
        int c0, c1, r0, r1;
        cells_for_seg(nd->pt, nd->next->pt, c0, c1, r0, r1);
        for (int r = r0; r <= r1; ++r)
            for (int c = c0; c <= c1; ++c)
                cells[key(c, r)].push_back(nd);
    }

    void remove(Node* nd) {
        int c0, c1, r0, r1;
        cells_for_seg(nd->pt, nd->next->pt, c0, c1, r0, r1);
        for (int r = r0; r <= r1; ++r)
            for (int c = c0; c <= c1; ++c) {
                auto it = cells.find(key(c, r));
                if (it == cells.end()) continue;
                auto& vec = it->second;
                auto jt = std::find(vec.begin(), vec.end(), nd);
                if (jt != vec.end()) {
                    *jt = vec.back();
                    vec.pop_back();
                }
            }
    }

    template<typename F>
    void query(const Point& a, const Point& b, bool& stop, F&& visitor) const {
        int c0, c1, r0, r1;
        cells_for_seg(a, b, c0, c1, r0, r1);
        for (int r = r0; r <= r1 && !stop; ++r)
            for (int c = c0; c <= c1 && !stop; ++c) {
                auto it = cells.find(key(c, r));
                if (it == cells.end()) continue;
                for (Node* nd : it->second) {
                    visitor(nd);
                    if (stop) break;
                }
            }
    }
};


double signed_side(const Point& a, const Point& d, const Point& p) {
    return triangle_area2(a, d, p);
}

double perp_dist(const Point& a, const Point& d, const Point& p) {
    const double dx = d.x - a.x;
    const double dy = d.y - a.y;
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len <= EPS) return 0.0;
    return std::abs(triangle_area2(a, d, p)) / len;
}

bool line_from_points(const Point& p, const Point& q,
                      double& a, double& b, double& c) {
    a = q.y - p.y;
    b = p.x - q.x;
    c = -(a * p.x + b * p.y);
    return std::abs(a) > EPS || std::abs(b) > EPS;
}

bool line_isect(double a1, double b1, double c1,
                double a2, double b2, double c2,
                Point& out) {
    const double det = a1 * b2 - a2 * b1;
    if (std::abs(det) <= EPS) return false;
    out.x = (b1 * c2 - b2 * c1) / det;
    out.y = (c1 * a2 - c2 * a1) / det;
    return true;
}

Point choose_E(const Point& a, const Point& b,
               const Point& c, const Point& d) {
    const double a_coef = d.y - a.y;
    const double b_coef = a.x - d.x;
    const double c_coef = -b.y * a.x + (a.y - c.y) * b.x
                        + (b.y - d.y) * c.x + c.y * d.x;

    double ab_a, ab_b, ab_c, cd_a, cd_b, cd_c;
    const bool ok_ab = line_from_points(a, b, ab_a, ab_b, ab_c);
    const bool ok_cd = line_from_points(c, d, cd_a, cd_b, cd_c);

    Point e_ab{}, e_cd{};
    const bool has_ab = ok_ab &&
        line_isect(a_coef, b_coef, c_coef, ab_a, ab_b, ab_c, e_ab);
    const bool has_cd = ok_cd &&
        line_isect(a_coef, b_coef, c_coef, cd_a, cd_b, cd_c, e_cd);

    const double sb = signed_side(a, d, b);
    const double sc = signed_side(a, d, c);
    const double db = perp_dist(a, d, b);
    const double dc = perp_dist(a, d, c);

    double se = 0.0;
    if (has_ab) se = signed_side(a, d, e_ab);
    else if (has_cd) se = signed_side(a, d, e_cd);

    const bool same_side_bc = (sb > EPS && sc > EPS) || (sb < -EPS && sc < -EPS);
    const bool same_side_b_e = (sb > EPS && se > EPS) || (sb < -EPS && se < -EPS);

    if (same_side_bc) {
        if (db > dc + EPS) {
            if (has_ab) return e_ab;
            if (has_cd) return e_cd;
        } else if (dc > db + EPS) {
            if (has_cd) return e_cd;
            if (has_ab) return e_ab;
        } else {
            if (same_side_b_e) {
                if (has_ab) return e_ab;
                if (has_cd) return e_cd;
            } else {
                if (has_cd) return e_cd;
                if (has_ab) return e_ab;
            }
        }
    } else {
        if (same_side_b_e) {
            if (has_ab) return e_ab;
            if (has_cd) return e_cd;
        } else {
            if (has_cd) return e_cd;
            if (has_ab) return e_ab;
        }
    }

    if (has_ab && has_cd) return {(e_ab.x + e_cd.x) * 0.5,
                                   (e_ab.y + e_cd.y) * 0.5};
    if (has_ab) return e_ab;
    if (has_cd) return e_cd;
    return {(b.x + c.x) * 0.5, (b.y + c.y) * 0.5};
}

double areal_displacement(const Point& a, const Point& b,
                          const Point& c, const Point& e) {
    const double twice =
          (a.x * b.y - b.x * a.y)
        + (b.x * c.y - c.x * b.y)
        + (c.x * e.y - e.x * c.y)
        + (e.x * a.y - a.x * e.y);
    return 0.5 * std::abs(twice);
}

bool local_topology_ok(const Point& a, const Point& e, const Point& d,
                       const Node* node_a, const Node* node_b,
                       const Node* node_c,
                       const SegGrid& grid) {
    bool ok = true;

    static int query_stamp = 0;
    ++query_stamp;
    const int cur_stamp = query_stamp;

    auto test_seg = [&](Node* nd) {
        if (!nd->alive) return;
        if (nd->visit_stamp == cur_stamp) return;
        nd->visit_stamp = cur_stamp;

        if (nd == node_a || nd == node_b || nd == node_c) return;

        const Point& ps = nd->pt;
        const Point& qs = nd->next->pt;

        if (segments_properly_intersect(a, e, ps, qs, EPS) ||
            segments_properly_intersect(e, d, ps, qs, EPS)) {
            ok = false;
        }
    };

    bool stop = false;
    auto test_seg_stop = [&](Node* nd) {
        test_seg(nd);
        if (!ok) stop = true;
    };
    grid.query(a, e, stop, test_seg_stop);
    if (!ok) return false;
    stop = false;
    grid.query(e, d, stop, test_seg_stop);
    return ok;
}

HeapEntry make_entry(Node* node_b, int ring_id,
                     const std::vector<std::vector<Node*>>& ring_nodes,
                     const SegGrid& grid) {
    HeapEntry he;
    he.node_b = node_b;
    he.node_c = node_b->next;
    he.cost   = std::numeric_limits<double>::infinity();

    Node* node_a = node_b->prev;
    Node* node_c = he.node_c;
    Node* node_d = node_c->next;

    if (node_d == node_b || node_d == node_a) {
        he.node_b = nullptr;
        return he;
    }

    if (static_cast<int>(ring_nodes[ring_id].size()) < 4) {
        he.node_b = nullptr;
        return he;
    }

    const Point& a = node_a->pt;
    const Point& b = node_b->pt;
    const Point& c = node_c->pt;
    const Point& d = node_d->pt;

    he.e = choose_E(a, b, c, d);

    {
        const double before = (a.x * b.y - b.x * a.y)
                            + (b.x * c.y - c.x * b.y)
                            + (c.x * d.y - d.x * c.y);
        const double after  = (a.x * he.e.y - he.e.x * a.y)
                            + (he.e.x * d.y - d.x * he.e.y);
        if (std::abs(before - after) > AREA_EPS * 2.0) {
            he.node_b = nullptr;
            return he;
        }
    }

    if (!local_topology_ok(a, he.e, d, node_a, node_b, node_c, grid)) {
        he.node_b = nullptr;
        return he;
    }

    he.cost  = areal_displacement(a, b, c, he.e);
    he.gen_a = node_a->gen;
    he.gen_b = node_b->gen;
    he.gen_c = node_c->gen;
    he.gen_d = node_d->gen;
    return he;
}

bool entry_is_fresh(const HeapEntry& he) {
    if (!he.node_b || !he.node_c) return false;
    const Node* node_a = he.node_b->prev;
    const Node* node_d = he.node_c->next;
    return he.node_b->alive && he.node_c->alive
        && node_a->gen == he.gen_a
        && he.node_b->gen == he.gen_b
        && he.node_c->gen == he.gen_c
        && node_d->gen == he.gen_d;
}

Node* do_collapse(Node* node_b, Node* node_c, const Point& e,
                  std::vector<Node*>& ring_node_list,
                  SegGrid& grid) {
    Node* node_a = node_b->prev;
    Node* node_d = node_c->next;

    grid.remove(node_a);   // segment A->B
    grid.remove(node_b);   // segment B->C
    grid.remove(node_c);   // segment C->D

    node_b->pt   = e;
    node_b->next = node_d;
    node_d->prev = node_b;

    node_c->alive = false;
    {
        auto it = std::find(ring_node_list.begin(),
                            ring_node_list.end(), node_c);
        if (it != ring_node_list.end()) ring_node_list.erase(it);
    }

    grid.insert(node_a);   // segment A->E  (node_a->next == node_b == E)
    grid.insert(node_b);   // segment E->D  (node_b->next == node_d)

    ++node_a->gen;
    ++node_b->gen;
    ++node_d->gen;

    return node_b;  // the E node
}

using PQ = std::priority_queue<HeapEntry,
                               std::vector<HeapEntry>,
                               std::greater<HeapEntry>>;

void push_windows_around(Node* n, int ring_id,
                         const std::vector<std::vector<Node*>>& ring_nodes,
                         const SegGrid& grid,
                         PQ& pq) {
    Node* bs[4] = {
        n->prev->prev,
        n->prev,
        n,
        n->next
    };
    for (Node* b : bs) {
        if (!b || !b->alive) continue;
        HeapEntry he = make_entry(b, ring_id, ring_nodes, grid);
        if (he.node_b) pq.push(he);
    }
}


SimplificationResult simplify_polygon(const Polygon& polygon,
                                      int target_vertices) {
    if (target_vertices < 3)
        throw std::runtime_error("target_vertices must be at least 3");

    SimplificationResult result;
    result.total_areal_displacement = 0.0;

    int total_verts = total_vertex_count(polygon);
    if (total_verts <= target_vertices) {
        result.polygon = polygon;
        return result;
    }

    const int R = static_cast<int>(polygon.rings.size());

    std::list<Node> pool;

    std::vector<std::vector<Node*>> ring_nodes(R);
    std::vector<double> ring_areas(R);

    for (int r = 0; r < R; ++r) {
        const Ring& ring = polygon.rings[r];
        ring_areas[r] = signed_area(ring);
        const int n = static_cast<int>(ring.vertices.size());
        ring_nodes[r].reserve(n);

        for (int i = 0; i < n; ++i) {
            pool.push_back(Node{});
            Node* nd    = &pool.back();
            nd->pt      = ring.vertices[i];
            nd->ring_id = r;
            nd->gen     = 0;
            nd->alive   = true;
            ring_nodes[r].push_back(nd);
        }
        for (int i = 0; i < n; ++i) {
            ring_nodes[r][i]->prev = ring_nodes[r][(i - 1 + n) % n];
            ring_nodes[r][i]->next = ring_nodes[r][(i + 1) % n];
        }
    }

    SegGrid grid;
    grid.build(ring_nodes, total_verts);

    PQ pq;
    {
        std::vector<HeapEntry> init_entries;
        init_entries.reserve(total_verts);
        for (int r = 0; r < R; ++r)
            for (Node* b : ring_nodes[r]) {
                HeapEntry he = make_entry(b, r, ring_nodes, grid);
                if (he.node_b) init_entries.push_back(he);
            }
        pq = PQ(std::greater<HeapEntry>{},
                std::move(init_entries));
    }

    while (total_verts > target_vertices && !pq.empty()) {
        HeapEntry he = pq.top();
        pq.pop();

        if (!entry_is_fresh(he)) continue;

        Node* node_b = he.node_b;
        Node* node_c = he.node_c;
        const int r  = node_b->ring_id;

        if (static_cast<int>(ring_nodes[r].size()) < 4) continue;

        Node* node_e = do_collapse(node_b, node_c, he.e,
                                   ring_nodes[r], grid);
        --total_verts;
        result.total_areal_displacement += he.cost;

        push_windows_around(node_e, r, ring_nodes, grid, pq);
    }

    result.polygon.rings.resize(R);
    for (int r = 0; r < R; ++r) {
        Ring& out = result.polygon.rings[r];
        out.ring_id = polygon.rings[r].ring_id;
        out.vertices.clear();
        out.vertices.reserve(ring_nodes[r].size());
        for (const Node* nd : ring_nodes[r])
            out.vertices.push_back(nd->pt);
    }

    return result;
}

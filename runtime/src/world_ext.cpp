// The world beyond the 4:3 view (widescreen, Settings > Video > Build the world beyond the view; off by default, the
// zoom being the default answer): a DS game builds only as much of its world as its narrow view shows, so a widened
// view finds nothing at its far sides. This carries the ground on: each frame, the game's 3D is watched as it is
// submitted (FfccVertexHook: every vertex, with the geometry engine's matrices and texture state), the rows of level
// ground reaching the edge of the view are found, and at the end of the frame, before the game swaps its 3D buffers,
// each is extruded outward along x (the town is drawn on a drum around the x axis, so the ground does not change along
// x), with its own texture mapping, colour and polygon attributes. Rows above the fitted ground height (roofs, railing
// tops) and rows ending at a wall are left alone.
//
// The extra polygons go to the geometry engine the way the game sends its own (the direct GX command ports), from
// the hook at the end of the frame (game/src/cfie_hooks.cpp: mq_frame_end), and are tagged with polygon ID 62 so
// they are not taken for the game's own on the next frame.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <vector>

#include "GPU3D.h"
#include "gx_hook.hpp"
#include "ntr/recomp.h"

namespace melonDS {
extern void (*FfccVertexHook)(void* context, GPU3D* gpu, Vertex* vertex);
extern void* FfccVertexHookContext;
}  // namespace melonDS

namespace {

constexpr uint32_t kOurId = 62;  // polygon ID of the polygons drawn here

struct V3 {
    double x = 0, y = 0, z = 0;
};
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, double k) { return {a.x * k, a.y * k, a.z * k}; }
double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double len(V3 a) { return std::sqrt(dot(a, a)); }
V3 unit(V3 a) {
    const double l = len(a);
    return l > 0 ? a * (1.0 / l) : a;
}

struct Vert {
    V3 p;          // the game's position-matrix space (units of 1.0; the camera is in the projection)
    double ndc_x;  // x on the screen, -1 to 1 across the (wide) view
    double s, t;   // texture coordinates (1/16 texel)
    uint8_t col[3];
};

struct Poly {
    Vert v[4];
    int n = 0;
    uint32_t tex = 0, pal = 0, attr = 0;
    V3 normal;     // unit, towards the camera
    double area = 0;
    int proj = 0;  // index of its projection in State::projs
};

struct State {
    bool enabled = false;
    int wide_extra = 0;
    std::vector<Poly> polys;
    Vert prim[4];  // the current primitive's last vertices
    std::vector<std::array<double, 16>> projs;  // the projections the frame's polygons were drawn with
};

// The camera's position in the space a projection (camera included) maps from: where x, y and w are all 0.
V3 eye_of(const double* m) {
    // rows 0, 1, 3 of the 4x4 (element (r, c) at [c * 4 + r]) times (x, y, z, 1) = 0
    const double a[3][4] = {{m[0], m[4], m[8], m[12]}, {m[1], m[5], m[9], m[13]}, {m[3], m[7], m[11], m[15]}};
    const double det = a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
                       a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    if (std::fabs(det) < 1e-12) return {};
    auto solve = [&](int col) {
        double b[3][3];
        for (int r = 0; r < 3; r++)
            for (int k = 0; k < 3; k++) b[r][k] = k == col ? -a[r][3] : a[r][k];
        return (b[0][0] * (b[1][1] * b[2][2] - b[1][2] * b[2][1]) - b[0][1] * (b[1][0] * b[2][2] - b[1][2] * b[2][0]) +
                b[0][2] * (b[1][0] * b[2][1] - b[1][1] * b[2][0])) / det;
    };
    return {solve(0), solve(1), solve(2)};
}

State& st() {
    static State s;
    return s;
}

void add_poly(const Vert* v, int n, const melonDS::GPU3D* g) {
    State& s = st();
    if (s.polys.size() >= 6000) return;
    Poly p;
    p.n = n;
    for (int i = 0; i < n; i++) p.v[i] = v[i];
    p.tex = g->TexParam;
    p.pal = g->TexPalette;
    p.attr = g->CurPolygonAttr;
    std::array<double, 16> proj;
    for (int i = 0; i < 16; i++) proj[i] = g->ProjMatrix[i] / 4096.0;
    auto found = std::find(s.projs.begin(), s.projs.end(), proj);
    if (found == s.projs.end()) {
        if (s.projs.size() >= 32) return;
        s.projs.push_back(proj);
        found = s.projs.end() - 1;
    }
    p.proj = int(found - s.projs.begin());
    V3 nrm;
    for (int i = 1; i + 1 < n; i++) nrm = nrm + cross(v[i].p - v[0].p, v[i + 1].p - v[0].p);
    p.area = len(nrm) * 0.5;
    if (p.area <= 0) return;
    nrm = unit(nrm);
    if (dot(nrm, eye_of(proj.data()) - v[0].p) < 0) nrm = nrm * -1.0;  // (towards the camera)
    p.normal = nrm;
    s.polys.push_back(p);
}

void on_vertex(void*, melonDS::GPU3D* g, melonDS::Vertex* vt) {
    if (melonDS::FfccGxReplaying) return;
    if (((g->CurPolygonAttr >> 24) & 0x3F) == kOurId) return;
    const int fmt = (g->TexParam >> 26) & 7;
    if (fmt == 0) return;  // (untextured: not ground)
    // (orthographic geometry, 2D drawn with the 3D: not the world)
    if (g->ProjMatrix[11] == 0 && g->ProjMatrix[15] == 0x1000) return;
    const int32_t* m = g->PosMatrix;
    const double x = g->CurVertex[0], y = g->CurVertex[1], z = g->CurVertex[2];
    Vert v;
    v.p.x = (x * m[0] + y * m[4] + z * m[8]) / (4096.0 * 4096.0) + m[12] / 4096.0;
    v.p.y = (x * m[1] + y * m[5] + z * m[9]) / (4096.0 * 4096.0) + m[13] / 4096.0;
    v.p.z = (x * m[2] + y * m[6] + z * m[10]) / (4096.0 * 4096.0) + m[14] / 4096.0;
    v.ndc_x = vt->Position[3] > 0 ? double(vt->Position[0]) / double(vt->Position[3]) : 0.0;
    v.s = vt->TexCoords[0];
    v.t = vt->TexCoords[1];
    v.col[0] = g->VertexColor[0];
    v.col[1] = g->VertexColor[1];
    v.col[2] = g->VertexColor[2];
    State& s = st();
    const uint32_t i = g->VertexNum;  // (this vertex's index in the primitive: counted on after this hook)
    switch (g->PolygonMode) {
    case 0:  // triangles
        s.prim[i % 3] = v;
        if (i % 3 == 2) add_poly(s.prim, 3, g);
        break;
    case 1:  // quads
        s.prim[i % 4] = v;
        if (i % 4 == 3) add_poly(s.prim, 4, g);
        break;
    case 2:  // triangle strip
        s.prim[0] = s.prim[1];
        s.prim[1] = s.prim[2];
        s.prim[2] = v;
        if (i >= 2) add_poly(s.prim, 3, g);
        break;
    default:  // quad strip: each new pair makes a quad with the pair before
        s.prim[0] = s.prim[2];
        s.prim[1] = s.prim[3];
        if (i % 2 == 0) s.prim[2] = v;
        else {
            s.prim[3] = v;
            if (i >= 3) {
                const Vert q[4] = {s.prim[0], s.prim[1], s.prim[3], s.prim[2]};
                add_poly(q, 4, g);
            }
        }
        break;
    }
}

struct P2 {
    double a, b;
};

void port(NtrCpu* c, uint32_t command, uint32_t param) { ntr_wr32(c, 0x04000400u + command * 4, param); }
uint32_t fx(double v) { return uint32_t(int32_t(std::lround(v * 4096.0))); }

}  // namespace

namespace ntr {

// Turned on with widescreen (VideoSettings::world_extend and a wide picture): see runtime.cpp.
void world_extend_enable(bool on, int wide_extra) {
    State& s = st();
    s.enabled = on && wide_extra > 0;
    s.wide_extra = wide_extra;
    melonDS::FfccVertexHook = s.enabled ? &on_vertex : nullptr;
    if (!s.enabled) {
        s.polys.clear();
        s.projs.clear();
    }
}

}  // namespace ntr

// (MQ_LOG_WORLD: why nothing was drawn, now and then)
static void why(const char* reason, double a = 0, double b = 0) {
    static const bool log = std::getenv("MQ_LOG_WORLD") != nullptr;
    static int count = 0;
    if (log && count++ % 20 == 0) std::fprintf(stderr, "world: none (%s %.3f %.3f)\n", reason, a, b);
}

namespace {

// One side (-1 left, +1 right). The town is drawn on a drum (Animal Crossing's rolling world: the ground curves away
// from the camera around an axis along x), so it does not change along x: each row of the ground goes on sideways
// unchanged. For each row, the ground polygon reaching furthest towards this side is carried on from its outer
// edge along x, in its own plane, with its own texture mapping continued (the textures repeat), out past the edge of
// the wide view. Where the row's end is not ground (a wall, a building), nothing is added for it.
void extend_side(NtrCpu* c, const std::vector<Poly>& polys, const std::vector<std::array<double, 16>>& projs, int wide_extra,
                 int side) {
    const double narrow = (256.0 + 2.0 * wide_extra) / 256.0;  // (the DS's own view: x scaled by this)
    struct Row {
        const Poly* poly;
        int e0, e1;     // its outer edge's two vertices
        double outer;   // their x
    };
    std::vector<Row> rows;
    for (const Poly& p : polys) {
        if (p.n < 3) continue;
        if (((p.attr >> 4) & 3) == 3 || ((p.attr >> 16) & 31) != 31) continue;  // (solid only)
        if (std::fabs(p.normal.x) > 0.15 || p.normal.y < 0.25) continue;      // (ground: faces up, level along x)
        // (not a decal laid on the ground: a texture with see-through parts, a manhole, a pattern)
        const uint32_t fmt = (p.tex >> 26) & 7;
        if (fmt == 1 || fmt == 6 || ((fmt == 2 || fmt == 3 || fmt == 4) && (p.tex & (1u << 29)))) continue;
        double reach = -1e9, lo = 1e9, hi = -1e9;
        for (int i = 0; i < p.n; i++) {
            const double x = narrow * p.v[i].ndc_x;
            reach = std::max(reach, x * side), lo = std::min(lo, x), hi = std::max(hi, x);
        }
        if (reach < 0.5) continue;           // (not near this edge)
        if (lo < -1.2 && hi > 1.2) continue;  // (a backdrop across the whole view: the sky)
        // The outer edge: the two vertices furthest towards this side along x (they must share that x: an edge
        // across the row).
        int e0 = -1, e1 = -1;
        for (int i = 0; i < p.n; i++)
            if (e0 < 0 || p.v[i].p.x * side > p.v[e0].p.x * side) e0 = i;
        for (int i = 0; i < p.n; i++)
            if (i != e0 && (e1 < 0 || p.v[i].p.x * side > p.v[e1].p.x * side)) e1 = i;
        if (std::fabs(p.v[e0].p.x - p.v[e1].p.x) > 0.02) continue;  // (a slanted end: not a row's end)
        // (a speck: the top of a character's hair or hat at the view's edge is level too, but small; the ground's
        // pieces are tenths of a unit and more across)
        if (p.area < 0.05 && len(p.v[e0].p - p.v[e1].p) < 0.3) continue;
        const double outer = p.v[e0].p.x;
        // The same row: an outer edge at the same place across (y, z). Keep the one furthest out.
        auto key_match = [&](const Row& r) {
            const Vert& a0 = r.poly->v[r.e0];
            const Vert& a1 = r.poly->v[r.e1];
            const Vert& b0 = p.v[e0];
            const Vert& b1 = p.v[e1];
            auto close = [](const Vert& x, const Vert& y) { return std::fabs(x.p.y - y.p.y) < 0.05 && std::fabs(x.p.z - y.p.z) < 0.05; };
            return (close(a0, b0) && close(a1, b1)) || (close(a0, b1) && close(a1, b0));
        };
        auto r = std::find_if(rows.begin(), rows.end(), key_match);
        if (r == rows.end()) rows.push_back({&p, e0, e1, outer});
        else if (outer * side > r->outer * side) *r = {&p, e0, e1, outer};
    }
    // Only the ground itself: a level surface well above the ground (a roof, the top of a railing or a wall, a table)
    // is part of something that ends, not of the world going on. The ground's height along z is fitted to the level
    // surfaces of the whole view, by area (the drum's curve, near enough a parabola over the view), dropping the
    // ones far from the fit each round so the largest surface, the ground, decides it.
    {
        struct Pt {
            double z, y, w;
        };
        std::vector<Pt> pts;
        // (not the sky's level backdrops: two of about 25000 square units, 130-150 units below the camera's point,
        // which the fit followed by their area, tens to hundreds of units off the ground (fishing by the bookshop), so
        // the test below dropped or kept rows at random, a student's hair at the view's edge among those kept; the
        // town's ground pieces are at most a cell, 16-20 square units. z is taken about the surfaces' middle, zc, so
        // the fit in z and z^2 is well conditioned.)
        double zc = 0, zw = 0;
        for (const Poly& p : polys) {
            if (p.n < 3 || p.normal.y < 0.7 || std::fabs(p.normal.x) > 0.15 || p.area <= 0 || p.area > 400) continue;
            if (((p.attr >> 4) & 3) == 3) continue;
            double y = 0, z = 0;
            for (int i = 0; i < p.n; i++) y += p.v[i].p.y, z += p.v[i].p.z;
            pts.push_back({z / p.n, y / p.n, p.area});
            zc += z / p.n * p.area, zw += p.area;
        }
        if (zw > 0) zc /= zw;
        for (Pt& q : pts) q.z -= zc;
        if (static const bool fl = std::getenv("MQ_LOG_WORLD_FIT") != nullptr; fl) {
            static int n = 0;
            if (n++ % 240 == 0) {
                std::fprintf(stderr, "world fit: zc %.2f %zu pts:", zc, pts.size());
                for (const Pt& q : pts) std::fprintf(stderr, " (%.1f %.2f %.2f)", q.z, q.y, q.w);
                std::fprintf(stderr, "\n");
            }
        }
        double k[3] = {0, 0, 0}, spread = 1e9;
        bool fitted = false;
        for (int round = 0; round < 5 && pts.size() >= 3; round++) {
            // weighted least squares of y = k0 + k1 z + k2 z^2 over the points within `spread` of the last fit
            double a[3][4] = {};
            double wsum = 0;
            for (const Pt& q : pts) {
                if (fitted && std::fabs(q.y - (k[0] + k[1] * q.z + k[2] * q.z * q.z)) > spread) continue;
                const double f[3] = {1, q.z, q.z * q.z};
                for (int r = 0; r < 3; r++) {
                    for (int cc = 0; cc < 3; cc++) a[r][cc] += q.w * f[r] * f[cc];
                    a[r][3] += q.w * f[r] * q.y;
                }
                wsum += q.w;
            }
            if (wsum <= 0) break;
            for (int r = 0; r < 3; r++) a[r][r] += 1e-9 * wsum;
            // (Gaussian elimination, partial pivoting)
            bool ok = true;
            for (int col = 0; col < 3 && ok; col++) {
                int piv = col;
                for (int r = col + 1; r < 3; r++)
                    if (std::fabs(a[r][col]) > std::fabs(a[piv][col])) piv = r;
                if (std::fabs(a[piv][col]) < 1e-12) ok = false;
                else {
                    for (int cc = 0; cc < 4; cc++) std::swap(a[col][cc], a[piv][cc]);
                    for (int r = 0; r < 3; r++)
                        if (r != col) {
                            const double f = a[r][col] / a[col][col];
                            for (int cc = 0; cc < 4; cc++) a[r][cc] -= f * a[col][cc];
                        }
                }
            }
            if (!ok) break;
            for (int r = 0; r < 3; r++) k[r] = a[r][3] / a[r][r];
            fitted = true;
            // the next round keeps what lies within twice the area-weighted median distance (at least 0.1)
            std::vector<std::pair<double, double>> d;
            for (const Pt& q : pts) d.push_back({std::fabs(q.y - (k[0] + k[1] * q.z + k[2] * q.z * q.z)), q.w});
            std::sort(d.begin(), d.end());
            double acc = 0, med = 0;
            for (const auto& [dist, w] : d)
                if ((acc += w) >= wsum * 0.5) {
                    med = dist;
                    break;
                }
            spread = std::max(0.1, 2.0 * med);
            if (static const bool fl = std::getenv("MQ_LOG_WORLD_FIT") != nullptr; fl) {
                static int n = 0;
                if (n++ % 1200 < 5) std::fprintf(stderr, "world fit: round %d k %.4f %.4f %.5f spread %.3f wsum %.1f\n", round, k[0], k[1], k[2], spread, wsum);
            }
        }
        if (fitted) {
            static const double tol = std::getenv("MQ_WORLD_TOL") ? std::atof(std::getenv("MQ_WORLD_TOL")) : 0.35;
            static const bool log = std::getenv("MQ_LOG_WORLD") != nullptr;
            static int count = 0;
            const bool show = log && count++ % 120 == 0;
            rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const Row& r) {
                const V3 a = r.poly->v[r.e0].p, b = r.poly->v[r.e1].p;
                const double z = (a.z + b.z) / 2 - zc, y = (a.y + b.y) / 2;
                const double above = y - (k[0] + k[1] * z + k[2] * z * z);
                if (show) std::fprintf(stderr, "world: side %d row y %.2f z %.2f above %.2f%s\n", side, y, z + zc, above, above > tol ? " (dropped)" : "");
                return above > tol;
            }), rows.end());
        }
    }
    // A row that ends at a wall (a room's side, a building) is not carried on: the world there is the wall's.
    rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const Row& r) {
        const V3 a = r.poly->v[r.e0].p, b = r.poly->v[r.e1].p;
        const double zlo = std::min(a.z, b.z) - 0.3, zhi = std::max(a.z, b.z) + 0.3, ytop = std::max(a.y, b.y) + 0.25;
        for (const Poly& q : polys) {
            if (std::fabs(q.normal.x) < 0.5) continue;  // (a wall faces along x)
            for (int i = 0; i < q.n; i++) {
                const V3& v = q.v[i].p;
                if (std::fabs(v.x - r.outer) < 0.6 && v.z > zlo && v.z < zhi && v.y > ytop) return true;
            }
        }
        return false;
    }), rows.end());
    {
        static const bool log = std::getenv("MQ_LOG_WORLD") != nullptr;
        static int count = 0;
        if (log && count++ % 60 < 2) {
            std::fprintf(stderr, "world: side %d: %zu polys, %zu ground rows\n", side, polys.size(), rows.size());
            for (const Row& r : rows) {
                const V3 a = r.poly->v[r.e0].p, b = r.poly->v[r.e1].p;
                std::fprintf(stderr, "world:   row x %.2f y %.2f z %.2f edge %.2f area %.3f attr %08x tex %08x n %.2f %.2f %.2f\n", r.outer,
                             a.y, a.z, len(a - b), r.poly->area, r.poly->attr, r.poly->tex, r.poly->normal.x, r.poly->normal.y,
                             r.poly->normal.z);
            }
        }
    }
    if (rows.empty()) return;
    if (rows.size() > 200) rows.resize(200);

    // Our polygons, grouped by projection and look (a BEGIN per group).
    struct Quad {
        V3 p[4];
        double s[4], t[4];
    };
    struct Batch {
        int proj;
        uint32_t tex, pal, attr, color;
        std::vector<Quad> quads;
    };
    std::vector<Batch> batches;
    double max_coord = 1;
    for (const Row& row : rows) {
        const Poly& p = *row.poly;
        const double* proj = projs[p.proj].data();
        // Texture mapping over the polygon's plane: (s, t) affine in (along x, across) from three vertices.
        const V3 n = p.normal;
        const V3 ax{1, 0, 0};
        const V3 across = unit(cross(n, ax));
        auto coord = [&](const V3& q) { return P2{q.x, dot(q, across)}; };
        const P2 q0 = coord(p.v[0].p), q1 = coord(p.v[1].p), q2 = coord(p.v[2].p);
        const double det = (q1.a - q0.a) * (q2.b - q0.b) - (q2.a - q0.a) * (q1.b - q0.b);
        if (std::fabs(det) < 1e-9) continue;
        auto solve = [&](double f0, double f1, double f2, double& ka, double& kb, double& kc) {
            ka = ((f1 - f0) * (q2.b - q0.b) - (f2 - f0) * (q1.b - q0.b)) / det;
            kb = ((q1.a - q0.a) * (f2 - f0) - (q2.a - q0.a) * (f1 - f0)) / det;
            kc = f0 - ka * q0.a - kb * q0.b;
        };
        double sa, sb, sc, ta, tb, tc;
        solve(p.v[0].s, p.v[1].s, p.v[2].s, sa, sb, sc);
        solve(p.v[0].t, p.v[1].t, p.v[2].t, ta, tb, tc);
        const double tex_w = 8 << ((p.tex >> 20) & 7), tex_h = 8 << ((p.tex >> 23) & 7);
        const double period_s = tex_w * 16, period_t = tex_h * 16;
        // How far out: until past the wide view's edge (the projected x of the edge moved along x).
        const V3 e0 = p.v[row.e0].p, e1 = p.v[row.e1].p;
        auto ndc = [&](const V3& q) {
            const double x = proj[0] * q.x + proj[4] * q.y + proj[8] * q.z + proj[12];
            const double w = proj[3] * q.x + proj[7] * q.y + proj[11] * q.z + proj[15];
            return w > 1e-6 ? x / w : 1e9 * side;
        };
        double length = std::max(0.5, std::fabs(p.v[row.e0].p.x - p.v[(row.e0 + 2) % p.n].p.x));
        for (int k = 0; k < 12 && (ndc(e0 + ax * (length * side)) * side < 1.05 || ndc(e1 + ax * (length * side)) * side < 1.05); k++)
            length *= 2;
        length = std::min(length, 400.0);
        // In segments short enough for 16-bit texture coordinates (two repeats at most).
        const double per_x = std::max(std::fabs(sa) / period_s, std::fabs(ta) / period_t);  // repeats per unit along x
        const double seg = per_x > 0 ? std::min(length, 2.0 / per_x) : length;
        const int count = std::min(64, int(std::ceil(length / seg)));
        Batch* batch = nullptr;
        const uint32_t tex = (p.tex & ~(3u << 30)) | (3u << 16);  // (texture coordinates as given, repeating)
        const uint32_t attr = (p.attr & ~(0x3Fu << 24) & ~0xFu) | (kOurId << 24) | 0xC0;
        int col[3] = {0, 0, 0};
        for (int i = 0; i < p.n; i++)
            for (int k = 0; k < 3; k++) col[k] += p.v[i].col[k];
        const uint32_t color = uint32_t((col[0] / p.n) & 31) | uint32_t((col[1] / p.n) & 31) << 5 | uint32_t((col[2] / p.n) & 31) << 10;
        for (Batch& bt : batches)
            if (bt.proj == p.proj && bt.tex == tex && bt.pal == p.pal && bt.attr == attr && bt.color == color) batch = &bt;
        if (!batch) {
            batches.push_back({p.proj, tex, p.pal, attr, color, {}});
            batch = &batches.back();
        }
        for (int k = 0; k < count; k++) {
            const double x0 = seg * k * side, x1 = std::min(seg * (k + 1), length) * side;
            Quad q;
            q.p[0] = e0 + ax * x0;
            q.p[1] = e1 + ax * x0;
            q.p[2] = e1 + ax * x1;
            q.p[3] = e0 + ax * x1;
            double s0 = 0, t0 = 0;
            for (int i = 0; i < 4; i++) {
                const P2 cq = coord(q.p[i]);
                q.s[i] = sa * cq.a + sb * cq.b + sc;
                q.t[i] = ta * cq.a + tb * cq.b + tc;
                if (i == 0) s0 = std::floor(q.s[0] / period_s) * period_s, t0 = std::floor(q.t[0] / period_t) * period_t;
                q.s[i] -= s0;
                q.t[i] -= t0;
                max_coord = std::max({max_coord, std::fabs(q.p[i].x), std::fabs(q.p[i].y), std::fabs(q.p[i].z)});
            }
            batch->quads.push_back(q);
        }
    }
    size_t total = 0;
    for (const Batch& bt : batches) total += bt.quads.size();
    if (total == 0 || total > 700) return why("quads", double(total));
    const double scale = std::ceil(max_coord / 7.5 + 0.01);  // (vertices are 16-bit: through a scaled matrix)
    if (scale > 120) return why("scale", scale);

    for (const Batch& bt : batches) {
        const double* proj = projs[bt.proj].data();
        port(c, 0x10, 0);  // MTX_MODE projection
        port(c, 0x11, 0);  // MTX_PUSH
        for (int i = 0; i < 16; i++) port(c, 0x16, fx(proj[i]));  // MTX_LOAD_4x4
        port(c, 0x10, 2);  // MTX_MODE position & vector
        port(c, 0x11, 0);
        const double m[12] = {scale, 0, 0, 0, scale, 0, 0, 0, scale, 0, 0, 0};
        for (double v : m) port(c, 0x17, fx(v));  // MTX_LOAD_4x3: a scale
        port(c, 0x29, bt.attr);  // POLYGON_ATTR
        port(c, 0x2A, bt.tex);   // TEXIMAGE_PARAM
        port(c, 0x2B, bt.pal);   // PLTT_BASE
        port(c, 0x20, bt.color); // COLOR
        port(c, 0x40, 1);        // BEGIN_VTXS quads
        for (const Quad& q : bt.quads)
            for (int i = 0; i < 4; i++) {
                const int32_t s16 = int32_t(std::lround(q.s[i])), t16 = int32_t(std::lround(q.t[i]));
                port(c, 0x22, uint32_t(uint16_t(int16_t(std::clamp(s16, -32768, 32767)))) |
                                  uint32_t(uint16_t(int16_t(std::clamp(t16, -32768, 32767)))) << 16);  // TEXCOORD
                const int32_t x = int32_t(std::lround(q.p[i].x / scale * 4096.0));
                const int32_t y = int32_t(std::lround(q.p[i].y / scale * 4096.0));
                const int32_t z = int32_t(std::lround(q.p[i].z / scale * 4096.0));
                port(c, 0x23, uint32_t(uint16_t(int16_t(x))) | uint32_t(uint16_t(int16_t(y))) << 16);  // VTX_16
                port(c, 0x23, uint32_t(uint16_t(int16_t(z))));
            }
        port(c, 0x41, 0);  // END_VTXS
        port(c, 0x10, 2);
        port(c, 0x12, 1);  // MTX_POP
        port(c, 0x10, 0);
        port(c, 0x12, 1);
        port(c, 0x10, 2);
    }
}

}  // namespace

// At the end of a frame, before the game swaps its 3D buffers: the ground on into the sides (see the top).
extern "C" void ntr_world_extend(NtrCpu* c) {
    State& s = st();
    if (!s.enabled) return;
    std::vector<Poly> polys;
    polys.swap(s.polys);
    std::vector<std::array<double, 16>> projs;
    projs.swap(s.projs);
    if (polys.size() < 8) return;
    if (std::getenv("MQ_LOG_PROJ")) {  // (research: the projections the frame was drawn with)
        static int count = 0;
        if (count++ % 200 < 4)
            for (const auto& m : projs) {
                std::fprintf(stderr, "proj %d:", count);
                for (double v : m) std::fprintf(stderr, " %.4f", v);
                std::fprintf(stderr, "\n");
            }
    }
    if (std::getenv("MQ_LOG_EXTENT")) {  // (research: how far the game's geometry reaches, by x, from the camera)
        static int count = 0;
        if (count++ % 30 == 0) {
            const V3 eye = eye_of(projs[0].data());
            double lo = 1e9, hi = -1e9;
            int far_l = 0, far_r = 0;
            for (const Poly& p : polys)
                for (int i = 0; i < p.n; i++) {
                    lo = std::min(lo, p.v[i].p.x), hi = std::max(hi, p.v[i].p.x);
                    if (p.v[i].ndc_x < -0.95) far_l++;
                    if (p.v[i].ndc_x > 0.95) far_r++;
                }
            std::fprintf(stderr, "extent: eye %.2f %.2f %.2f x %.2f..%.2f (%.2f..%.2f from the eye) polys %zu edge verts %d %d\n",
                         eye.x, eye.y, eye.z, lo, hi, lo - eye.x, hi - eye.x, polys.size(), far_l, far_r);
        }
        if (std::getenv("MQ_WORLD_DRY")) return;
    }
    extend_side(c, polys, projs, s.wide_extra, -1);
    extend_side(c, polys, projs, s.wide_extra, +1);
}

// The world beyond the 4:3 view (widescreen): a DS game builds only as much of its world as its narrow view shows, so
// a widened view finds nothing at its far sides. This builds the ground on: each frame, the game's 3D is watched as
// it is submitted (FfccVertexHook: every vertex, with the geometry engine's matrices and texture state), the ground is
// found (the largest textured surface facing the camera that lies in one plane), and at the end of the frame, before
// the game swaps its 3D buffers, more of that ground is drawn into the sides outside the original view: the same
// plane, texture (tiled on from the game's own mapping), colour and polygon attributes, just behind the real geometry
// so whatever the game draws stays in front. What is above the horizon (the sky, far buildings) is left to the
// compositor's edge fill.
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

// 4x4 inverse (general, column-major as melonDS stores matrices: element (row r, column c) at [c * 4 + r]).
bool invert(const double* m, double* out) {
    double inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    const double det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (std::fabs(det) < 1e-12) return false;
    for (int i = 0; i < 16; i++) out[i] = inv[i] / det;
    return true;
}

// A camera space point from normalised device coordinates (x, y, z), through the inverse projection.
V3 unproject(const double* inv, double x, double y, double z) {
    const double cx = inv[0] * x + inv[4] * y + inv[8] * z + inv[12];
    const double cy = inv[1] * x + inv[5] * y + inv[9] * z + inv[13];
    const double cz = inv[2] * x + inv[6] * y + inv[10] * z + inv[14];
    const double cw = inv[3] * x + inv[7] * y + inv[11] * z + inv[15];
    return {cx / cw, cy / cw, cz / cw};
}

struct P2 {
    double a, b;
};

// Keeps the part of a convex polygon (plane coordinates) where f(a, b) = k0 * a + k1 * b + k2 >= 0.
std::vector<P2> clip(const std::vector<P2>& in, double k0, double k1, double k2) {
    std::vector<P2> out;
    for (size_t i = 0; i < in.size(); i++) {
        const P2 p = in[i], q = in[(i + 1) % in.size()];
        const double fp = k0 * p.a + k1 * p.b + k2, fq = k0 * q.a + k1 * q.b + k2;
        if (fp >= 0) out.push_back(p);
        if ((fp >= 0) != (fq >= 0)) {
            const double t = fp / (fp - fq);
            out.push_back({p.a + (q.a - p.a) * t, p.b + (q.b - p.b) * t});
        }
    }
    return out;
}

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
        if (log && count++ % 60 < 2) std::fprintf(stderr, "world: side %d: %zu polys, %zu ground rows\n", side, polys.size(), rows.size());
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
    extend_side(c, polys, projs, s.wide_extra, -1);
    extend_side(c, polys, projs, s.wide_extra, +1);
}

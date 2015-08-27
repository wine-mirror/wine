/*
 * Copyright 2015 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

#define D2D_CDT_EDGE_FLAG_FREED         0x80000000u
#define D2D_CDT_EDGE_FLAG_VISITED(r)    (1u << (r))

static const D2D1_MATRIX_3X2_F identity =
{
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,
};

enum d2d_cdt_edge_next
{
    D2D_EDGE_NEXT_ORIGIN = 0,
    D2D_EDGE_NEXT_ROT = 1,
    D2D_EDGE_NEXT_SYM = 2,
    D2D_EDGE_NEXT_TOR = 3,
};

struct d2d_figure
{
    D2D1_POINT_2F *vertices;
    size_t vertices_size;
    size_t vertex_count;

    struct d2d_bezier *beziers;
    size_t beziers_size;
    size_t bezier_count;
};

struct d2d_cdt_edge_ref
{
    size_t idx;
    enum d2d_cdt_edge_next r;
};

struct d2d_cdt_edge
{
    struct d2d_cdt_edge_ref next[4];
    size_t vertex[2];
    unsigned int flags;
};

struct d2d_cdt
{
    struct d2d_cdt_edge *edges;
    size_t edges_size;
    size_t edge_count;
    size_t free_edge;

    const D2D1_POINT_2F *vertices;
};

static void d2d_point_subtract(D2D1_POINT_2F *out,
        const D2D1_POINT_2F *a, const D2D1_POINT_2F *b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
}

static float d2d_point_ccw(const D2D1_POINT_2F *a, const D2D1_POINT_2F *b, const D2D1_POINT_2F *c)
{
    D2D1_POINT_2F ab, ac;

    d2d_point_subtract(&ab, b, a);
    d2d_point_subtract(&ac, c, a);

    return ab.x * ac.y - ab.y * ac.x;
}

static BOOL d2d_array_reserve(void **elements, size_t *capacity, size_t element_count, size_t element_size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (element_count <= *capacity)
        return TRUE;

    max_capacity = ~(size_t)0 / element_size;
    if (max_capacity < element_count)
        return FALSE;

    new_capacity = max(*capacity, 4);
    while (new_capacity < element_count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;

    if (new_capacity < element_count)
        new_capacity = max_capacity;

    if (*elements)
        new_elements = HeapReAlloc(GetProcessHeap(), 0, *elements, new_capacity * element_size);
    else
        new_elements = HeapAlloc(GetProcessHeap(), 0, new_capacity * element_size);

    if (!new_elements)
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

static BOOL d2d_figure_insert_vertex(struct d2d_figure *figure, size_t idx, D2D1_POINT_2F vertex)
{
    if (!d2d_array_reserve((void **)&figure->vertices, &figure->vertices_size,
            figure->vertex_count + 1, sizeof(*figure->vertices)))
    {
        ERR("Failed to grow vertices array.\n");
        return FALSE;
    }

    memmove(&figure->vertices[idx + 1], &figure->vertices[idx],
            (figure->vertex_count - idx) * sizeof(*figure->vertices));
    figure->vertices[idx] = vertex;
    ++figure->vertex_count;
    return TRUE;
}

static BOOL d2d_figure_add_vertex(struct d2d_figure *figure, D2D1_POINT_2F vertex)
{
    if (!d2d_array_reserve((void **)&figure->vertices, &figure->vertices_size,
            figure->vertex_count + 1, sizeof(*figure->vertices)))
    {
        ERR("Failed to grow vertices array.\n");
        return FALSE;
    }

    figure->vertices[figure->vertex_count] = vertex;
    ++figure->vertex_count;
    return TRUE;
}

/* FIXME: No inside/outside testing is done for beziers. */
static BOOL d2d_figure_add_bezier(struct d2d_figure *figure, D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2)
{
    struct d2d_bezier *b;
    unsigned int idx1, idx2;
    float sign;

    if (!d2d_array_reserve((void **)&figure->beziers, &figure->beziers_size,
            figure->bezier_count + 1, sizeof(*figure->beziers)))
    {
        ERR("Failed to grow beziers array.\n");
        return FALSE;
    }

    if (d2d_point_ccw(&p0, &p1, &p2) > 0.0f)
    {
        sign = -1.0f;
        idx1 = 1;
        idx2 = 2;
    }
    else
    {
        sign = 1.0f;
        idx1 = 2;
        idx2 = 1;
    }

    b = &figure->beziers[figure->bezier_count];
    b->v[0].position = p0;
    b->v[0].texcoord.u = 0.0f;
    b->v[0].texcoord.v = 0.0f;
    b->v[0].texcoord.sign = sign;
    b->v[idx1].position = p1;
    b->v[idx1].texcoord.u = 0.5f;
    b->v[idx1].texcoord.v = 0.0f;
    b->v[idx1].texcoord.sign = sign;
    b->v[idx2].position = p2;
    b->v[idx2].texcoord.u = 1.0f;
    b->v[idx2].texcoord.v = 1.0f;
    b->v[idx2].texcoord.sign = sign;
    ++figure->bezier_count;

    if (sign > 0.0f && !d2d_figure_add_vertex(figure, p1))
        return FALSE;
    if (!d2d_figure_add_vertex(figure, p2))
        return FALSE;
    return TRUE;
}

static void d2d_cdt_edge_rot(struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    dst->idx = src->idx;
    dst->r = (src->r + D2D_EDGE_NEXT_ROT) & 3;
}

static void d2d_cdt_edge_sym(struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    dst->idx = src->idx;
    dst->r = (src->r + D2D_EDGE_NEXT_SYM) & 3;
}

static void d2d_cdt_edge_tor(struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    dst->idx = src->idx;
    dst->r = (src->r + D2D_EDGE_NEXT_TOR) & 3;
}

static void d2d_cdt_edge_next_left(const struct d2d_cdt *cdt,
        struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    d2d_cdt_edge_rot(dst, &cdt->edges[src->idx].next[(src->r + D2D_EDGE_NEXT_TOR) & 3]);
}

static void d2d_cdt_edge_next_origin(const struct d2d_cdt *cdt,
        struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    *dst = cdt->edges[src->idx].next[src->r];
}

static void d2d_cdt_edge_prev_origin(const struct d2d_cdt *cdt,
        struct d2d_cdt_edge_ref *dst, const struct d2d_cdt_edge_ref *src)
{
    d2d_cdt_edge_rot(dst, &cdt->edges[src->idx].next[(src->r + D2D_EDGE_NEXT_ROT) & 3]);
}

static size_t d2d_cdt_edge_origin(const struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *e)
{
    return cdt->edges[e->idx].vertex[e->r >> 1];
}

static size_t d2d_cdt_edge_destination(const struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *e)
{
    return cdt->edges[e->idx].vertex[!(e->r >> 1)];
}

static void d2d_cdt_edge_set_origin(const struct d2d_cdt *cdt,
        const struct d2d_cdt_edge_ref *e, size_t vertex)
{
    cdt->edges[e->idx].vertex[e->r >> 1] = vertex;
}

static void d2d_cdt_edge_set_destination(const struct d2d_cdt *cdt,
        const struct d2d_cdt_edge_ref *e, size_t vertex)
{
    cdt->edges[e->idx].vertex[!(e->r >> 1)] = vertex;
}

static float d2d_cdt_ccw(const struct d2d_cdt *cdt, size_t a, size_t b, size_t c)
{
    return d2d_point_ccw(&cdt->vertices[a], &cdt->vertices[b], &cdt->vertices[c]);
}

static BOOL d2d_cdt_rightof(const struct d2d_cdt *cdt, size_t p, const struct d2d_cdt_edge_ref *e)
{
    return d2d_cdt_ccw(cdt, p, d2d_cdt_edge_destination(cdt, e), d2d_cdt_edge_origin(cdt, e)) > 0.0f;
}

static BOOL d2d_cdt_leftof(const struct d2d_cdt *cdt, size_t p, const struct d2d_cdt_edge_ref *e)
{
    return d2d_cdt_ccw(cdt, p, d2d_cdt_edge_origin(cdt, e), d2d_cdt_edge_destination(cdt, e)) > 0.0f;
}

/* Determine if point D is inside or outside the circle defined by points A,
 * B, C. As explained in the paper by Guibas and Stolfi, this is equivalent to
 * calculating the signed volume of the tetrahedron defined by projecting the
 * points onto the paraboloid of revolution x = x² + y²,
 * λ:(x, y) → (x, y, x² + y²). I.e., D is inside the cirlce if
 *
 * |λ(A) 1|
 * |λ(B) 1| > 0
 * |λ(C) 1|
 * |λ(D) 1|
 *
 * After translating D to the origin, that becomes:
 *
 * |λ(A-D)|
 * |λ(B-D)| > 0
 * |λ(C-D)| */
static BOOL d2d_cdt_incircle(const struct d2d_cdt *cdt, size_t a, size_t b, size_t c, size_t d)
{
    const D2D1_POINT_2F *p = cdt->vertices;
    const struct
    {
        double x, y;
    }
    da = {p[a].x - p[d].x, p[a].y - p[d].y},
    db = {p[b].x - p[d].x, p[b].y - p[d].y},
    dc = {p[c].x - p[d].x, p[c].y - p[d].y};

    return (da.x * da.x + da.y * da.y) * (db.x * dc.y - db.y * dc.x)
            + (db.x * db.x + db.y * db.y) * (dc.x * da.y - dc.y * da.x)
            + (dc.x * dc.x + dc.y * dc.y) * (da.x * db.y - da.y * db.x) > 0.0;
}

static void d2d_cdt_splice(const struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *a,
        const struct d2d_cdt_edge_ref *b)
{
    struct d2d_cdt_edge_ref ta, tb, alpha, beta;

    ta = cdt->edges[a->idx].next[a->r];
    tb = cdt->edges[b->idx].next[b->r];
    cdt->edges[a->idx].next[a->r] = tb;
    cdt->edges[b->idx].next[b->r] = ta;

    d2d_cdt_edge_rot(&alpha, &ta);
    d2d_cdt_edge_rot(&beta, &tb);

    ta = cdt->edges[alpha.idx].next[alpha.r];
    tb = cdt->edges[beta.idx].next[beta.r];
    cdt->edges[alpha.idx].next[alpha.r] = tb;
    cdt->edges[beta.idx].next[beta.r] = ta;
}

static BOOL d2d_cdt_create_edge(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *e)
{
    struct d2d_cdt_edge *edge;

    if (cdt->free_edge != ~0u)
    {
        e->idx = cdt->free_edge;
        cdt->free_edge = cdt->edges[e->idx].next[D2D_EDGE_NEXT_ORIGIN].idx;
    }
    else
    {
        if (!d2d_array_reserve((void **)&cdt->edges, &cdt->edges_size, cdt->edge_count + 1, sizeof(*cdt->edges)))
        {
            ERR("Failed to grow edges array.\n");
            return FALSE;
        }
        e->idx = cdt->edge_count++;
    }
    e->r = 0;

    edge = &cdt->edges[e->idx];
    edge->next[D2D_EDGE_NEXT_ORIGIN] = *e;
    d2d_cdt_edge_tor(&edge->next[D2D_EDGE_NEXT_ROT], e);
    d2d_cdt_edge_sym(&edge->next[D2D_EDGE_NEXT_SYM], e);
    d2d_cdt_edge_rot(&edge->next[D2D_EDGE_NEXT_TOR], e);
    edge->flags = 0;

    return TRUE;
}

static void d2d_cdt_destroy_edge(struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *e)
{
    struct d2d_cdt_edge_ref next, sym, prev;

    d2d_cdt_edge_next_origin(cdt, &next, e);
    if (next.idx != e->idx || next.r != e->r)
    {
        d2d_cdt_edge_prev_origin(cdt, &prev, e);
        d2d_cdt_splice(cdt, e, &prev);
    }

    d2d_cdt_edge_sym(&sym, e);

    d2d_cdt_edge_next_origin(cdt, &next, &sym);
    if (next.idx != sym.idx || next.r != sym.r)
    {
        d2d_cdt_edge_prev_origin(cdt, &prev, &sym);
        d2d_cdt_splice(cdt, &sym, &prev);
    }

    cdt->edges[e->idx].flags |= D2D_CDT_EDGE_FLAG_FREED;
    cdt->edges[e->idx].next[D2D_EDGE_NEXT_ORIGIN].idx = cdt->free_edge;
    cdt->free_edge = e->idx;
}

static BOOL d2d_cdt_connect(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *e,
        const struct d2d_cdt_edge_ref *a, const struct d2d_cdt_edge_ref *b)
{
    struct d2d_cdt_edge_ref tmp;

    if (!d2d_cdt_create_edge(cdt, e))
        return FALSE;
    d2d_cdt_edge_set_origin(cdt, e, d2d_cdt_edge_destination(cdt, a));
    d2d_cdt_edge_set_destination(cdt, e, d2d_cdt_edge_origin(cdt, b));
    d2d_cdt_edge_next_left(cdt, &tmp, a);
    d2d_cdt_splice(cdt, e, &tmp);
    d2d_cdt_edge_sym(&tmp, e);
    d2d_cdt_splice(cdt, &tmp, b);

    return TRUE;
}

static BOOL d2d_cdt_merge(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *left_outer,
        struct d2d_cdt_edge_ref *left_inner, struct d2d_cdt_edge_ref *right_inner,
        struct d2d_cdt_edge_ref *right_outer)
{
    struct d2d_cdt_edge_ref base_edge, tmp;

    /* Create the base edge between both parts. */
    for (;;)
    {
        if (d2d_cdt_leftof(cdt, d2d_cdt_edge_origin(cdt, right_inner), left_inner))
        {
            d2d_cdt_edge_next_left(cdt, left_inner, left_inner);
        }
        else if (d2d_cdt_rightof(cdt, d2d_cdt_edge_origin(cdt, left_inner), right_inner))
        {
            d2d_cdt_edge_sym(&tmp, right_inner);
            d2d_cdt_edge_next_origin(cdt, right_inner, &tmp);
        }
        else
        {
            break;
        }
    }

    d2d_cdt_edge_sym(&tmp, right_inner);
    if (!d2d_cdt_connect(cdt, &base_edge, &tmp, left_inner))
        return FALSE;
    if (d2d_cdt_edge_origin(cdt, left_inner) == d2d_cdt_edge_origin(cdt, left_outer))
        d2d_cdt_edge_sym(left_outer, &base_edge);
    if (d2d_cdt_edge_origin(cdt, right_inner) == d2d_cdt_edge_origin(cdt, right_outer))
        *right_outer = base_edge;

    for (;;)
    {
        struct d2d_cdt_edge_ref left_candidate, right_candidate, sym_base_edge;
        BOOL left_valid, right_valid;

        /* Find the left candidate. */
        d2d_cdt_edge_sym(&sym_base_edge, &base_edge);
        d2d_cdt_edge_next_origin(cdt, &left_candidate, &sym_base_edge);
        if ((left_valid = d2d_cdt_leftof(cdt, d2d_cdt_edge_destination(cdt, &left_candidate), &sym_base_edge)))
        {
            d2d_cdt_edge_next_origin(cdt, &tmp, &left_candidate);
            while (d2d_cdt_edge_destination(cdt, &tmp) != d2d_cdt_edge_destination(cdt, &sym_base_edge)
                    && d2d_cdt_incircle(cdt,
                    d2d_cdt_edge_origin(cdt, &sym_base_edge), d2d_cdt_edge_destination(cdt, &sym_base_edge),
                    d2d_cdt_edge_destination(cdt, &left_candidate), d2d_cdt_edge_destination(cdt, &tmp)))
            {
                d2d_cdt_destroy_edge(cdt, &left_candidate);
                left_candidate = tmp;
                d2d_cdt_edge_next_origin(cdt, &tmp, &left_candidate);
            }
        }
        d2d_cdt_edge_sym(&left_candidate, &left_candidate);

        /* Find the right candidate. */
        d2d_cdt_edge_prev_origin(cdt, &right_candidate, &base_edge);
        if ((right_valid = d2d_cdt_rightof(cdt, d2d_cdt_edge_destination(cdt, &right_candidate), &base_edge)))
        {
            d2d_cdt_edge_prev_origin(cdt, &tmp, &right_candidate);
            while (d2d_cdt_edge_destination(cdt, &tmp) != d2d_cdt_edge_destination(cdt, &base_edge)
                    && d2d_cdt_incircle(cdt,
                    d2d_cdt_edge_origin(cdt, &sym_base_edge), d2d_cdt_edge_destination(cdt, &sym_base_edge),
                    d2d_cdt_edge_destination(cdt, &right_candidate), d2d_cdt_edge_destination(cdt, &tmp)))
            {
                d2d_cdt_destroy_edge(cdt, &right_candidate);
                right_candidate = tmp;
                d2d_cdt_edge_prev_origin(cdt, &tmp, &right_candidate);
            }
        }

        if (!left_valid && !right_valid)
            break;

        /* Connect the appropriate candidate with the base edge. */
        if (!left_valid || (right_valid && d2d_cdt_incircle(cdt,
                d2d_cdt_edge_origin(cdt, &left_candidate), d2d_cdt_edge_destination(cdt, &left_candidate),
                d2d_cdt_edge_origin(cdt, &right_candidate), d2d_cdt_edge_destination(cdt, &right_candidate))))
        {
            if (!d2d_cdt_connect(cdt, &base_edge, &right_candidate, &sym_base_edge))
                return FALSE;
        }
        else
        {
            if (!d2d_cdt_connect(cdt, &base_edge, &sym_base_edge, &left_candidate))
                return FALSE;
        }
    }

    return TRUE;
}

/* Create a Delaunay triangulation from a set of vertices. This is an
 * implementation of the divide-and-conquer algorithm described by Guibas and
 * Stolfi. Should be called with at least two vertices. */
static BOOL d2d_cdt_triangulate(struct d2d_cdt *cdt, size_t start_vertex, size_t vertex_count,
        struct d2d_cdt_edge_ref *left_edge, struct d2d_cdt_edge_ref *right_edge)
{
    struct d2d_cdt_edge_ref left_inner, left_outer, right_inner, right_outer, tmp;
    size_t cut;

    /* Only two vertices, create a single edge. */
    if (vertex_count == 2)
    {
        struct d2d_cdt_edge_ref a;

        if (!d2d_cdt_create_edge(cdt, &a))
            return FALSE;
        d2d_cdt_edge_set_origin(cdt, &a, start_vertex);
        d2d_cdt_edge_set_destination(cdt, &a, start_vertex + 1);

        *left_edge = a;
        d2d_cdt_edge_sym(right_edge, &a);

        return TRUE;
    }

    /* Three vertices, create a triangle. */
    if (vertex_count == 3)
    {
        struct d2d_cdt_edge_ref a, b, c;
        float det;

        if (!d2d_cdt_create_edge(cdt, &a))
            return FALSE;
        if (!d2d_cdt_create_edge(cdt, &b))
            return FALSE;
        d2d_cdt_edge_sym(&tmp, &a);
        d2d_cdt_splice(cdt, &tmp, &b);

        d2d_cdt_edge_set_origin(cdt, &a, start_vertex);
        d2d_cdt_edge_set_destination(cdt, &a, start_vertex + 1);
        d2d_cdt_edge_set_origin(cdt, &b, start_vertex + 1);
        d2d_cdt_edge_set_destination(cdt, &b, start_vertex + 2);

        det = d2d_cdt_ccw(cdt, start_vertex, start_vertex + 1, start_vertex + 2);
        if (det != 0.0f && !d2d_cdt_connect(cdt, &c, &b, &a))
            return FALSE;

        if (det < 0.0f)
        {
            d2d_cdt_edge_sym(left_edge, &c);
            *right_edge = c;
        }
        else
        {
            *left_edge = a;
            d2d_cdt_edge_sym(right_edge, &b);
        }

        return TRUE;
    }

    /* More than tree vertices, divide. */
    cut = vertex_count / 2;
    if (!d2d_cdt_triangulate(cdt, start_vertex, cut, &left_outer, &left_inner))
        return FALSE;
    if (!d2d_cdt_triangulate(cdt, start_vertex + cut, vertex_count - cut, &right_inner, &right_outer))
        return FALSE;
    /* Merge the left and right parts. */
    if (!d2d_cdt_merge(cdt, &left_outer, &left_inner, &right_inner, &right_outer))
        return FALSE;

    *left_edge = left_outer;
    *right_edge = right_outer;
    return TRUE;
}

static int d2d_cdt_compare_vertices(const void *a, const void *b)
{
    const D2D1_POINT_2F *p0 = a;
    const D2D1_POINT_2F *p1 = b;
    float diff = p0->x - p1->x;

    if (diff == 0.0f)
        diff = p0->y - p1->y;

    return diff == 0.0f ? 0 : (diff > 0.0f ? 1 : -1);
}

/* Determine whether a given point is inside the geometry, using the current
 * fill mode rule. */
static BOOL d2d_path_geometry_point_inside(const struct d2d_geometry *geometry, const D2D1_POINT_2F *probe)
{
    const D2D1_POINT_2F *p0, *p1;
    D2D1_POINT_2F v_p, v_probe;
    unsigned int score;
    size_t i, j;

    for (i = 0, score = 0; i < geometry->u.path.figure_count; ++i)
    {
        const struct d2d_figure *figure = &geometry->u.path.figures[i];

        p0 = &figure->vertices[figure->vertex_count - 1];
        for (j = 0; j < figure->vertex_count; p0 = p1, ++j)
        {
            p1 = &figure->vertices[j];
            d2d_point_subtract(&v_p, p1, p0);
            d2d_point_subtract(&v_probe, probe, p0);

            if ((probe->y < p0->y) != (probe->y < p1->y) && v_probe.x < v_p.x * (v_probe.y / v_p.y))
            {
                if (geometry->u.path.fill_mode == D2D1_FILL_MODE_ALTERNATE || (probe->y < p0->y))
                    ++score;
                else
                    --score;
            }
        }
    }

    return geometry->u.path.fill_mode == D2D1_FILL_MODE_ALTERNATE ? score & 1 : score;
}

static BOOL d2d_path_geometry_add_face(struct d2d_geometry *geometry, const struct d2d_cdt *cdt,
        const struct d2d_cdt_edge_ref *base_edge)
{
    struct d2d_cdt_edge_ref tmp;
    struct d2d_face *face;
    D2D1_POINT_2F probe;

    if (cdt->edges[base_edge->idx].flags & D2D_CDT_EDGE_FLAG_VISITED(base_edge->r))
        return TRUE;

    if (!d2d_array_reserve((void **)&geometry->faces, &geometry->faces_size,
            geometry->face_count + 1, sizeof(*geometry->faces)))
    {
        ERR("Failed to grow faces array.\n");
        return FALSE;
    }

    face = &geometry->faces[geometry->face_count];

    /* It may seem tempting to use the center of the face as probe origin, but
     * multiplying by powers of two works much better for preserving accuracy. */

    tmp = *base_edge;
    cdt->edges[tmp.idx].flags |= D2D_CDT_EDGE_FLAG_VISITED(tmp.r);
    face->v[0] = d2d_cdt_edge_origin(cdt, &tmp);
    probe.x = cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].x * 0.25f;
    probe.y = cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].y * 0.25f;

    d2d_cdt_edge_next_left(cdt, &tmp, &tmp);
    cdt->edges[tmp.idx].flags |= D2D_CDT_EDGE_FLAG_VISITED(tmp.r);
    face->v[1] = d2d_cdt_edge_origin(cdt, &tmp);
    probe.x += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].x * 0.25f;
    probe.y += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].y * 0.25f;

    d2d_cdt_edge_next_left(cdt, &tmp, &tmp);
    cdt->edges[tmp.idx].flags |= D2D_CDT_EDGE_FLAG_VISITED(tmp.r);
    face->v[2] = d2d_cdt_edge_origin(cdt, &tmp);
    probe.x += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].x * 0.50f;
    probe.y += cdt->vertices[d2d_cdt_edge_origin(cdt, &tmp)].y * 0.50f;

    d2d_cdt_edge_next_left(cdt, &tmp, &tmp);
    if (tmp.idx == base_edge->idx && d2d_path_geometry_point_inside(geometry, &probe))
        ++geometry->face_count;

    return TRUE;
}

static BOOL d2d_cdt_generate_faces(const struct d2d_cdt *cdt, struct d2d_geometry *geometry)
{
    struct d2d_cdt_edge_ref base_edge;
    size_t i;

    for (i = 0; i < cdt->edge_count; ++i)
    {
        if (cdt->edges[i].flags & D2D_CDT_EDGE_FLAG_FREED)
            continue;

        base_edge.idx = i;
        base_edge.r = 0;
        if (!d2d_path_geometry_add_face(geometry, cdt, &base_edge))
            goto fail;
        d2d_cdt_edge_sym(&base_edge, &base_edge);
        if (!d2d_path_geometry_add_face(geometry, cdt, &base_edge))
            goto fail;
    }

    return TRUE;

fail:
    HeapFree(GetProcessHeap(), 0, geometry->faces);
    geometry->faces = NULL;
    geometry->faces_size = 0;
    geometry->face_count = 0;
    return FALSE;
}

static BOOL d2d_cdt_fixup(struct d2d_cdt *cdt, const struct d2d_cdt_edge_ref *base_edge)
{
    struct d2d_cdt_edge_ref candidate, next, new_base;
    unsigned int count = 0;

    d2d_cdt_edge_next_left(cdt, &next, base_edge);
    if (next.idx == base_edge->idx)
    {
        ERR("Degenerate face.\n");
        return FALSE;
    }

    candidate = next;
    while (d2d_cdt_edge_destination(cdt, &next) != d2d_cdt_edge_origin(cdt, base_edge))
    {
        if (d2d_cdt_incircle(cdt, d2d_cdt_edge_origin(cdt, base_edge), d2d_cdt_edge_destination(cdt, base_edge),
                d2d_cdt_edge_destination(cdt, &candidate), d2d_cdt_edge_destination(cdt, &next)))
            candidate = next;
        d2d_cdt_edge_next_left(cdt, &next, &next);
        ++count;
    }

    if (count > 1)
    {
        if (!d2d_cdt_connect(cdt, &new_base, &candidate, base_edge))
            return FALSE;
        if (!d2d_cdt_fixup(cdt, &new_base))
            return FALSE;
        d2d_cdt_edge_sym(&new_base, &new_base);
        if (!d2d_cdt_fixup(cdt, &new_base))
            return FALSE;
    }

    return TRUE;
}

static void d2d_cdt_cut_edges(struct d2d_cdt *cdt, struct d2d_cdt_edge_ref *end_edge,
        const struct d2d_cdt_edge_ref *base_edge, size_t start_vertex, size_t end_vertex)
{
    struct d2d_cdt_edge_ref next;

    d2d_cdt_edge_next_left(cdt, &next, base_edge);
    if (d2d_cdt_edge_destination(cdt, &next) == end_vertex)
    {
        *end_edge = next;
        return;
    }

    if (d2d_cdt_ccw(cdt, d2d_cdt_edge_destination(cdt, &next), end_vertex, start_vertex) > 0.0f)
        d2d_cdt_edge_next_left(cdt, &next, &next);

    d2d_cdt_edge_sym(&next, &next);
    d2d_cdt_cut_edges(cdt, end_edge, &next, start_vertex, end_vertex);
    d2d_cdt_destroy_edge(cdt, &next);
}

static BOOL d2d_cdt_insert_segment(struct d2d_cdt *cdt, struct d2d_geometry *geometry,
        const struct d2d_cdt_edge_ref *origin, size_t end_vertex)
{
    struct d2d_cdt_edge_ref base_edge, current, next, target;

    for (current = *origin;; current = next)
    {
        d2d_cdt_edge_next_origin(cdt, &next, &current);

        if (d2d_cdt_edge_destination(cdt, &current) == end_vertex)
            return TRUE;

        if (d2d_cdt_rightof(cdt, end_vertex, &next) && d2d_cdt_leftof(cdt, end_vertex, &current))
        {
            d2d_cdt_edge_next_left(cdt, &base_edge, &current);

            d2d_cdt_edge_sym(&base_edge, &base_edge);
            d2d_cdt_cut_edges(cdt, &target, &base_edge, d2d_cdt_edge_origin(cdt, origin), end_vertex);
            d2d_cdt_destroy_edge(cdt, &base_edge);

            if (!d2d_cdt_connect(cdt, &base_edge, &target, &current))
                return FALSE;
            if (!d2d_cdt_fixup(cdt, &base_edge))
                return FALSE;
            d2d_cdt_edge_sym(&base_edge, &base_edge);
            if (!d2d_cdt_fixup(cdt, &base_edge))
                return FALSE;

            return TRUE;
        }

        if (next.idx == origin->idx)
        {
            ERR("Triangle not found.\n");
            return FALSE;
        }
    }
}

static BOOL d2d_cdt_insert_segments(struct d2d_cdt *cdt, struct d2d_geometry *geometry)
{
    size_t start_vertex, end_vertex, i, j, k;
    const struct d2d_figure *figure;
    struct d2d_cdt_edge_ref edge;
    const D2D1_POINT_2F *p;

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        figure = &geometry->u.path.figures[i];

        p = bsearch(&figure->vertices[figure->vertex_count - 1], cdt->vertices,
                geometry->vertex_count, sizeof(*p), d2d_cdt_compare_vertices);
        start_vertex = p - cdt->vertices;

        for (j = 0; j < figure->vertex_count; start_vertex = end_vertex, ++j)
        {
            p = bsearch(&figure->vertices[j], cdt->vertices,
                    geometry->vertex_count, sizeof(*p), d2d_cdt_compare_vertices);
            end_vertex = p - cdt->vertices;

            if (start_vertex == end_vertex)
                continue;

            for (k = 0; k < cdt->edge_count; ++k)
            {
                if (cdt->edges[k].flags & D2D_CDT_EDGE_FLAG_FREED)
                    continue;

                edge.idx = k;
                edge.r = 0;

                if (d2d_cdt_edge_origin(cdt, &edge) == start_vertex)
                {
                    if (!d2d_cdt_insert_segment(cdt, geometry, &edge, end_vertex))
                        return FALSE;
                    break;
                }
                d2d_cdt_edge_sym(&edge, &edge);
                if (d2d_cdt_edge_origin(cdt, &edge) == start_vertex)
                {
                    if (!d2d_cdt_insert_segment(cdt, geometry, &edge, end_vertex))
                        return FALSE;
                    break;
                }
            }
        }
    }

    return TRUE;
}

/* Intersect the geometry's segments with themselves. This uses the
 * straightforward approach of testing everything against everything, but
 * there certainly exist more scalable algorithms for this. */
/* FIXME: Beziers can't currently self-intersect. */
static BOOL d2d_geometry_intersect_self(struct d2d_geometry *geometry)
{
    D2D1_POINT_2F p0, p1, q0, q1, v_p, v_q, v_qp, intersection;
    struct d2d_figure *figure_p, *figure_q;
    size_t i, j, k, l, min_j, min_l, max_l;
    float s, t, det;

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        figure_p = &geometry->u.path.figures[i];
        p0 = figure_p->vertices[figure_p->vertex_count - 1];
        min_j = 0;
        min_l = 0;
        for (k = 0; k < figure_p->vertex_count; p0 = p1, ++k)
        {
            p1 = figure_p->vertices[k];
            d2d_point_subtract(&v_p, &p1, &p0);
            for (j = min_j, min_j = 0, l = min_l, min_l = 0; j < i || (j == i && k); ++j, l = 0)
            {
                figure_q = &geometry->u.path.figures[j];
                max_l = j == i ? k - 1 : figure_q->vertex_count;
                q0 = figure_q->vertices[l == 0 ? figure_q->vertex_count - 1 : l - 1];
                for (; l < max_l; q0 = q1, ++l)
                {
                    q1 = figure_q->vertices[l];
                    d2d_point_subtract(&v_q, &q1, &q0);
                    d2d_point_subtract(&v_qp, &p0, &q0);

                    det = v_p.x * v_q.y - v_p.y * v_q.x;
                    if (det == 0.0f)
                        continue;

                    s = (v_q.x * v_qp.y - v_q.y * v_qp.x) / det;
                    t = (v_p.x * v_qp.y - v_p.y * v_qp.x) / det;

                    if (s < 0.0f || s > 1.0f || t < 0.0f || t > 1.0f)
                        continue;

                    intersection.x = p0.x + v_p.x * s;
                    intersection.y = p0.y + v_p.y * s;

                    if (t > 0.0f && t < 1.0f)
                    {
                        if (!d2d_figure_insert_vertex(figure_q, l, intersection))
                            return FALSE;
                        if (j == i)
                            ++k;
                        ++max_l;
                        ++l;
                    }

                    if (s > 0.0f && s < 1.0f)
                    {
                        if (!d2d_figure_insert_vertex(figure_p, k, intersection))
                            return FALSE;
                        min_j = j;
                        min_l = l+1;
                        p1 = intersection;
                        d2d_point_subtract(&v_p, &p1, &p0);
                    }
                }
            }
        }
    }

    return TRUE;
}

static HRESULT d2d_path_geometry_triangulate(struct d2d_geometry *geometry)
{
    struct d2d_cdt_edge_ref left_edge, right_edge;
    size_t vertex_count, i, j;
    struct d2d_cdt cdt = {0};
    D2D1_POINT_2F *vertices;

    for (i = 0, vertex_count = 0; i < geometry->u.path.figure_count; ++i)
    {
        vertex_count += geometry->u.path.figures[i].vertex_count;
    }

    if (vertex_count < 3)
    {
        WARN("Geometry has %lu vertices.\n", (long)vertex_count);
        return S_OK;
    }

    if (!(vertices = HeapAlloc(GetProcessHeap(), 0, vertex_count * sizeof(*vertices))))
        return E_OUTOFMEMORY;

    for (i = 0, j = 0; i < geometry->u.path.figure_count; ++i)
    {
        memcpy(&vertices[j], geometry->u.path.figures[i].vertices,
                geometry->u.path.figures[i].vertex_count * sizeof(*vertices));
        j += geometry->u.path.figures[i].vertex_count;
    }

    /* Sort vertices, eliminate duplicates. */
    qsort(vertices, vertex_count, sizeof(*vertices), d2d_cdt_compare_vertices);
    for (i = 1; i < vertex_count; ++i)
    {
        if (!memcmp(&vertices[i - 1], &vertices[i], sizeof(*vertices)))
        {
            --vertex_count;
            memmove(&vertices[i], &vertices[i + 1], (vertex_count - i) * sizeof(*vertices));
            --i;
        }
    }

    geometry->vertices = vertices;
    geometry->vertex_count = vertex_count;

    cdt.free_edge = ~0u;
    cdt.vertices = vertices;
    if (!d2d_cdt_triangulate(&cdt, 0, vertex_count, &left_edge, &right_edge))
        goto fail;
    if (!d2d_cdt_insert_segments(&cdt, geometry))
        goto fail;
    if (!d2d_cdt_generate_faces(&cdt, geometry))
        goto fail;

    HeapFree(GetProcessHeap(), 0, cdt.edges);
    return S_OK;

fail:
    geometry->vertices = NULL;
    geometry->vertex_count = 0;
    HeapFree(GetProcessHeap(), 0, vertices);
    HeapFree(GetProcessHeap(), 0, cdt.edges);
    return E_FAIL;
}

static BOOL d2d_path_geometry_add_figure(struct d2d_geometry *geometry)
{
    struct d2d_figure *figure;

    if (!d2d_array_reserve((void **)&geometry->u.path.figures, &geometry->u.path.figures_size,
            geometry->u.path.figure_count + 1, sizeof(*geometry->u.path.figures)))
    {
        ERR("Failed to grow figures array.\n");
        return FALSE;
    }

    figure = &geometry->u.path.figures[geometry->u.path.figure_count];
    memset(figure, 0, sizeof(*figure));

    ++geometry->u.path.figure_count;
    return TRUE;
}

static void d2d_geometry_cleanup(struct d2d_geometry *geometry)
{
    HeapFree(GetProcessHeap(), 0, geometry->beziers);
    HeapFree(GetProcessHeap(), 0, geometry->faces);
    HeapFree(GetProcessHeap(), 0, geometry->vertices);
    ID2D1Factory_Release(geometry->factory);
}

static void d2d_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        const D2D1_MATRIX_3X2_F *transform, const struct ID2D1GeometryVtbl *vtbl)
{
    geometry->ID2D1Geometry_iface.lpVtbl = vtbl;
    geometry->refcount = 1;
    ID2D1Factory_AddRef(geometry->factory = factory);
    geometry->transform = *transform;
}

static inline struct d2d_geometry *impl_from_ID2D1GeometrySink(ID2D1GeometrySink *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, u.path.ID2D1GeometrySink_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_sink_QueryInterface(ID2D1GeometrySink *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1GeometrySink)
            || IsEqualGUID(iid, &IID_ID2D1SimplifiedGeometrySink)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1GeometrySink_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_geometry_sink_AddRef(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Geometry_AddRef(&geometry->ID2D1Geometry_iface);
}

static ULONG STDMETHODCALLTYPE d2d_geometry_sink_Release(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Geometry_Release(&geometry->ID2D1Geometry_iface);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_SetFillMode(ID2D1GeometrySink *iface, D2D1_FILL_MODE mode)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    geometry->u.path.fill_mode = mode;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_SetSegmentFlags(ID2D1GeometrySink *iface, D2D1_PATH_SEGMENT flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_BeginFigure(ID2D1GeometrySink *iface,
        D2D1_POINT_2F start_point, D2D1_FIGURE_BEGIN figure_begin)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p, start_point {%.8e, %.8e}, figure_begin %#x.\n",
            iface, start_point.x, start_point.y, figure_begin);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_OPEN)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    if (figure_begin != D2D1_FIGURE_BEGIN_FILLED)
        FIXME("Ignoring figure_begin %#x.\n", figure_begin);

    if (!d2d_path_geometry_add_figure(geometry))
    {
        ERR("Failed to add figure.\n");
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    if (!d2d_figure_add_vertex(&geometry->u.path.figures[geometry->u.path.figure_count - 1], start_point))
        ERR("Failed to add vertex.\n");

    geometry->u.path.state = D2D_GEOMETRY_STATE_FIGURE;
    ++geometry->u.path.segment_count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddLines(ID2D1GeometrySink *iface,
        const D2D1_POINT_2F *points, UINT32 count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    unsigned int i;

    TRACE("iface %p, points %p, count %u.\n", iface, points, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    for (i = 0; i < count; ++i)
    {
        if (!d2d_figure_add_vertex(&geometry->u.path.figures[geometry->u.path.figure_count - 1], points[i]))
        {
            ERR("Failed to add vertex.\n");
            return;
        }
    }

    geometry->u.path.segment_count += count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddBeziers(ID2D1GeometrySink *iface,
        const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    D2D1_POINT_2F p;
    unsigned int i;

    TRACE("iface %p, beziers %p, count %u.\n", iface, beziers, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    for (i = 0; i < count; ++i)
    {
        /* FIXME: This tries to approximate a cubic bezier with a quadratic one. */
        p.x = (beziers[i].point1.x + beziers[i].point2.x) * 0.75f;
        p.y = (beziers[i].point1.y + beziers[i].point2.y) * 0.75f;
        p.x -= (figure->vertices[figure->vertex_count - 1].x + beziers[i].point3.x) * 0.25f;
        p.y -= (figure->vertices[figure->vertex_count - 1].y + beziers[i].point3.y) * 0.25f;
        if (!d2d_figure_add_bezier(figure, figure->vertices[figure->vertex_count - 1], p, beziers[i].point3))
        {
            ERR("Failed to add bezier.\n");
            return;
        }
    }

    geometry->u.path.segment_count += count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_EndFigure(ID2D1GeometrySink *iface, D2D1_FIGURE_END figure_end)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p, figure_end %#x.\n", iface, figure_end);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    if (figure_end != D2D1_FIGURE_END_CLOSED)
        FIXME("Ignoring figure_end %#x.\n", figure_end);

    geometry->u.path.state = D2D_GEOMETRY_STATE_OPEN;
}

static void d2d_path_geometry_free_figures(struct d2d_geometry *geometry)
{
    size_t i;

    if (!geometry->u.path.figures)
        return;

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        HeapFree(GetProcessHeap(), 0, geometry->u.path.figures[i].beziers);
        HeapFree(GetProcessHeap(), 0, geometry->u.path.figures[i].vertices);
    }
    HeapFree(GetProcessHeap(), 0, geometry->u.path.figures);
    geometry->u.path.figures = NULL;
    geometry->u.path.figures_size = 0;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_sink_Close(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    HRESULT hr = E_FAIL;
    size_t i, start;

    TRACE("iface %p.\n", iface);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_OPEN)
    {
        if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return D2DERR_WRONG_STATE;
    }
    geometry->u.path.state = D2D_GEOMETRY_STATE_CLOSED;

    if (!d2d_geometry_intersect_self(geometry))
        goto done;
    if (FAILED(hr = d2d_path_geometry_triangulate(geometry)))
        goto done;

    for (i = 0; i < geometry->u.path.figure_count; ++i)
    {
        geometry->bezier_count += geometry->u.path.figures[i].bezier_count;
    }

    if (!(geometry->beziers = HeapAlloc(GetProcessHeap(), 0,
            geometry->bezier_count * sizeof(*geometry->beziers))))
    {
        ERR("Failed to allocate beziers array.\n");
        geometry->bezier_count = 0;
        hr = E_OUTOFMEMORY;
        goto done;
    }

    for (i = 0, start = 0; i < geometry->u.path.figure_count; ++i)
    {
        struct d2d_figure *figure = &geometry->u.path.figures[i];
        if (figure->bezier_count)
        {
            memcpy(&geometry->beziers[start], figure->beziers,
                    figure->bezier_count * sizeof(*figure->beziers));
            start += figure->bezier_count;
        }
    }

done:
    d2d_path_geometry_free_figures(geometry);
    if (FAILED(hr))
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
    return hr;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddLine(ID2D1GeometrySink *iface, D2D1_POINT_2F point)
{
    TRACE("iface %p, point {%.8e, %.8e}.\n", iface, point.x, point.y);

    d2d_geometry_sink_AddLines(iface, &point, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddBezier(ID2D1GeometrySink *iface, const D2D1_BEZIER_SEGMENT *bezier)
{
    TRACE("iface %p, bezier %p.\n", iface, bezier);

    d2d_geometry_sink_AddBeziers(iface, bezier, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddQuadraticBezier(ID2D1GeometrySink *iface,
        const D2D1_QUADRATIC_BEZIER_SEGMENT *bezier)
{
    TRACE("iface %p, bezier %p.\n", iface, bezier);

    ID2D1GeometrySink_AddQuadraticBeziers(iface, bezier, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddQuadraticBeziers(ID2D1GeometrySink *iface,
        const D2D1_QUADRATIC_BEZIER_SEGMENT *beziers, UINT32 bezier_count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);
    struct d2d_figure *figure = &geometry->u.path.figures[geometry->u.path.figure_count - 1];
    unsigned int i;

    TRACE("iface %p, beziers %p, bezier_count %u.\n", iface, beziers, bezier_count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    for (i = 0; i < bezier_count; ++i)
    {
        if (!d2d_figure_add_bezier(figure, figure->vertices[figure->vertex_count - 1],
                beziers[i].point1, beziers[i].point2))
        {
            ERR("Failed to add bezier.\n");
            return;
        }
    }

    geometry->u.path.segment_count += bezier_count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddArc(ID2D1GeometrySink *iface, const D2D1_ARC_SEGMENT *arc)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, arc %p stub!\n", iface, arc);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    if (!d2d_figure_add_vertex(&geometry->u.path.figures[geometry->u.path.figure_count - 1], arc->point))
    {
        ERR("Failed to add vertex.\n");
        return;
    }

    ++geometry->u.path.segment_count;
}

static const struct ID2D1GeometrySinkVtbl d2d_geometry_sink_vtbl =
{
    d2d_geometry_sink_QueryInterface,
    d2d_geometry_sink_AddRef,
    d2d_geometry_sink_Release,
    d2d_geometry_sink_SetFillMode,
    d2d_geometry_sink_SetSegmentFlags,
    d2d_geometry_sink_BeginFigure,
    d2d_geometry_sink_AddLines,
    d2d_geometry_sink_AddBeziers,
    d2d_geometry_sink_EndFigure,
    d2d_geometry_sink_Close,
    d2d_geometry_sink_AddLine,
    d2d_geometry_sink_AddBezier,
    d2d_geometry_sink_AddQuadraticBezier,
    d2d_geometry_sink_AddQuadraticBeziers,
    d2d_geometry_sink_AddArc,
};

static inline struct d2d_geometry *impl_from_ID2D1PathGeometry(ID2D1PathGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_QueryInterface(ID2D1PathGeometry *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1PathGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1PathGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_path_geometry_AddRef(ID2D1PathGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_path_geometry_Release(ID2D1PathGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        d2d_path_geometry_free_figures(geometry);
        d2d_geometry_cleanup(geometry);
        HeapFree(GetProcessHeap(), 0, geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_path_geometry_GetFactory(ID2D1PathGeometry *iface, ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetBounds(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetWidenedBounds(ID2D1PathGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_StrokeContainsPoint(ID2D1PathGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, stroke_width %.8e, stroke_style %p, "
            "transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_FillContainsPoint(ID2D1PathGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_CompareWithGeometry(ID2D1PathGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Simplify(ID2D1PathGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Tessellate(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_CombineWithGeometry(ID2D1PathGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Outline(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputeArea(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputeLength(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputePointAtLength(ID2D1PathGeometry *iface, float length,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point, D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Widen(ID2D1PathGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Open(ID2D1PathGeometry *iface, ID2D1GeometrySink **sink)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, sink %p.\n", iface, sink);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_INITIAL)
        return D2DERR_WRONG_STATE;

    *sink = &geometry->u.path.ID2D1GeometrySink_iface;
    ID2D1GeometrySink_AddRef(*sink);

    geometry->u.path.state = D2D_GEOMETRY_STATE_OPEN;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Stream(ID2D1PathGeometry *iface, ID2D1GeometrySink *sink)
{
    FIXME("iface %p, sink %p stub!\n", iface, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetSegmentCount(ID2D1PathGeometry *iface, UINT32 *count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    *count = geometry->u.path.segment_count;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetFigureCount(ID2D1PathGeometry *iface, UINT32 *count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    *count = geometry->u.path.figure_count;

    return S_OK;
}

static const struct ID2D1PathGeometryVtbl d2d_path_geometry_vtbl =
{
    d2d_path_geometry_QueryInterface,
    d2d_path_geometry_AddRef,
    d2d_path_geometry_Release,
    d2d_path_geometry_GetFactory,
    d2d_path_geometry_GetBounds,
    d2d_path_geometry_GetWidenedBounds,
    d2d_path_geometry_StrokeContainsPoint,
    d2d_path_geometry_FillContainsPoint,
    d2d_path_geometry_CompareWithGeometry,
    d2d_path_geometry_Simplify,
    d2d_path_geometry_Tessellate,
    d2d_path_geometry_CombineWithGeometry,
    d2d_path_geometry_Outline,
    d2d_path_geometry_ComputeArea,
    d2d_path_geometry_ComputeLength,
    d2d_path_geometry_ComputePointAtLength,
    d2d_path_geometry_Widen,
    d2d_path_geometry_Open,
    d2d_path_geometry_Stream,
    d2d_path_geometry_GetSegmentCount,
    d2d_path_geometry_GetFigureCount,
};

void d2d_path_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory)
{
    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_path_geometry_vtbl);
    geometry->u.path.ID2D1GeometrySink_iface.lpVtbl = &d2d_geometry_sink_vtbl;
}

static inline struct d2d_geometry *impl_from_ID2D1RectangleGeometry(ID2D1RectangleGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_QueryInterface(ID2D1RectangleGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RectangleGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RectangleGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_rectangle_geometry_AddRef(ID2D1RectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_rectangle_geometry_Release(ID2D1RectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        d2d_geometry_cleanup(geometry);
        HeapFree(GetProcessHeap(), 0, geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_rectangle_geometry_GetFactory(ID2D1RectangleGeometry *iface, ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_GetBounds(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_GetWidenedBounds(ID2D1RectangleGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_StrokeContainsPoint(ID2D1RectangleGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, stroke_width %.8e, stroke_style %p, "
            "transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_FillContainsPoint(ID2D1RectangleGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_CompareWithGeometry(ID2D1RectangleGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Simplify(ID2D1RectangleGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Tessellate(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_CombineWithGeometry(ID2D1RectangleGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Outline(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputeArea(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputeLength(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputePointAtLength(ID2D1RectangleGeometry *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Widen(ID2D1RectangleGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_rectangle_geometry_GetRect(ID2D1RectangleGeometry *iface, D2D1_RECT_F *rect)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);

    TRACE("iface %p, rect %p.\n", iface, rect);

    *rect = geometry->u.rectangle.rect;
}

static const struct ID2D1RectangleGeometryVtbl d2d_rectangle_geometry_vtbl =
{
    d2d_rectangle_geometry_QueryInterface,
    d2d_rectangle_geometry_AddRef,
    d2d_rectangle_geometry_Release,
    d2d_rectangle_geometry_GetFactory,
    d2d_rectangle_geometry_GetBounds,
    d2d_rectangle_geometry_GetWidenedBounds,
    d2d_rectangle_geometry_StrokeContainsPoint,
    d2d_rectangle_geometry_FillContainsPoint,
    d2d_rectangle_geometry_CompareWithGeometry,
    d2d_rectangle_geometry_Simplify,
    d2d_rectangle_geometry_Tessellate,
    d2d_rectangle_geometry_CombineWithGeometry,
    d2d_rectangle_geometry_Outline,
    d2d_rectangle_geometry_ComputeArea,
    d2d_rectangle_geometry_ComputeLength,
    d2d_rectangle_geometry_ComputePointAtLength,
    d2d_rectangle_geometry_Widen,
    d2d_rectangle_geometry_GetRect,
};

HRESULT d2d_rectangle_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory, const D2D1_RECT_F *rect)
{
    d2d_geometry_init(geometry, factory, &identity, (ID2D1GeometryVtbl *)&d2d_rectangle_geometry_vtbl);
    geometry->u.rectangle.rect = *rect;

    if (!(geometry->vertices = HeapAlloc(GetProcessHeap(), 0, 4 * sizeof(*geometry->vertices))))
    {
        d2d_geometry_cleanup(geometry);
        return E_OUTOFMEMORY;
    }
    geometry->vertex_count = 4;
    if (!d2d_array_reserve((void **)&geometry->faces, &geometry->faces_size, 2, sizeof(*geometry->faces)))
    {
        d2d_geometry_cleanup(geometry);
        return E_OUTOFMEMORY;
    }
    geometry->face_count = 2;

    geometry->vertices[0].x = min(rect->left, rect->right);
    geometry->vertices[0].y = min(rect->top, rect->bottom);
    geometry->vertices[1].x = min(rect->left, rect->right);
    geometry->vertices[1].y = max(rect->top, rect->bottom);
    geometry->vertices[2].x = max(rect->left, rect->right);
    geometry->vertices[2].y = min(rect->top, rect->bottom);
    geometry->vertices[3].x = max(rect->left, rect->right);
    geometry->vertices[3].y = max(rect->top, rect->bottom);

    geometry->faces[0].v[0] = 0;
    geometry->faces[0].v[1] = 2;
    geometry->faces[0].v[2] = 1;
    geometry->faces[1].v[0] = 1;
    geometry->faces[1].v[1] = 2;
    geometry->faces[1].v[2] = 3;

    return S_OK;
}

static inline struct d2d_geometry *impl_from_ID2D1TransformedGeometry(ID2D1TransformedGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_QueryInterface(ID2D1TransformedGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1TransformedGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1TransformedGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_transformed_geometry_AddRef(ID2D1TransformedGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_transformed_geometry_Release(ID2D1TransformedGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        geometry->beziers = NULL;
        geometry->faces = NULL;
        geometry->vertices = NULL;
        ID2D1Geometry_Release(geometry->u.transformed.src_geometry);
        d2d_geometry_cleanup(geometry);
        HeapFree(GetProcessHeap(), 0, geometry);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_transformed_geometry_GetFactory(ID2D1TransformedGeometry *iface,
        ID2D1Factory **factory)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = geometry->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_GetBounds(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_GetWidenedBounds(ID2D1TransformedGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_StrokeContainsPoint(ID2D1TransformedGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, stroke_width %.8e, stroke_style %p, "
            "transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_FillContainsPoint(ID2D1TransformedGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_CompareWithGeometry(ID2D1TransformedGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Simplify(ID2D1TransformedGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Tessellate(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_CombineWithGeometry(ID2D1TransformedGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Outline(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_ComputeArea(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_ComputeLength(ID2D1TransformedGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_ComputePointAtLength(ID2D1TransformedGeometry *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_transformed_geometry_Widen(ID2D1TransformedGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_transformed_geometry_GetSourceGeometry(ID2D1TransformedGeometry *iface,
        ID2D1Geometry **src_geometry)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);

    TRACE("iface %p, src_geometry %p.\n", iface, src_geometry);

    ID2D1Geometry_AddRef(*src_geometry = geometry->u.transformed.src_geometry);
}

static void STDMETHODCALLTYPE d2d_transformed_geometry_GetTransform(ID2D1TransformedGeometry *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_geometry *geometry = impl_from_ID2D1TransformedGeometry(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = geometry->transform;
}

static const struct ID2D1TransformedGeometryVtbl d2d_transformed_geometry_vtbl =
{
    d2d_transformed_geometry_QueryInterface,
    d2d_transformed_geometry_AddRef,
    d2d_transformed_geometry_Release,
    d2d_transformed_geometry_GetFactory,
    d2d_transformed_geometry_GetBounds,
    d2d_transformed_geometry_GetWidenedBounds,
    d2d_transformed_geometry_StrokeContainsPoint,
    d2d_transformed_geometry_FillContainsPoint,
    d2d_transformed_geometry_CompareWithGeometry,
    d2d_transformed_geometry_Simplify,
    d2d_transformed_geometry_Tessellate,
    d2d_transformed_geometry_CombineWithGeometry,
    d2d_transformed_geometry_Outline,
    d2d_transformed_geometry_ComputeArea,
    d2d_transformed_geometry_ComputeLength,
    d2d_transformed_geometry_ComputePointAtLength,
    d2d_transformed_geometry_Widen,
    d2d_transformed_geometry_GetSourceGeometry,
    d2d_transformed_geometry_GetTransform,
};

void d2d_transformed_geometry_init(struct d2d_geometry *geometry, ID2D1Factory *factory,
        ID2D1Geometry *src_geometry, const D2D_MATRIX_3X2_F *transform)
{
    struct d2d_geometry *src_impl;

    d2d_geometry_init(geometry, factory, transform, (ID2D1GeometryVtbl *)&d2d_transformed_geometry_vtbl);
    ID2D1Geometry_AddRef(geometry->u.transformed.src_geometry = src_geometry);
    src_impl = unsafe_impl_from_ID2D1Geometry(src_geometry);
    geometry->vertices = src_impl->vertices;
    geometry->vertex_count = src_impl->vertex_count;
    geometry->faces = src_impl->faces;
    geometry->face_count = src_impl->face_count;
    geometry->beziers = src_impl->beziers;
    geometry->bezier_count = src_impl->bezier_count;
}

struct d2d_geometry *unsafe_impl_from_ID2D1Geometry(ID2D1Geometry *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_path_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_rectangle_geometry_vtbl
            || iface->lpVtbl == (const ID2D1GeometryVtbl *)&d2d_transformed_geometry_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

#include "Subface.hpp"

namespace subface {

void Vertex::ComputeStartFaceAndBoundary()
{
    const Face *f = start_face, *f_last = nullptr;

    /* Use `f_next == f_last` to detect opposite neighbor triangles. */
    do {
        const Face* fn = f->PrevNeighbor(this);
        // Check the comments in `Vertex::TraverseFaces()` for the reason of checking `fn == f_last`.
        // Besides, traversal here can stop as long as `fn == nullptr`. This is the reason of checking `fn`.
        if (fn && fn == f_last)
            fn = f->NextNeighbor(this);
        f_last = f;
        f = fn;
    } while (f && f != start_face);

    /* Use `OppositeNeighbor()` to detect opposite neighbor triangles. */
    // bool f_opposite = false; // Always treat the first face as non-opposite.
    // do {
    //     int fn_id = PREV(f->VertexId(v));
    //     // Check the comments in `Vertex::TraverseFaces()` for the reason of checking `f_opposite`.
    //     if (f_opposite)
    //         fn_id = NEXT(fn_id);
    //     const Face* fn = f->neighbors[fn_id];
    //     f_opposite ^= f->OppositeNeighbor(fn_id);
    //     f_last = f;
    //     f = fn;
    // } while (f && f != v->start_face);

    if (f == nullptr) {
        boundary = true;
        // Update `start_face`, especially for boundary vertexes.
        start_face = f_last;
    }
}

void Vertex::ComputeValence()
{
    valence = boundary ? 1 : 0;
    TraverseFaces([&](const Face*) {
        ++valence;
    });
}

void Vertex::ComputeRegular()
{
    //   \ /   //
    // -- * -- //
    //   / \   //
    if (!boundary && valence == 6)
        regular = true;
    //   \ /   //
    // -- * -- //
    else if (boundary && valence == 4)
        regular = true;
    else
        regular = false;
}

std::vector<const Vertex*> Vertex::OneRing() const
{
    std::vector<const Vertex*> ring(valence);
    size_t i = 0;

    /* Use `v_next == v_last` to detect opposite neighbor triangles. */
    if (boundary) {
        if (start_face->PrevNeighbor(this) == nullptr)
            ring[i++] = start_face->PrevVertex(this);
        else
            ring[i++] = start_face->NextVertex(this);
    }
    TraverseFaces([&](const Face* f) {
        // Check the comments in `Vertex::TraverseFaces()` for the reason of checking `f->NextVertex(this) == ring[i - 1]`.
        if (i != 0 && f->NextVertex(this) == ring[i - 1])
            ring[i++] = f->PrevVertex(this);
        else
            ring[i++] = f->NextVertex(this);
    });

    /* Use `OppositeNeighbor()` to detect opposite neighbor triangles. */
    // bool f_opposite = false;
    // if (boundary) {
    //     f_opposite = start_face->PrevNeighbor(this) != nullptr;
    //     if (f_opposite)
    //         ring[i++] = start_face->NextVertex(this);
    //     else
    //         ring[i++] = start_face->PrevVertex(this);
    // }
    // TraverseFaces([&](const Face* f) {
    //     int vi = f->VertexId(this);
    //     // Check the comments in `Vertex::TraverseFaces()` for the reason of checking `f_opposite`.
    //     if (f_opposite)
    //         ring[i++] = f->v[PREV(vi)];
    //     else
    //         ring[i++] = f->v[NEXT(vi)];
    //     int fn_id = f_opposite ? PREV(vi) : vi;
    //     f_opposite ^= f->OppositeNeighbor(fn_id);
    // });

    return ring;
}

std::vector<const Face*> Vertex::OneSweep() const
{
    std::vector<const Face*> sweep;
    TraverseFaces([&](const Face* f) {
        sweep.push_back(f);
    });
    return sweep;
}

std::vector<const Vertex*> Vertex::BoundaryNeighbors() const
{
    assert(boundary == true);
    const Face* end_face = TraverseFaces([](const Face*) {});
    return {
        start_face->PrevNeighbor(this) == nullptr ? start_face->PrevVertex(this) : start_face->NextVertex(this),
        end_face->NextNeighbor(this) == nullptr ? end_face->NextVertex(this) : end_face->PrevVertex(this),
    };
}

const Face* Vertex::TraverseFaces(const std::function<void(const Face*)>& func) const
{
    const Face *f = start_face, *f_last = nullptr;

    /* Use `f_next == f_last` to detect opposite neighbor triangles. */
    do {
        func(f);

        const Face* fn = f->NextNeighbor(this);
        // In the diagram, 'x' means "normal points out of the surface", '.' means "normal points into the surface".
        // Suppose traversal of the 4 triangles is from left to right.
        // If no such reverse, traversal will get stuck into infinite switches between the last 2 triangles.
        // *---*---*
        // |x /|\ .|
        // | / | \ |
        // |/ x|x \|
        // *---*---*
        // If `fn == f_last` for the first iteration (they are both `nullptr`), that means the first triangle uses a wrong direction, hence the reverse.
        // Or only the first triangle is traversed, like the leftmost triangle in the following diagram.
        // *---*---*
        // |. /|\ x|
        // | / | \ |
        // |/ x|x \|
        // *---*---*
        if (fn == f_last)
            fn = f->PrevNeighbor(this);
        f_last = f;
        f = fn;
    } while (f && f != start_face);

    /* Use `OppositeNeighbor()` to detect opposite neighbor triangles. */
    // bool f_opposite = f->NextNeighbor(this) == nullptr; // The facing of the first face defines "opposite" for the whole traversal.
    // do {
    //     func(f);
    //
    //     int fn_id = f->VertexId(this);
    //     // Check the comments in the above implementation for the reason of checking `f_opposite`.
    //     if (f_opposite)
    //         fn_id = PREV(fn_id);
    //     const Face* fn = f->neighbors[fn_id];
    //     f_opposite ^= f->OppositeNeighbor(fn_id);
    //     f_last = f;
    //     f = fn;
    // } while (f && f != start_face);

    return f_last;
}

}

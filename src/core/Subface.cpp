#include "Subface.hpp"

namespace subface {

void Vertex::ComputeValence()
{
    valence = boundary ? 1 : 0;
    TraverseFaces([&](const Face*) {
        ++valence;
    });
}

std::vector<const Vertex*> Vertex::OneRing() const
{
    std::vector<const Vertex*> ring(valence);
    size_t i = 0;
    if (boundary) {
        if (start_face->PrevNeighbor(this) == nullptr)
            ring[i++] = start_face->PrevVertex(this);
        else
            ring[i++] = start_face->NextVertex(this);
    }
    TraverseFaces([&](const Face* f) {
        if (i != 0 && f->NextVertex(this) == ring[i - 1])
            ring[i++] = f->PrevVertex(this);
        else
            ring[i++] = f->NextVertex(this);
    });
    return ring;
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
    return f_last;
}

}

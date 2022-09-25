#include "Subface.hpp"

namespace subface {

int Vertex::Valence()
{
    const Face* f = start_face;
    if (!boundary) {
        int nf = 1;
        while ((f = f->NextNeighbor(this)) != start_face)
            ++nf;
        return nf;
    } else {
        int nf = 1;
        while ((f = f->NextNeighbor(this)) != nullptr)
            ++nf;
        f = start_face;
        while ((f = f->PrevNeighbor(this)) != nullptr)
            ++nf;
        return nf + 1;
    }
}

void Vertex::OneRing(std::vector<glm::vec3>& ring)
{
    const Face* f = start_face;
    uint32_t i = 0;

    if (boundary) {
        const Face* first_face;
        while ((first_face = f->PrevNeighbor(this)) != nullptr)
            f = first_face;
        ring[i++] = f->PrevVertex(this)->p;
    }

    for (; i < ring.size(); ++i) {
        ring[i] = f->NextVertex(this)->p;
        f = f->NextNeighbor(this);
    }
}

std::vector<glm::vec3> Vertex::BoundaryNeighbors()
{
    assert(boundary == true);
    const Face *first_face = start_face, *last_face = start_face, *f;
    while ((f = first_face->PrevNeighbor(this)) != nullptr)
        first_face = f;
    while ((f = last_face->NextNeighbor(this)) != nullptr)
        last_face = f;

    return {
        first_face->PrevVertex(this)->p,
        last_face->NextVertex(this)->p
    };
}

}

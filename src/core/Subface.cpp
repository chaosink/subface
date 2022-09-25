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
    const Face* face = start_face;
    uint32_t i = 0;

    if (boundary) {
        const Face* f;
        while ((f = face->PrevNeighbor(this)) != nullptr)
            face = f;
        ring[i++] = face->PrevVertex(this)->p;
    }

    for (; i < ring.size(); ++i) {
        ring[i] = face->NextVertex(this)->p;
        face = face->NextNeighbor(this);
    }
}

}

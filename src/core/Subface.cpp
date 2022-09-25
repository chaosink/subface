#include "Subface.hpp"

namespace subface {

void Vertex::ComputeValence()
{
    valence = boundary ? 1 : 0;
    const Face* f = start_face;
    do {
        ++valence;
        f = f->NextNeighbor(this);
    } while (f && f != start_face);
}

std::vector<glm::vec3> Vertex::OneRing()
{
    std::vector<glm::vec3> ring(valence);
    uint32_t i = 0;
    if (boundary)
        ring[i++] = start_face->PrevVertex(this)->p;
    const Face* f = start_face;
    do {
        ring[i++] = f->NextVertex(this)->p;
        f = f->NextNeighbor(this);
    } while (f && f != start_face);
    return ring;
}

std::vector<glm::vec3> Vertex::BoundaryNeighbors()
{
    assert(boundary == true);
    const Face *end_face = nullptr, *f = start_face;
    do {
        end_face = f;
        f = f->NextNeighbor(this);
    } while(f);
    return {
        start_face->PrevVertex(this)->p,
        end_face->NextVertex(this)->p
    };
}

}

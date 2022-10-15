#pragma once

#include <functional>
#include <memory>
#include <string>

#include <glm/glm.hpp>

namespace subface {

constexpr float PI = 3.14159265358979323846f;

#define NEXT(i) (((i) + 1) % 3)
#define PREV(i) (((i) + 2) % 3)

struct Face;

struct Vertex {
    glm::vec3 p;
    const Face* start_face = nullptr;
    Vertex* child = nullptr;
    bool regular = false;
    bool boundary = false;
    uint32_t valence = 0;

    Vertex(const glm::vec3& p = glm::vec3(0, 0, 0))
        : p(p)
    {
    }

    void ComputeStartFaceAndBoundary();
    void ComputeValence();
    void ComputeRegular();
    std::vector<const Vertex*> OneRing() const;
    std::vector<const Face*> OneSweep() const;
    std::vector<const Vertex*> BoundaryNeighbors() const;
    const Face* TraverseFaces(const std::function<void(const Face*)>& func) const;
};

struct Edge {
    const Vertex* v[2];
    Face* f;
    int id;

    Edge(const Vertex* v0 = nullptr, const Vertex* v1 = nullptr);
    bool operator<(const Edge& e) const;
};

struct Face {
    const Vertex* v[3];
    const Face* neighbors[3];
    Face* children[4];

    Face();

    int VertexId(const Vertex* vertex) const;
    void Shift(int k);
    // Detect opposite neighbor triangles.
    // For traversal, `f_next == f_last` or `v_next == v_last` also work.
    bool OppositeNeighbor(int k) const;

    const Face* NextNeighbor(const Vertex* vertex) const
    {
        return neighbors[VertexId(vertex)];
    }
    const Face* PrevNeighbor(const Vertex* vertex) const
    {
        return neighbors[PREV(VertexId(vertex))];
    }
    const Vertex* NextVertex(const Vertex* vertex) const
    {
        return v[NEXT(VertexId(vertex))];
    }
    const Vertex* PrevVertex(const Vertex* vertex) const
    {
        return v[PREV(VertexId(vertex))];
    }
    const Vertex* OtherVertex(const Vertex* v0, const Vertex* v1) const
    {
        for (int i = 0; i < 3; ++i)
            if (v[i] != v0 && v[i] != v1)
                return v[i];
        return nullptr;
    }
};

class MemoryPool {
    const size_t page_size_ = 4 * 1024 * 1024;
    std::vector<std::unique_ptr<char[]>> pool_;
    size_t size_ = page_size_;

public:
    template <typename T>
    T* New()
    {
        if (page_size_ - size_ < sizeof(T)) {
            pool_.push_back(std::make_unique<char[]>(page_size_));
            size_ = 0;
        }

        T* p = reinterpret_cast<T*>(&pool_.back()[size_]);
        size_ += sizeof(T);
        return p;
    }
};

class Subface {
    int level_ = 0;
    size_t result_face_count_ = 0;

    std::vector<glm::vec3> origin_positions_;
    std::vector<uint32_t> origin_indexes_;

    std::vector<Vertex> vertexes_;
    std::vector<Face> faces_;

    std::vector<glm::vec3> unindexed_positions_;
    std::vector<glm::vec3> unindexed_smooth_normals_;
    std::vector<glm::vec3> unindexed_flat_normals_;

    std::vector<glm::vec3> indexed_positions_;
    std::vector<glm::vec3> indexed_smooth_normals_;
    std::vector<glm::vec3> indexed_flat_normals_;

    std::vector<int> vertex_indexes_;
    std::vector<int> smooth_normal_indexes_;
    std::vector<int> flat_normal_indexes_;

    // Only for non-boundary vertexes.
    static float Beta(int valence);
    // Only for non-boundary vertexes.
    static float LoopGamma(int valence);
    static glm::vec3 WeightOneRing(Vertex* vertex, float beta);
    // Only for boundary vertexes.
    static glm::vec3 WeightBoundary(Vertex* v, float beta);
    static void BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes,
        std::vector<Vertex>& vertexes, std::vector<Face>& faces);

    void ComputeNormalsAndPositions(const std::vector<Vertex*>& vertexes, const std::vector<Face*>& faces);
    bool CheckLevel(const std::string& func_name, int level, int base);

public:
    void BuildTopology(const std::vector<glm::vec3>& vertexes, const std::vector<uint32_t>& indexes);
    // Same as Tessellate4(int level) if `flat==true`.
    // `compute_limit` matters only when `flat==false`.
    void LoopSubdivide(int level, bool flat, bool compute_limit);
    // Same as LoopSubdivide(int level, bool flat=true).
    void Tessellate4(int level);
    // Another 1-to-4 triangle tessellation pattern than `Tessellate4()`.
    void Tessellate4_1(int level);
    // 1-to-3 triangle tessellation by connecting the center to each vertex.
    void Tessellate3(int level);
    // Use the implementations from https://github.com/zeux/meshoptimizer
    // Reduces the number of triangles in the mesh.
    // if `sloppy==false`:
    //     Attempte to preserve mesh appearance as much as possible.
    // else:
    //     Sacrifice mesh appearance for simplification performance.
    void Decimate(int level, bool midpoint);
    void MeshoptDecimate(int level, bool sloppy);
    void ExportObj(const std::string& file_name, bool smooth) const;

    enum EProcessingMethod {
        PM_SubdivideSmooth = 0,
        PM_SubdivideSmoothNoLimit = 1,
        PM_SubdivideFlat = 2,
        PM_Tessellate4 = 3,
        PM_Tessellate4_1 = 4,
        PM_Tessellate3 = 5,

        PM_Decimate_Start = 6,

        PM_Decimate_ShortestEdge_V0 = 6,
        PM_Decimate_ShortestEdge_Midpoint = 7,
        PM_MeshoptDecimate = 8,
        PM_MeshoptDecimateSloppy = 9,

        PM_Decimate_End = 10,
        PM_Count = 10,
    };
    struct ProcessingMethod {
        std::string name;
        std::function<void(Subface& sf, int level)> process;
    };
    static const Subface::ProcessingMethod& GetProcessingMethod(EProcessingMethod method);

    const std::vector<glm::vec3>& Position() const
    {
        return unindexed_positions_;
    }
    const std::vector<glm::vec3>& NormalSmooth() const
    {
        return unindexed_smooth_normals_;
    }
    const std::vector<glm::vec3>& NormalFlat() const
    {
        return unindexed_flat_normals_;
    }
};

}

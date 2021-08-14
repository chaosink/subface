#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <memory>
#include <vector>

namespace subface {

const float PI = 3.14159265358979323846f;

#define NEXT(i) (((i) + 1 ) % 3)
#define PREV(i) (((i) + 2 ) % 3)

struct Face;

struct Vertex {
	glm::vec3 p;
	const Face *start_face = nullptr;
	Vertex *child = nullptr;
	bool regular = false;
	bool boundary = false;

	Vertex(const glm::vec3 &p = glm::vec3(0, 0, 0)) : p(p) {}

	int Valence();
	void OneRing(std::vector<glm::vec3> &ring);
};

struct Edge {
	const Vertex *v[2];
	Face *f;
	int edge_num;

	Edge(const Vertex *v0 = nullptr, const Vertex *v1 = nullptr) {
		v[0] = std::min(v0, v1);
		v[1] = std::max(v0, v1);
		f = nullptr;
		edge_num = -1;
	}

	bool operator<(const Edge &e2) const {
		if(v[0] == e2.v[0]) return v[1] < e2.v[1];
		return v[0] < e2.v[0];
	}
};

struct Face {
	const Vertex *v[3];
	const Face *neighbors[3];
	Face *children[4];

	Face() {
		for(int i = 0; i < 3; ++i) {
			v[i] = nullptr;
			neighbors[i] = nullptr;
		}
		for(int i = 0; i < 4; ++i)
			children[i] = nullptr;
	}

	int VNum(const Vertex *vertex) const {
		for(int i = 0; i < 3; ++i)
			if(v[i] == vertex) return i;
		return -1;
	}

	const Face *NextNeighbor(const Vertex *vertex) const {
		return neighbors[VNum(vertex)];
	}
	const Face *PrevNeighbor(const Vertex *vertex) const {
		return neighbors[PREV(VNum(vertex))];
	}
	const Vertex *NextVertex(const Vertex *vertex) const {
		return v[NEXT(VNum(vertex))];
	}
	const Vertex *PrevVertex(const Vertex *vertex) const {
		return v[PREV(VNum(vertex))];
	}
	const Vertex *OtherVertex(const Vertex *v0, const Vertex *v1) const {
		for(int i = 0; i < 3; ++i)
			if(v[i] != v0 && v[i] != v1) return v[i];
		return nullptr;
	}
};

class MemoryPool {
	const size_t page_size_ = 4 * 1024 * 1024;
	std::vector<std::unique_ptr<char[]>> pool_;
	size_t size_ = page_size_;

public:
	template<typename T>
	T* New() {
		if(page_size_ - size_ < sizeof(T)) {
			pool_.push_back(std::make_unique<char[]>(page_size_));
			size_ = 0;
		}

		T* p = reinterpret_cast<T*>(&pool_.back()[size_]);
		size_ += sizeof(T);
		return p;
	}
};

}

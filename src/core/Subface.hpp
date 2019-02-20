#pragma once

#include <glm/glm.hpp>
#include <algorithm>

namespace subface {

#define NEXT(i) (((i) + 1 ) % 3)
#define PREV(i) (((i) + 2 ) % 3)

struct Face;

struct Vertex {
	glm::vec3 p;
	Face *start_face = nullptr;
	Vertex *child = nullptr;
	bool regular = false;
	bool boundary = false;

	Vertex(const glm::vec3 &p = glm::vec3(0, 0, 0)) : p(p) {}

	int Valence();
	void OneRing(std::vector<glm::vec3> ring);
};

struct Edge {
	Vertex *v[2];
	Face *f;
	int edge_num;

	Edge(Vertex *v0 = nullptr, Vertex *v1 = nullptr) {
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
	Vertex *v[3];
	Face *neighbors[3];
	Face *children[4];

	Face() {
		for(int i = 0; i < 3; ++i) {
			v[i] = nullptr;
			neighbors[i] = nullptr;
		}
		for(int i = 0; i < 4; ++i)
			children[i] = nullptr;
	}

	int VNum(Vertex *vertex) const {
		for(int i = 0; i < 3; ++i)
			if(v[i] == vertex) return i;
		return -1;
	}

	Face *NextNeighbor(Vertex *vertex) {
		return neighbors[VNum(vertex)];
	}
	Face *PrevNeighbor(Vertex *vertex) {
		return neighbors[PREV(VNum(vertex))];
	}
	Vertex *NextVertex(Vertex *vertex) {
		return v[NEXT(VNum(vertex))];
	}
	Vertex *PrevVertex(Vertex *vertex) {
		return v[PREV(VNum(vertex))];
	}
	Vertex *OtherVertex(Vertex *v0, Vertex *v1) {
		for(int i = 0; i < 3; ++i)
			if(v[i] != v0 && v[i] != v1) return v[i];
		return nullptr;
	}
};

}

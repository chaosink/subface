#pragma once

#include "Subface.hpp"

namespace subface {

class LoopSubface {
	std::vector<Vertex> vertexes_;
	std::vector<Face> faces_;

	static float Beta(int valence);
	static float LoopGamma(int valence);
	static glm::vec3 WeightOneRing(Vertex *vertex, float beta);
	static glm::vec3 WeightBoundary(Vertex *v, float beta);

public:
	void BuildTopology(std::vector<glm::vec3> &vertexes, std::vector<int> &indexes);
	void Subdivide(int level);
};

}

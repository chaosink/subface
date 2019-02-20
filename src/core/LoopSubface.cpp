#include "LoopSubface.hpp"

#include <set>
#include <map>
#include <iostream>
using namespace std;

namespace subface {

void LoopSubface::BuildTopology(std::vector<glm::vec3> &vertexes, std::vector<int> &indexes) {
	int n_vertexes = vertexes.size();
	int n_faces = indexes.size() / 3;

	vertexes_.resize(n_vertexes);
	for(int i = 0; i < n_vertexes; ++i)
		vertexes_[i].p = vertexes[i];

	faces_.resize(n_faces);
	for(int i = 0; i < n_faces; ++i)
		for(int j = 0; j < 3; j++) {
			faces_[i].v[j] = &vertexes_[indexes[i * 3 + j]];
			vertexes_[indexes[i * 3 + j]].start_face = &faces_[i];
		}

	std::set<Edge> edges;
	for(int i = 0; i < n_faces; ++i) {
		Face *f = &faces_[i];
		for(int edge_num = 0; edge_num < 3; ++edge_num) {
			int v0 = edge_num, v1 = NEXT(edge_num);
			Edge e(f->v[v0], f->v[v1]);
			if(edges.find(e) == edges.end()) {
				e.f = f;
				e.edge_num = edge_num;
				edges.insert(e);
			} else {
				e = *edges.find(e);
				e.f->neighbors[e.edge_num] = f;
				f->neighbors[edge_num] = e.f;
				edges.erase(e);
			}
		}
	}

	for(int i = 0; i < n_vertexes; ++i) {
		Vertex *v = &vertexes_[i];
		Face *f = v->start_face;
		do {
			f = f->NextNeighbor(v);
		} while(f && f != v->start_face);
		v->boundary = (f == nullptr);

		int valence = v->Valence();
		if(!v->boundary && valence == 6)
			v->regular = true;
		else if(v->boundary && valence == 4)
			v->regular = true;
		else
			v->regular = false;
	}
}

float LoopSubface::Beta(int valence) {
	if(valence == 3)
		return 3.f / 16.f;
	else
		return 3.f / (8.f * valence);
}

float LoopSubface::LoopGamma(int valence) {
	return 1.f / (valence + 3.f / (8.f * Beta(valence)));
}

glm::vec3 LoopSubface::WeightOneRing(Vertex *v, float beta) {
	int valence = v->Valence();
	std::vector<glm::vec3> ring(valence);
	v->OneRing(ring);
	glm::vec3 p = (1 - valence * beta) * v->p;
	for(int i = 0; i < valence; ++i)
		p += beta * ring[i];
	return p;
}

glm::vec3 LoopSubface::WeightBoundary(Vertex *v, float beta) {
	int valence = v->Valence();
	std::vector<glm::vec3> ring(valence);
	v->OneRing(ring);
	glm::vec3 p = (1 - 2 * beta) * v->p;
	p += beta * ring[0];
	p += beta * ring[valence - 1];
	return p;
}

void LoopSubface::Subdivide(int level) {
	std::vector<Vertex*> v(vertexes_.size());
	std::vector<Face*> f(faces_.size());
	for(size_t i = 0; i < vertexes_.size(); i++)
		v[i] = &vertexes_[i];
	for(size_t i = 0; i < faces_.size(); i++)
		f[i] = &faces_[i];

	std::vector<Vertex> vertex_buffer;
	std::vector<Face> face_buffer;
	for(int i = 0; i < level; ++i) {
		std::vector<Vertex*> new_vertexes;
		std::vector<Face*> new_faces(f.size() * 4);

		for(auto &vertex: v) {
			vertex_buffer.emplace_back();
			new_vertexes.emplace_back(&vertex_buffer.back());
			// vertex.child = &new_vertexes.back();
			// vertex.child->regular = vertex.regular;
			// vertex.child->boundary = vertex.boundary;
		}
		// for(size_t j = 0; j < f.size(); ++j)
		// 	for(int k = 0; k < 4; ++k)
		// 		f[j].children[k] = &new_faces[j * 4 + k];
		//
		// for(Vertex &vertex: v)
		// 	cout << vertex.Valence() << endl;
		//
		// for(Vertex &vertex: v)
		// 	if(!vertex.boundary) {
		// 		if(vertex.regular)
		// 			vertex.child->p = WeightOneRing(&vertex, 1.f / 16.f);
		// 		else
		// 			vertex.child->p = WeightOneRing(&vertex, Beta(vertex.Valence()));
		// 	} else {
		// 		vertex.child->p = WeightBoundary(&vertex, 1.f / 8.f);
		// 	}

	// 	std::map<Edge, Vertex*> edge_vertex;
	// 	for(Face &face: f) {
	// 		for(int k = 0; k < 3; ++k) {
	// 			Edge edge(face.v[k], face.v[NEXT(k)]);
	// 			Vertex *vertex = edge_vertex[edge];
	// 			if(!vertex) {
	// 				new_vertexes.emplace_back();
	// 				vertex = &new_vertexes.back();
	// 				vertex->regular = true;
	// 				vertex->boundary = (face.neighbors[k] == nullptr);
	// 				vertex->start_face = face.children[3];
	//
	// 				if(vertex->boundary) {
	// 					vertex->p = 0.5f * edge.v[0]->p;
	// 					vertex->p += 0.5f * edge.v[1]->p;
	// 				} else {
	// 					vertex->p = 3.f / 8.f * edge.v[0]->p;
	// 					vertex->p += 3.f / 8.f * edge.v[1]->p;
	// 					vertex->p += 1.f / 8.f
	// 						* face.OtherVertex(edge.v[0], edge.v[1])->p;
	// 					vertex->p += 1.f / 8.f
	// 						* face.neighbors[k]->OtherVertex(edge.v[0], edge.v[1])->p;
	// 				}
	// 				edge_vertex[edge] = vertex;
	// 			}
	// 		}
	// 	}
	//
	// 	for(Vertex &vertex: v) {
	// 		int v_num = vertex.start_face->VNum(&vertex);
	// 		vertex.child->start_face = vertex.start_face->children[v_num];
	// 	}
	//
	// 	for(Face &face : f) {
	// 		for(int j = 0; j < 3; ++j) {
	// 			face.children[3]->neighbors[j] = face.children[NEXT(j)];
	// 			face.children[j]->neighbors[NEXT(j)] = face.children[3];
	//
	// 			Face *f2 = face.neighbors[j];
	// 			face.children[j]->neighbors[j] =
	// 				f2 ? f2->children[f2->VNum(face.v[j])] : nullptr;
	// 			f2 = face.neighbors[PREV(j)];
	// 			face.children[j]->neighbors[PREV(j)] =
	// 				f2 ? f2->children[f2->VNum(face.v[j])] : nullptr;
	// 		}
	// 	}
	//
	// 	for(Face &face: f) {
	// 		for(int j = 0; j < 3; ++j) {
	// 			face.children[j]->v[j] = face.v[j]->child;
	//
	// 			Vertex *vertex =
	// 				edge_vertex[Edge(face.v[j], face.v[NEXT(j)])];
	// 			face.children[j]->v[NEXT(j)] = vertex;
	// 			face.children[NEXT(j)]->v[j] = vertex;
	// 			face.children[3]->v[j] = vertex;
	// 		}
	// 	}
	//
	// 	v = std::move(new_vertexes);
	// 	f = std::move(new_faces);
	}

	// std::vector<glm::vec3> limit(v.size());
	// for(size_t i = 0; i < v.size(); ++i) {
	// 	if(v[i].boundary)
	// 		limit[i] = WeightBoundary(&v[i], 1.f / 5.f);
	// 	else
	// 		limit[i] = WeightOneRing(&v[i], LoopGamma(v[i].Valence()));
	// }
	// for(size_t i = 0; i < v.size(); ++i)
	// 	v[i].p = limit[i];
	//
	// std::vector<glm::vec3> normals;
	// normals.reserve(v.size());
	// std::vector<glm::vec3> ring(16);
	// for(auto &v: ring)
	// 	std::cout << v.x << " " << v.y << " " << v.z << std::endl;

	// for(Vertex *vertex : v) {
	// 	Vector3f S(0, 0, 0), T(0, 0, 0);
	// 	int valence = vertex->valence();
	// 	if(valence > (int)pRing.size()) pRing.resize(valence);
	// 	vertex->oneRing(&pRing[0]);
	// 	if(!vertex->boundary) {
	// 		for(int j = 0; j < valence; ++j) {
	// 			S += std::cos(2 * Pi * j / valence) * Vector3f(pRing[j]);
	// 			T += std::sin(2 * Pi * j / valence) * Vector3f(pRing[j]);
	// 		}
	// 	} else {
	// 		// Compute tangents of boundary face
	// 		S = pRing[valence - 1] - pRing[0];
	// 		if(valence == 2)
	// 			T = Vector3f(pRing[0] + pRing[1] - 2 * vertex->p);
	// 		else if(valence == 3)
	// 			T = pRing[1] - vertex->p;
	// 		else if(valence == 4)  // regular
	// 			T = Vector3f(-1 * pRing[0] + 2 * pRing[1] + 2 * pRing[2] +
	// 						 -1 * pRing[3] + -2 * vertex->p);
	// 		else {
	// 			Float theta = Pi / float(valence - 1);
	// 			T = Vector3f(std::sin(theta) * (pRing[0] + pRing[valence - 1]));
	// 			for(int k = 1; k < valence - 1; ++k) {
	// 				Float wt = (2 * std::cos(theta) - 2) * std::sin((k)*theta);
	// 				T += Vector3f(wt * pRing[k]);
	// 			}
	// 			T = -T;
	// 		}
	// 	}
	// 	Ns.push_back(Normal3f(Cross(S, T)));
	// }
	//
	// // Create triangle mesh from subdivision mesh
	// {
	// 	size_t ntris = f.size();
	// 	std::unique_ptr<int[]> verts(new int[3 * ntris]);
	// 	int *vp = verts.get();
	// 	size_t totVerts = v.size();
	// 	std::map<SDVertex *, int> usedVerts;
	// 	for(size_t i = 0; i < totVerts; ++i) usedVerts[v[i]] = i;
	// 	for(size_t i = 0; i < ntris; ++i) {
	// 		for(int j = 0; j < 3; ++j) {
	// 			*vp = usedVerts[f[i]->v[j]];
	// 			++vp;
	// 		}
	// 	}
	// 	return CreateTriangleMesh(ObjectToWorld, WorldToObject,
	// 							  reverseOrientation, ntris, verts.get(),
	// 							  totVerts, limit.get(), nullptr, &Ns[0],
	// 							  nullptr, nullptr, nullptr);
	// }
}

}

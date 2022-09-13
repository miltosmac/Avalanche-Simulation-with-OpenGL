#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "model.h"
#include <glm/gtx/string_cast.hpp>

class Cube 
{
public:
	int i, j, k;
	int cubeindex;
	std::vector<int> primitive_index;
	std::vector<glm::vec3> Vertexes;
	std::vector<glm::vec3> Normals;

	Cube(int i, int j, int k, bool*** SquareGrid);

	int CreateTriangles(float& max_height, glm::vec2& max_v, glm::vec2& min_v);
	//glm::vec3 VertexInterpolation(float** HeightMap, glm::vec2& max_v, glm::vec2& min_v);
	
};
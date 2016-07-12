/*Class for creating a Koch snowflake fractal with given dimensions and length of an edge of a triangle of a first dimension*/

#pragma once
#ifndef _FLAKE_H
#define _FLAKE_H

#include <gl\glew.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtx\vector_angle.hpp>

#include <vector>
#include <iostream>


class Shader;

class KochSnowflake
{
private:

	unsigned int m_vaoID;		// vertex array object
	unsigned int m_vboID[2];	// two VBOs - used for colours and vertex data

public:

	KochSnowflake();
	
	std::vector<glm::vec3> verts; // vertex of the fractal
	std::vector<glm::vec3> pentadaOfPoints; // holds 5 points for rounding the corners of a fractal
	std::vector<glm::vec3> triadaOfPoints; // holds 3 points for rounding the corners of a fractal
	
	// Constructs the geometry of a fractal of given dimension. EdgeLength is the length of the edge of the triangle of a 0 dimension fractal.
	void constructGeometry(unsigned int edgeLength, unsigned int dimension);
	
	// Constructs geometry for a fractal and adds points at the corners of the fractal to round them
	void constructGeometryRounded(unsigned int edgeLength, unsigned int dimension, float radiusOfTube);

	// Computes points of one higher dimension fractal triangle on an edge of a triangle of lower dimension. Parameters are points of a segment making
	//the edge of the triangle.
	std::vector<glm::vec3> getTrianglePoints(const glm::vec3& startPoint, const glm::vec3& endPoint);

	// Gets an angle of a point relative to the x axis and the angle of a corner of a fractal
	glm::vec2 getAngleAndCornerType(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);

	// Renders the fractal
	void render();

	// Creates buffers for the fractal
	void createBuffers(Shader* myShader);
};

#endif _FLAKE_H

#include "tube.h"
#include <shaders\Shader.h>

Tube::Tube() {}

void Tube::constructGeometry(float radiusOfSegments, unsigned int edgeLength, unsigned int dimension, unsigned int numberOfVertexesOfOneSegment)
{

	flake.constructGeometryRounded(edgeLength, dimension, radiusOfSegments);

	Radius = radiusOfSegments;
	base = Radius * glm::sqrt(2 * (1 - glm::cos(glm::radians(30.0f))));

	int angle;

	for (size_t i = 0; i < flake.verts.size(); i++)
	{
		  if (i == 0)
		 {
			angle = getAngleForSegmentPositioning(flake.verts[flake.verts.size() - 1], flake.verts[i], flake.verts[i + 1]);			
		  }
		  else if (i == flake.verts.size() -1)
		  {
			  angle = getAngleForSegmentPositioning(flake.verts[i-1], flake.verts[i], flake.verts[0]);	
		  }
		  else
		  {
			  angle = getAngleForSegmentPositioning(flake.verts[i - 1], flake.verts[i], flake.verts[i + 1]);
    	  }
	
		  addSegment(radiusOfSegments, flake.verts[i], angle, glm::vec3(0.0f, 1.0f, 0.0f), numberOfVertexesOfOneSegment);

		//last segment is the first
		  if (i == flake.verts.size()-1) 
		{

			addSegment(radiusOfSegments, flake.verts[0], -30.0f, glm::vec3(0.0f, 1.0f, 0.0f), numberOfVertexesOfOneSegment);
	
		}
	}

	getTriangleVerts(numberOfVertexesOfOneSegment);
	getTriangleNormals(numberOfVertexesOfOneSegment);
	calcBoundingBoxs(numberOfVertexesOfOneSegment);
}

// adds a segment for the tube
void Tube::addSegment(float r, const glm::vec3& center, float angle,  const glm::vec3& axisOfRotation, unsigned int numberOfVertexes)
{
	unsigned int n = numberOfVertexes;
	float angleOfTrianle = 360.0f / n;

	glm::mat4 rotMat = glm::mat4(1.0);
	
		rotMat = glm::rotate(rotMat, angle, axisOfRotation); // creating a rotation matrix

	std::vector<glm::vec3> circleVertexes; // holds coordinates of all vertices of a circle

	for (unsigned int i = 0; i < n; i++) 
	{
		glm::vec3 vert; 
	
		vert.x = cos(angleOfTrianle * i * deg2rad) * r;
		vert.y = sin(angleOfTrianle * i * deg2rad) * r;
		vert.z = 0.0;
		
	  
		vert = glm::vec3(rotMat * glm::vec4(vert, 1.0)); //rotating the vertex
		vert = vert + center; // translating the vertex
	
		circleVertexes.push_back(vert);
	}

	verts.insert(verts.end(), circleVertexes.begin(), circleVertexes.end()); 

	for (unsigned int i = 0; i < n; i++)
	{
		glm::vec3 colour = glm::vec3(0.0, 0.0, 0.0);
		cols.emplace_back(colour);
	}

	int index = (verts.size() / n) - 1;

	int offset = (index - 1) * n;

	if (index > 0) 
	{
		for (size_t i = 0; i < n; i++)
		{
			tris.push_back(offset + i);
			tris.push_back(offset + n + i);
		}

		tris.push_back(offset);
		tris.push_back(offset + n);
	}
}

void checkGLErrors()
{
	auto errorCode = GL_NO_ERROR;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		auto error = gluErrorString(errorCode);
		std::cout << error << std::endl;
	}
}

// returns an oriented angle between two vectors, when 3 points are given as parameters
float Tube::getAngle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
	glm::vec3 ba = a - b;
	glm::vec3 ca = c - b;

	ba = glm::normalize(ba);
	ca = glm::normalize(ca);

	return glm::angle(glm::vec2(ba.x, ba.z), glm::vec2(ca.x, ca.z));
}

float Tube::getAngleForSegmentPositioning(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
	glm::vec3 ba = b - a;
	glm::vec3 cb = c - b;

	glm::vec3 bd = ba + cb;

	bd = glm::normalize(bd);

	float angle = glm::orientedAngle(glm::vec2(bd.x, bd.z), glm::vec2(1.0f, 0.0f)) + 90;

	angle = (std::round(angle / 10)) * 10;
	
	return angle;
}


void Tube::createBuffers(Shader* myShader)
{
	checkGLErrors();

	// VAO allocation
	glGenVertexArrays(1, &m_vaoID);

	// First VAO setup
	glBindVertexArray(m_vaoID);

	glGenBuffers(2, m_vboID);

	float* v = new float[verts.size() * 3];
	for (int i = 0; i < verts.size(); i++)
	{
		v[i * 3] = verts[i].x;
		v[i * 3+1] = verts[i].y;
		v[i * 3+2] = verts[i].z;
	}
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[0]);
	//initialises data storage of vertex buffer object
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 3 * sizeof(GLfloat), v, GL_STATIC_DRAW);
	GLint vertexLocation = glGetAttribLocation(myShader->handle(), "in_Position");
	glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vertexLocation);


	float* c = new float[cols.size() * 3];
	for (int i = 0; i < cols.size(); i++)
	{
		c[i * 3] = cols[i].x;
		c[i * 3 + 1] = cols[i].y;
		c[i * 3 + 2] = cols[i].z;
	}
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, cols.size() * 3 * sizeof(GLfloat), c, GL_STATIC_DRAW);
	GLint colsLocation = glGetAttribLocation(myShader->handle(), "in_Color");
	glVertexAttribPointer(colsLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colsLocation);
	
	delete[] v;
	delete[] c;
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tris.size() * sizeof(unsigned int), &tris[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glUseProgram(0); //turn off the current shader

	checkGLErrors();
}

void Tube::render()
{
	//draw objects
	glBindVertexArray(m_vaoID);		// select VAO

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_TRIANGLE_STRIP, tris.size(), GL_UNSIGNED_INT, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindVertexArray(0); //unbind the vertex array object
	glUseProgram(0); //turn off the current shader
}

void Tube::getTriangleVerts(unsigned int numberOfVertexesOfOneSegment)
{
	int numberOfVertexesInTrisInSegment = numberOfVertexesOfOneSegment * 2 + 2;

	for (int i = 0; i < tris.size(); i = i + numberOfVertexesInTrisInSegment)
	{
		for (int j = 0; j < numberOfVertexesOfOneSegment * 2; j++)
		{
			int a = tris[i + j];
			int b = tris[i + j + 1];
			int c = tris[i + j + 2];

			glm::vec3 triangle(a, b, c);
			triangles.push_back(triangle);
		}
	}
}

void Tube::getTriangleNormals(unsigned int numberOfVertexesOfOneSegment)
{
	for (int i = 0; i < triangles.size(); i++)
	{
		glm::vec3 a1 = verts[triangles[i].x];
		glm::vec3 b1 = verts[triangles[i].y];
		glm::vec3 c1 = verts[triangles[i].z];

		glm::vec3 edge1_1 = b1 - a1;
		glm::vec3 edge2_1 = c1 - a1;

		glm::vec3 normal_local = glm::cross(edge1_1, edge2_1);

		if (glm::all(glm::equal(normal_local, glm::vec3(0.0f, 0.0f, 0.0f))))
		{
			glm::vec3 normal_local_2 = norms[norms.size() - numberOfVertexesOfOneSegment * 2 + 1];
			norms.push_back(normal_local_2);
		}
		else
		{
			normal_local = glm::normalize(normal_local);
			norms.push_back(normal_local);
		}
	}
}

bool Tube::collisionBetweenPoint(glm::vec3& v, float threshold, int BBlimitF, int BBlimitL)
{
	std::vector<unsigned int> triaglesToCheckNow = triaglesToCheck(v, BBlimitF, BBlimitL);

	//printf("triaglesToCheckNow %d - %d \r", triaglesToCheckNow[0], triaglesToCheckNow[1]);

	if (triaglesToCheckNow[0] < triaglesToCheckNow[1])
	{
		for (int i = triaglesToCheckNow[0]; i < triaglesToCheckNow[1]; i++)
		{
			float dist = glm::dot((v - verts[triangles[i].x]), norms[i]);

			if (BarycentricCalculation(v, dist, i))
			{
				if (abs(dist) < threshold)
				{
					//printf("collision!!! with %d \n", i);
					return false;
				}
			}
		}
	}
	else
	{
		for (int i = triaglesToCheckNow[0]; i < triangles.size(); i++)
		{
			float dist = glm::dot((v - verts[triangles[i].x]), norms[i]);

			if (BarycentricCalculation(v, dist, i))
			{
				if (abs(dist) < threshold)
				{
					//printf("collision!!! with %d \n", i);
					return false;
				}
			}
		}
		for (int i = 0; i < triaglesToCheckNow[1]; i++)
		{
			float dist = glm::dot((v - verts[triangles[i].x]), norms[i]);

			if (BarycentricCalculation(v, dist, i))
			{
				if (abs(dist) < threshold)
				{
					//printf("collision!!! with %d \n", i);
					return false;
				}
			}
		}
	}

	return true;
}

std::vector<unsigned int> Tube::triaglesToCheck(glm::vec3& point, int BBlimitF, int BBlimitL)
{
	std::vector<unsigned int> tringlesToCheckNow;

	for (int i = 0; i < boundingBoxes.size(); i++)
	{
		if (point.x > boundingBoxes[i][0].x & point.x < boundingBoxes[i][1].x)
		{
			if (point.y > boundingBoxes[i][0].y & point.y < boundingBoxes[i][1].y)
			{
				if (point.z > boundingBoxes[i][0].z & point.z < boundingBoxes[i][1].z)
				{
					//printf("Point in BB %d \r", i);

					if (i - BBlimitF < 0)
					{
						int BBnumF = trianglesInBoxe.size() - i - BBlimitF;
						int firstTriangle = trianglesInBoxe[BBnumF][0];
						tringlesToCheckNow.push_back(firstTriangle);
					}
					else
					{
						int firstTriangle = trianglesInBoxe[i - BBlimitF][0];
						tringlesToCheckNow.push_back(firstTriangle);
					}

					if (i + BBlimitL >= trianglesInBoxe.size())
					{
						int BBnumL = i + BBlimitL - trianglesInBoxe.size();
						int lastTriangle = trianglesInBoxe[BBnumL][0];
						tringlesToCheckNow.push_back(lastTriangle);
					}
					else
					{
						int lastTriangle = trianglesInBoxe[i + BBlimitL][0];
						tringlesToCheckNow.push_back(lastTriangle);
					}

					return tringlesToCheckNow;
				}
			}
		}
	}

	tringlesToCheckNow.push_back(0);
	tringlesToCheckNow.push_back(0);
	return tringlesToCheckNow;
}


bool Tube::BarycentricCalculation(glm::vec3& point, float dist, int i)
{
	glm::vec3 P = point - norms[i] * dist;

	// Compute vectors        
	glm::vec3 v0 = verts[triangles[i].z] - verts[triangles[i].x];
	glm::vec3 v1 = verts[triangles[i].y] - verts[triangles[i].x];
	glm::vec3 v2 = P - verts[triangles[i].x];

	// Compute dot products
	float dot00 = glm::dot(v0, v0);
	float dot01 = glm::dot(v0, v1);
	float dot02 = glm::dot(v0, v2);
	float dot11 = glm::dot(v1, v1);
	float dot12 = glm::dot(v1, v2);

	// Compute barycentric coordinates
	float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	if ((u >= 0) && (v >= 0) && (u + v < 1))
		return true;
	else
		return false;
}

void Tube::calcBoundingBoxs(unsigned int numberOfVertexesOfOneSegment)
{
	getTriangleSets(numberOfVertexesOfOneSegment);

	int numberOfTrianglesInSegment = numberOfVertexesOfOneSegment * 2;

	for (int i = 0; i < trianglesInBoxe.size(); i++)
	{
		std::vector<glm::vec3> boundingBox;

		float minX = verts[triangles[trianglesInBoxe[i][0]].x].x;
		float minY = verts[triangles[trianglesInBoxe[i][0]].x].y;
		float minZ = verts[triangles[trianglesInBoxe[i][0]].x].z;
		float maxX = verts[triangles[trianglesInBoxe[i][0]].x].x;
		float maxY = verts[triangles[trianglesInBoxe[i][0]].x].y;
		float maxZ = verts[triangles[trianglesInBoxe[i][0]].x].z;

		for (int j = 0; j < numberOfTrianglesInSegment; j++)
		{
			float x_a, y_a, z_a, x_b, y_b, z_b, x_c, y_c, z_c;

			glm::vec3 a = verts[triangles[trianglesInBoxe[i][j]].x];
			glm::vec3 b = verts[triangles[trianglesInBoxe[i][j]].y];
			glm::vec3 c = verts[triangles[trianglesInBoxe[i][j]].z];

			x_a = a.x; y_a = a.y; z_a = a.z;
			x_b = b.x; y_b = b.y; z_b = b.z;
			x_c = c.x; y_c = c.y; z_c = c.z;

			if (x_a < minX)
				minX = x_a;
			if (x_a > maxX)
				maxX = x_a;
			if (y_a < minY)
				minY = y_a;
			if (y_a > maxY)
				maxY = y_a;
			if (z_a < minZ)
				minZ = z_a;
			if (z_a > maxZ)
				maxZ = z_a;

			if (x_b < minX)
				minX = x_b;
			if (x_b > maxX)
				maxX = x_b;
			if (y_b < minY)
				minY = y_b;
			if (y_b > maxY)
				maxY = y_b;
			if (z_b < minZ)
				minZ = z_b;
			if (z_b > maxZ)
				maxZ = z_b;

			if (x_c < minX)
				minX = x_c;
			if (x_c > maxX)
				maxX = x_c;
			if (y_c < minY)
				minY = y_c;
			if (y_c > maxY)
				maxY = y_c;
			if (z_c < minZ)
				minZ = z_c;
			if (z_c > maxZ)
				maxZ = z_c;

		}

		glm::vec3 minBB(minX, minY, minZ);
		boundingBox.push_back(minBB);
		glm::vec3 maxBB(maxX, maxY, maxZ);
		boundingBox.push_back(maxBB);

		boundingBoxes.push_back(boundingBox);
	}
}

void Tube::getTriangleSets(unsigned int numberOfVertexesOfOneSegment)
{
	int numberOfTrianglesInSegment = numberOfVertexesOfOneSegment * 2;

	for (int i = 0; i < triangles.size(); i = i + numberOfTrianglesInSegment)
	{
		std::vector<unsigned int> trianglesInOneBoxe;

		for (int j = 0; j < numberOfTrianglesInSegment; j++)
		{
			trianglesInOneBoxe.push_back(i + j);
		}

		trianglesInBoxe.push_back(trianglesInOneBoxe);
	}
}

void Tube::playerPosition(glm::vec3& point, glm::vec3& camTagret, glm::mat4& directionMat, glm::mat4& turnMat, float speed, float Zturn)
{	
	glm::vec3 A = flake.verts[first];
	glm::vec3 B, C, Z;
	glm::vec3 P = point;
	glm::quat q_S;

	if (first == 0)
	{
		Z = flake.verts[flake.verts.size() - 1];
	}
	else
	{
		Z = flake.verts[first - 1];
	}

	if (first + 1 >= flake.verts.size())
	{
		B = flake.verts[0];
		C = flake.verts[1];
	}
	else if (first + 2 >= flake.verts.size())
	{
		B = flake.verts[first + 1];
		C = flake.verts[0];
	}
	else
	{
		B = flake.verts[first + 1];
		C = flake.verts[first + 2];
	}

	glm::vec3 ZA = A - Z;
	glm::vec3 AB = B - A;
	glm::vec3 AP = P - A;
	glm::vec3 PB = B - P;
	glm::vec3 BC = C - B;

	glm::vec3 P_proj = A + glm::dot(AP, AB) / glm::dot(AB, AB) * AB;

	glm::vec3 nowDirection = glm::normalize(AB);
	glm::vec3 nextDirection = glm::normalize(BC);
	glm::vec3 previousDirection = glm::normalize(ZA);

	glm::quat q_turn = glm::angleAxis(Zturn, glm::vec3(0.0, 0.0, -1.0));
	turnMat = turnMat * glm::toMat4(q_turn);

	if (glm::length(AP) > glm::length(AB))
	{
		first++;
		if (first == flake.verts.size())
		{
			first = 0;
		}
	}
	else 
	{
		float angle_now = -glm::degrees(glm::acos(glm::dot(nowDirection, glm::vec3(0.0, 0.0, -1.0))));
		glm::vec3 q_now_axis = glm::normalize(glm::cross(nowDirection, glm::vec3(0.0, 0.0, -1.0)));
		glm::quat q_now = glm::angleAxis(angle_now, q_now_axis);

		float angle_previous = -glm::degrees(glm::acos(glm::dot(previousDirection, glm::vec3(0.0, 0.0, -1.0))));
		glm::vec3 q_previous_axis = glm::normalize(glm::cross(previousDirection, glm::vec3(0.0, 0.0, -1.0)));
		glm::quat q_previous = glm::angleAxis(angle_previous, q_previous_axis);

		float angle_next = -glm::degrees(glm::acos(glm::dot(nextDirection, glm::vec3(0.0, 0.0, -1.0))));
		glm::vec3 q_next_axis = glm::normalize(glm::cross(nextDirection, glm::vec3(0.0, 0.0, -1.0)));
		glm::quat q_next = glm::angleAxis(angle_next, q_next_axis);

		float alpha1 = glm::length(AP) / base + 0.5;
		float alpha2 = glm::length(PB) / base + 0.5;

		if (alpha1 < 1.0f)
		{
			q_S = glm::slerp(q_previous, q_now, alpha1);

			directionMat = glm::toMat4(q_S) * turnMat;
		}
		else if (alpha2 < 1.0f)
		{
			q_S = glm::slerp(q_next, q_now, alpha2);

			directionMat = glm::toMat4(q_S) * turnMat;
		}
		else
		{
			directionMat = glm::toMat4(q_now) * turnMat;
		}
	}

	camTagret = P_proj;

	point += speed * nowDirection;
}

void Tube::obstaclePositions(float partLength, std::vector<glm::vec3>& obstaclePoints, std::vector<glm::vec3>& obstacleDirections, std::vector<float>& obstacleRotationS)
{
	for (int i = 0; i < flake.verts.size(); i++)
	{
		glm::vec3 A = flake.verts[i];
		glm::vec3 B;
		if (i + 1 == flake.verts.size())
		{
			B = flake.verts[0];
		}
		else
		{
			B = flake.verts[i + 1];
		}
		glm::vec3 AB = B - A;

		if (glm::length(AB) > base + 0.5f)
		{
			glm::vec3 Direction = glm::normalize(AB);

			for (float AM = partLength; AM < glm::length(AB); AM = AM + partLength)
			{
				float MB = glm::length(AB) - AM;
				float deltaAB = AM / MB;
				float xM = (A.x + deltaAB * B.x) / (1 + deltaAB);
				float yM = (A.y + deltaAB * B.y) / (1 + deltaAB);
				float zM = (A.z + deltaAB * B.z) / (1 + deltaAB);
				glm::vec3 M (xM, yM, zM);
				float rot_speed = rand() % 10 + 4;
				rot_speed = rot_speed / 10;
				int sign = rand() % 2;
				if (sign == 1) rot_speed = -rot_speed;
				obstaclePoints.push_back(M);
				obstacleDirections.push_back(Direction);
				obstacleRotationS.push_back(rot_speed);

			}
		}
	}
}
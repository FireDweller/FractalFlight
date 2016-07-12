#include "kochSnowflake.h"
#include <Shaders\Shader.h>
#include <iostream>
#include <sstream>

KochSnowflake::KochSnowflake() {}

void KochSnowflake::constructGeometry(unsigned int edgeLength, unsigned int dimension)
{
	// create the vertexes for the 0 dimension triangle-----------------------------------------------------------------------------------------

	glm::mat4 rotMat = glm::mat4(1.0);
	rotMat = glm::rotate(rotMat, 60.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // creating a rotation matrix
	glm::vec3 newTrianglePeak = glm::vec3(rotMat * glm::vec4(glm::vec3(edgeLength, 0.0f, 0.0f), 1.0));
	
	// adding 3 points of a equilateral triangle to the vector containing vertexes(draws counterclockwice)
	verts.push_back(glm::vec3(0.0f, 0.0f, 0.0f));//start(left) point of triangle base
	verts.push_back(glm::vec3(edgeLength, 0.0f, 0.0f));//end(right) point of triangle base
	verts.push_back(newTrianglePeak);//peak point of triangle
	//------------------------------------------------------------------------------------------------------------------------

	//---Create triangle points for higher dimnesions--------------------------------------------------------------------------
	std::vector<glm::vec3> trianglePoints;
	std::vector<glm::vec3> pointsOfAllTrianglesReceived;

		int amountOfVertexes;
		int initVecSize;

		for (unsigned int i = 1; i <= dimension; i++)
		{

			amountOfVertexes = verts.size();

			for (int j = 0; j < amountOfVertexes; j++)

				if (j == amountOfVertexes - 1)// if it is the last vertex
				{
					//last edge of triangle consists of the last and first point in the vector, so we compute a new triangle on it
					trianglePoints = getTrianglePoints(verts[j], verts[0]);
					pointsOfAllTrianglesReceived.insert(pointsOfAllTrianglesReceived.end(), trianglePoints.begin(), trianglePoints.end());
				}
				else
				{
					trianglePoints = getTrianglePoints(verts[j], verts[j + 1]);
					pointsOfAllTrianglesReceived.insert(pointsOfAllTrianglesReceived.end(), trianglePoints.begin(), trianglePoints.end());
				}

			initVecSize = verts.size();

			for (int i = initVecSize; i > 0; i--)
			{
				//
				verts.insert(verts.begin() + i, pointsOfAllTrianglesReceived.end() - 3, pointsOfAllTrianglesReceived.end());
				// clearing the last 3 points in the vector of triangle vertexes
				pointsOfAllTrianglesReceived.erase(pointsOfAllTrianglesReceived.end() - 3, pointsOfAllTrianglesReceived.end());
			}
		}
	//------------------------------------------------------------------------------------------------------------------------
}

void KochSnowflake::constructGeometryRounded(unsigned int edgeLength, unsigned int dimension, float radiusOfTube) 
{
	glm::vec3 point(radiusOfTube, 0.0f, 0.0f);
	float angle;
	glm::vec3 rotatedPoint;
	glm::mat4 rotMat;

 //-----Creating points for rounding the angles of a fractal-----------------------------------------------------------------
	for (int i = 0; i < 5; i++) 
	{

		angle = (FLOAT)(60 - i*30);
		rotMat = glm::mat4(1.0);

		rotMat = glm::rotate(rotMat, angle , glm::vec3(0.0f, 1.0f, 0.0f)); // creating a rotation matrix
	    rotatedPoint = glm::vec3(rotMat * glm::vec4(point, 1.0));
		pentadaOfPoints.push_back(rotatedPoint - glm::vec3(radiusOfTube, 0.0f, 0.0f));
		
		if (i > 0 && i < 4) 
		 {
			triadaOfPoints.push_back(rotatedPoint - glm::vec3(radiusOfTube, 0.0f, 0.0f));
		 }
	}

//----------------------------------------------------------------------------------------------------------------------------
	
	//creating points fora fractal
	constructGeometry(edgeLength, dimension);

// --- Creating points for a fractal with rounded corners by putting pentadas and triadas of points at corners of a fractal---
	glm::vec2 angleAndCornerType;
	int  vertexQuantity = verts.size();
	std::vector<glm::vec3> roundCornerGeometry; // vector for holding points of a fractal with rounded corners
	
	for (int i = 0; i < vertexQuantity; i++)
		{
		
			if (i == 0) { angleAndCornerType = getAngleAndCornerType(verts[verts.size() - 1], verts[i], verts[i + 1]); }
			else if (i == vertexQuantity - 1) { angleAndCornerType = getAngleAndCornerType(verts[i - 1], verts[i], verts[0]); }
			else angleAndCornerType = getAngleAndCornerType(verts[i - 1], verts[i], verts[i+1]);


			switch ((INT)angleAndCornerType.y) {
			case 60: // corner of 60 degrees angle			
				rotMat = glm::mat4(1.0);
				rotMat = glm::rotate(rotMat, angleAndCornerType.x, glm::vec3(0.0f, 1.0f, 0.0f));
				for (int j = 4; j >= 0; j--)
				{
					rotatedPoint = glm::vec3(rotMat * glm::vec4(pentadaOfPoints[j], 1.0));
					roundCornerGeometry.push_back(rotatedPoint + verts[i]);
				}
				break;
			
			case 120: // corner of 120 degree angle		
				rotMat = glm::mat4(1.0);
				rotMat = glm::rotate(rotMat, angleAndCornerType.x, glm::vec3(0.0f, 1.0f, 0.0f));
				for (int j = 0; j < 3; j++)
				{
					rotatedPoint = glm::vec3(rotMat * glm::vec4(triadaOfPoints[j], 1.0));
					roundCornerGeometry.push_back(rotatedPoint+verts[i]);
				}
				break;
			default:
				std::cout << "Error in computing a corner for a fractal. Angle of a corner: " << angleAndCornerType.y  << std::endl;
			break;
		}
	}
	verts = roundCornerGeometry;
 //----------------------------------------------------------------------------------------------------------------------------------------
}

std::vector<glm::vec3> KochSnowflake::getTrianglePoints(const glm::vec3& startPoint, const glm::vec3& endPoint)
{
	std::vector<glm::vec3> trianglePoints;

	float newSegLengthX = (endPoint.x - startPoint.x) / 3; 
	float newSegLengthZ = (endPoint.z - startPoint.z) / 3; 

	/*-----creating 2 base points for the triangle-------------------------------------------------------------*/
	glm::vec3 newSegStartPoint(startPoint.x + newSegLengthX, 0.0f, startPoint.z + newSegLengthZ);
	glm::vec3 newSegEndPoint(startPoint.x + 2 * newSegLengthX, 0.0f, startPoint.z + 2 * newSegLengthZ);
	
	/*-----creating the peak point of the triangle-------------------------------------------------------------*/

    glm::vec3 newTrianglePeak = newSegEndPoint - newSegStartPoint;

	glm::mat4 rotMat = glm::mat4(1.0);

	rotMat = glm::rotate(rotMat, -60.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // creating a rotation matrix

	newTrianglePeak = glm::vec3(rotMat * glm::vec4(newTrianglePeak, 1.0));

	newTrianglePeak = newTrianglePeak + newSegStartPoint; //translate back the end point

	trianglePoints.push_back(newSegStartPoint); // adding vertex to the vector
	trianglePoints.push_back(newTrianglePeak); 
	trianglePoints.push_back(newSegEndPoint); 
	
	return trianglePoints;
}

glm::vec2 KochSnowflake::getAngleAndCornerType(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
	glm::vec3 ba = a - b;
	glm::vec3 bc = c - b;

	glm::vec3 bd = ba + bc;

	ba = glm::normalize(ba);
	bc = glm::normalize(bc);
	bd = glm::normalize(-bd);

	float angle;
	int cornerType = (INT) glm::angle(ba, bc); // 60 or 120 degree angle corner
	
	// corner types approximation to 60 or 120 degrees
	if (30 < cornerType && cornerType < 90) { cornerType = 60; }
	if (90 <= cornerType && cornerType <= 150) { cornerType = 120; }

		angle = glm::orientedAngle(glm::vec2(bd.x, bd.z), glm::vec2(1.0f, 0.0f));
		angle = glm::orientedAngle(glm::vec2(bd.x, bd.z), glm::vec2(1.0f, 0.0f));

	return glm::vec2(angle, cornerType);
}

void KochSnowflake::createBuffers(Shader* myShader)
{
	// VAO allocation
	glGenVertexArrays(1, &m_vaoID);

	// First VAO setup
	glBindVertexArray(m_vaoID);
	glGenBuffers(2, m_vboID);

	float* v = new float[verts.size() * 3];
	for (int i = 0; i < verts.size(); i++)
	{
		v[i * 3] = verts[i].x;
		v[i * 3 + 1] = verts[i].y;
		v[i * 3 + 2] = verts[i].z;
	}
	//initialises data storage of vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[0]);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 3 * sizeof(GLfloat), v, GL_STATIC_DRAW);
	GLint vertexLocation = glGetAttribLocation(myShader->handle(), "in_Position");
	glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vertexLocation);

	float* c = new float[verts.size() * 3];
	for (int i = 0; i < verts.size(); i++)
	{
		c[i * 3] = 0.0;
		c[i * 3 + 1] = 0.0;
		c[i * 3 + 2] = 0.0;
	}
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 3 * sizeof(GLfloat), c, GL_STATIC_DRAW);
	GLint colsLocation = glGetAttribLocation(myShader->handle(), "in_Color");
	glVertexAttribPointer(colsLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colsLocation);
	delete[] v;
	delete[] c;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void KochSnowflake::render()
{
	//draw objects
	glPointSize(3.0f);
	glBindVertexArray(m_vaoID);		// select VAO
	glDrawArrays(GL_LINE_LOOP, 0, verts.size());
}
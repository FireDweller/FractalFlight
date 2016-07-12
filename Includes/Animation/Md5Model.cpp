/////////////////////////////////////////////////////////////////////////////
//
// Md5Model.h -- Copyright (c) 2006-2007 David Henry
// last modification: jan. 28, 2007
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
//
// Implementation of MD5 Model classes.
//
/////////////////////////////////////////////////////////////////////////////
#include <GL/glew.h>
#include <Images/imageloader.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <map>
#include "Md5Model.h"

//global stuff

const int kMd5Version = 10;

//Sort functor for joints
struct SortByDepth :
	public std::binary_function < Md5Joint_t, Md5Joint_t, bool >
{
	bool operator() (const Md5Joint_t &j1, const Md5Joint_t &j2) const {
		return (j1.pos._z < j2.pos._z);
	}
};

/****************************************************************************
 *	Class Md5Skeleton Implementation										*
 *																			*
*****************************************************************************/

Md5Skeleton::Md5Skeleton(){}
Md5Skeleton::Md5Skeleton(std::ifstream &ifs, int numJoints) throw (Md5Exception){
	string token, buffer;

	if (!ifs.is_open())
		throw Md5Exception("Input stream not opened!", "skeleton");

	// Read all joints
	for (int i = 0; i < numJoints; ++i)
	{
		// NOTE: hope there isn't any comment between
		// two lines of joints data...
		Md5JointPtr j(new Md5Joint_t);

		ifs >> j->name;
		ifs >> j->parent;
		ifs >> token; // "("
		ifs >> j->pos._x;
		ifs >> j->pos._y;
		ifs >> j->pos._z;
		ifs >> token; // ")"
		ifs >> token; // "("
		ifs >> j->orient._x;
		ifs >> j->orient._y;
		ifs >> j->orient._z;
		ifs >> token; // ")"

		// Eat up rest of the line
		std::getline(ifs, buffer);

		// Compute orient quaternion's w value
		j->orient.computeW();

		// Add joint to joints vector
		_joints.push_back(j);
	}
}
Md5Skeleton::~Md5Skeleton(){}
void Md5Skeleton::addJoint(Md5Joint_t *thisJoint){_joints.push_back(Md5JointPtr(thisJoint));}
void Md5Skeleton::setNumJoints(int numJoints)
{
	_joints.reserve(numJoints);
}
Md5Skeleton* Md5Skeleton::clone() const
{
	Md5Skeleton *cpy = new Md5Skeleton;
	cpy->setNumJoints(_joints.size());

	for (size_t i = 0; i < _joints.size(); ++i)
		cpy->addJoint(new Md5Joint_t(*_joints[i]));

	return cpy;
}
int Md5Skeleton::numJoints() const {return Md5Skeleton::_joints.size();}
Md5Joint_t* Md5Skeleton::joint(int index) const{return _joints[index];}
/////////////////////////////////////////////////////////////////////////////
//
// class Md5Mesh implementation.
//
/////////////////////////////////////////////////////////////////////////////

Md5Mesh::Md5Mesh(std::ifstream &ifs)
throw (Md5Exception)
: _renderState(kShow), _numVerts(0), _numTris(0), _numWeights(0),//,
_decal(NULL), _specMap(NULL), _normalMap(NULL), _heightMap(NULL)
{
	string token, buffer;

	if (!ifs.is_open())
		throw Md5Exception("Input stream not opened!", "mesh");

	do
	{
		// Read first token line from file
		ifs >> token;

		if (token == "shader")
		{
			ifs >> _shader;

			// Remove quote marks from the shader string
			_shader = _shader.substr(_shader.find_first_of('\"') + 1,
				_shader.find_last_of('\"') - 1);
			//GLuint img = SOIL_load_OGL_texture(_shader.c_str(),
			//	SOIL_LOAD_AUTO,
			//	SOIL_CREATE_NEW_ID,
			//	SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
			GLint tex = ImageLoader::loadPNG(_shader.c_str());
			setDecalMap(tex);
			setSpecularMap(tex);
			setNormalMap(tex);
			setHeightMap(tex);
			// Get mesh name from shader string
			_name = _shader.c_str() + _shader.find_last_of('/') + 1;
		}
		else if (token == "numverts")
		{
			ifs >> _numVerts;
			_verts.reserve(_numVerts);
		}
		else if (token == "numtris")
		{
			ifs >> _numTris;
			_tris.reserve(_numTris);
		}
		else if (token == "numweights")
		{
			ifs >> _numWeights;
			_weights.reserve(_numWeights);
		}
		else if (token == "vert")
		{
			Md5VertexPtr vert(new Md5Vertex_t);

			// Read vertex data
			ifs >> token; // index
			ifs >> token; // "("
			ifs >> vert->st[0];
			ifs >> vert->st[1];
			ifs >> token; // ")"
			ifs >> vert->startWeight;
			ifs >> vert->countWeight;

			_verts.push_back(vert);
		}
		else if (token == "tri")
		{
			Md5TrianglePtr tri(new Md5Triangle_t);

			// Read triangle data
			ifs >> token; // index
			ifs >> tri->index[0];
			ifs >> tri->index[1];
			ifs >> tri->index[2];

			_tris.push_back(tri);
		}
		else if (token == "weight")
		{
			Md5WeightPtr weight(new Md5Weight_t);

			// Read weight data
			ifs >> token; // index
			ifs >> weight->joint;
			ifs >> weight->bias;
			ifs >> token; // "("
			ifs >> weight->pos._x;
			ifs >> weight->pos._y;
			ifs >> weight->pos._z;
			ifs >> token; // ")"

			_weights.push_back(weight);
		}

		// Eat up rest of the line
		std::getline(ifs, buffer);

	} while ((token != "}") && !ifs.eof());

	// Memory allocation for vertex arrays
	allocVertexArrays();
	
	// Setup texture coordinates array
	setupTexCoordArray();

}
Md5Mesh::~Md5Mesh(){}
void Md5Mesh::updateVertexArrays(Md5Skeleton *skel){
	for (int i = 0; i < _numVerts; ++i)
	{
		Vector3f finalVertex = kZeroVectorf;
		Vector3f finalNormal = kZeroVectorf;
		Vector3f finalTangent = kZeroVectorf;

		// Calculate final vertex to draw with weights
		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			const Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			// Calculate transformed vertex for this weight
			Vector3f wv = pWeight->pos;
			pJoint->orient.rotate(wv);

			// The sum of all pWeight->bias should be 1.0
			finalVertex += (pJoint->pos + wv) * pWeight->bias;

			// Calculate transformed normal for this weight
			Vector3f wn = pWeight->norm;
			pJoint->orient.rotate(wn);

			finalNormal += wn * pWeight->bias;

			//// Calculate transformed tangent for this weight
			//Vector3f wt = pWeight->tan;
			//pJoint->orient.rotate (wt);

			//finalTangent += wt * pWeight->bias;
		}

		// We can omit to normalize normal and tangent,
		// because they should have been already normalized
		// when they were computed. We can gain some time
		// avoiding some heavy calculus.

		//finalNormal.normalize ();
		//finalTangent.normalize ();

	  {
		  // Fill in the vertex arrays with the freshly vertex, normal
		  // and tangent computed.

		  GLfloat *vertexPointer = &_vertexArray[i * 3];
		  GLfloat *normalPointer = &_normalArray[i * 3];
		  //GLfloat *tangentPointer = &_tangentArray[i * 3];

		  vertexPointer[0] = finalVertex._x;
		  vertexPointer[1] = finalVertex._y;
		  vertexPointer[2] = finalVertex._z;

		  normalPointer[0] = finalNormal._x;
		  normalPointer[1] = finalNormal._y;
		  normalPointer[2] = finalNormal._z;

		  //tangentPointer[0] = finalTangent._x;
		  //tangentPointer[1] = finalTangent._y;
		  //tangentPointer[2] = finalTangent._z;
	  }
	}
}
void Md5Mesh::setupVertexArrays(Md5Skeleton *skel)
{
	for (int i = 0; i < _numVerts; ++i)
	{
		Vector3f finalVertex = kZeroVectorf;
		Vector3f finalNormal = kZeroVectorf;
		Vector3f finalTangent = kZeroVectorf;

		// Calculate final vertex to draw with weights
		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			const Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			// Calculate transformed vertex for this weight
			Vector3f wv = pWeight->pos;
			pJoint->orient.rotate(wv);

			// The sum of all pWeight->bias should be 1.0
			finalVertex += (pJoint->pos + wv) * pWeight->bias;

			// Calculate transformed normal for this weight
			Vector3f wn = pWeight->norm;
			pJoint->orient.rotate(wn);

			finalNormal += wn * pWeight->bias;

			//// Calculate transformed tangent for this weight
			//Vector3f wt = pWeight->tan;
			//pJoint->orient.rotate (wt);

			//finalTangent += wt * pWeight->bias;
		}

		// We can omit to normalize normal and tangent,
		// because they should have been already normalized
		// when they were computed. We can gain some time
		// avoiding some heavy calculus.

		//finalNormal.normalize ();
		//finalTangent.normalize ();

	  {
		  // Fill in the vertex arrays with the freshly vertex, normal
		  // and tangent computed.

		  GLfloat *vertexPointer = &_vertexArray[i * 3];
		  GLfloat *normalPointer = &_normalArray[i * 3];
		  //GLfloat *tangentPointer = &_tangentArray[i * 3];

		  vertexPointer[0] = finalVertex._x;
		  vertexPointer[1] = finalVertex._y;
		  vertexPointer[2] = finalVertex._z;

		  normalPointer[0] = finalNormal._x;
		  normalPointer[1] = finalNormal._y;
		  normalPointer[2] = finalNormal._z;

		  //tangentPointer[0] = finalTangent._x;
		  //tangentPointer[1] = finalTangent._y;
		  //tangentPointer[2] = finalTangent._z;
	  }
	}
	
	glGenBuffers(sizeof(glBuffers)/sizeof(GLuint), glBuffers);

}
void Md5Mesh::computeWeightNormals(Md5Skeleton *skel)
{
	vector<Vector3f> bindposeVerts(_numVerts);
	vector<Vector3f> bindposeNorms(_numVerts);

	for (int i = 0; i < _numVerts; ++i)
	{
		// Zero out final vertex position and final vertex normal
		bindposeVerts[i] = kZeroVectorf;
		bindposeNorms[i] = kZeroVectorf;

		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			const Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			// Calculate transformed vertex for this weight
			Vector3f wv = pWeight->pos;
			pJoint->orient.rotate(wv);

			bindposeVerts[i] += (pJoint->pos + wv) * pWeight->bias;
		}
	}

	// Compute triangle normals
	for (int i = 0; i < _numTris; ++i)
	{
		const Md5Triangle_t *pTri = _tris[i];

		Vector3f triNorm(-ComputeNormal(bindposeVerts[pTri->index[0]],
			bindposeVerts[pTri->index[1]], bindposeVerts[pTri->index[2]]));

		for (int j = 0; j < 3; ++j)
			bindposeNorms[pTri->index[j]] += triNorm;
	}

	// "Average" the surface normals, by normalizing them
	for (int i = 0; i < _numVerts; ++i)
		bindposeNorms[i].normalize();

	//
	// At this stage we have all vertex normals computed
	// for the model geometry in bind-pos
	//

	// Zero out all weight normals
	for (int i = 0; i < _numWeights; ++i)
		_weights[i]->norm = kZeroVectorf;

	// Compute weight normals by invert-transforming the normal
	// by the bone-space matrix
	for (int i = 0; i < _numVerts; ++i)
	{
		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			Vector3f wn = bindposeNorms[i];

			// Compute inverse quaternion rotation
			Quaternionf invRot = Inverse(pJoint->orient);
			invRot.rotate(wn);

			pWeight->norm += wn;
		}
	}

	// Normalize all weight normals
	for (int i = 0; i < _numWeights; ++i)
		_weights[i]->norm.normalize();
}
void Md5Mesh::computeWeightTangents(Md5Skeleton *skel)
{
	vector<Vector3f> bindposeVerts(_numVerts);
	vector<Vector3f> bindposeNorms(_numVerts);
	vector<Vector3f> bindposeTans(_numVerts);

	vector<Vector3f> sTan(_numVerts);
	vector<Vector3f> tTan(_numVerts);

	// Zero out all weight tangents (thank you Valgrind)
	for (int i = 0; i < _numWeights; ++i)
		_weights[i]->tan = kZeroVectorf;

	// Compute bind-pose vertices and normals
	for (int i = 0; i < _numVerts; ++i)
	{
		// Zero out final vertex position, normal and tangent
		bindposeVerts[i] = kZeroVectorf;
		bindposeNorms[i] = kZeroVectorf;
		bindposeTans[i] = kZeroVectorf;

		// Zero s-tangents and t-tangents
		sTan[i] = kZeroVectorf;
		tTan[i] = kZeroVectorf;

		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			const Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			// Calculate transformed vertex for this weight
			Vector3f wv = pWeight->pos;
			pJoint->orient.rotate(wv);

			bindposeVerts[i] += (pJoint->pos + wv) * pWeight->bias;

			// Calculate transformed normal for this weight
			Vector3f wn = pWeight->norm;
			pJoint->orient.rotate(wn);

			bindposeNorms[i] += wn * pWeight->bias;
		}
	}

	// Calculate s-tangeants and t-tangeants at triangle level
	for (int i = 0; i < _numTris; ++i)
	{
		const Md5Triangle_t *pTri = _tris[i];

		const Vector3f &v0 = bindposeVerts[pTri->index[0]];
		const Vector3f &v1 = bindposeVerts[pTri->index[1]];
		const Vector3f &v2 = bindposeVerts[pTri->index[2]];

		const vec2_t &w0 = _verts[pTri->index[0]]->st;
		const vec2_t &w1 = _verts[pTri->index[1]]->st;
		const vec2_t &w2 = _verts[pTri->index[2]]->st;

		float x1 = v1._x - v0._x;
		float x2 = v2._x - v0._x;
		float y1 = v1._y - v0._y;
		float y2 = v2._y - v0._y;
		float z1 = v1._z - v0._z;
		float z2 = v2._z - v0._z;

		float s1 = w1[0] - w0[0];
		float s2 = w2[0] - w0[0];
		float t1 = w1[1] - w0[1];
		float t2 = w2[1] - w0[1];

		float r = (s1 * t2) - (s2 * t1);

		if (r == 0.0f)
			// Prevent division by zero
			r = 1.0f;

		float oneOverR = 1.0f / r;

		Vector3f sDir((t2 * x1 - t1 * x2) * oneOverR,
			(t2 * y1 - t1 * y2) * oneOverR,
			(t2 * z1 - t1 * z2) * oneOverR);
		Vector3f tDir((s1 * x2 - s2 * x1) * oneOverR,
			(s1 * y2 - s2 * y1) * oneOverR,
			(s1 * z2 - s2 * z1) * oneOverR);

		for (int j = 0; j < 3; ++j)
		{
			sTan[pTri->index[j]] += sDir;
			tTan[pTri->index[j]] += tDir;
		}
	}

	// Calculate vertex tangent
	for (int i = 0; i < _numVerts; ++i)
	{
		const Vector3f &n = bindposeNorms[i];
		const Vector3f &t = sTan[i];

		// Gram-Schmidt orthogonalize
		bindposeTans[i] = (t - n * DotProduct(n, t));
		bindposeTans[i].normalize();

		// Calculate handedness
		if (DotProduct(CrossProduct(n, t), tTan[i]) < 0.0f)
			bindposeTans[i] = -bindposeTans[i];

		// Compute weight tangent
		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			Vector3f wt = bindposeTans[i];

			// Compute inverse quaternion rotation
			Quaternionf invRot = Inverse(pJoint->orient);
			invRot.rotate(wt);

			pWeight->tan += wt;
		}
	}

	// Normalize all weight tangents
	for (int i = 0; i < _numWeights; ++i)
		_weights[i]->tan.normalize();
}
void Md5Mesh::computeBoundingBox(Md5Skeleton *skel)
{
	Vector3f maxx(-99999.0f, -99999.0f, -99999.0f);
	Vector3f minn(99999.0f, 99999.0f, 99999.0f);

	for (int i = 0; i < _numVerts; ++i)
	{
		Vector3f finalVertex = kZeroVectorf;

		// Calculate final vertex to draw with weights
		for (int j = 0; j < _verts[i]->countWeight; ++j)
		{
			const Md5Weight_t *pWeight
				= _weights[_verts[i]->startWeight + j];
			const Md5Joint_t *pJoint
				= skel->joint(pWeight->joint);

			// Calculate transformed vertex for this weight
			Vector3f wv = pWeight->pos;
			pJoint->orient.rotate(wv);

			// The sum of all pWeight->bias should be 1.0
			finalVertex += (pJoint->pos + wv) * pWeight->bias;
		}

		if (finalVertex._x > maxx._x)
			maxx._x = finalVertex._x;

		if (finalVertex._x < minn._x)
			minn._x = finalVertex._x;

		if (finalVertex._y > maxx._y)
			maxx._y = finalVertex._y;

		if (finalVertex._y < minn._y)
			minn._y = finalVertex._y;

		if (finalVertex._z > maxx._z)
			maxx._z = finalVertex._z;

		if (finalVertex._z < minn._z)
			minn._z = finalVertex._z;
	}

	_boundingBox.minn = minn;
	_boundingBox.maxx = maxx;
}void Md5Mesh::preRenderVertexArrays() const
{
}
void Md5Mesh::postRenderVertexArrays() const
{
}

void Md5Mesh::initVBO(Shader *myShader){
}
void Md5Mesh::renderVertexArrays(Shader *myShader) const
{

	// Ensable shader/program's stuff

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Upload mesh data to OpenGL
	glVertexPointer(3, GL_FLOAT, 0, &_vertexArray.front());
	glNormalPointer(GL_FLOAT, 0, &_normalArray.front());
	glTexCoordPointer(2, GL_FLOAT, 0, &_texCoordArray.front());

	glBindBuffer(GL_ARRAY_BUFFER, glBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, _vertexArray.size()*3*sizeof(GLfloat), &_vertexArray.front(), GL_STATIC_DRAW);
	GLint vertexLocation= glGetAttribLocation(myShader->handle(), "in_Position");
	glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(vertexLocation);

	glBindBuffer(GL_ARRAY_BUFFER, glBuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, _normalArray.size()*3*sizeof(GLfloat), &_normalArray.front(), GL_STATIC_DRAW);
	GLint normalLocation = glGetAttribLocation(myShader->handle(), "in_Normal");
	glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 0,0);
	glEnableVertexAttribArray(normalLocation);

	glBindBuffer(GL_ARRAY_BUFFER, glBuffers[2]);
	glBufferData(GL_ARRAY_BUFFER, _texCoordArray.size()*2*sizeof(GLfloat), &_texCoordArray.front(), GL_STATIC_DRAW);
	GLint texCoordLocation = glGetAttribLocation(myShader->handle(), "in_TexCoord");
	glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0,0);
	glEnableVertexAttribArray(texCoordLocation);

	// Bind to mesh's textures
	//setupTexture (_heightMap, GL_TEXTURE3);
	//setupTexture (_normalMap, GL_TEXTURE2);
	//setupTexture (_specMap, GL_TEXTURE1);
	setupTexture(_decal, GL_TEXTURE0, myShader);

	

	// Draw the mesh
	glDrawElements(GL_TRIANGLES, _numTris * 3, GL_UNSIGNED_INT, &_vertIndices.front());

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	//setupTexture (NULL, GL_TEXTURE0, myShader);
	//// Disable shader/program's stuff
	//glEnableVertexAttribArray(0);

	//glBindVertexArray(0);
	
}
void Md5Mesh::setState(int state) { Md5Mesh::_renderState = state; }
void Md5Mesh::setDecalMap(GLuint tex) { _decal = tex; }
void Md5Mesh::setSpecularMap(GLuint tex) { _specMap = tex; }
void Md5Mesh::setNormalMap(GLuint tex) { _normalMap = tex; }
void Md5Mesh::setHeightMap(GLuint tex) { _heightMap = tex; }
void Md5Mesh::setupTexture(GLuint tex, GLenum texUnit, Shader *myShader) const
{
	// Disable texture and return
//	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(glGetUniformLocation(myShader->handle(), "DiffuseMap"), texUnit);
	//glMatrixMode (GL_TEXTURE);
	//glLoadIdentity ();
	//glScalef (1.0f, -1.0f, 1.0f);
	//glTranslatef (0.0f, -1.0f, 0.0f);
	//glMatrixMode (GL_MODELVIEW);
}
const string& Md5Mesh::name() const { return Md5Mesh::_name; }
const BoundingBox_t& Md5Mesh::boundingBox() const { return Md5Mesh::_boundingBox; }
bool Md5Mesh::hiden() const { return (_renderState == kHide); }
bool Md5Mesh::noDraw() const { return (_renderState == kNoDraw); }
bool Md5Mesh::show() const { return (_renderState == kShow); }
void Md5Mesh::allocVertexArrays()
{
	//_vertexArray.reserve (_numVerts * 3);
	_vertexArray.clear();
	for (int i = 0; i < _numVerts * 3; i++) _vertexArray.push_back(0);
	//_normalArray.reserve (_numVerts * 3);
	_normalArray.clear();
	for (int i = 0; i < _numVerts * 3; i++) _normalArray.push_back(0);
	//_tangentArray.reserve (_numVerts * 3);
	_tangentArray.clear();
	for (int i = 0; i < _numVerts * 3; i++) _tangentArray.push_back(0);
	//_texCoordArray.reserve (_numVerts * 2);
	_texCoordArray.clear();
	for (int i = 0; i < _numVerts * 2; i++) _texCoordArray.push_back(0);

	_vertIndices.reserve(_numTris * 3);

	// We can already initialize the vertex index array (we won't have
	// to do it each time we want to draw)
	for (int i = 0; i < _numTris; ++i)
	{
		for (int j = 0; j < 3; ++j)
			_vertIndices.push_back(_tris[i]->index[j]);
	}
}
void Md5Mesh::setupTexCoordArray()
{
	for (int i = 0, j = 0; i < _numVerts; ++i, j += 2)
	{
		j = i * 2;
		_texCoordArray[j + 0] = _verts[i]->st[0];
		_texCoordArray[j + 1] = _verts[i]->st[1];
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Model implementation.
//
/////////////////////////////////////////////////////////////////////////////

Md5Model::Md5Model(const string &filename)
throw (Md5Exception)
: _numJoints(0), _numMeshes(0)
{
	// Open file
	std::ifstream ifs(filename.c_str(), std::ios::in);

	if (ifs.fail())
		throw Md5Exception("Couldn't open file", filename);

	while (!ifs.eof())
	{
		string token, buffer;
		int version;

		// Read next token
		ifs >> token;

		if (token == "//")
		{
			// This is the begining of a comment
			// Eat up rest of the line
			std::getline(ifs, buffer);
		}
		else if (token == "MD5Version")
		{
			ifs >> version;

			if (version != kMd5Version)
				throw Md5Exception("Bad ifs version", filename);
		}
		else if (token == "numJoints")
		{
			ifs >> _numJoints;
		}
		else if (token == "numMeshes")
		{
			ifs >> _numMeshes;
			_meshes.reserve(_numMeshes);
		}
		else if (token == "joints")
		{
			// Base skeleton data
			ifs >> token; // "{"

			Md5Skeleton *skel = new Md5Skeleton(ifs, _numJoints);
			_baseSkeleton = Md5SkeletonPtr(skel);
			ifs >> token; // "}"
		}
		else if (token == "mesh")
		{
			ifs >> token; // "{"

			// Create and load a new model mesh
			Md5MeshPtr mesh = Md5MeshPtr(new Md5Mesh(ifs));

			// Compute bounding box in bind-pose
			mesh->computeBoundingBox(_baseSkeleton);

			// Compute weight normals
			mesh->computeWeightNormals(_baseSkeleton);

			// Compute weight tangents
			mesh->computeWeightTangents(_baseSkeleton);

			mesh->setupVertexArrays(_baseSkeleton);
			
			_meshes.push_back(mesh);
		}
	}
	
	ifs.close();

	// Compute the bounding box in bind-pose
	computeBindPoseBoundingBox();
}
Md5Model::~Md5Model(){}
void Md5Model::prepare(Md5Skeleton *skel)
{
	for (int i = 0; i < _numMeshes; ++i)
	{
		if (!_meshes[i]->hiden())
			// Prepare for drawing with interpolated skeleton
			_meshes[i]->setupVertexArrays(skel);
	}
}
void Md5Model::prepare_2(Md5Skeleton *skel)
{
	for (int i = 0; i < _numMeshes; ++i)
	{
		if (!_meshes[i]->hiden())
			// Prepare for drawing with interpolated skeleton
			//_meshes[i]->setupVertexArrays(skel);
			_meshes[i]->updateVertexArrays(skel);
	}
}

void Md5Model::calcCentrePoint(){

}
void Md5Model::drawModel(Shader *myShader) const
{
	glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
			//glPushMatrix();
			for (int i = 0; i < _numMeshes; ++i)
			{
				if (_meshes[i]->show())
				{
					_meshes[i]->renderVertexArrays(myShader);
				}
			}
			//glPopMatrix();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}
bool Md5Model::validityCheck(Md5Animation *anim) const
{
	if (!anim)
		return false;

	if (_numJoints != anim->frame(0)->numJoints())
		return false;

	for (int i = 0; i < _numJoints; ++i)
	{
		const Md5Joint_t *modelJoint = _baseSkeleton->joint(i);
		const Md5Joint_t *animJoint = anim->frame(0)->joint(i);

		if (modelJoint->name != animJoint->name)
			return false;

		if (modelJoint->parent != animJoint->parent)
			return false;
	}

	return true;
}
void Md5Model::setMeshRenderState(const string &name, int state)
{
	if (Md5Mesh *mesh = getMeshByName(name))
		mesh->setState(state);
}
int Md5Model::numJoints() const { return _numJoints; }
const Md5Skeleton* Md5Model::baseSkeleton() const { return _baseSkeleton; }
const BoundingBox_t& Md5Model::bindPoseBoundingBox() const { return _bindPoseBox; }
void Md5Model::computeBindPoseBoundingBox()
{
	Vector3f maxx(-99999.0f, -99999.0f, -99999.0f);
	Vector3f minn(99999.0f, 99999.0f, 99999.0f);

	// Get the min and the maxx from all mesh bounding boxes
	for (int i = 0; i < _numMeshes; ++i)
	{
		const BoundingBox_t &meshBox = _meshes[i]->boundingBox();

		if (meshBox.maxx._x > maxx._x)
			maxx._x = meshBox.maxx._x;

		if (meshBox.minn._x < minn._x)
			minn._x = meshBox.minn._x;

		if (meshBox.maxx._y > maxx._y)
			maxx._y = meshBox.maxx._y;

		if (meshBox.minn._y < minn._y)
			minn._y = meshBox.minn._y;

		if (meshBox.maxx._z > maxx._z)
			maxx._z = meshBox.maxx._z;

		if (meshBox.minn._z < minn._z)
			minn._z = meshBox.minn._z;
	}

	_bindPoseBox.minn = minn;
	_bindPoseBox.maxx = maxx;
}
Md5Mesh* Md5Model::getMeshByName(const string &name) const
{
	for (int i = 0; i < _numMeshes; ++i)
	{
		if (_meshes[i]->name() == name)
			return _meshes[i];
	}

	return NULL;
}


void Md5Model::initVBO(Shader *myShader){
	for (int i = 0; i < _numMeshes; ++i)
	{
		_meshes[i]->initVBO(myShader);
	}
}




/////////////////////////////////////////////////////////////////////////////
//
// class Md5Animation implementation.
//
/////////////////////////////////////////////////////////////////////////////
Md5Animation::Md5Animation(const string &filename)
throw (Md5Exception)
: _numFrames(0), _frameRate(0)
{
	vector<JointInfo> jointInfos;
	vector<BaseFrameJoint> baseFrame;
	vector<float> animFrameData;
	int numJoints = 0;
	int numAnimatedComponents = 0;

	// Open file
	std::ifstream ifs(filename.c_str(), std::ios::in);

	if (ifs.fail())
		throw Md5Exception("Couldn't open file", filename);

	while (!ifs.eof())
	{
		string token, buffer;
		int version;
		int i;

		// Read next token
		ifs >> token;

		if (token == "//")
		{
			// This is the begining of a comment
			// Eat up rest of the line
			std::getline(ifs, buffer);
		}
		else if (token == "MD5Version")
		{
			ifs >> version;

			if (version != kMd5Version)
				throw Md5Exception("Bad file version", filename);
		}
		else if (token == "numFrames")
		{
			ifs >> _numFrames;
			_skelframes.reserve(_numFrames);
			_bboxes.reserve(_numFrames);
		}
		else if (token == "numJoints")
		{
			ifs >> numJoints;
			jointInfos.reserve(numJoints);
			baseFrame.reserve(numJoints);
		}
		else if (token == "frameRate")
		{
			ifs >> _frameRate;
		}
		else if (token == "numAnimatedComponents")
		{
			ifs >> numAnimatedComponents;
			animFrameData.reserve(numAnimatedComponents);
		}
		else if (token == "hierarchy")
		{
			// Read all joint infos
			ifs >> token; // "{"

			// Read all joint infos
			for (i = 0; i < numJoints; ++i)
			{
				JointInfo jinfo;

				ifs >> jinfo.name;
				ifs >> jinfo.parent;
				ifs >> jinfo.flags.value;
				ifs >> jinfo.startIndex;

				jointInfos.push_back(jinfo);

				// Eat up rest of the line
				std::getline(ifs, buffer);
			}

			ifs >> token; // "}"
		}
		else if (token == "bounds")
		{
			ifs >> token; // "{"

			// Read frame bounds
			for (int i = 0; i < _numFrames; ++i)
			{
				BoundingBoxPtr bbox(new BoundingBox_t);

				ifs >> token; // "("
				ifs >> bbox->minn._x;
				ifs >> bbox->minn._y;
				ifs >> bbox->minn._z;
				ifs >> token; // ")"
				ifs >> token; // "("
				ifs >> bbox->maxx._x;
				ifs >> bbox->maxx._y;
				ifs >> bbox->maxx._z;
				ifs >> token; // ")"

				_bboxes.push_back(bbox);
			}

			ifs >> token; // "}"
		}
		else if (token == "baseframe")
		{
			// We should have an opening bracket for the baseframe joint list
			ifs >> token; // "{"

			// Read baseframe data
			for (i = 0; i < numJoints; ++i)
			{
				BaseFrameJoint bfj;

				ifs >> token; // "("
				ifs >> bfj.pos._x;
				ifs >> bfj.pos._y;
				ifs >> bfj.pos._z;
				ifs >> token; // ")"
				ifs >> token; // "("
				ifs >> bfj.orient._x;
				ifs >> bfj.orient._y;
				ifs >> bfj.orient._z;
				ifs >> token; // ")"

				baseFrame.push_back(bfj);

				// Eat up rest of the line
				std::getline(ifs, buffer);
			}

			ifs >> token; // "}"
		}
		else if (token == "frame")
		{
			int frameIndex;
			ifs >> frameIndex;
			ifs >> token; // "{"

			animFrameData.clear();

			// Read all frame data
			float afvalue;
			for (i = 0; i < numAnimatedComponents; ++i)
			{
				// NOTE about coding style: beeuuarg *vomit*
				ifs >> afvalue;
				animFrameData.push_back(afvalue);
			}

			ifs >> token; // "}"

			// Build skeleton for this frame
			buildFrameSkeleton(jointInfos, baseFrame, animFrameData);
		}
	}

	ifs.close();

	// Extract animation's name
	string szfile = filename;
	string::size_type start = szfile.find_last_of('/');
	string::size_type end = szfile.find_last_of(".md5anim");
	_name = szfile.substr(start + 1, end - start - 8);

	isTransition = false;
}
Md5Animation::~Md5Animation()
{
}
void Md5Animation::buildFrameSkeleton(vector<JointInfo> &jointInfos,
	vector<BaseFrameJoint> &baseFrame, 
	vector<float> &animFrameData)
{
	// Allocate memory for this frame
	Md5SkeletonPtr skelframe(new Md5Skeleton);
	_skelframes.push_back(skelframe);

	skelframe->setNumJoints(jointInfos.size());

	// Setup all joints for this frame
	for (unsigned int i = 0; i < jointInfos.size(); ++i)
	{
		BaseFrameJoint *baseJoint = &baseFrame[i];
		Vector3f animatedPos = baseJoint->pos;
		Quaternionf animatedOrient = baseJoint->orient;
		int j = 0;

		if (jointInfos[i].flags.tx) // Tx
		{
			animatedPos._x = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags.ty) // Ty
		{
			animatedPos._y = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags.tz) // Tz
		{
			animatedPos._z = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags.qx) // Qx
		{
			animatedOrient._x = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags.qy) // Qy
		{
			animatedOrient._y = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags.qz) // Qz
		{
			animatedOrient._z = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		// Compute orient quaternion's w value
		animatedOrient.computeW();

		// NOTE: we assume that this joint's parent has
		// already been calculated, i.e. joint's ID should
		// never be smaller than its parent ID.
		Md5Joint_t *thisJoint = new Md5Joint_t;
		skelframe->addJoint(thisJoint);

		int parent = jointInfos[i].parent;
		thisJoint->parent = parent;
		thisJoint->name = jointInfos[i].name;

		// has parent?
		if (thisJoint->parent < 0)
		{
			thisJoint->pos = animatedPos;
			thisJoint->orient = animatedOrient;
		}
		else
		{
			const Md5Joint_t *parentJoint = skelframe->joint(parent);

			parentJoint->orient.rotate(animatedPos);
			thisJoint->pos = animatedPos + parentJoint->pos;

			thisJoint->orient = parentJoint->orient * animatedOrient;
			thisJoint->orient.normalize();
		}
	}
}
void Md5Animation::interpolateFrames(int frameA, int frameB,
	float interp, Md5Skeleton *out) const
{
	for (int i = 0; i < out->numJoints(); ++i)
	{
		const Md5Joint_t *pJointA = _skelframes[frameA]->joint(i);
		const Md5Joint_t *pJointB = _skelframes[frameB]->joint(i);
		Md5Joint_t *pFinalJoint = out->joint(i);

		pFinalJoint->parent = pJointA->parent;
		pFinalJoint->pos = pJointA->pos + interp * (pJointB->pos - pJointA->pos);
		pFinalJoint->orient = Slerp(pJointA->orient, pJointB->orient, interp);
	}
}
void Md5Animation::interpolateAnims(Md5Animation* from, Md5Animation *to, float interp, Md5Skeleton *out)
{
	for (int i = 0; i < out->numJoints(); ++i)
	{
		const Md5Joint_t *pJointA = from->_skelframes[from->maxFrame()]->joint(i);
		const Md5Joint_t *pJointB = to->_skelframes[0]->joint(i);
		Md5Joint_t *pFinalJoint = out->joint(i);

		pFinalJoint->parent = pJointA->parent;
		pFinalJoint->pos = pJointA->pos + interp * (pJointB->pos - pJointA->pos);
		pFinalJoint->orient = Slerp(pJointA->orient, pJointB->orient, interp);
	}
}
float Md5Animation::getTotalTime()
{
	return ((float)_numFrames) * 1 / _frameRate;
}

/////////////////////////////////////////////////////////////////////////////
//
// class Md5Object implementation.
//
/////////////////////////////////////////////////////////////////////////////

Md5Object::Md5Object()
	: _model(NULL), _animatedSkeleton(NULL), _softwareTransformation(false),
	_currAnim(NULL), _currFrame(0), _nextFrame(1), _last_time(0.0),
	_max_time(0.0), _renderFlags(kDrawModel), totalTime(0)
{
}
Md5Object::~Md5Object()
{
	delete _animatedSkeleton;
}
void Md5Object::animate(double dt)
{
	// Animate only if there is an animation...
	if (_currAnim)
	{
		unsigned int maxFrame = _currAnim->maxFrame();

		_currFrame = floor(dt*maxFrame);

		_last_time = dt*maxFrame - _currFrame;

		if (_currFrame < maxFrame)
			_nextFrame = _currFrame + 1;

		if (_currFrame > maxFrame)
		{
			_currFrame = 0;

		}

		if (_nextFrame > maxFrame)
		{
			_nextFrame = 0;
		}
	}
}
void Md5Object::animateFinal(float time)
{
	float timeDistance = updateCurNextAnimsReturnTimeBetween(time);

	if (timeDistance < 0)
		animate(-timeDistance);
	else _last_time = timeDistance;

	prepare_2(timeDistance < 0);
}
float Md5Object::updateCurNextAnimsReturnTimeBetween(float time)
{
	AnimMapTimed::const_iterator itor = timeLineAnimList.begin();
	int i;
	int timelineAnimlistLength = timeLineAnimList.size();

	for (i = 0; i < timelineAnimlistLength; i++)
	{
		float curStart = itor->first;
		float curEnd = itor->first + itor->second->getTotalTime();
		if (itor->second->isTransition)
		{
			int i = 0;
			i++;
		}
		_currAnim = itor->second;
		_max_time = 1.0 / static_cast<double>(_currAnim->frameRate());

		if (time >= curStart&&time <= curEnd)
		{

			_nextAnim = _currAnim;
			return -(time - curStart) / (curEnd - curStart);
		}
		else if (i < timeLineAnimList.size() - 1)
		{
			float nextStart = *this->timeLineAnimStartTimeByIndex(i + 1);
			if (time < nextStart)
			{
				_nextAnim = this->timeLineAnimByIndex(i + 1);
				return (time - curEnd) / (nextStart - curEnd);
			}
		}

		itor++;
	}

	return -1;
}
void Md5Object::computeBoundingBox()
{
	BoundingBox_t bbox;

	if (_currAnim)
	{
		// Interpolate frames' bounding box in order
		// to get animated AABB in object space
		const BoundingBox_t *boxA, *boxB;
		boxA = _currAnim->frameBounds(_currFrame);
		boxB = _currAnim->frameBounds(_nextFrame);

		float interp = _last_time * _currAnim->frameRate();

		bbox.minn = boxA->minn + (boxB->minn - boxA->minn) * interp;
		bbox.maxx = boxA->maxx + (boxB->maxx - boxA->maxx) * interp;
	}
	else
	{
		// Get bind-pose model's bouding box
		bbox = _model->bindPoseBoundingBox();
	}

	// Compute oriented bounding box
	_bbox.world = _modelView;
	_bbox.center = Vector3f((bbox.maxx._x + bbox.minn._x) * 0.5f,
		(bbox.maxx._y + bbox.minn._y) * 0.5f,
		(bbox.maxx._z + bbox.minn._z) * 0.5f);
	_bbox.extent = Vector3f(bbox.maxx._x - _bbox.center._x,
		bbox.maxx._y - _bbox.center._y,
		bbox.maxx._z - _bbox.center._z);
}
void Md5Object::prepare(bool interpAnims)
{
	if (interpAnims)
	{
		// Interpolate current and next frame skeletons
		_currAnim->interpolateFrames(_currFrame, _nextFrame, _last_time, _animatedSkeleton);
	}
	else
	{
		// Intertolate current and next animations
		_currAnim->interpolateAnims(_currAnim, _nextAnim, _last_time, _animatedSkeleton);
	}

	// Setup vertex arrays
	_model->prepare(_animatedSkeleton);
}

void Md5Object::prepare_2(bool interpAnims)
{
	if (interpAnims)
	{
		// Interpolate current and next frame skeletons
		_currAnim->interpolateFrames(_currFrame, _nextFrame, _last_time, _animatedSkeleton);
	}
	else
	{
		// Intertolate current and next animations
		_currAnim->interpolateAnims(_currAnim, _nextAnim, _last_time, _animatedSkeleton);
	}

	// Setup vertex arrays
	_model->prepare_2(_animatedSkeleton);
}

void Md5Object::render(Shader *myShader) const
{
	glPushMatrix();

	_model->drawModel(myShader);

	glPopMatrix();
}
bool Md5Object::addAnim(const string &filename, bool isTransition)
{
	Md5Animation *pAnim = new Md5Animation(filename);
	pAnim->isTransition = isTransition;
	// Check for compatibility
	if (!this->_model->validityCheck(pAnim))
	{
		cerr << filename << " isn't a valid animation"
			<< " for this model!" << endl;
		delete pAnim;
		return false;
	}

	const string name(pAnim->name());

	if (!isTransition)
	{
		// If there is already an animation with same name,
		// delete it
		AnimMap::iterator itor = _animList.find(name);
		if (itor != _animList.end())
			_animList.erase(itor);

		// Insert the new animation
		_animList.insert(AnimMap::value_type(name, Md5AnimationPtr(pAnim)));
	}
	else
	{
		// If there is already a transition with same name,
		// delete it
		AnimMap::iterator itor = _transList.find(name);
		if (itor != _transList.end())
			_transList.erase(itor);

		// Insert the new transition
		_transList.insert(AnimMap::value_type(name, Md5AnimationPtr(pAnim)));
	}


	return true;
}
bool Md5Object::setupTransition(Md5Animation * anim)
{
	if (anim->name().compare("Idle") == 0)
	{
		storeTransition("SwingLR", "IdleToSwingLR", anim);
		storeTransition("SwingRL", "IdleToSwingRL", anim);
	}
	else if (anim->name().compare("Thrust") == 0)
	{
		storeTransition("Idle", "ThrustToIdle", anim);
	}
	else if (anim->name().compare("SwingLR") == 0)
	{
		storeTransition("Idle", "SwingLRToIdle", anim);
		storeTransition("SwingLR", "SwingRL", anim);
	}
	else if (anim->name().compare("SwingRL") == 0)
	{
		storeTransition("Idle", "SwingRLToIdle", anim);
		storeTransition("SwingRL", "SwingLR", anim);
	}
	return true;
}
bool Md5Object::storeTransition(char* animName, char* transName, Md5Animation* anim)
{
	AnimMap::const_iterator transItor;
	AnimMap::const_iterator animItor;

	animItor = this->_animList.find(animName);
	transItor = this->_transList.find(transName);

	Md5Transition *transition = new Md5Transition();
	transition->animation = animItor->second;
	transition->transition = transItor->second;

	anim->transitions.push_back(*transition);
	return true;
}
Md5Animation* Md5Object::getTransition(int animIndex)

{
	Md5Animation*     anim = timeLineAnimByIndex(animIndex);
	Md5Animation* nextAnim = timeLineAnimByIndex(animIndex + 1);

	for (int i = 0; i < anim->transitions.size(); i++)
	{
		if (anim->transitions[i].animation->name().compare(nextAnim->name()) == 0)
			return anim->transitions[i].transition;
	}
	return NULL;
}
bool Md5Object::addModel(Md5Model * model)
{
	models.push_back(model);
	setMd5Model(model);

	return true;
}
bool Md5Object::dropAnim(float startTime, Md5AnimationPtr anim)
{
	if (anim != NULL)
	{
		this->timeLineAnimList.insert(AnimMapTimed::value_type(startTime + 0.001, Md5AnimationPtr(anim)));

		AnimMapTimed::iterator itor = timeLineAnimList.find(startTime + 0.001);

		itor++;

		for (; itor != timeLineAnimList.end(); itor++)
		{
			float *startTimePtr = (float *)&(itor->first);

			*startTimePtr += anim->getTotalTime();
		}

		return true;
	}
	return false;
}
void Md5Object::deleteAnim(int index)
{
	AnimMapTimed::iterator itor = timeLineAnimList.begin();

	for (int i = 0; i < index; i++)
		itor++;

	timeLineAnimList.erase(itor);
}
void Md5Object::setMd5Model(Md5Model *model)
{
	if (_model != model)
	{
		_model = model; // Link to the model

		//Delete previous skeletons because the new model is different and 
		//its skeleton can hold more joints.
		delete _animatedSkeleton;

		//Copy skeleton joints name
		_animatedSkeleton = _model->baseSkeleton()->clone();

		//// Reset animation
		//_currAnim = NULL;
		//_currAnimName.clear ();
	}
}
void Md5Object::setAnim(const string &name)
{
	if (_model)
	{
		// Retrieve animation from model's animation list
		if ((_currAnim = anim(name)))
		{
			_currAnimName = _currAnim->name();

			// Compute max frame time and reset _last_time
			_max_time = 1.0 / static_cast<double>(_currAnim->frameRate());
			//_last_time = 0.0;

			// Reset current and next frames
			//_currFrame = 0;
			//_nextFrame = (_currAnim->maxFrame () > 0) ? 1 : 0;
		}
		else
		{
			delete _animatedSkeleton;
			_currAnimName.clear();

			// Rebuild animated skeleton with model's base skeleton
			_animatedSkeleton = _model->baseSkeleton()->clone();
		}
	}
}

void Md5Object::setRenderFlags(int flags) { Md5Object::_renderFlags = flags; }
void Md5Object::updateTotalTime()
{
	//totalTime=0;

	AnimMapTimed::reverse_iterator itor = timeLineAnimList.rbegin();

	//for(int i=0; i<_animList.size(); i++)
	//{
	//	totalTime+=itor->second->getTotalTime();
	//	itor++;
	//}
	totalTime = itor->first + itor->second->getTotalTime();
}
Md5Animation *Md5Object::animByIndex(int index)
{
	AnimMap::const_iterator itor = _animList.begin();

	for (int i = 0; i < index; i++)
		itor++;

	return itor->second;
}
Md5Animation *Md5Object::timeLineAnimByIndex(int index)
{
	AnimMapTimed::const_iterator itor = timeLineAnimList.begin();

	for (int i = 0; i < index; i++)
		itor++;

	return itor->second;
}
float*	Md5Object::timeLineAnimStartTimeByIndex(int index)
{
	AnimMapTimed::iterator itor = timeLineAnimList.begin();
	int i;

	for (i = 0; i < index; i++)
		itor++;

	//if(i
	return (float*)&(itor->first);
}
Md5Animation* Md5Object::anim(const string &name)
{
	AnimMap::const_iterator itor = _animList.find(name);
	if (itor != _animList.end())
		return itor->second;

	return NULL;
}
Md5Animation* Md5Object::nextAnim()
{
	AnimMap::const_iterator itor = _animList.find(_currAnimName);
	itor++;
	if (itor != _animList.end())
	{
		_currAnimName = itor->second->name();
		return itor->second;
	}
	return Md5AnimationPtr(_currAnim);

}
const AnimMap& Md5Object::anims() const { return _animList; }

Md5Transition::Md5Transition(){}
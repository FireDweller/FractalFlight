/**************************************************************************************************************
 *	File			:	Md5Model.h
 *	Copyright (c)	:	2006, David Henry
 						This code is licenced under the MIT license.
						This software is provided "as is" without express or implied warranties. You may 
						freely copy and compile this source into applications you distribute provided that 
						the copyright text below is included in the resulting source code.
 *	Author			:	David Henry
 *	Date			:	mar. 2, 2006 (David Henry)
 *	Description		:	Declarations for MD5 Model Classes (object, mesh, animation and skeleton).
 *	History			:	mar. 2, 2006

***************************************************************************************************************/
#ifndef __MD5_H__
#define __MD5_H__

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <stdexcept>
#include <GL/gl.h>

#include "Mathlib.h"
#include <Shaders/Shader.h>
using namespace std;


// Forward declarations
class Md5Skeleton;
class Md5Mesh;
class Md5Model;
class Md5Animation;
class Md5Object;

// MD5 Constants
extern const int kMd5Version;

// OpenGL vector types
typedef GLfloat vec2_t[2];
typedef GLfloat vec3_t[3];
typedef GLfloat vec4_t[4];

  typedef Md5Skeleton* Md5SkeletonPtr;
  typedef Md5Mesh* Md5MeshPtr;
  typedef Md5Animation* Md5AnimationPtr;
  typedef map<string, Md5AnimationPtr> AnimMap;
  typedef map<float, Md5AnimationPtr> AnimMapTimed;

struct Md5Joint_t
{
  string name;
  int parent;

  Vector3f pos;
  Quaternionf orient;
};


struct Md5Vertex_t
{
  float st[2]; // Texture coordinates

  int startWeight; // Start index weights
  int countWeight; // Number of weights
};


struct Md5Triangle_t
{
  int index[3]; // Vertex indices
};


struct Md5Weight_t
{
  int joint; // Joint index
  float bias;

  Vector3f pos;
  Vector3f norm;
  Vector3f tan;
};


struct BoundingBox_t
{
  Vector3f minn;
  Vector3f maxx;
};


struct OBBox_t
{
  Matrix4x4f world;

  Vector3f center;
  Vector3f extent;
};


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Exception - Exception class for MD5 loader classes.
// This acts like a standard runtime_error exception but
// know which file or mesh has failed to be loaded.
//
/////////////////////////////////////////////////////////////////////////////

class Md5Exception : public std::runtime_error
{
public:
	// Constructors
	Md5Exception (const string &error)
		: std::runtime_error (error) { }
	Md5Exception (const string &error, const string &name)
		: std::runtime_error (error), _which (name) { }
	virtual ~Md5Exception () throw () { }

	// Public interface
	virtual const char *which () const throw () {
		return _which.c_str ();
	}

private:
	// Member variables
	string _which;
};


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Skeleton - Skeleton model data class.
//
/////////////////////////////////////////////////////////////////////////////

class Md5Skeleton
{
	private:// Internal types
		typedef Md5Joint_t* Md5JointPtr;
		vector<Md5JointPtr> _joints;


	public:
		Md5Skeleton();
		Md5Skeleton (std::ifstream &file, int numJoints)
			throw (Md5Exception);									//Constructor
		~Md5Skeleton();												//Deconstructor


	public:	//Public interface
		
		void setNumJoints (int numJoints);							//Add a joint to the skeleton.
		void addJoint (Md5Joint_t *thisJoint);						//Reserve memory to hold numJoints joints.
		Md5Skeleton *clone () const;								//Dupplicate the skeleton.
		int numJoints() const;
		Md5Joint_t *joint(int index) const;
};


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Mesh - Mesh data class.
//
/////////////////////////////////////////////////////////////////////////////

class Md5Mesh
{
public:
	// Public internal types
	enum{
		kHide,   // Skip mesh
		kNoDraw, // Don't draw but prepare vertices
		kShow,   // Draw mesh
	};
	typedef Md5Vertex_t* Md5VertexPtr;
	typedef Md5Triangle_t* Md5TrianglePtr;
	typedef Md5Weight_t* Md5WeightPtr;


	Md5Mesh (std::ifstream &ifs)
		throw (Md5Exception);			//Constructor.  
										//Load mesh data from a <.md5mesh> file.
	~Md5Mesh ();						//Deconstructor. Free all data allocated for the mesh, i.e. vertices,
										//triangles, weights and vertex arrays.

	void updateVertexArrays(Md5Skeleton *skel);
	void setupVertexArrays (Md5Skeleton *skel);		//Compute vertices' position, normal and tangent.
	void computeWeightNormals (Md5Skeleton *skel);	// --------------------------------------------------------------------------
													// Md5Mesh::computeWeightNormals
													//
													// der_ton said:
													//
													// * First you have to get the bind-pose model-space normals by calculating
													//   them from the model geometry in bind-pose.
													//
													// * Then you calculate the weight's normal (which is in bone-space) by
													//   invert-transforming the normal by the bone-space matrix.
													//
													// * So afterwards when animating, you'll transform the weight normal with
													//   the animated bone-space matrix and add them all up and you'll get
													//   back your animated vertex normal.
													// --------------------------------------------------------------------------

	void computeWeightTangents (Md5Skeleton *skel);	//Compute per-vertex tangent vectors and then, calculate the weight tangent.
	void computeBoundingBox (Md5Skeleton *skel);	//Compute mesh bounding box for a given skeleton.
	void renderVertexArrays (Shader *myShader) const;				//Render mesh with vertex array.
	void initVBO(Shader *myshader);
  
	void setState(int state);// Hide/NoDraw/Show state
	
	// Texture setters
	void setDecalMap(GLuint tex);
	void setSpecularMap(GLuint tex);
	void setNormalMap(GLuint tex);
	void setHeightMap(GLuint tex);
	void setupTexture(GLuint tex, GLenum texUnit, Shader *myShader) const; //Setup texture 'tex' for the given texture unit.
	//Accessors
	const string &name() const;
	const BoundingBox_t &boundingBox() const;

	//Mesh render state
	bool hiden() const;
	bool noDraw() const;
	bool show() const;

private:
	void preRenderVertexArrays() const;					//Pre-rendering preparation.  Setup some render-path dependent stuff,
														//like tangent arrays for ARB programs and shaders.
	void postRenderVertexArrays() const;				//Post-rendering operations.  Render-path dependent cleaning up after
														//mesh rendering, like disabling tangent arrays for ARB programs and
														//shaders.
	void allocVertexArrays();							//Allocate memory for vertex arrays.  NOTE: we need to have triangle
														//data first! (_tris)
	void setupTexCoordArray();							//Compute texture coordinate array.
	

	// Member variables
	string _name;
	string _shader;
	int _renderState;

	BoundingBox_t _boundingBox;

	int _numVerts;
	int _numTris;
	int _numWeights;

	// Original mesh data
	vector<Md5VertexPtr> _verts;
	vector<Md5TrianglePtr> _tris;
	vector<Md5WeightPtr> _weights;

	// Final mesh data; vertex arrays for fast rendering
	vector<GLfloat> _vertexArray;
	vector<GLfloat> _normalArray;
	vector<GLfloat> _tangentArray;
	vector<GLfloat> _texCoordArray;
	vector<GLuint> _vertIndices;

	GLuint glBuffers[3];

	// Textures
	GLuint _decal;
	GLuint _specMap;
	GLuint _normalMap;
	GLuint _heightMap;

	GLuint m_vaoID;
	GLuint ibo;
};


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Model - MD5 Mesh Model class.
//
/////////////////////////////////////////////////////////////////////////////

class Md5Model
{
public:
	Md5Model (const string &filename)
		throw (Md5Exception);										//Constructor. Load MD5 mesh from file.
	~Md5Model ();													//Deconstructor. Free all data allocated for the model.

public:
	// Public interface
	void prepare(Md5Skeleton *skel);								//Prepare each mesh of the model for drawing, i.e. compute final vertex
																	//positions, normals and other vertex's related data.
	void prepare_2(Md5Skeleton *skel);
	void calcCentrePoint();
	void scaleModel(float, float, float);
	void translateModel(float, float, float);
	void rotateModel(float, float, float, float);
	void drawModel(Shader *myShader) const;							//Draw each mesh of the model.
	bool validityCheck(Md5Animation *anim) const;					//Check if an animation is valid for this model or not.  A valid
																	//animation must have a skeleton with the same number of joints with
																	//model's skeleton and for each skeleton joint, name and parent Id must
																	//match.
	void setMeshRenderState(const string &name, int state);			//Setup mesh's state or texture.

	int numJoints() const;
	const Md5Skeleton *baseSkeleton() const;
	const BoundingBox_t &bindPoseBoundingBox() const;

	void initVBO(Shader *myShader);
private:
	void computeBindPoseBoundingBox ();								//Compute model's bounding box in bind-pose.
	Md5Mesh *getMeshByName (const string &name) const;				//Get pointer to a mesh given its name.  Return NULL if there is no mesh
																	//with such a name.
	int _numJoints;
	int _numMeshes;
	Md5SkeletonPtr _baseSkeleton;
	vector<Md5MeshPtr> _meshes;
	BoundingBox_t _bindPoseBox;
};

class Md5Transition
{
public:
	Md5Transition();
	
	Md5Animation* transition;
	Md5Animation* animation;
};

/////////////////////////////////////////////////////////////////////////////
//
// class Md5Animation - MD5 model animation class.
//
/////////////////////////////////////////////////////////////////////////////

class Md5Animation
{
public:
	Md5Animation(const string &filename)			//Constructor. Load MD5 animation from <.md5anim> file.
		throw (Md5Exception);
	~Md5Animation ();								//Deconstructor

private:
	struct JointInfo
  {
    string name;
    int parent;

    // NOTE: this structure is stored in
    // little-endian format
    union JointFlags
    {
      short value;

      struct
      {
	bool tx: 1;
	bool ty: 1;
	bool tz: 1;

	bool qx: 1;
	bool qy: 1;
	bool qz: 1;
      };
    } flags;

    int startIndex;
  };
	struct BaseFrameJoint
  {
    Vector3f pos;
    Quaternionf orient;
  };
	typedef Md5Skeleton* Md5SkeletonPtr;
	typedef BoundingBox_t* BoundingBoxPtr;

public:
	void interpolateFrames (int frameA, int frameB,		//Build an interpolated skeleton given two frame indexes and an
		float interp, Md5Skeleton *out) const;			//interpolation percentage.  'out' must be non-null.
	void interpolateAnims	 (Md5Animation* from, 
		Md5Animation *to, float interp, 
		Md5Skeleton *out);

	int maxFrame () const { return _numFrames - 1; }
	int frameRate () const { return _frameRate; }
	const string& name () const { return _name; }
	float getTotalTime();

  Md5Skeleton *frame (int frame) const {
    return _skelframes[frame];
  }

  const BoundingBox_t *frameBounds (int frame) const {
    return _bboxes[frame];
  }

private:
	void buildFrameSkeleton (vector<JointInfo> &jointInfos,	//Build a skeleton for a particular frame.  The skeleton is transformed
		vector<BaseFrameJoint> &baseFrame,					//by the given modelview matrix so that it is possible to obtain the
		vector<float> &animFrameData);						//skeleton in absolute coordinates.

public:
	// Member variables
	int _numFrames;
	int _frameRate;
	string _name;
	// Store each frame as a skeleton
	vector<Md5SkeletonPtr> _skelframes;

	// Bounding boxes for each frame
	vector<BoundingBoxPtr> _bboxes;
  
	bool isTransition;
	vector<Md5Transition> transitions;
};


/////////////////////////////////////////////////////////////////////////////
//
// class Md5Object - MD5 object class.
//
/////////////////////////////////////////////////////////////////////////////

class Md5Object
{
public:
	enum
	{
		kDrawModel = 1,
		kDrawSkeleton = 2,
		kDrawJointLabels = 4,
	};
	
	Md5Object ();					//constructor
	virtual ~Md5Object ();			//deconstructor
	
	void animate (double dt);		//Compute current and next frames for model's animation.
	void animateFinal(float time);
	float updateCurNextAnimsReturnTimeBetween(float time);
	
	void computeBoundingBox ();		//Compute object's oriented bounding box.
	void prepare (bool);			//Prepare object to be drawn.  Interpolate skeleton frames if the
									//object is animated, or copy model's base skeleton to current
									//skeleton if not.
									//Apply model view transformation if asked.
	void prepare_2(bool);

	void render (Shader *myShader) const;			//Draw the object.
	bool addAnim (const string &filename, bool w = false);
	//bool addTrans (const string &filename);//not implemented
	bool setupTransition(Md5Animation*);
	bool storeTransition(char* animName, char* transName, Md5Animation* anim);
	Md5Animation* getTransition(int animIndex);
	bool addModel(Md5Model * model);
	bool dropAnim (float startTime, Md5AnimationPtr anim);
	void deleteAnim(int index);
	
	// Setters
	void setMd5Model (Md5Model *model);//Attach MD5 Model to Object.
	void setAnim (const string &name);// Set the current animation to play.
	void setRenderFlags(int flags);
	void updateTotalTime();

	// Accessors
	int renderFlags () const { return _renderFlags; }
	const Md5Model *getModelPtr () const { return _model; }
	const string currAnimName () const { return _currAnimName; }
	const OBBox_t &boundingBox () const { return _bbox; }
	float &getTotalTime() { return totalTime; };

	Md5Animation *animByIndex(int index);
	Md5Animation *timeLineAnimByIndex(int index);
	float		 *timeLineAnimStartTimeByIndex(int index);
	Md5Animation *anim (const string &name);
	Md5Animation *nextAnim ();
	const AnimMap &anims() const;

	// Member variables;
	Md5Model *_model;
	Md5Skeleton *_animatedSkeleton;
	Matrix4x4f _modelView;
	bool _softwareTransformation;
	Md5Animation *_currAnim;
	Md5Animation *_nextAnim;
	
	string _currAnimName;
	unsigned int _currFrame;
	unsigned int _nextFrame;
	
	double _last_time;
	double _max_time;
	float totalTime;
	
	int _renderFlags;
	
	AnimMap		_animList;
	AnimMap		_transList;
	AnimMapTimed	timeLineAnimList;
	
	vector<Md5Model*> models;
	OBBox_t _bbox;
};

#endif // __MD5_H__

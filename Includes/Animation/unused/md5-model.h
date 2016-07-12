//
// Class to show md5 meshes from Doom III
//
// Author: Alex V. Boreskoff <alexboreskoff@mtu-net.ru> <steps3d@narod.ru>
//

#ifndef	__MD5_MODEL__
#define	__MD5_MODEL__

#include	<string>
#include	"../Structures/Vector3d.h"
#include	"../Structures/Vector2d.h"
#include	"Matrix3D.h"
#include	"Quaternion.h"
#include	"md5-defs.h"

using namespace std;

class	Data;

class	Md5Mesh
{
public:
	string		shader;
	int			numVertices;
	Vertex    * vertices;
	int			numTris;
	Triangle  * tris;
	int			numWeights;
	Weight    * weights;
	Vector3d  * points;
	int		 	maxJpv;						// maximum joints (weights) per vertex
	unsigned	diffuseMap;
	unsigned	specularMap;
	unsigned	bumpMap;

public:
	Md5Mesh  ();
	~Md5Mesh ();

	void	setNumVertices ( int n );
	void	setNumTris     ( int n );
	void	setNumWeights  ( int n );

	void	setShader      ( const string& sh )
	{
		shader = sh;
	}

	void	setVertex ( int index, const Vector2D& tex, int blendIndex, int blendCount );
	void	setTri    ( int index, int i1, int i2, int i3 );
	void	setWeight ( int index, int bone, float bias, const Vector3D& weight );

	void	calcPoints ( Joint * joints );			// compute real points from bones data
	void	draw       ();							// draw itself
	bool	loadShader ( const string& path );		// load textures

	friend class Model;
};

class	Md5Model
{
public:
	int		    numJoints;
	Joint     * joints;
	int		    numMeshes;
	Md5Mesh   * meshes;

public:
	Md5Model  ();
	~Md5Model ();

	int		getNumJoints () const
	{
		return numJoints;
	}

	Joint * getJoints () const
	{
		return joints;
	}

	void	setNumJoints ( int n );
	void	setNumMeshes ( int n );

	bool	load          ( const string& modelName );
	bool	loadJoints    ( Data * data );

	void	compute      ( Joint * joints );
	void	draw         ();
	void	drawSkeleton ( bool absoluteJoints, Joint * joints );

protected:
	void	drawJoint    ( Joint * joints, int i1, int i2, bool absoluteJoints );
};

bool	getLine   ( Data * data, string& str );
void	splitPath ( const string& fullName, string& fileName, string& ext );

#endif

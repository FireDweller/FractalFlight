//
// Basic structs used for md5 models rendering & animation
//

#ifndef	__MD5_DEFS__
#define	__MD5_DEFS__

#include	"Vector2D.h"
#include	"Vector3D.h"
#include	"Quaternion.h"
#include	<string>

using namespace std;

#ifndef	DOOM_PATH
	#ifdef	_WIN32
		#define	DOOM_PATH	"C:/Games/1C/Activision/DooM 3/base/"
//		#define	DOOM_PATH	"c:/Games/Doom 3/base/"
	#else
	#ifdef	MACOSX
		#define	DOOM_PATH	"/Games/DooM 3/base/"
	#else
	    #define	DOOM_PATH	"/mnt/drive-c/Games/1C/Activision/DooM 3/base/"
	#endif
	#endif	    
#endif

struct	Vertex								// mesh vertex
{
	Vector2D	tex;
	int			weightIndex;
	int			weightCount;
};

struct	Triangle
{
	int	index [3];
};

struct	Weight
{
	int			jointIndex;					// for whuch bone this weight is for
	float		weight;						// looks like a weight of this pos
	Vector3D	pos;
};

class	HierarchyItem
{
public:
	string	name;
	int		parent;
	int		flags;
	int		startIndex;
};

class	BaseFrameJoint
{
public:
	Vector3D	pos;
	Quaternion	orient;
};

class	Joint
{
public:
	string		name;
	int			parentIndex;
	Joint     * parent;
	Vector3D	pos;
	Quaternion	orient;

	Vector3D	transform ( const Vector3D& v ) const
	{
		return orient.rotate ( v ) + pos;
	}

	Vector3D	invTransform ( const Vector3D& v ) const
	{
		Quaternion	c ( orient );

		return c.conj ().rotate ( v - pos );
	}
};

#endif

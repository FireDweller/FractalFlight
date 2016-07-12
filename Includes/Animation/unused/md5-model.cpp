//
// Class to show md5 meshes from Doom III
//
// Author: Alex V. Boreskoff <steps3d@gamil.com> <steps3d@narod.ru>
//

#ifdef	_WIN32
	#include	<windows.h>
#endif

#ifdef	MACOSX
	#include	<OpenGL/gl.h>
#else
	#include	<GL/gl.h>
#endif

#include	<stdio.h>
#include	<ctype.h>

#include	"md5-model.h"
#include	"libTexture.h"
#include	"Data.h"
#include	"StringUtils.h"

void	renormalize ( Quaternion& q );

											// returns false when no more lines left
bool	getLine ( Data * data, string& str )
{
	if ( !data -> getString ( str, '\r' ) )
		return false;
		
	str = trim ( str );

	return true;
}

void	splitPath ( const string& fullName, string& fileName, string& ext )
{
	int	pos = fullName.rfind ( '.' );

	if ( pos != string :: npos )
	{
		fileName = fullName.substr ( 0, pos  );
		ext      = fullName.substr ( pos + 1 );
	}
	else
	{
		fileName = fullName;
		ext      = "";
	}
}

bool	loadMesh ( Data * data, Md5Mesh& mesh )
{
	string	str, cmd, args;

	for ( ; ; )
	{
		if ( !getLine ( data, str ) )
			return false;

		parseString ( str, cmd, args );

		if ( cmd.empty () )
			continue;

		if ( cmd == "shader" )
			mesh.setShader ( dequote ( args ) );
		else
		if ( cmd == "numverts" )
			mesh.setNumVertices ( atoi ( args.c_str () ) );
		else
		if ( cmd == "vert" )
		{
			int		index;
			float	u, v;
			int		weightIndex;
			int		weightCount;

			sscanf ( args.c_str (), "%d ( %f %f ) %d %d", &index, &u, &v, &weightIndex, &weightCount );

			mesh.setVertex ( index, Vector2D ( u, v ), weightIndex, weightCount );
		}
		else
		if ( cmd == "numtris" )
			mesh.setNumTris ( atoi ( args.c_str () ) );
		else
		if ( cmd == "tri" )
		{
			int	index, i1, i2, i3;

			sscanf ( args.c_str (), "%d %d %d %d", &index, &i1, &i2, &i3 );

			mesh.setTri ( index, i1, i2, i3 );
		}
		else
		if ( cmd == "numweights" )
			mesh.setNumWeights ( atoi ( args.c_str () ) );
		else
		if ( cmd == "weight" )
		{
			int		index;
			int		joint;
			float	bias;
			float	x, y, z;

			sscanf ( args.c_str (), "%d %d %f ( %f %f %f )", &index, &joint, &bias, &x, &y, &z );

			mesh.setWeight ( index, joint, bias, Vector3D ( x, y, z ) );
		}
		else
		if ( cmd == "}" )
			break;
	}

							// compute max joints per vertex
	mesh.maxJpv = 0;
	
	for ( int i = 0; i < mesh.numVertices; i++ )
		if ( mesh.vertices [i].weightCount > mesh.maxJpv )
			mesh.maxJpv = mesh.vertices [i].weightCount;
	
//	printf ( "maxJpv = %d\n", mesh.maxJpv );
	
	return true;
}

Md5Mesh :: Md5Mesh ()
{
	numVertices = 0;
	numTris     = 0;
	numWeights  = 0;

	vertices    = NULL;
	tris        = NULL;
	weights     = NULL;
	points      = NULL;
}

Md5Mesh :: ~Md5Mesh ()
{
	delete [] vertices;
	delete [] tris;
	delete [] weights;
	delete [] points;
}

void	Md5Mesh :: setNumVertices ( int n )
{
	numVertices = n;
	vertices    = new Vertex   [n];
	points      = new Vector3D [n];
}

void	Md5Mesh :: setNumTris ( int n )
{
	numTris = n;
	tris    = new Triangle [n];
}

void	Md5Mesh :: setNumWeights ( int n )
{
	numWeights = n;
	weights    = new Weight [n];
}

void	Md5Mesh :: setVertex ( int index, const Vector2D& tex, int weightIndex, int weightCount )
{
	vertices [index].tex         = tex;
	vertices [index].weightIndex = weightIndex;
	vertices [index].weightCount = weightCount;
}

void	Md5Mesh :: setTri ( int index, int i1, int i2, int i3 )
{
	tris [index].index [0] = i1;
	tris [index].index [1] = i2;
	tris [index].index [2] = i3;
}

void	Md5Mesh :: setWeight ( int index, int joint, float bias, const Vector3D& pos )
{
	weights [index].jointIndex = joint;
	weights [index].weight     = bias;
	weights [index].pos        = pos;
}

void	Md5Mesh :: calcPoints ( Joint * joints )		// compute real points from bones data
{
	for ( int i = 0; i < numVertices; i++ )
	{
		Vertex&	v = vertices [i];				// current vertex

		Vector3D	p ( 0, 0, 0 );

		for ( int k = 0; k < v.weightCount; k++ )
		{
			Weight&	weight = weights [v.weightIndex + k];
			Joint&	joint  = joints  [weight.jointIndex];

												// transform weight.pos by bone with weight
			p += joint.transform ( weight.pos ) * weight.weight;
		}

		points [i] = Vector3D ( p.y, p.x, p.z );
	}
}

void	Md5Mesh :: draw ()						// draw itself
{
	glBindTexture ( GL_TEXTURE_2D, diffuseMap );
	glBegin       ( GL_TRIANGLES );

	for ( int i = 0; i < numTris; i++ )
	{
		for ( int k = 0; k < 3; k++ )
		{
			int	index = tris [i].index [k];

			glTexCoord2fv ( vertices [index].tex );
			glVertex3fv   ( points   [index] );
		}
	}

	glEnd ();
}

bool	Md5Mesh :: loadShader ( const string& modelName )	// load textures
{
	string	fileName        = shader;				// texture template
	string	ext             = "tga";
	string	diffuseMapName  = fileName + "_d." + ext;
	string	diffuseMapName2 = fileName + "_h." + ext;
	string	diffuseMapName3 = fileName + "_b." + ext;
	string	diffuseMapName4 = fileName + "."   + ext;

	if ( ( diffuseMap = createTexture2D ( true, diffuseMapName.c_str () ) ) == 0 )
		if ( ( diffuseMap = createTexture2D ( true, diffuseMapName2.c_str () ) ) == 0 )
			if ( ( diffuseMap = createTexture2D ( true, diffuseMapName3.c_str () ) ) == 0 )
				if ( ( diffuseMap = createTexture2D ( true, diffuseMapName4.c_str () ) ) == 0 )
					printf ( "File %s not found.\n", diffuseMapName4.c_str () );

											// locate specular map
	string	specularMapName  = fileName + "_s."  + ext;
	string	specularMapName2 = fileName + "_sp." + ext;

	if ( ( specularMap = createTexture2D ( true, specularMapName.c_str () ) ) == 0 )
		printf ( "File %s not found.\n", specularMapName.c_str () );

											// locate bumpmap
	string	bumpMapName = fileName + "_local." + ext;

	if ( ( bumpMap = createTexture2D ( true, specularMapName.c_str () ) ) == 0 )
		printf ( "File %s not found.\n", bumpMapName.c_str () );


	return true;
}

///////////////////////////////// Md5Model methods ///////////////////////////////////

Md5Model :: Md5Model  ()
{
	numMeshes = 0;
	numJoints = 0;
	joints    = NULL;
	meshes    = NULL;
}

Md5Model :: ~Md5Model ()
{
	delete [] meshes;
}

void	Md5Model :: setNumJoints  ( int n )
{
	numJoints = n;
	joints    = new Joint [n];
}

void	Md5Model :: setNumMeshes ( int n )
{
	numMeshes = n;
	meshes    = new Md5Mesh [n];
}

bool	Md5Model :: loadJoints ( Data * data )
{
	string		str, cmd, args;
	Vector3D	pos, orient;
	int			index;
	Quaternion	q;
	int			no = 0;

	for ( ; ; )
	{
		if ( !getLine ( data, str ) )
			return false;

		parseString ( str, cmd, args );

		if ( cmd == "}" )
			return true;

		sscanf ( args.c_str (), "%d ( %f %f %f ) ( %f %f %f )", &index, &pos.x, &pos.y, &pos.z, &q.x, &q.y, &q.z );

		renormalize ( q );
		
		joints [no  ].parentIndex = index;
		joints [no  ].parent      = index >= 0 ? &joints [index] : NULL;
		joints [no  ].pos         = pos;
		joints [no++].orient      = q;
	}
}

bool	Md5Model :: load ( const string& modelName )
{
	string	fileName = string ( "models/md5/monsters/" ) + modelName + "/" + modelName + ".md5mesh";
	Data  * data     = getFile ( fileName.c_str () );

	if ( data == NULL )
	{
		printf ( "Error loading %s\n", fileName.c_str () );

		return false;
	}

	string	str, cmd, args;
	int		meshNo = 0;

	for ( ; ; )
	{
		if ( !getLine ( data, str ) )
			break;

		parseString ( str, cmd, args );

		if ( cmd.empty () )
			continue;

		if ( cmd == "MD5Version" )
		{

		}
		else
		if ( cmd == "commandline" )
		{

		}
		else
		if ( cmd == "numJoints" )
			setNumJoints ( atoi ( args.c_str () ) );
		else
		if ( cmd == "joints" )
			loadJoints ( data );
		else
		if ( cmd == "numMeshes" )
			setNumMeshes ( atoi ( args.c_str () ) );
		else
		if ( cmd == "mesh" )
			loadMesh ( data, meshes [meshNo++] );
	}

	delete data;
											// load all textures
	for ( int i = 0; i < numMeshes; i++ )
		meshes [i].loadShader ( modelName );

	return true;
}

void	Md5Model :: compute ( Joint * joints )
{
	for ( int i = 0; i < numMeshes; i++ )
		meshes [i].calcPoints ( joints );
}

void	Md5Model :: draw ()
{
	for ( int i = 0; i < numMeshes; i++ )
		meshes [i].draw ();
}

void	Md5Model :: drawSkeleton ( bool absoluteJoints, Joint * joints )
{
	glDisable ( GL_DEPTH_TEST );
	glDisable ( GL_TEXTURE_2D );
	glColor3f ( 1, 1, 1 );
	
	for ( int i = 1; i < numJoints; i++ )
		drawJoint ( joints, i, joints [i].parentIndex, absoluteJoints );

	glEnable ( GL_TEXTURE_2D );
	glEnable ( GL_DEPTH_TEST );
}

void	Md5Model :: drawJoint ( Joint * j, int i1, int i2, bool absoluteJoints )
{
	if ( i1 < 0 || i2 < 0 )
		return;
		
	Vector3D	p1 ( j [i1].pos );
	Vector3D	p2 ( j [i2].pos );
	int		i;

	if ( !absoluteJoints )
	{
		for ( i = j [i1].parentIndex; i >= 0; i = j [i].parentIndex )
			p1 += j [i].pos;

		for ( i = j [i2].parentIndex; i >= 0; i = j [i].parentIndex )
			p2 += j [i].pos;	
	}
	
	glBegin ( GL_LINES );
		glVertex3f ( p1.y, p1.x, p1.z );
		glVertex3f ( p2.y, p2.x, p2.z );
	glEnd  ();
}


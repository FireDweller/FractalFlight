//
// Classes to show Doom III models
// Author: Alex V. Boreskoff <alexboreskoff@mtu-net.ru> <steps3d@narod.ru>
//

#ifdef	_WIN32
	#pragma	warning (disable:4786)
#endif

#include	<string>

//#include	"libExt.h"
#include	"libTexture.h"
#include	"Data.h"
#include	"Vector3D.h"
#include	"Quaternion.h"
#include	"StringUtils.h"
#include	"md5-defs.h"

#ifdef	MACOSX
	#include	<GLUT/glut.h>
#else
	#include	<glut.h>
#endif

Vector3D	eye   ( 0, 0, 150 );		// camera position
Vector3D	rot ( 0, 0, 0 );
int			mouseOldX = 0;
int			mouseOldY = 0;
int			frame     = 0;
Vector3D	boxMin ( 0, 0, 0 );			// bounding box for an entire animation
Vector3D	boxMax ( 0, 0, 0 );

void	renormalize ( Quaternion& q )
{
	double	len = 1 - q.x*q.x - q.y*q.y - q.z*q.z;

	if ( len < 1e-8 )
		q.w = 0;
	else
		q.w = -sqrt ( len );
}

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

HierarchyItem  * hierarchy = NULL;
BaseFrameJoint * baseFrame = NULL;
float         ** frames    = NULL;
Joint          * joints    = NULL;
int				numJoints  = 0;
int				numFrames  = 0;
int				numComponents = 0;						// numAnimatedComponents

void	setNumJoints ( int n )
{
	hierarchy = new HierarchyItem  [numJoints = n];
	baseFrame = new BaseFrameJoint [numJoints];
}
	
void	setNumFrames ( int n )
{
	frames = new float * [numFrames = n];
}

bool	loadHierarchy ( Data * data )
{
	string	str, cmd, args;
	int		parentJointIndex, flags, startIndex;

	for ( int i = 0; ; )
	{
		if ( !getLine ( data, str ) )
			break;

		parseString ( str, cmd, args );

		if ( cmd.empty () )
			continue;

		if ( cmd == "}" )
			return true;

		sscanf ( args.c_str (), "%d %d %d", &parentJointIndex, &flags, &startIndex );

		if ( i < numJoints )
		{
			hierarchy [i].name       = cmd;
			hierarchy [i].parent     = parentJointIndex;
			hierarchy [i].flags      = flags;
			hierarchy [i].startIndex = startIndex;

			i++;
		}
	}

	return true;
}

bool	loadBounds ( Data * data )
{
	string		str, cmd, args;
	Vector3D	vMin, vMax;

	for ( int i = 0; ; )
	{
		if ( !getLine ( data, str ) )
			break;

		parseString ( str, cmd, args );

		if ( cmd.empty () )
			continue;

		if ( cmd == "}" )
			return true;

		if ( cmd == "(" )
			args = cmd + args;

		sscanf ( args.c_str (), "( %f %f %f ) ( %f %f %f )", &vMin.x, &vMin.y, &vMin.z, &vMax.x, &vMax.y, &vMax.z );

		if ( i < numFrames )
		{
//			bounds [i].vMin = vMin;
//			bounds [i].vMax = vMax;
			boxMin = Vector3D :: vmin ( boxMin, vMin );
			boxMax = Vector3D :: vmax ( boxMax, vMax );
			i++;
		}
	}

	return true;
}

bool	loadBaseFrame ( Data * data )
{
	string		str, cmd, args;
	Vector3D	pos, rot;

	for ( int i = 0; ; )
	{
		if ( !getLine ( data, str ) )
			break;

		str = trim ( str );

		if ( str == "}" )
			return true;

		int c = sscanf ( str.c_str (), "( %f %f %f ) ( %f %f %f )", &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z );

		baseFrame [i].pos    = pos;
		baseFrame [i].orient = Quaternion ( rot.x, rot.y, rot.z, 0 );

		renormalize ( baseFrame [i++].orient );
	}

	return true;
}

bool	loadFrame ( Data * data, int no )
{
	string	str, cmd, args;

	frames [no] = new float [numComponents];

	for ( int i = 0; ; )
	{
		if ( !getLine ( data, str ) )
			break;

		do
		{
			parseString ( str, cmd, args );

			if ( cmd.empty () )
				break;

			if ( cmd == "}" )
				return true;

			sscanf ( cmd.c_str (), "%f", &frames [no][i++] );

			str = args;
		} while ( args != "" );
	}

	return true;
}

void	buildJoints ()
{
	for ( int i = 0; i < numJoints; i++ )
		if ( joints [i].parent != NULL )
		{
			joints [i].pos    = joints [i].parent -> pos + joints [i].parent -> orient.rotate ( joints [i].pos );
			joints [i].orient = joints [i].parent -> orient * joints [i].orient;
		}
}

void	resetJoints ()
{
	for ( int i = 0; i < numJoints; i++ )
	{
		joints [i].name        = hierarchy [i].name;
		joints [i].parentIndex = hierarchy [i].parent;
		joints [i].parent      = joints [i].parentIndex == -1 ? NULL : &joints [joints[i].parentIndex];
		joints [i].pos         = baseFrame [i].pos;
		joints [i].orient      = baseFrame [i].orient;
	}
}

bool	load ( const string& fileName )
{
	Data * data = getFile ( fileName.c_str () );

	printf ( "Loading animation %s\n", fileName.c_str () );

	if ( data == NULL )
	{
		printf ( "Animation %s not found.\n", fileName.c_str () );

		return false;
	}

	string	str, cmd, args;

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
		if ( cmd == "numFrames" )
			setNumFrames ( atoi ( args.c_str () ) );
		else
		if ( cmd == "numJoints" )
			setNumJoints ( atoi ( args.c_str () ) );
		else
		if ( cmd == "frameRate" )
			;//setFrameRate ( atoi ( args.c_str () ) );
		else
		if ( cmd == "numAnimatedComponents" )
			numComponents = atoi ( args.c_str () );
		else
		if ( cmd == "hierarchy" )
			loadHierarchy ( data );
		else
		if ( cmd == "bounds" )
			loadBounds ( data );
		else
		if ( cmd == "baseframe" )
			loadBaseFrame ( data );
		else
		if ( cmd == "frame" )
			loadFrame ( data, atoi ( args.c_str () ) );
	}

	delete data;

	printf ( "Preparing joints\n" );

	joints = new Joint [numJoints];
	
	resetJoints ();
	buildJoints ();
	
	return true;
}

void	setFrame ( int no )
{
	if ( no < 0 )
		no = numFrames - 1;
		
	if ( no >= numFrames )
		no = 0;
	
	frame = no;
	
	resetJoints ();
	
	for ( int i = 0; i < numJoints; i++ )
	{
		int	flags = hierarchy [i].flags;
		int	pos   = hierarchy [i].startIndex;
		
		if ( flags & 1 )
			joints [i].pos.x = frames [no][pos++];
			
		if ( flags & 2 )
			joints [i].pos.y = frames [no][pos++];
			
		if ( flags & 4 )
			joints [i].pos.z = frames [no][pos++];
			
		if ( flags & 8 )
			joints [i].orient.x = frames [no][pos++];
			
		if ( flags & 16 )
			joints [i].orient.y = frames [no][pos++];
			
		if ( flags & 32 )
			joints [i].orient.z = frames [no][pos++];
		
		renormalize ( joints [i].orient );
	}
	
	buildJoints ();
	
	printf ( "Frame:%d\n", frame );
}

void init ()
{
	glClearColor ( 0.0, 0.0, 0.0, 1.0 );
	glEnable     ( GL_DEPTH_TEST );
	glEnable     ( GL_TEXTURE_2D );
	glDepthFunc  ( GL_LEQUAL );
	glHint       ( GL_POLYGON_SMOOTH_HINT,         GL_NICEST );
	glHint       ( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
}

void display ()
{
	glClear       ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glMatrixMode ( GL_MODELVIEW );
	glPushMatrix ();

	Vector3D	len   = boxMax - boxMin;
	float		scale = 100.0f / len.length ();
	
	glTranslatef ( -eye.x, -eye.y, -eye.z );
    glRotatef    ( rot.x, 1, 0, 0 );
    glRotatef    ( rot.y, 0, 1, 0 );
    glRotatef    ( rot.z, 0, 0, 1 );
	glScalef     ( scale, scale, scale );

	for ( int i = 1; i < numJoints; i++ )
	{
		Vector3D	p1 = joints [i].parent -> pos;
		Vector3D	p2 = joints [i].pos;
		
		glBegin ( GL_LINES );
			glVertex3f ( p1.y, p1.x, p1.z );
			glVertex3f ( p2.y, p2.x, p2.z );
		glEnd  ();
	}
	
	glPopMatrix     ();
	glutSwapBuffers ();
}

void reshape ( int w, int h )
{
   glViewport     ( 0, 0, (GLsizei)w, (GLsizei)h );
   glMatrixMode   ( GL_PROJECTION );
   glLoadIdentity ();
   gluPerspective ( 60.0, (GLfloat)w/(GLfloat)h, 1, 5000.0 );
   glMatrixMode   ( GL_MODELVIEW );
   glLoadIdentity ();
}

void key ( unsigned char key, int x, int y )
{
	if ( key == 27 || key == 'q' || key == 'Q' )
    	exit ( 0 );
	else
	if ( key == '+' )
		setFrame ( frame + 1 );
	else
	if ( key == '-' )
		setFrame ( frame - 1 );
		
	glutPostRedisplay ();
}

void	animate ()
{
	glutPostRedisplay ();
}

void motion ( int x, int y )
{
    rot.x -= ((mouseOldY - y) * 180.0f) / 200.0f;
    rot.z -= ((mouseOldX - x) * 180.0f) / 200.0f;

    if ( rot.x > 360 )
		rot.x -= 360;

	if ( rot.x < -360 )
		rot.x += 360;

    if ( rot.y > 360 )
		rot.y -= 360;

	if ( rot.y < -360 )
        rot.y += 360;

    mouseOldX = x;
    mouseOldY = y;
}

void mouse ( int button, int state, int x, int y )
{
    if ( state == GLUT_DOWN )
    {
        mouseOldX = x;
        mouseOldY = y;
	}
}

int main ( int argc, char * argv [] )
{

								//initialise glut
	glutInit            ( &argc, argv );
	glutInitDisplayMode ( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
	glutInitWindowSize  ( 700, 700 );


								// create window
	glutCreateWindow ( "Doom III model skeleton animation" );

								// register handlers
	glutDisplayFunc  ( display );
	glutReshapeFunc  ( reshape );
	glutKeyboardFunc ( key     );
	glutMouseFunc    ( mouse   );
    glutMotionFunc   ( motion  );
	glutIdleFunc     ( animate );

	init           ();
	initExtensions ();

	string	doomPath  = DOOM_PATH;
	string	modelName = "maggot3";
	string	animName  = "walk.md5anim";

	if ( argc > 1 )
		modelName =  argv [1];
		
	if ( argc > 2 )
		animName = string ( argv [2] ) + ".md5anim";
		
	if ( argc > 3 )
		doomPath = string ( argv [3] ) + "/";

	string	pak0 = doomPath + "pak000.pk4";
	string	pak1 = doomPath + "pak001.pk4";
	string	pak2 = doomPath + "pak002.pk4";
	string	pak3 = doomPath + "pak003.pk4";
	string	pak4 = doomPath + "pak004.pk4";

	printf           ( "loading model %s.\n", modelName.c_str () );
	addZipFileSystem ( pak2.c_str () );
	addZipFileSystem ( pak4.c_str () );
		
	string	fileName  = string  ( "models/md5/monsters/" ) + modelName + "/" + animName;
		
	if ( !load ( fileName.c_str () ) )
	{
		printf ( "Error loading anim file \"%s\".\n", modelName.c_str () );
		
		return 1;
	}
	
	printf ( "Model loaded.\n" );

	glutMainLoop ();

	return 0;
}

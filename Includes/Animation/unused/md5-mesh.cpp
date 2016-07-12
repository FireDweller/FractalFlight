//
// Doom III model viewer
//
// Author: Alex V. Boreskoff
//

//#include	"libExt.h"
#include	"libTexture.h"
#include	"Vector3D.h"
#include	"Vector2D.h"
#include	"StringUtils.h"
#include	"Data.h"
#include	"md5-model.h"

#ifdef	MACOSX
	#include	<GLUT/glut.h>
#else
	#include	<glut.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>

Vector3D	eye   ( 0, 0, 150 );		// camera position
Vector3D	light ( 0.5, 0, 1 );		// light position
unsigned	normCubeMap;				// normalization cubemap id
unsigned	bumpMap;					// normal map
unsigned	diffuseMap;					// decal (diffuse) texture
unsigned	specularMap;				// specular map

Vector3D	rot ( 0, 0, 0 );
int			mouseOldX = 0;
int			mouseOldY = 0;
float		scale     = 0.07;
bool		drawSkeleton = false;

Md5Model	model;

void	renormalize ( Quaternion& q )
{
	double	len = 1 - q.x*q.x - q.y*q.y - q.z*q.z;

	if ( len < 1e-8 )
		q.w = 0;
	else
		q.w = -sqrt ( len );
}

void init ()
{
	glClearColor ( 0.0, 0.0, 0.0, 1.0 );
	glEnable     ( GL_DEPTH_TEST );
	glEnable     ( GL_TEXTURE_2D );
	glDepthFunc  ( GL_LEQUAL );

	glHint ( GL_POLYGON_SMOOTH_HINT,         GL_NICEST );
	glHint ( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
}

void display ()
{
	glClear       ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glBindTexture ( GL_TEXTURE_2D, diffuseMap );

	glMatrixMode ( GL_MODELVIEW );
	glPushMatrix ();

	glTranslatef ( -eye.x, -eye.y, -eye.z );
    glRotatef    ( rot.x, 1, 0, 0 );
    glRotatef    ( rot.y, 0, 1, 0 );
    glRotatef    ( rot.z, 0, 0, 1 );

	model.draw ();

	if ( drawSkeleton )
		model.drawSkeleton ( true, model.getJoints () );
		
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
		
	if ( key == ' ' )
		drawSkeleton = !drawSkeleton;
		
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
	glutInitWindowSize  ( 500, 500 );

								// create window
	glutCreateWindow ( "Doom III model" );

								// register handlers
	glutDisplayFunc  ( display );
	glutReshapeFunc  ( reshape );
	glutKeyboardFunc ( key     );
	glutMouseFunc    ( mouse   );
    glutMotionFunc   ( motion  );
	glutIdleFunc     ( animate );

	init           ();
	initExtensions ();

	string	modelName = "maggot3";
	string	doomPath  = DOOM_PATH;

	if ( argc > 1 )
		modelName = argv [1];
		
	if ( argc > 2 )
		doomPath = string ( argv [2] ) + "/";

	string	pak0 = doomPath + "pak000.pk4";
	string	pak1 = doomPath + "pak001.pk4";
	string	pak2 = doomPath + "pak002.pk4";
	string	pak3 = doomPath + "pak003.pk4";
	string	pak4 = doomPath + "pak004.pk4";

	printf           ( "loading model %s.\n", modelName.c_str () );
	addZipFileSystem ( pak2.c_str () );
	addZipFileSystem ( pak4.c_str () );

	model.load ( modelName );
	model.compute ( model.getJoints () );

	printf ( "Model loaded.\nPress space bar to toggle skeleton" );

	glutMainLoop ();

	return 0;
}

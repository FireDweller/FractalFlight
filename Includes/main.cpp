//OPENGL 3.2 Game Lab Project

#include <windows.h>			// Header File For Windows
#include <vector>
#include <gl\glew.h>
#include <gl\wglew.h>

#include <RedirectIOToConsole.h>
#include <shaders\Shader.h>		// include shader header file, this is not part of OpenGL

Shader* ObjectShader;			//shader object
Shader* TubeShader;

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtx\vector_angle.hpp>

//#include <Animation\Md5Model.h>

#include <tube.h>
#include <kochSnowflake.h>

#include <Time/FPS.h>			// FPS class
Fps fps;

#include <Text/FreeType.h>
#include <Utilities/MatrixRoutines.h>
using namespace freetype;
Font myfont;

//--- constants ---
float const Pi = 3.14159265359;
float deg2rad = Pi / 180.0f;	//convertion from degree to radians
float rad2deg = 180.0f / Pi;	//convertion from radians to degrees
//-----------------

//---Tube creation---
Tube testTube;
KochSnowflake flake;

float radiusOfSegment = 120.0f;
int edgeLength = 30000;
int dimention = 3;
int vertInSegment = 16;
int edgePartition = 30;
//-------------------

//---MODEL LOADING---
#include <3DStruct\threeDModel.h>
#include <Obj\OBJLoader.h>

ThreeDModel serfer, ball, obstacle, obstacle_n;
OBJLoader objLoader;
//------------------

//------------------
glm::mat4 ProjectionMatrixMain;		// matrix for the orthographic projection
glm::mat4 ModelViewMatrix;		// matrix for the modelling and viewing
glm::mat4 viewingMatrix;		// camera matrix

glm::mat3 normalMatrix;

glm::vec4 View1OFFe;
glm::vec4 View1OFFt;
glm::vec3 View2e;
glm::vec3 View2t;
glm::vec3 View2n;
bool View1, View2;
float ViewPerspective;
//-----------------

//Material properties
float Material_Ambient[4] = {0.1, 0.1, 0.1, 1.0};
float Material_Diffuse[4] = {0.8, 0.8, 0.5, 1.0};
float Material_Specular[4] = {0.9, 0.9, 0.8, 1.0};
float Material_Shininess = 50;
//---------------------

//Light Properties
float Light_Ambient_And_Diffuse[4] = {0.8, 0.8, 0.6, 1.0};
float Light_Specular[4] = {1.0,1.0,1.0,1.0};
float LightPos[4] = {0.0, 100.0, 0.0, 0.0};
//---------------------

//--- Movements -------
int	mouse_x = 0, mouse_y = 0;
bool LeftPressed = false;
int screenWidth = 900, screenHeight = 900;

bool keys[256];

glm::quat q;
glm::vec3 playerPosition;
glm::mat4 playerDirectionMat = glm::mat4(1.0f);
glm::mat4 playerTurn;
glm::mat4 playerTransformations;
glm::mat4 cameraRotation;		// rotation about the local axis
glm::vec3 cameraDir;			// direction of camera
glm::vec3 cameraNorm;			// camera normal
glm::vec3 cameraTarget;
glm::mat4 fireDirectonMat;
glm::vec4 firePoint;

vector<glm::vec3> obstaclePoints;
vector<glm::vec3> obstacleDirections;
vector<float> obstacleRotationSpeed;
glm::vec4 ObstaclePositionNow;

bool fire;
int hit_count;

float speed_delta;
float speed = 3.0f;
float spin_delta;
float spin = 1.0f;

float Xpos = 0.0f;
float Ypos = 0.0f;
float Zpos = 0.0f;
float speedZ = 0.0f;

glm::vec3 PPos;				// movment on XYZ
glm::vec3 PDir;				// direction of plane
glm::vec3 PNorm;			// plane normal
//---------------------

////----- Anim -----
//Md5Model *model;
//Md5Model *model2;
//Md5Object *object;
//Md5Object *object2;
//vector<Md5Model*> models;
//vector<float> animlength;
//float animTime;
////----------------

//OPENGL FUNCTION PROTOTYPES
void init();					//called in winmain when the program starts.
void display();					//called in winmain to draw everything to the screen
void reshape();					//called when the window is resized
void processKeys();				//called in winmain to process keyboard input
void update();					//called in winmain to update variables
void updateTransform(float xinc, float yinc, float zinc);
void objectLoading(char *path, ThreeDModel& model, Shader *shader);
//void initialiseModel();
//void loadAnimations();
//void updateAnim(double elapsedTime);
//---------------------

/*************    START OF OPENGL FUNCTIONS   ****************/

void init()
{
	glClearColor(1.0, 1.0, 1.0, 0.0);						//glClear(GL_COLOR_BUFFER_BIT) in the display function
															//will clear the buffer to this colour
	glEnable(GL_DEPTH_TEST);

	ObjectShader = new Shader;
	if (!ObjectShader->load("Object Shader", "GLSL_Files/basicTransformations.vert", "GLSL_Files/basicTransformations.frag"))
	{
		cout << "failed to load shader" << endl;
	}

	TubeShader = new Shader;
	if (!TubeShader->load("Tube Shader", "GLSL_Files/basic.vert", "GLSL_Files/basic.frag"))
	{
		cout << "failed to load shader" << endl;
	}

	cout << " Loading tube : " << endl;
	cout << "  Radius Of Segment = " << radiusOfSegment << endl;
	cout << "  Edge Length = " << edgeLength << endl;
	cout << "  Dimention = " << dimention << endl;
	cout << "  Vert In Segment = " << vertInSegment << endl;

	testTube.constructGeometry(radiusOfSegment, edgeLength, dimention, vertInSegment);
	testTube.createBuffers(TubeShader);

	//flake.constructGeometryRounded(edgeLength, dimention, radiusOfSegment);
	//flake.createBuffers(mySimpleShader);

	float edgePart = edgeLength / edgePartition;
	testTube.obstaclePositions(edgePart, obstaclePoints, obstacleDirections, obstacleRotationSpeed);

	cout << " Tube loaded : " << endl;

	glUseProgram(ObjectShader->handle());  // use the shader

	glEnable(GL_TEXTURE_2D);

	cout << " Loading models : " << endl;

	objectLoading("Models/ss6.obj", serfer, ObjectShader);
	objectLoading("Models/ball.obj", ball, ObjectShader);
	objectLoading("Models/ball2.obj", obstacle_n, ObjectShader);
	objectLoading("Models/ball2.obj", obstacle, ObjectShader);

	glUseProgram(0); //turn off the current shader

	Font::loadShader("GLSL_Files/basicTexture"); //load a shader for use rendering text
	float colorBlack[] = { 0.0, 0.0, 0.0 };
	myfont.init("Includes/Text/font/BKANT.TTF", 22, colorBlack);	 //load a font 

	//initialiseModel();

	playerPosition = testTube.flake.verts[0];

	float Xc = edgeLength / 2;
	float Yc = 0.0f;
	float Zc = -(glm::sqrt(3.0f) / 6) * edgeLength;

	View2e = glm::vec3(Xc, Yc + edgeLength * 1.5, Zc);
	View2t = glm::vec3(Xc, Yc, Zc);
	View2n = glm::vec3(0.0f, 0.0f, -1.0f);

	View1 = true;
	View2 = false;
	fire = false;
}

void objectLoading(char *path, ThreeDModel& model, Shader *shader)
{
	if (objLoader.loadModel(path, model))//returns true if the model is loaded, puts the model in the model parameter
	{
		cout << " Model loaded " << endl;
		//if you want to translate the object to the origin of the screen,
		//first calculate the centre of the object, then move all the vertices
		//back so that the centre is on the origin.
		model.calcCentrePoint();
		model.centreOnZero();
		model.calcVertNormalsUsingOctree();	//the method will construct the octree if it hasn't already been created.
												//turn on VBO by setting useVBO to true in threeDmodel.cpp default constructor - only permitted on 8 series cards and higher
		model.initDrawElements();
		model.initVBO(shader);
		model.deleteVertexFaceData();
	}
	else
	{
		cout << " model failed to load " << endl;
	}
}

//void initialiseModel()
//{
//	// Delete previous model and object if existing
//	delete model;
//	delete object;
//	//delete model2;
//	//delete object2;
//
//	object = new Md5Object();
//	//object2 = new Md5Object();
//
//	// Load mesh model
//	//model2 = new Md5Model("Animation/Md5Files/miaSwordBigSwing.md5mesh");
//	//models.push_back(model2);
//	//object2->setMd5Model(model2);
//	model = new Md5Model("Animation/Md5Files/mia.md5mesh");
//	models.push_back(model);
//	object->setMd5Model(model);
//
//	loadAnimations();
//}

//void loadAnimations()
//{
//	animlength = vector<float>();
//	animlength.push_back(0.0f);
//
//	object->addAnim("Animation/Md5Files/Idle.md5anim");
//	object->addAnim("Animation/Md5Files/Thrust.md5anim");
//	//object->addAnim("Animation/Md5Files/SwingLR.md5anim");
//	//object->addAnim("Animation/Md5Files/SwingRL.md5anim");
//	//object->addAnim("Animation/Md5Files/SwingLR.md5anim");
//	//object->addAnim("Animation/Md5Files/SwingRL.md5anim");
//
//	////load enemy animations
//	//object2->addAnim("Animation/Md5Files/Idle.md5anim");
//	//object2->addAnim("Animation/Md5Files/Thrust.md5anim");
//
//
//	AnimMap::const_iterator itor = object->_animList.begin();
//	//AnimMap::const_iterator itor2 = object2->_animList.begin();
//
//	object->dropAnim(0.0, Md5AnimationPtr(object->anim("Thrust")));
//	object->updateTotalTime();
//	animlength.push_back(object->getTotalTime());
//
//	//object->dropAnim(animlength[1] , Md5AnimationPtr(object->anim("Thrust")));
//	//object->updateTotalTime();
//	//animlength.push_back(object->getTotalTime());
//
//	printf("0 = %f; 1 = %f;\n", animlength[0], animlength[1]);
//
//	//object2->dropAnim(0.0, Md5AnimationPtr(object2->anim("Idle")));
//}

std::vector<glm::vec3> getPointsForSegmentPositioning(const glm::vec3& startPoint, const glm::vec3& endPoint, float distFromEdgePoint)
{
	float dist = glm::distance(startPoint, endPoint);

	std::vector<glm::vec3> points;

	glm::vec3 v = endPoint - startPoint;

	float angle = glm::orientedAngle(glm::vec2(v.x, v.z), glm::vec2(1.0f, 0.0f)); //angle between vector v and x axis

	glm::vec3 point(glm::cos(angle) / distFromEdgePoint, 0.0f, glm::sin(angle) / distFromEdgePoint);
	points.push_back(point);
	point = glm::vec3(glm::cos(angle) / dist - distFromEdgePoint, 0.0f, glm::sin(angle) / dist - distFromEdgePoint);
	points.push_back(point);

	return points;
}

void display()									
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);

	glUseProgram(ObjectShader->handle());  // use the shader

	GLuint matLocation = glGetUniformLocation(ObjectShader->handle(), "ProjectionMatrix");
	glUniformMatrix4fv(matLocation, 1, GL_FALSE, &ProjectionMatrixMain[0][0]);

	glm::vec3 View1eye = glm::vec3(cameraTarget.x + View1OFFe.x, cameraTarget.y + View1OFFe.y, cameraTarget.z + View1OFFe.z);
	glm::vec3 View1target = cameraTarget;
	glm::vec3 View1normal = cameraNorm;

	if (View1)
	{
		viewingMatrix = glm::lookAt(View1eye, View1target, View1normal);
		ProjectionMatrixMain = glm::perspective(60.0f, (GLfloat)screenWidth / (GLfloat)screenHeight, 1.0f, float(edgeLength / 10));
	}
	if (View2)
	{
		viewingMatrix = glm::lookAt(View2e, View2t, View2n);
		ProjectionMatrixMain = glm::perspective(60.0f, (GLfloat)screenWidth / (GLfloat)screenHeight, 1.0f, -(glm::sqrt(3.0f) / 6) * edgeLength - radiusOfSegment);
	}

	glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ViewMatrix"), 1, GL_FALSE, &viewingMatrix[0][0]);

	float LightPos [4] = {playerPosition.x, playerPosition.y + 50, playerPosition.z, 0.0};

	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "LightPos"), 1, LightPos);
	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "light_ambient"), 1, Light_Ambient_And_Diffuse);
	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "light_diffuse"), 1, Light_Ambient_And_Diffuse);
	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "light_specular"), 1, Light_Specular);

	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "material_ambient"), 1, Material_Ambient);
	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "material_diffuse"), 1, Material_Diffuse);
	glUniform4fv(glGetUniformLocation(ObjectShader->handle(), "material_specular"), 1, Material_Specular);
	glUniform1f(glGetUniformLocation(ObjectShader->handle(), "material_shininess"), Material_Shininess);
	glUseProgram(0); //turn off the current shader

	//DRAW THE MODEL
	glUseProgram(TubeShader->handle());  // use the shader
	glm::mat4 TubeMat = viewingMatrix;
	glUniformMatrix4fv(glGetUniformLocation(TubeShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &TubeMat[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(TubeShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, &ProjectionMatrixMain[0][0]);
	testTube.render();
	//flake.render();
	glUseProgram(0); //turn off the current shader
	//--------------

	// PLAYER
	glUseProgram(ObjectShader->handle());  // use the shader
	glm::mat4 Player = glm::translate(viewingMatrix, glm::vec3(playerPosition.x, playerPosition.y, playerPosition.z));		// movement on XYZ
	Player = Player * playerDirectionMat;
	Player = glm::translate(Player, glm::vec3(0.0f, -radiusOfSegment*0.7, 0.0f));
	normalMatrix = glm::inverseTranspose(glm::mat3(Player));
	glUniformMatrix3fv(glGetUniformLocation(ObjectShader->handle(), "NormalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Player[0][0]);
	serfer.drawElementsUsingVBO(ObjectShader);
	glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, &ProjectionMatrixMain[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Player[0][0]);
	glUseProgram(0); //turn off the current shader
	//---------

	// PROJ
	if (fire)
	{
		glUseProgram(ObjectShader->handle());  // use the shader
		glm::mat4 Ball = glm::translate(viewingMatrix, glm::vec3(firePoint.x, firePoint.y, firePoint.z));		// movement on XYZ
		normalMatrix = glm::inverseTranspose(glm::mat3(Ball));
		glUniformMatrix3fv(glGetUniformLocation(ObjectShader->handle(), "NormalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Ball[0][0]);
		ball.drawElementsUsingVBO(ObjectShader);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, &ProjectionMatrixMain[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Ball[0][0]);
		glUseProgram(0); //turn off the current shader
	}
	//---------

	// Obstacle object
	if (!obstaclePoints.empty())
	{
		static int num, num_n;
		static float spinO, spinO_n;

		if (glm::length(obstaclePoints[num] - playerPosition) - speedZ < 10.0f)
		{
			num++;
			if (num == obstaclePoints.size()) num = 0;
			num_n = num + 1;
			if (num_n == obstaclePoints.size()) num_n = 0;
			spinO = spinO_n;
		}

		spinO += obstacleRotationSpeed[num];
		spinO_n += obstacleRotationSpeed[num_n];
		if (spinO > 360) spinO = 0;
		if (spinO_n > 360) spinO_n = 0;


		glUseProgram(ObjectShader->handle());  // use the shader
		glm::mat4 Obstacle = glm::translate(viewingMatrix, obstaclePoints[num]);		// movement on XYZ
		Obstacle = glm::rotate(Obstacle, spinO, obstacleDirections[num]);
		Obstacle = glm::translate(Obstacle, glm::vec3(0.0f, -radiusOfSegment*0.7, 0.0f));
		normalMatrix = glm::inverseTranspose(glm::mat3(Obstacle));
		glUniformMatrix3fv(glGetUniformLocation(ObjectShader->handle(), "NormalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Obstacle[0][0]);
		obstacle.drawElementsUsingVBO(ObjectShader);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, &ProjectionMatrixMain[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Obstacle[0][0]);
		glUseProgram(0); //turn off the current shader

		glUseProgram(ObjectShader->handle());  // use the shader
		glm::mat4 Obstacle_n = glm::translate(viewingMatrix, obstaclePoints[num_n]);		// movement on XYZ
		Obstacle_n = glm::rotate(Obstacle_n, spinO_n, obstacleDirections[num_n]);
		Obstacle_n = glm::translate(Obstacle_n, glm::vec3(0.0f, -radiusOfSegment*0.7, 0.0f));
		normalMatrix = glm::inverseTranspose(glm::mat3(Obstacle));
		glUniformMatrix3fv(glGetUniformLocation(ObjectShader->handle(), "NormalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Obstacle_n[0][0]);
		obstacle_n.drawElementsUsingVBO(ObjectShader);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, &ProjectionMatrixMain[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Obstacle_n[0][0]);
		glUseProgram(0); //turn off the current shader
	}
	//-----------------

	//// MD5MODEL
	//glUseProgram(ObjectShader->handle());  // use the shader
	//glm::mat4 Model = glm::translate(viewingMatrix, glm::vec3(0.0, -100.0, 0.0));		// movement on XYZ
	//normalMatrix = glm::inverseTranspose(glm::mat3(Model));
	//glUniformMatrix3fv(glGetUniformLocation(ObjectShader->handle(), "NormalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
	//glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Model[0][0]);
	//model->drawModel(ObjectShader);
	////model->mesh->renderVertexArrays(ObjectShader);
	//glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, &ProjectionMatrixMain[0][0]);
	//glUniformMatrix4fv(glGetUniformLocation(ObjectShader->handle(), "ModelViewMatrix"), 1, GL_FALSE, &Model[0][0]);
	//glUseProgram(0); //turn off the current shader
	////----------------

	// ----- TEXT -----
	glUseProgram(Font::myShader->handle());	// use the shader
	//change coordinate system to work in 2d
	float twoDprojection[16];
	MatrixRoutines<float>::Ortho2D(0.0, (float)screenWidth, 0.0, (float)screenHeight, twoDprojection);
	//set the projection matrix to the uniform variable used for the font shader
	glUniformMatrix4fv(glGetUniformLocation(Font::myShader->handle(), "ProjectionMatrix"), 1, GL_FALSE, twoDprojection);
	//font, x position, y position, string of text and a float
	int FPS_NOW = fps.get_fps();
	print(myfont, 20, screenHeight - 50, "FPS: %d", FPS_NOW);
	print(myfont, screenWidth - 150, screenHeight - 50, "Hits: %d", hit_count);
	glBindVertexArray(0); //unbind the vertex array object
	glUseProgram(0); //turn off the current shader
	// ----------------

	glFlush();
}

void reshape(int width, int height)		// Resize the OpenGL window
{
	screenWidth=width; screenHeight = height;           // to ensure the mouse coordinates match 
														// we will use these values to set the coordinate system

	glViewport(0,0,width,height);						// Reset The Current Viewport

	//Set the projection matrix
	ProjectionMatrixMain = glm::perspective(60.0f, (GLfloat)screenWidth/(GLfloat)screenHeight, 1.0f, 500.0f);
}

void processKeys()
{
	if (keys['D'])
	{
		Zpos = -spin_delta;
	}
	else if (keys['A'])
	{
		Zpos = spin_delta;
	}
	else
	{
		Zpos = 0;
	}
	//-----
	if (keys['W'])
	{
		speedZ = speed_delta;
	}
	else if (keys['S'])
	{
		speedZ = speed_delta;
	}
	else
	{
		speedZ = 0;
	}
	////-----
	//if (mouse_x < screenWidth / 3)
	//{
	//	Ypos = spin;
	//}
	//else if (mouse_x > screenWidth * 2 / 3)
	//{
	//	Ypos = -spin;
	//}
	//else
	//{
	//	Ypos = 0;
	//}
	////-----
	//if (mouse_y < screenHeight / 3)
	//{
	//	Xpos = -spin;
	//}	
	//else if (mouse_y > screenHeight * 2 / 3)
	//{
	//	Xpos = spin;
	//}
	//else
	//{
	//	Xpos = 0;
	//}
	////-----

	//if (keys['A'])
	//{
	//	Ypos = 5.0f;
	//}
	//if (keys['D'])
	//{
	//	Ypos = -5.0f;
	//}
	//if (keys['D'] == false & keys['A'] == false)
	//{
	//	Ypos = 0;
	//}
	//if (keys['D'] & keys['A'])
	//{
	//	Ypos = 0;
	//}
	////-----
	//if (keys['W'])
	//{
	//	Xpos = 5.0f;
	//}
	//if (keys['S'])
	//{
	//	Xpos = -5.0f;
	//}
	//if (keys['S'] == false & keys['W'] == false)
	//{
	//	Xpos = 0;
	//}
	//if (keys['S'] & keys['W'])
	//{
	//	Xpos = 0;
	//}
	////-----
	//if (keys[VK_RIGHT])
	//{
	//	Zpos = -5.0f;
	//}
	//if (keys[VK_LEFT])
	//{
	//	Zpos = 5.0f;
	//}
	//if (keys[VK_RIGHT] == false & keys[VK_LEFT] == false)
	//{
	//	Zpos = 0;
	//}
	//if (keys[VK_RIGHT] & keys[VK_LEFT])
	//{
	//	Zpos = 0;
	//}
	////-----
	//if (keys[VK_UP])
	//{
	//	speedZ = 3000.0f;
	//}
	//if (keys[VK_DOWN])
	//{
	//	speedZ = -3000.0f;
	//}
	//if (keys[VK_UP] == false & keys[VK_DOWN] == false)
	//{
	//	speedZ = 0;
	//}
	//if (keys[VK_UP] & keys[VK_DOWN])
	//{
	//	speedZ = 0;
	//}

	if (keys['1'])
	{
		View1 = true;
		View2 = false;
	}
	if (keys['2'])
	{
		View1 = false;
		View2 = true;
	}

	if (keys['F'])
	{
		fire = true;
		keys['F'] = false;
	}
	if (keys['R'])
	{
		fire = false;
	}

	//updateTransform(Xpos, Ypos, Zpos);
}

void updateTransform(float xinc, float yinc, float zinc)
{
	glm::mat4 matrixX, matrixXY;

	//rotation about the local x axis
	q = glm::angleAxis(xinc, glm::vec3(cameraRotation[0][0], cameraRotation[0][1], cameraRotation[0][2]));
	matrixX = glm::mat4_cast(q) * cameraRotation;

	//EXAMPLE FOR ACCESSING USING A 1D array
	const float *pSource = (const float*)glm::value_ptr(matrixX);
	//rotation about the local y axis
	q = glm::angleAxis(yinc, glm::vec3(pSource[4], pSource[5], pSource[6]));
	matrixXY = glm::mat4_cast(q) * matrixX;

	//EXAMPLE ACCESSING WITH 2D GLM structure.
	//rotation about the local z axis
	q = glm::angleAxis(zinc, glm::vec3(matrixXY[2][0], matrixXY[2][1], matrixXY[2][2]));
	cameraRotation = glm::mat4_cast(q) * matrixXY;
}

void update()
{
	fps.update();
	float fps_now = fps.get_fps();
	float frames_now = fps.get_frames();
	speed_delta = speed * (1 / frames_now);
	spin_delta = spin * (1 / frames_now);

	testTube.playerPosition(playerPosition, cameraTarget, playerDirectionMat, playerTurn, speedZ, Zpos);

	playerTransformations = glm::translate(glm::mat4(1.0), glm::vec3(playerPosition.x, playerPosition.y, playerPosition.z));
	playerTransformations = playerTransformations * playerDirectionMat;
	playerTransformations = glm::translate(playerTransformations, glm::vec3(0.0f, -radiusOfSegment*0.7, 0.0f));

	cameraNorm = glm::vec3(playerDirectionMat[1][0], playerDirectionMat[1][1], playerDirectionMat[1][2]);
	View1OFFe = playerDirectionMat * glm::vec4(0, 0, radiusOfSegment*1.5, 1);
	View1OFFt = playerDirectionMat * glm::vec4(cameraTarget, 1);

	glm::vec4 front_coord(0, 0, -50, 1);
	front_coord = playerTransformations * front_coord;

	if (fire)
	{
		float speedR = speed_delta * 3;
		firePoint += -speedR * glm::vec4(fireDirectonMat[2][0], fireDirectonMat[2][1], fireDirectonMat[2][2], fireDirectonMat[2][3]);
	}
	else
	{
		firePoint = front_coord;
		fireDirectonMat = playerTransformations;
	}
	if (!testTube.collisionBetweenPoint(glm::vec3(firePoint.x, firePoint.y, firePoint.z), speed_delta * 3, 0, 3))
	{
		fire = false;
	}

	//if (obstacle.collisionBetweenPoint(new Vector3d(front_coord.x, front_coord.y, front_coord.z), 50.0f))
	//{
	//	hit_count++;
	//}
}

//void updateAnim(double deltaTime)
//{
//	animTime += deltaTime;
//
//	int playAnim = 1;
//
//	if (animTime > animlength[playAnim]) {
//		animTime = 0;
//	}
//
//	object->animateFinal(animlength[playAnim - 1] + animTime);
//}

/**************** END OPENGL FUNCTIONS *************************/

//WIN32 functions
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc
void KillGLWindow();									// releases and destroys the window
bool CreateGLWindow(char* title, int width, int height); //creates the window
int WINAPI WinMain(	HINSTANCE, HINSTANCE, LPSTR, int);  // Win32 main function

//win32 global variabless
HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application


/******************* WIN32 FUNCTIONS ***************************/
int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	bool	done=false;								// Bool Variable To Exit Loop

	RedirectIOToConsole();

	//RECT desktop;
	//// Get a handle to the desktop window
	//const HWND hDesktop = GetDesktopWindow();
	//// Get the size of screen to the variable desktop
	//GetWindowRect(hDesktop, &desktop);
	//// The top left corner will have coordinates (0,0)
	//// and the bottom right corner will have coordinates
	//// (horizontal, vertical)
	////screenWidth = desktop.right; screenHeight = desktop.bottom;

	// Create Our OpenGL Window
	if (!CreateGLWindow("GameLab Project",screenWidth,screenHeight))
	{
		return 0;									// Quit If Window Was Not Created
	}

	//SetCursorPos(screenWidth / 2, screenHeight / 2);
	//ShowCursor(false);

	//float start_time = GetTickCount();
	//float elapsed_time = 0.0f;
	//float elapsed_time_prev = 0.0f;

	update();

	while(!done)									// Loop That Runs While done=FALSE
	{
		//elapsed_time = (GetTickCount() - start_time) / 1000;
		//float delta_time = elapsed_time - elapsed_time_prev;

		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=true;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			if(keys[VK_ESCAPE])
				done = true;

			processKeys();			//process keyboard
			display();				// Draw The Scene
			update();				// update variables
			//updateAnim(delta_time);
			SwapBuffers(hDC);		// Swap Buffers (Double Buffering)
		}

		//elapsed_time_prev = elapsed_time;
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return (int)(msg.wParam);						// Exit The Program
}

//WIN32 Processes function - useful for responding to user inputs or other events.
LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}
		break;

		case WM_SIZE:								// Resize The OpenGL Window
		{
			reshape(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
		break;

		case WM_LBUTTONDOWN:
			{
	            mouse_x = LOWORD(lParam);          
				mouse_y = screenHeight - HIWORD(lParam);
				LeftPressed = true;
			}
		break;

		case WM_LBUTTONUP:
			{
	            LeftPressed = false;
			}
		break;

		case WM_MOUSEMOVE:
			{
	            mouse_x = LOWORD(lParam);          
				mouse_y = screenHeight  - HIWORD(lParam);
			}
		break;
		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = true;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}
		break;
		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = false;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}
		break;
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

void KillGLWindow()								// Properly Kill The Window
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*/
 
bool CreateGLWindow(char* title, int width, int height)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;											// Return FALSE
	}
	
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;					// Window Extended Style
	dwStyle = WS_OVERLAPPEDWINDOW;// | WS_MAXIMIZE;					// Windows Style
	
	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		24,											// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		24,											// 24Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	HGLRC tempContext = wglCreateContext(hDC);
	wglMakeCurrent(hDC, tempContext);

	glewInit();

	int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};
	
    if(wglewIsSupported("WGL_ARB_create_context") == 1)
    {
		hRC = wglCreateContextAttribsARB(hDC,0, attribs);
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(tempContext);
		wglMakeCurrent(hDC, hRC);
	}
	else
	{	//It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
		hRC = tempContext;
		cout << " not possible to make context "<< endl;
	}

	//Checking GL version
	const GLubyte *GLVersionString = glGetString(GL_VERSION);

	cout << GLVersionString << endl;

	//OpenGL 3.2 way of checking the version
	int OpenGLVersion[2];
	glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

	cout << OpenGLVersion[0] << " " << OpenGLVersion[1] << endl;

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	reshape(width, height);					// Set Up Our Perspective GL Screen

	init();
	
	return true;									// Success
}




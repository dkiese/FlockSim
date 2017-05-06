/**
 * Author:	Andrew Robert Owens
 * Email:	arowens [at] ucalgary.ca
 * Date:	January, 2017
 * Course:	CPSC 587/687 Fundamental of Computer Animation
 * Organization: University of Calgary
 *
 * Copyright (c) 2017 - Please give credit to the author.
 *
 * Took code snippets from learnopengl.com (instancing) and integrated it
 *
 * File:	main.cpp
 *
 * Summary:
 *
 * This is a (very) basic program to
 * 1) load shaders from external files, and make a shader program
 * 2) make Vertex Array Object and Vertex Buffer Object for the quad
 *
 * take a look at the following sites for further readings:
 * opengl-tutorial.org -> The first triangle (New OpenGL, great start)
 * antongerdelan.net -> shaders pipeline explained
 * ogldev.atspace.co.uk -> good resource
 */
using namespace std;

#include <iostream>
#include <cmath>
#include <chrono>
#include <limits>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include "Camera.h"

#include <string>
#include <sstream>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

//==================== GLOBAL VARIABLES ====================//
/*	Put here for simplicity. Feel free to restructure into
*	appropriate classes or abstractions.
*/

// Drawing Program
GLuint BasicProgramID;
GLuint instanceProgramID;

// Data needed for Quad
GLuint vaoID;
GLuint vertBufferID;
Mat4f M;

// Data needed for line
GLuint line_vaoID;
GLuint line_vertBufferID;
Mat4f line_M;

// Only one camera so only one veiw and perspective matrix are needed.
Mat4f V;
Mat4f P;

// Only one thing is rendered at a time, so only need one MVP
// When drawing different objects, update M and MVP = M * V * P
Mat4f MVP;

//Instancing shit
GLuint instanceVBO;
//drawing variables
int amountofQuads;
int amountofLines;
std::vector<Vec3f> verts;

//the grid we are creating size
int row;
int column;
GLfloat ObjectsToDraw = 0.0;
//char* pathname = (char *)"./obj/scene.txt";
char* pathname;
float PI = 3.14159265359;
float ZMAX = 0;
float iterations = 0;
float Radius = 0;
float boundSize = 0;
float boundSpeed = 0;
Vec3f Wind;
float limVelocity;

struct Cell {
	int ID;
	Vec3f position;
	Vec3f velocity;
	Mat4f orientation;
	float heading;
	bool isEmpty = true;
};

struct Index {
	int index;
	Vec3f velocity;
};

std::vector<Index> objectIndex;
std::vector<Cell> globalObjects;

// Camera and veiwing Stuff
Camera camera;
int g_moveUpDown = 0;
int g_moveLeftRight = 0;
int g_moveBackForward = 0;
int g_rotateLeftRight = 0;
int g_rotateUpDown = 0;
int g_rotateRoll = 0;
float g_rotationSpeed = 0.015625;
float g_panningSpeed = 0.25;
bool g_cursorLocked;
float g_cursorX, g_cursorY;

bool g_play = false;

int WIN_WIDTH = 800, WIN_HEIGHT = 800;
int FB_WIDTH = 800, FB_HEIGHT = 600;
float WIN_FOV = 60;
float WIN_NEAR = 0.01;
float WIN_FAR = 1000;

//==================== FUNCTION DECLARATIONS ====================//
void displayFunc();
void resizeFunc();
void init();
void generateIDs();
void deleteIDs();
void setupVAO();
void loadQuadGeometryToGPU();
void loadInstanceGeometryToGPU();
void reloadProjectionMatrix();
void loadModelViewMatrix();
void setupModelViewProjectionTransform();
void windowSetSizeFunc();
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowSetSizeFunc(GLFWwindow *window, int width, int height);
void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height);
void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void animateQuad(float t);
void moveCamera();
void reloadMVPUniform();
void reloadColorUniform(float r, float g, float b);
void reloadColorUniformLine(float r, float g, float b);
std::string GL_ERROR();
int main(int, char **);
void loadLineGeometeryToGPU(std::vector<Vec3f> verts);
std::vector<Vec3f> scale(std::vector<Vec3f> vectors, float scaler);
double length(Vec3f a, Vec3f b);
void updateInstanceGeometry();
void loadScene();
void loadLineGeometeryToGPU(std::vector<Vec3f> verts);
void loadBoundary();
void simulate(float t);
Vec3f rule1(Cell boid);
Vec3f rule2(Cell boid);
Vec3f rule3(Cell boid);
Vec3f bound_position(Cell boid);
Vec3f wind(Cell boid);
//==================== FUNCTION DEFINITIONS ====================//
// scaling any vectors;
std::vector<Vec3f> scale(std::vector<Vec3f> vectors, float scaler){
	for(int i = 0; i < vectors.size(); i++){
		vectors[i].x() = vectors[i].x()*scaler;
		vectors[i].y() = vectors[i].y()*scaler;
		vectors[i].z() = vectors[i].z()*scaler;
	}
	return vectors;
}

void displayFunc() {
  glClearColor(0,0,1,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use our shader
  glUseProgram(instanceProgramID);

  // ===== DRAW QUAD ====== //

	MVP = P * V * M;
	reloadMVPUniform();
	reloadColorUniform(1, 0, 1);

	// Use VAO that holds buffer bindings
	// and attribute config of buffers
	glBindVertexArray(vaoID);
	// Draw Quads, start at vertex 0, draw 4 of them (for a quad)
	glDrawArraysInstanced(GL_TRIANGLES, 0, amountofQuads, ObjectsToDraw);	

	 // ==== DRAW LINE ===== //
	glUseProgram(BasicProgramID);
  MVP = P * V * line_M;
  reloadMVPUniform();

  reloadColorUniformLine(0, 0, 1);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(line_vaoID);
  // Draw lines
  glDrawArrays(GL_LINES, 0, amountofLines);
	
}

void animateQuad(float t) {
	//M = TranslateMatrix(0,0,0) * M; // using M will rotate every quad at the same time
	updateInstanceGeometry();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
	simulate(t);
}

float vmax;
float pmax;
// START FLOCKING NOW
void simulate(float t){
	Vec3f v1,v2,v3,v4,v5;
	for(int i = 0; i < globalObjects.size(); i++){
		v1 = rule1(globalObjects[i]);
		v2 = rule2(globalObjects[i]);
		v3 = rule3(globalObjects[i]);
		v4 = bound_position(globalObjects[i]);
		v5 = wind(globalObjects[i]);
	
		globalObjects[i].velocity += v1 + v3 + v2 + v4 + v5;

		//cout << "velocity" << abs(globalObjects[i].velocity.x()) << endl;
		float limit = limVelocity;
		if(abs(globalObjects[i].velocity.x()) > limit){
			globalObjects[i].velocity.x() = globalObjects[i].velocity.x() / abs(globalObjects[i].velocity.x()) * limit;
		}
		if(abs(globalObjects[i].velocity.y()) > limit){
			globalObjects[i].velocity.y() = globalObjects[i].velocity.y() / abs(globalObjects[i].velocity.y()) * limit;
		}
		if(abs(globalObjects[i].velocity.z()) > limit){
			globalObjects[i].velocity.z() = globalObjects[i].velocity.z() / abs(globalObjects[i].velocity.z()) * limit;
		}

		globalObjects[i].position += globalObjects[i].velocity * t;
		if(globalObjects[i].velocity.z() > vmax){
			vmax = globalObjects[i].velocity.z();
		}
		if(globalObjects[i].position.z() > pmax){
			pmax = globalObjects[i].position.z();
		}
		cout << "PMAX" << pmax << endl;
		cout << "VAMX" << vmax << endl;
		cout << "boid 0 x velocity:" << globalObjects[0].velocity.x() << endl;
		cout << "boid 0 y velocity:" << globalObjects[0].velocity.y() << endl; 
		cout << "boid 0 z velocity:" << globalObjects[0].velocity.z() << endl;  

		cout << "boid 0 x position:" << globalObjects[0].position.x() << endl;
		cout << "boid 0 y position:" << globalObjects[0].position.y() << endl; 
		cout << "boid 0 z position:" << globalObjects[0].position.z() << endl;  
	}
}

//Rule 1: Boid try to fly towards the centre of mass of neighbouring boids prob
Vec3f rule1(Cell boid){
	Vec3f center;
	/*calculating the perceived center which is the average positon of all the boids 
		except for the boid we are currently looking at, assuming everyone has the same mass */
	for(int i = 0; i < globalObjects.size(); i++){
		if(globalObjects[i].ID != boid.ID){
			center += globalObjects[i].position;
		}
	}
	center = center / (globalObjects.size()-1);
	center = (center - boid.position) / 100;
	return center;
}

//Rule 2: Boids try to keep a small distance away from other objects
Vec3f rule2(Cell boid){
	Vec3f distance = Vec3f(0.0f,0.0f,0.0f);
	for(int i = 0; i < globalObjects.size(); i++){
		if(globalObjects[i].ID != boid.ID){
			if(abs(globalObjects[i].position.length() - boid.position.length()) < Radius){
				distance += -(globalObjects[i].position - boid.position);
			}
		}
	}
	
	return distance;
}

//Rule 3: Boids try to match velocity with near boids
Vec3f rule3(Cell boid){
	// averaging the velocity of the boids instead of position
	Vec3f velocity;
	for(int i = 0; i < globalObjects.size(); i++){
		if(globalObjects[i].ID != boid.ID){
			velocity += globalObjects[i].velocity;
		}
	}
	
	velocity = velocity / (globalObjects.size() -1);
	velocity = (velocity - boid.velocity) / 8;
	return velocity;// we only need about an eith of the speed its on the bottom of the heirachy
}

Vec3f bound_position(Cell boid){
	int Ymin = -boundSize;
	int Ymax = boundSize;

	int Zmin = -boundSize;
	int Zmax = boundSize;

	int Xmin = -boundSize;
	int Xmax = boundSize;

	Vec3f boundary;	
	if(boid.position.y() < Ymin)
		boundary.y() = boundSpeed;
	else if(boid.position.y() > Ymax)
		boundary.y() = -boundSpeed;

	if(boid.position.z() < Zmin){
		boundary.z() = boundSpeed;
	}
	else if(boid.position.z() > Zmax)
		boundary.z() = -boundSpeed;

	if(boid.position.x() < Xmin)
		boundary.x() = boundSpeed;
	else if(boid.position.z() > Xmax)
		boundary.x() = -boundSpeed;

	return boundary;
}

Vec3f wind(Cell boid){
	Vec3f wind = Wind;
	return wind;
}

void loadQuadGeometryToGPU() {

	GLfloat quadVertices[] = {
		// Positions   				 // Colors
    0.0f,  0.05f,  0.0f,  1.0f, 0.0f, 0.0f,
   -0.05f,-0.05f, 0.05f,  0.0f, 1.0f, 0.0f,
    0.05f,-0.05f, 0.05f,  0.0f, 0.0f, 1.0f,

		0.0f,  0.05f,  0.0f,  1.0f, 0.0f, 0.0f,
    0.05f,-0.05f, 0.05f,  0.0f, 0.0f, 1.0f,
    0.05f,-0.05f,-0.05f,  0.0f, 1.0f, 0.0f,

		0.0f,  0.05f,  0.0f,  1.0f, 0.0f, 0.0f,
    0.05f,-0.05f,-0.05f,  0.0f, 1.0f, 0.0f,
   -0.05f,-0.05f,-0.05f,  0.0f, 0.0f, 1.0f,

		0.0f,  0.05f,  0.0f,  1.0f, 0.0f, 0.0f,
   -0.05f,-0.05f,-0.05f,  0.0f, 0.0f, 1.0f,
   -0.05f,-0.05f, 0.05f,  0.0f, 1.0f, 0.0f,

	};	
	amountofQuads = 12;
  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(quadVertices), // byte size of Vec3f, 4 of them
               quadVertices,      // pointer (Vec3f*) to contents of verts
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}

void loadLineGeometeryToGPU(std::vector<Vec3f> verts) {
  // Just basic layout of floats, for a quad
  // 3 floats per vertex, 4 vertices
	amountofLines = verts.size();

  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * amountofLines, // byte size of Vec3f, 4 of them
               verts.data(),      // pointer (Vec3f*) to contents of verts
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}
void loadScene(){
	std::ifstream file(pathname);
	
	if(file)
	{
		std::stringstream buffer;
		buffer << file.rdbuf();
		file.close();
		//opertaions on the buffer
		std::string temp;
		int numobjects;
		// ignore the first row till we hit end of comments
		buffer.ignore(256, '\\');
		buffer >> temp >> row >> temp >> column; //"Row:, #, Column:, #
		buffer.ignore(256, '\\'); //ignoring till we done with comments
		buffer >> temp >> temp >> temp >> numobjects; //Number, of, objects:, #
		buffer >> temp >> temp >> Radius; // Vision Radius: #
		buffer >> temp >> temp >> boundSize;
		buffer >> temp >> temp >> boundSpeed;
		float tempx;
		float tempy;
		float tempz;
		buffer >> temp >> tempx >> tempy >> tempz;
		Wind.x() = tempx;
		Wind.y() = tempy;
		Wind.z() = tempz;
		buffer >> temp >> temp >> limVelocity;
		
		for(int i = 0; i <numobjects; i++){
			int tempIndex;
			float tempVx;
			float tempVy;
			float tempVz;
			Index tempIndexer;
			buffer >> temp >> tempIndex >> tempVx >> tempVy >> tempVz; // o, position #, velocity
			tempIndexer.index = tempIndex - 1;
			tempIndexer.velocity = Vec3f(tempVx,tempVy,tempVz);
			objectIndex.push_back(tempIndexer);
		}
	}
}

void loadInstanceGeometryToGPU(){
	loadScene();
	int grid = row*column;
	// create grid data
	std::vector<Cell> Map;
	int objects = 0;
	// populating the map
	for(int i = 0; i < grid; i++){
		struct Cell space;
		Map.push_back(space);
	}

	//telling which parts of the map have an object
	for(int i = 0; i < objectIndex.size(); i++){
		Map[objectIndex[i].index].isEmpty = false;
		Map[objectIndex[i].index].velocity = objectIndex[i].velocity;
		objects++;
	}

	//printf("objects %i\n", objects);

	// create offset data	
	Vec3f translations[grid];
	GLfloat offset = 2.0;
	int index = 0;
	for(int x = 0; x < column; x++){
		for(int z = 0; z < row; z++){
			Vec3f translation;
			translation.x() = (x /10.0f) * offset;
			translation.y() = 0.0f;
			translation.z() = -(z /10.0f) * offset;
			translations[index] = translation;
			index++;
		}
	}	

	//only draw the ones with objects inside of them
	Vec3f draw[objects];
	int drawIndex = 0;		
	for(int i = 0; i < Map.size(); i++){
		if(!Map[i].isEmpty)
		{
			//printf("found you object %i\n", drawIndex);
			draw[drawIndex] = translations[i];
			Cell object;
			object.isEmpty = false;
			object.position = translations[i];
			/*if(i < grid/2){
			finally learned rotation about an arbitrary axis ! 270 is a right turn. 90 is a left turn		
				Map[i].orientation = TranslateMatrix(translations[i].x(),translations[i].y(),translations[i].z()) * RotateAboutXMatrix(0) * TranslateMatrix(0,0,0);
				Map[i].orientation = IdentityMatrix();
				Map[i].heading = 270;
			}
			if(i >= grid/2){
				Map[i].orientation =  TranslateMatrix(translations[i].x(),translations[i].y(),translations[i].z()) * RotateAboutXMatrix(0) * TranslateMatrix(0,0,0);
				Map[i].heading = 90;
				//Map[i].orientation = IdentityMatrix();
			}*/		

			//object.orientation = Map[i].orientation;
			object.velocity = Map[i].velocity;
			//object.heading = Map[i].heading;
			object.ID = i;
			globalObjects.push_back(object);

			drawIndex++;
		}	
	}
	
	ObjectsToDraw = drawIndex;

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3f) * drawIndex, &draw[0], GL_STATIC_DRAW);

	//if we got the right amount of global objects we can start the assignment !
	std::cout << "GlobalObjects on the grid: " << globalObjects.size() << std::endl;
	
}

void updateInstanceGeometry(){
	//update Geometry
	Vec3f draw[globalObjects.size()];
	for(int i = 0; i < globalObjects.size(); i++){
		draw[i] = globalObjects[i].position;
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3f) * globalObjects.size(), &draw[0], GL_STATIC_DRAW);
}

void setupVAO() {
	
	//set quad data
  glBindVertexArray(vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
	glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1); // match layout # in shader
  glVertexAttribPointer(1,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        6*sizeof(GLfloat),        // stride
                        (GLvoid*)(3*sizeof(GLfloat)) // array buffer offset
                   );

	// set instance data
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glVertexAttribPointer(2,3, GL_FLOAT, GL_FALSE, 3*(sizeof(GLfloat)), (GLvoid*)0);
	glVertexAttribDivisor(2,1);

	// set line data
	glBindVertexArray(line_vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );
	

	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);




}

void reloadProjectionMatrix() {
  // Perspective Only

  // field of view angle 60 degrees
  // window aspect ratio
  // near Z plane > 0
  // far Z plane

  P = PerspectiveProjection(WIN_FOV, // FOV
                            static_cast<float>(WIN_WIDTH) /
                                WIN_HEIGHT, // Aspect
                            WIN_NEAR,       // near plane
                            WIN_FAR);       // far plane depth
}

void loadModelViewMatrix() {
  M = IdentityMatrix();
	line_M = IdentityMatrix();
  // view doesn't change, but if it did you would use this
  V = camera.lookatMatrix();
}

void reloadViewMatrix() { V = camera.lookatMatrix(); }

void setupModelViewProjectionTransform() {
  MVP = P * V * M; // transforms vertices from right to left (odd huh?)
}
void reloadMVPUniform() {

		GLint id = glGetUniformLocation(instanceProgramID, "M");

		glUseProgram(instanceProgramID);
		glUniformMatrix4fv(id,        // ID
		                   1,         // only 1 matrix
		                   GL_TRUE,   // transpose matrix, Mat4f is row major
		                   M.data() // pointer to data in Mat4f
		                   );

		id = glGetUniformLocation(instanceProgramID, "V");

		glUseProgram(instanceProgramID);
		glUniformMatrix4fv(id,        // ID
		                   1,         // only 1 matrix
		                   GL_TRUE,   // transpose matrix, Mat4f is row major
		                   V.data() // pointer to data in Mat4f
		                   );

		id = glGetUniformLocation(instanceProgramID, "P");

		glUseProgram(instanceProgramID);
		glUniformMatrix4fv(id,        // ID
		                   1,         // only 1 matrix
		                   GL_TRUE,   // transpose matrix, Mat4f is row major
		                   P.data() // pointer to data in Mat4f
		                   );
	
		/*for(int i = 0; i < globalObjects.size(); i++){
			id = glGetUniformLocation(instanceProgramID, ("ROT[" + std::to_string(i) + "]").c_str());
			Mat4f draw[globalObjects.size()];
			glUseProgram(instanceProgramID);
			glUniformMatrix4fv(id,        // ID
		                   1,         // only 1 matrix
		                   GL_TRUE,   // transpose matrix, Mat4f is row major
		                   globalObjects[i].orientation.data()// pointer to data in Mat4f
		                   );
		}*/
	
		glUseProgram(BasicProgramID);
		id = glGetUniformLocation(BasicProgramID, "MVP");		
		glUniformMatrix4fv(id,        // ID
		                   1,         // only 1 matrix
		                   GL_TRUE,   // transpose matrix, Mat4f is row major
		                   MVP.data() // pointer to data in Mat4f
		                   );
		
}

void reloadColorUniform(float r, float g, float b) {
  GLint id = glGetUniformLocation(instanceProgramID, "inputColor");

  glUseProgram(instanceProgramID);
  glUniform3f(id, // ID in basic_vs.glsl
              r, g, b);
}
void reloadColorUniformLine(float r, float g, float b){
	glUseProgram(BasicProgramID);

	GLint id = glGetUniformLocation(BasicProgramID, "inputColor");
  glUniform3f(id, // ID in basic_vs.glsl
              r, g, b);
}

void generateIDs() {
  // shader ID from OpenGL
  std::string vsSource = loadShaderStringfromFile("./shaders/instance_vs.glsl");
  std::string fsSource = loadShaderStringfromFile("./shaders/instance_fs.glsl");
  instanceProgramID = CreateShaderProgram(vsSource, fsSource);

	vsSource = loadShaderStringfromFile("./shaders/basic_vs.glsl");
  fsSource = loadShaderStringfromFile("./shaders/basic_fs.glsl");
  BasicProgramID = CreateShaderProgram(vsSource, fsSource);

  // VAO and buffer IDs given from OpenGL
  glGenVertexArrays(1, &vaoID);
  glGenBuffers(1, &vertBufferID);
	glGenBuffers(1, &instanceVBO);
	glGenVertexArrays(1, &line_vaoID);
  glGenBuffers(1, &line_vertBufferID);
}

void deleteIDs() {
  glDeleteProgram(instanceProgramID);
	glDeleteProgram(BasicProgramID);

  glDeleteVertexArrays(1, &vaoID);
  glDeleteBuffers(1, &vertBufferID);
	glDeleteBuffers(1, &instanceVBO);
	glDeleteVertexArrays(1, &line_vaoID);
  glDeleteBuffers(1, &line_vertBufferID);
}
double length(Vec3f a, Vec3f b){
	return sqrt( pow( (a.x() - b.x()),2 ) + pow((a.y() - b.y()),2) + pow((a.z() - b.z()),2) );
}

void loadBoundary(){
	// create offset data
	int BoundC = 8;
	int BoundR = 8;
	int grid = BoundC * BoundR;	
	Vec3f translations[grid];
	GLfloat offset = 1.0;
	int index = 0;
	for(int x = 0; x < BoundC; x++){
		for(int y = 0; y < BoundR; y++){
			Vec3f translation;
			translation.x() = (x /10.0f) * offset;
			translation.y() = -(y /10.0f) * offset;
			translation.z() = 0.0f;
			translations[index] = translation;
			index++;
		}
	}	
	
	int depth = 1;
	int length = 1;
	int width = 1;

	verts.push_back(translations[(BoundC*BoundR)-1] + Vec3f(width,-length,-depth));
	verts.push_back(translations[row-1] + Vec3f(-width,-length,-depth));  // *----|

	verts.push_back(translations[(BoundC*BoundR)-1] + Vec3f(width,-length,-depth)); // |----*
	verts.push_back(translations[(BoundC*BoundR)-1] + Vec3f(width,-length,depth)); // |----*
	
	verts.push_back(translations[BoundR-1] + Vec3f(-width,-length,-depth));  // *----|
	verts.push_back(translations[BoundR-1] + Vec3f(-width,-length,depth));  // *----|

	verts.push_back(translations[(BoundC*BoundR)-1] + Vec3f(width,-length,depth));
	verts.push_back(translations[BoundR-1] + Vec3f(-width,-length,depth));  // *----|

  loadLineGeometeryToGPU(verts);
}
void init() {

  glEnable(GL_DEPTH_TEST);
  glPointSize(50);

  camera = Camera(Vec3f{0, 0, 5}, Vec3f{0, 0, -1}, Vec3f{0, 1, 0});
  // SETUP SHADERS, BUFFERS, VAOs, Offsets	
	
  generateIDs();
	setupVAO();  
	loadQuadGeometryToGPU();
	loadInstanceGeometryToGPU();
	//loadBoundary();
  loadModelViewMatrix();
  reloadProjectionMatrix();

	//M = RotateAboutXMatrix(90) * RotateAboutZMatrix(-90) * M;

  setupModelViewProjectionTransform();
  reloadMVPUniform();

}

int main(int argc, char **argv) {
	if (argc != 2){
		cout << "usage: " << argv[0] << " <filename>" << endl;
		exit(0);
	}
	else{
		ifstream file = ifstream(argv[1]);
		if(!file.is_open()){
			cout << "Could not open file" << endl;
			exit(0);
		}
		else{
			pathname = argv[1];
			file.close();
		}
	}
  GLFWwindow *window;

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window =
      glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Assignment 4: Flock", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowSizeCallback(window, windowSetSizeFunc);
  glfwSetFramebufferSizeCallback(window, windowSetFramebufferSizeFunc);
  glfwSetKeyCallback(window, windowKeyFunc);
  glfwSetCursorPosCallback(window, windowMouseMotionFunc);
  glfwSetMouseButtonCallback(window, windowMouseButtonFunc);

  glfwGetFramebufferSize(window, &WIN_WIDTH, &WIN_HEIGHT);

  // Initialize glad
  if (!gladLoadGL()) {
    std::cerr << "Failed to initialise GLAD" << std::endl;
    return -1;
  }

  std::cout << "GL Version: :" << glGetString(GL_VERSION) << std::endl;
  std::cout << GL_ERROR() << std::endl;

  init(); // our own initialize stuff func
	
  //float t = 0;
  //float dt = 0.01;
	float old_t = 0;
	

  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         !glfwWindowShouldClose(window)) {		
    if (g_play) {
      //t += dt;
			float t = glfwGetTime();
			float passed = t - old_t;
			old_t = t;
      animateQuad(passed);
			iterations++;
    }

    displayFunc();
    moveCamera();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // clean up after loop
  deleteIDs();

  return 0;
}

//==================== CALLBACK FUNCTIONS ====================//

void windowSetSizeFunc(GLFWwindow *window, int width, int height) {
  WIN_WIDTH = width;
  WIN_HEIGHT = height;

  reloadProjectionMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height) {
  FB_WIDTH = width;
  FB_HEIGHT = height;

  glViewport(0, 0, FB_WIDTH, FB_HEIGHT);
}

void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      g_cursorLocked = GL_TRUE;
    } else {
      g_cursorLocked = GL_FALSE;
    }
  }
}

void windowMouseMotionFunc(GLFWwindow *window, double x, double y) {
  if (g_cursorLocked) {
    float deltaX = (x - g_cursorX) * 0.01;
    float deltaY = (y - g_cursorY) * 0.01;
    camera.rotateAroundFocus(deltaX, deltaY);

    reloadViewMatrix();
    setupModelViewProjectionTransform(); 
    reloadMVPUniform();
  }

  g_cursorX = x;
  g_cursorY = y;
}

void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods) {
  bool set = action != GLFW_RELEASE && GLFW_REPEAT;
  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, GL_TRUE);
    break;
  case GLFW_KEY_W:
    g_moveBackForward = set ? 1 : 0;
    break;
  case GLFW_KEY_S:
    g_moveBackForward = set ? -1 : 0;
    break;
  case GLFW_KEY_A:
    g_moveLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_D:
    g_moveLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_Q:
    g_moveUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_E:
    g_moveUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_UP:
    g_rotateUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_DOWN:
    g_rotateUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_LEFT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? -1 : 0;
    else
      g_rotateLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_RIGHT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? 1 : 0;
    else
      g_rotateLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_SPACE:
    g_play = set ? !g_play : g_play;
    break;
  case GLFW_KEY_LEFT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 0.5;
    } else {
      g_panningSpeed *= 0.5;
    }
    break;
  case GLFW_KEY_RIGHT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 1.5;
    } else {
      g_panningSpeed *= 1.5;
    }
    break;
  default:
    break;
  }
}

//==================== OPENGL HELPER FUNCTIONS ====================//

void moveCamera() {
  Vec3f dir;

  if (g_moveBackForward) {
    dir += Vec3f(0, 0, g_moveBackForward * g_panningSpeed);
  }
  if (g_moveLeftRight) {
    dir += Vec3f(g_moveLeftRight * g_panningSpeed, 0, 0);
  }
  if (g_moveUpDown) {
    dir += Vec3f(0, g_moveUpDown * g_panningSpeed, 0);
  }

  if (g_rotateUpDown) {
    camera.rotateUpDown(g_rotateUpDown * g_rotationSpeed);
  }
  if (g_rotateLeftRight) {
    camera.rotateLeftRight(g_rotateLeftRight * g_rotationSpeed);
  }
  if (g_rotateRoll) {
    camera.rotateRoll(g_rotateRoll * g_rotationSpeed);
  }

  if (g_moveUpDown || g_moveLeftRight || g_moveBackForward ||
      g_rotateLeftRight || g_rotateUpDown || g_rotateRoll) {
    camera.move(dir);
    reloadViewMatrix();
    setupModelViewProjectionTransform();
    reloadMVPUniform();
  }
}

std::string GL_ERROR() {
  GLenum code = glGetError();

  switch (code) {
  case GL_NO_ERROR:
    return "GL_NO_ERROR";
  case GL_INVALID_ENUM:
    return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:
    return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:
    return "GL_INVALID_OPERATION";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_OUT_OF_MEMORY:
    return "GL_OUT_OF_MEMORY";
  default:
    return "Non Valid Error Code";
  }
}

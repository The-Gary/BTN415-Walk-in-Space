/////////////////////////////////////////////////////////////////////////////////////
//
// This code is used to teach the course "game engine foundations" in Seneca college
// Developed by Alireza Moghaddam on Sep. 2020 
//
////////////////////////////////////////////////////////////////////////////////////

#include <windows.networking.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <vector>
#include "vgl.h"
#include "LoadShaders.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\rotate_vector.hpp"
#include "..\SOIL\src\SOIL.h"
#include "Player.h"
#include "PlayerSerializer.h"

using namespace sdds;

//Added on Nov. 21 2021 by: Alireza Moghaddam
enum GameObject_Type {
	PLAYER,
	ENEMY,
	BULLET,
	OBSTACLE
};

struct GameObject {

	glm::vec3 location{};
	glm::vec3 rotation{};
	glm::vec3 scale{};
	glm::vec3 moving_direction{};
	GLfloat velocity{};
	GLfloat collider_dimension{}; //We use box as wrapper with radius = 0.9 * scale of the object Note: 0.9 is the original dimension of the boxes we generate
	int living_time{};
	int life_span{};		//In this code, the life span for obstacles is set to a negative value (Just so that they remain in the scene during the game)
	int type{};
	bool isAlive{};
	bool isCollided{};

};
//End of fragment added

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer};
enum Attrib_IDs { vPosition = 0 };

const GLint NumBuffers = 2;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint location;
GLuint cam_mat_location;
GLuint proj_mat_location;
GLuint texture[2];	//Array of pointers to textrure data in VRAM. We use two textures in this example.


const GLuint NumVertices = 28;

//Height of camera (player) from the level
float height = 0.8f;

//Player motion speed for movement and pitch/yaw
float travel_speed = 300.0f;		//Motion speed
float mouse_sensitivity = 0.01f;	//Pitch/Yaw speed

//Used for tracking mouse cursor position on screen
int x0 = 0;	
int y_0 = 0;
 
//Transformation matrices and camera vectors
glm::mat4 model_view;
glm::vec3 unit_z_vector = glm::vec3(0, 0, 1);	//Assigning a meaningful name to (0,0,1) :-)
glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, height);
glm::vec3 forward_vector = glm::vec3(1, 1, 0);	//Forward vector is parallel to the level at all times (No pitch)

//The direction which the camera is looking, at any instance
glm::vec3 looking_dir_vector = glm::vec3(1, 1, 0);
glm::vec3 up_vector = unit_z_vector;
glm::vec3 side_vector = glm::cross(up_vector, forward_vector);


//Used to measure time between two frames
int oldTimeSinceStart = 0;
int deltaTime;

//Creating and rendering bunch of objects on the scene to interact with
const int Num_Obstacles = 1;
float obstacle_data[Num_Obstacles][3];

// ADDED BY YOUSEF
constexpr char* CLIENT_NAME = "Yousef";
Player this_player(Location(), CLIENT_NAME);
std::vector<Player*> other_players{};
SOCKET ClientSocket{};
// END OF ADDITION BY YOUSEF
std::vector<GameObject> sceneGraph;

void addPlayer(float x, float y);
void refresh_screen();
void networkInitialize();

//Helper function to generate a random float number within a range
float randomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

// inititializing buffers, coordinates, setting up pipeline, etc.
void init(void)
{
	glEnable(GL_DEPTH_TEST);

	//Normalizing all vectors
	up_vector = glm::normalize(up_vector);
	forward_vector = glm::normalize(forward_vector);
	looking_dir_vector = glm::normalize(looking_dir_vector);
	side_vector = glm::normalize(side_vector);

	//Modified on Nov. 21 2021 by: Alireza Moghaddam

	//Randomizing the position and scale of obstacles
	//Creating obstacles and adding them to the GameScene
	for (int i = 0; i < Num_Obstacles; i++)
	{
		obstacle_data[i][0] = randomFloat(-50, 50); //X
		obstacle_data[i][1] = randomFloat(-50, 50); //Y
		obstacle_data[i][2] = randomFloat(0.1f, 10.0f); //Scale

		GameObject go;
		go.location = glm::vec3(obstacle_data[i][0], obstacle_data[i][1], 0);	//Let the object stay on the ground at the beginning
		go.rotation = glm::vec3(0, 0, 0);
		go.scale = glm::vec3(obstacle_data[i][2], obstacle_data[i][2], obstacle_data[i][2]);
		go.collider_dimension = 0.9 * go.scale.x; //0.9 is the length of an edge of the box used for the obstacle
		go.isAlive = true;
		go.living_time = 0;
		go.isCollided = false;
		go.velocity = 0;
		go.type = OBSTACLE;
		go.moving_direction = glm::vec3(0, 0, 0);
		go.life_span = -1;
		sceneGraph.push_back(go);
		
	}
	//End of modification

	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	GLuint program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up


	//Since we use texture mapping, to simplify the task of texture mapping, 
	//and to clarify the demonstration of texture mapping, we consider 4 vertices per face.
	//Overall, we will have 24 vertices and we have 4 vertices to render the sky (a large square).
	//Therefore, we'll have 28 vertices in total.
	GLfloat vertices[NumVertices][3] = {
		
		{ -100.0, -100.0, 0.0 }, //Plane to walk on and a sky
		{ 100.0, -100.0, 0.0 },
		{ 100.0, 100.0, 0.0 },
		{ -100.0, 100.0, 0.0 },

		{ -0.45, -0.45 ,0.01 }, // bottom face
		{ 0.45, -0.45 ,0.01 },
		{ 0.45, 0.45 ,0.01 },
		{ -0.45, 0.45 ,0.01 },

		{ -0.45, -0.45 ,0.9 }, //top face
		{ 0.45, -0.45 ,0.9 },
		{ 0.45, 0.45 ,0.9 },
		{ -0.45, 0.45 ,0.9 },

		{ 0.45, -0.45 , 0.01 }, //left face
		{ 0.45, 0.45 , 0.01 },
		{ 0.45, 0.45 ,0.9 },
		{ 0.45, -0.45 ,0.9 },

		{ -0.45, -0.45, 0.01 }, //right face
		{ -0.45, 0.45 , 0.01 },
		{ -0.45, 0.45 ,0.9 },
		{ -0.45, -0.45 ,0.9 },

		{ -0.45, 0.45 , 0.01 }, //front face
		{ 0.45, 0.45 , 0.01 },
		{ 0.45, 0.45 ,0.9 },
		{ -0.45, 0.45 ,0.9 },
	
		{ -0.45, -0.45 , 0.01 }, //back face
		{ 0.45, -0.45 , 0.01 },
		{ 0.45, -0.45 ,0.9 },
		{ -0.45, -0.45 ,0.9 },
	};

	//These are the texture coordinates for the second texture
	GLfloat textureCoordinates[28][2] = {
		0.0f, 0.0f,
		200.0f, 0.0f,
		200.0f, 200.0f,
		0.0f, 200.0f,
		
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
	};


	//Creating our texture:
	//This texture is loaded from file. To do this, we use the SOIL (Simple OpenGL Imaging Library) library.
	//When using the SOIL_load_image() function, make sure the you are using correct patrameters, or else, your image will NOT be loaded properly, or will not be loaded at all.
	GLint width1, height1;
	unsigned char* textureData1 = SOIL_load_image("grass.png", &width1, &height1, 0, SOIL_LOAD_RGB);

	GLint width2, height2;
	unsigned char* textureData2 = SOIL_load_image("apple.png", &width2, &height2, 0, SOIL_LOAD_RGB);

	glGenBuffers(2, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindAttribLocation(program, 0, "vPosition");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glBindAttribLocation(program, 1, "vTexCoord");
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1);

	location = glGetUniformLocation(program, "model_matrix");
	cam_mat_location = glGetUniformLocation(program, "camera_matrix");
	proj_mat_location = glGetUniformLocation(program, "projection_matrix");

	///////////////////////TEXTURE SET UP////////////////////////
	
	//Allocating two buffers in VRAM
	glGenTextures(2, texture);

	//First Texture: 

	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData1);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//And now, second texture: 

	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData2);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//////////////////////////////////////////////////////////////


	//Initlializing Network Components:
	networkInitialize();
}

//Modified on Nov. 21 2021 by: Alireza Moghaddam
//Helper function to draw a cube
void drawCube(glm::vec3 scale)
{
	model_view = glm::scale(model_view, scale);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

	//Select the second texture (apple.png) when drawing the second geometry (cube)
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glDrawArrays(GL_QUADS, 4, 24);
}
//End of Modification

//Added on Nov. 21 2021 by: Alireza Moghaddam

//This function takes in two game objects and finds out if they are colliding.
bool isColliding(GameObject one, GameObject two) {
	bool result = false;
	
	std::cout << one.scale.x << ", " << one.collider_dimension << ", " << two.scale.x << ", " << two.collider_dimension << std::endl;
	std::cout << glm::abs(one.location.x - two.location.x) << ", " << glm::abs(one.location.y - two.location.x) << std::endl;

	bool cond = glm::abs(one.location.x - two.location.x) <= (one.collider_dimension/2 + two.collider_dimension/2) &&
				glm::abs(one.location.y - two.location.y) <= (one.collider_dimension/2 + two.collider_dimension/2);

	if (cond) {
		//cout << one.type << " has collided with" << two.type << endl;
		//cout << one.location.x << ", " << one.location.y << ", " << one.location.z << endl;
		result = true;
	}
	return result;
}


//This function iterates through the scene graph and checks the collision status between each and every two objects
//When collided, the .isCollided property of the game object is set to true
void checkCollisions() {

	for (int i = 0u; i < sceneGraph.size(); i++) {
		for (int j = 0u; j < sceneGraph.size(); j++) {
			if (i != j && /*if i=j then it means that we are checking self-collilsion. We do NOT consider self-collision as a collision*/
				sceneGraph[i].isAlive && 
				sceneGraph[j].isAlive && 
				!(sceneGraph[i].type == OBSTACLE && sceneGraph[j].type == OBSTACLE) && //We ignore the collision between two obstacles :-)
				isColliding(sceneGraph[i], sceneGraph[j])) {	
				
				//if(!(sceneGraph[i].type == OBSTACLE && sceneGraph[j].type == OBSTACLE))	//We ignore the collision between two obstacles :-)
				//{
					sceneGraph[i].isCollided = true;
					sceneGraph[j].isCollided = true;
					//cout << sceneGraph[i].type << " has collided with" << sceneGraph[j].type << endl;
					//cout << sceneGraph[i].location.x << ", " << sceneGraph[i].location.y << ", " << sceneGraph[i].location.z << endl;
				//}
			}

		}
	}
}

//This function gets called every frame and updates the information written inside the sceneGraph.
void updateSceneGraph() {

	checkCollisions();	//Updating the collision status of all objects on the scene

	for (int i = 0; i < sceneGraph.size(); i++) {

		GameObject go = sceneGraph[i];
		//cout << "Life Span: " << go.life_span << ", isAlive: " << go.isAlive << ", Living time: " << go.living_time << endl;

		if (go.life_span > 0 && go.isAlive && go.living_time >= go.life_span)	//Check if the life of a Game Object is over
		{
		
			go.isAlive = false;
		}

		if (go.life_span > 0 && go.isAlive && go.living_time < go.life_span) {	//If the Game Object is still alive and the object is not an obstacle

			//1 - Updating the location
			go.location += ((GLfloat) deltaTime) * go.velocity * glm::normalize(go.moving_direction);

			
			//2 - updating Time To Live
			go.living_time += deltaTime;

		}

		sceneGraph[i] = go;	//Overwriting the game object data back into the SceneGraph
	}
}


//Renders level
void draw_level()
{
	//Select the first texture (grass.png) when drawing the first geometry (floor)
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glDrawArrays(GL_QUADS, 0, 4);

	updateSceneGraph();


	for (int i = 0; i < sceneGraph.size(); i++){

		GameObject go = sceneGraph[i];
		//Processing each and every object in the Scene Graph
		if (go.isAlive) {

			//Render the object on the scene
			model_view = glm::translate(model_view, go.location);
			model_view = glm::rotate(model_view, 0.0f, unit_z_vector);	//For now, we do not consider the rotation. 
			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
			
			//You may use different texture/geometry based on the game object type

			if (go.type == OBSTACLE) {
				drawCube(go.scale);
			}
			else if (go.type == BULLET) {
				//I am using the same geometry/texture for bullets, however, you may use different
				//cout << "Life Span: " << go.life_span << ", isAlive: " << go.isAlive << ", Living time: " << go.living_time << endl;
				drawCube(go.scale);
			}
			
			model_view = glm::mat4(1.0);
			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
		}	
		else {/*You can remove it from the Game Scene, Or it can remain inside the Game Scene with isAlive=false*/}

	}
}
//End of codes developed by Alireza Moghaddam Nov. 21


//---------------------------------------------------------------------
//
// display
//
void display(void)
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	model_view = glm::mat4(1.0);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

	//The 3D point in space that the camera is looking
	glm::vec3 look_at = cam_pos + looking_dir_vector;

	glm::mat4 camera_matrix = glm::lookAt(cam_pos, look_at, up_vector);
	glUniformMatrix4fv(cam_mat_location, 1, GL_FALSE, &camera_matrix[0][0]);

	glm::mat4 proj_matrix = glm::frustum(-0.01f, +0.01f, -0.01f, +0.01f, 0.01f, 100.0f);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &proj_matrix[0][0]);

	draw_level();

	refresh_screen();

	glFlush();
}


void keyboard(unsigned char key, int x, int y)
{
	if (key == 'a')
	{
		//Moving camera along opposit direction of side vector
		cam_pos += side_vector * travel_speed * ((float)deltaTime) / 1000.0f;
	}
	if (key == 'd')
	{
		//Moving camera along side vector
		cam_pos -= side_vector * travel_speed * ((float)deltaTime) / 1000.0f;
	}
	if (key == 'w')
	{
		//Moving camera along forward vector. To be more realistic, we use X=V.T equation in physics
		cam_pos += forward_vector * travel_speed * ((float)deltaTime) / 1000.0f;
	}
	if (key == 's')
	{
		//Moving camera along backward (negative forward) vector. To be more realistic, we use X=V.T equation in physics
		cam_pos -= forward_vector * travel_speed * ((float)deltaTime) / 1000.0f;
	}

	//Added on Nov. 21 2021 by: Alireza Moghaddam
	if (key == 'f')
	{
		//Create a bullet and place it inside the GameScene
		GameObject go;
		go.location = cam_pos;	//The bullet will spawn with an offset from the location of the player
		go.rotation = glm::vec3(0, 0, 0);
		go.scale = glm::vec3(0.01, 0.01, 0.01);
		go.collider_dimension = go.scale.x;
		go.isAlive = true;
		go.living_time = 0;
		go.isCollided = false;
		go.velocity = 0.0001;
		go.type = BULLET;
		go.moving_direction = looking_dir_vector;
		go.life_span = 2000;	//Each bullet lives for 2 seconds
		sceneGraph.push_back(go);
		
	}
	//End of codes Added 
}

//Controlling Pitch with vertical mouse movement
void mouse(int x, int y)
{
	//Controlling Yaw with horizontal mouse movement
	int delta_x = x - x0;
	
	//The following vectors must get updated during a yaw movement
	forward_vector = glm::rotate(forward_vector, -delta_x * mouse_sensitivity, unit_z_vector);
	looking_dir_vector = glm::rotate(looking_dir_vector, -delta_x * mouse_sensitivity, unit_z_vector);
	side_vector = glm::rotate(side_vector, -delta_x * mouse_sensitivity, unit_z_vector);
	up_vector = glm::rotate(up_vector, -delta_x * mouse_sensitivity, unit_z_vector);
	x0 = x;

	//The following vectors must get updated during a pitch movement
	int delta_y = y - y_0; 
	glm::vec3 tmp_up_vec = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
	glm::vec3 tmp_looking_dir = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);

	//The dot product is used to prevent the user from over-pitch (pitching 360 degrees)
	//The dot product is equal to cos(theta), where theta is the angle between looking_dir and forward vector
	GLfloat dot_product = glm::dot(tmp_looking_dir, forward_vector);

	//If the angle between looking_dir and forward vector is between (-90 and 90) degress 
	if (dot_product > 0)
	{
		up_vector = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
		looking_dir_vector = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);
	}
	y_0 = y;
}

void idle()
{
	//Calculating the delta time between two frames
	//We will use this delta time when moving forward (in keyboard function)
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;
	//cout << timeSinceStart << " " << oldTimeSinceStart << " " << deltaTime << endl;
	glutPostRedisplay();
}

void addPlayer(float x, float y)
{
	model_view = glm::mat4(1.0);
	model_view = glm::translate(model_view, glm::vec3(x, y, cam_pos.z));
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	drawCube(glm::vec3(1.0f, 1.0f, 1.0f));
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("Camera and Projection");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.

	init();

	glutDisplayFunc(display);

	glutKeyboardFunc(keyboard);

	glutIdleFunc(idle);

	glutPassiveMotionFunc(mouse);

	glutMainLoop();
	
	

}
/*************************WARNING!!!!!**************************/
/*************************WARNING!!!!!**************************/
/***************DO NOT TOUCH THE CODE ABOVE*********************/
/***************DO NOT TOUCH THE CODE ABOVE*********************/
/*************************WARNING!!!!!**************************/
/*************************WARNING!!!!!**************************/


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓ YOUR CODE GOES BELOW ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

/***************************************************************/
/***************************************************************/
/**************************Networking Code**********************/
/***************************************************************/
/***************************************************************/

//Add your network initialization code here
//To be added by Students
void networkInitialize()
{
	// starts Winsock DLLs
	WSADATA wsaData;
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
		return;

	// initializes socket. SOCK_STREAM: TCP
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET)
	{
		WSACleanup();
		return;
	}

	// Connect socket to specified server
	sockaddr_in SvrAddr{};
	SvrAddr.sin_family = AF_INET;						// Address family type itnernet
	SvrAddr.sin_port = htons(27000);					// port (host to network conversion)
	SvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	// IP address
	if ((connect(ClientSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR)
	{
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}
}

void refresh_screen() //This function gets called for every frame of the game (after every screen refresh)
{
	this_player.location.x = cam_pos.x; this_player.location.y = cam_pos.y; this_player.location.z = cam_pos.z;
	SerializedPlayer sp = SerializedPlayer::player_serializer(this_player);
	send(ClientSocket, *sp.data, sp.size, 0);
	char s[1]{};
	recv(ClientSocket, s, 1, 0);
	size_t data_size = std::strtol(s, NULL, 10);
	send(ClientSocket, "ack", 3, 0);
	std::unique_ptr<char*> recv_buffer = std::make_unique<char*>(new char[data_size * sizeof(Player)]{});
	recv(ClientSocket, *recv_buffer, data_size * sizeof(Player), 0);
	for (int i = 0; i < data_size; i++)
	{
		if (std::strlen(*recv_buffer) > 0)
		{
			Player recv_player = SerializedPlayer::player_deserializer(*recv_buffer);
			auto& const name = recv_player.name;
			auto found = std::find_if(other_players.begin(), other_players.end(), [&name](const Player* player)
				{
					return name == player->name;
				});
			if (found != other_players.cend())
			{
				(**found).location.update_loc(recv_player.location.x, recv_player.location.y, recv_player.location.z);
				printPlayerInfo(*found);
			}
			else
			{
				other_players.push_back(&recv_player);
				addPlayer(recv_player.location.x, recv_player.location.y);
			}
			*recv_buffer += sizeof(Player);
		}
	}
	
	//Your task here is to receive the data from the server about the locations of all players. Note: Set the coordinates between [-10, 10] for x and y
	//loop for each player p received from server
	//addPlayer(p.x, p.y)

	//You will need to transmit your location to the server frequently. Your location can be obtained by: (cam_pos.x, cam_pos.y)
}



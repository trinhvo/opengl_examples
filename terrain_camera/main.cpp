#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <chrono>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glew.h>
#include <GL/glfw.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> //perspective

#include "camera.h"
#include "camera_free.h"
#include "camera_fps.h"
#include "shader_helper.h"
#include "_terrain/terrain.h"
#include "transform.h"

void init();
void display();
void cleanup();
GLuint load_shader(char *path, GLenum shader_type);

Camera *cam;

Camera cam_fixed;
Camera_free cam_free;
Camera_fps cam_fps;

Terrain terrain;
glm::mat4x4 projection_mat;

GLfloat light_position[3];
bool moving_light;
GLfloat camera_position[3];
GLuint light_mode = 0;

bool activate_colour = true;
bool activate_heightmap = false;

const int win_width = 1280;
const int win_height = 720;

int main(){
   if( !glfwInit() ){
      std::cout << "Error to initialize GLFW" << std::endl;
      return -1;
   }

   glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
   glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
   glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   if( !glfwOpenWindow(win_width, win_height, 0,0,0,0, 32,0, GLFW_WINDOW) ){
      std::cout << "failed to open window" << std::endl;
      return -1;
   }

   glewExperimental = GL_TRUE;
   if(glewInit() != GLEW_NO_ERROR){
      std::cout << "glew error\n";
      return -1;
   }

   init();

   //main loop: control and drawing
   while(glfwGetKey(GLFW_KEY_ESC)!=GLFW_PRESS && glfwGetWindowParam(GLFW_OPENED)){

      if(glfwGetKey('0') == GLFW_PRESS){
         light_mode = 0;
      }
      else if(glfwGetKey('1') == GLFW_PRESS){
         light_mode = 1;
      }
      else if(glfwGetKey('2') == GLFW_PRESS){
         light_mode = 2;
      }

      //moving
      if(glfwGetKey('S') == GLFW_PRESS){
         cam->input_handling('S');
      }
      if(glfwGetKey('A') == GLFW_PRESS){
         cam->input_handling('A');
      }
      if(glfwGetKey('W') == GLFW_PRESS){
         cam->input_handling('W');
      }
      if(glfwGetKey('D') == GLFW_PRESS){
         cam->input_handling('D');
      }

      //change view direction
      if(glfwGetKey('L') == GLFW_PRESS){
         cam->input_handling('L');
      }
      if(glfwGetKey('J') == GLFW_PRESS){
         cam->input_handling('J');
      }
      if(glfwGetKey('K') == GLFW_PRESS){
         cam->input_handling('K');
      }
      if(glfwGetKey('I') == GLFW_PRESS){
         cam->input_handling('I');
      }

      //change camera type
      if(glfwGetKey('M') == GLFW_PRESS){
         cam = &cam_free;
      }
      if(glfwGetKey('N') == GLFW_PRESS){
         cam = &cam_fps;
      }

      //have a moving light or not
      if(glfwGetKey('B') == GLFW_PRESS){
         moving_light = false;
      }
      if(glfwGetKey('V') == GLFW_PRESS){
         moving_light = true;
      }

      //activate the colours of the terrain
      if(glfwGetKey('Y') == GLFW_PRESS){
         activate_colour = true;
      }
      if(glfwGetKey('X') == GLFW_PRESS){
         activate_colour = false;
      }

      //activate heightmap: display the height value as colour black=deep white=high
      if(glfwGetKey('T') == GLFW_PRESS){
         activate_heightmap = true;
      }
      if(glfwGetKey('Z') == GLFW_PRESS){
         activate_heightmap = false;
      }

      display();
      glfwSwapBuffers();
   }

   cleanup();

   return 0;
}

void init(){
   glClearColor(0.4, 0.8, 1.0, 1.0);

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);

   glEnable(GL_CULL_FACE);
   glFrontFace(GL_CCW);
   glCullFace(GL_BACK);

   glViewport(0,0,win_width,win_height);
   projection_mat = glm::perspective(3.1415f/2.0f, (float)win_width/(float)win_height, 0.01f, 1000.0f);

   //light far up in the sky
   light_position[0] = 0.0; //x
   light_position[1] = 1000.0; //up
   light_position[2] = 1000.0; //z

   moving_light = false;

   Transform transf;
   //change the scale of the terrain
   transf.scale(384.0f, 32.0f, 384.0f);

   std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
   terrain.init(512, 512);
   std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

   unsigned long duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

   printf("duration to generate the terrain (heightmap + buffers): %lums\n", duration);

   terrain.set_model(transf);
   cam_fixed.lookAt(1.5f, 1.5f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

   cam_free.lookAt(3.0f, 3.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

   cam_fps.lookAt(1.5f, 1.5f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
   cam_fps.init(0.5f, &terrain);

   cam = &cam_fps;
   cam_fps.update_pos();
}

void display(){
   glClear(GL_COLOR_BUFFER_BIT);
   glClear(GL_DEPTH_BUFFER_BIT);
   cam->get_position(camera_position);
   terrain.draw(cam->getMatrix(), projection_mat, light_position, camera_position, activate_colour, activate_heightmap);

   //move light up and down with time make the coord y of light go from 0 to 10
   if(moving_light){
      light_position[1] = 1000.0 - fabs(1000.0-fmod(200*glfwGetTime(), 2000.0));
   }
   else{
      light_position[1] = 1000.0f;
   }
}

void cleanup(){
}

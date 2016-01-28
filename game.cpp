#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GL/freeglut.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_IBEAM_CURSOR   0x00036002
#define GLFW_CROSSHAIR_CURSOR   0x00036003
#define GLFW_HAND_CURSOR   0x00036004
#define GLFW_HRESIZE_CURSOR   0x00036005
#define GLFW_VRESIZE_CURSOR   0x00036006

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
    int isCircle;
    int isRectangle;
    double length;
    double width;
    double radius;
    double origin[3];
    double velocity_x;
    double velocity_y;
    double velocity_angular;
    int isMoving;
    int isMovable;
    double rotation_angle;
    int isTarget;
    int isObstacle;
    int isTranslateable;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;
GLFWwindow* window;
int width = 1400;
int height = 1000;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;
    vao->isCircle = 0;
    vao->isRectangle = 0;
    vao->isMoving = 0;
    vao->isTarget = 0;
    vao->isObstacle = 0;
    vao->isMovable = 0;
    vao->isTranslateable = 0;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
float ball_angle = 45;
float ball_x = -3.75;
float ball_y = -2.8;
float ball_velocity_x = 5;
float ball_velocity_y = 5;
float time_elapsed;
double last_update_time=0 , current_time=0;
int flag=0;
double key_press_time=0,key_release_time=0;
int sco=0;
double xpos,ypos;
float zoom=0;
float display_x,display_y;
int chances = 7;
int no_objects=0;


/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_X:
                // do something ..
                break;
            case GLFW_KEY_SPACE:
                  chances--;
                  no_objects--;


                  key_release_time = glfwGetTime();
              //    printf("%lf\n", ball_angle);
                  ball_velocity_y = (key_release_time-key_press_time)*15*sin(ball_angle*M_PI/180.0f);
                  ball_velocity_x = (key_release_time - key_press_time)*15*cos(ball_angle*M_PI/180.0f);
                  flag=1;
                  break;

            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_SPACE:
                key_press_time = glfwGetTime();
                flag=0;
                ball_x = -3.75;
                ball_y = -2.8;
                break;
            case GLFW_KEY_A:
                printf("%lf\n", ball_angle);
                ball_angle+=5;
                printf("%lf\n",ball_angle );
                break;
            case GLFW_KEY_C:
                printf("%lf\n", ball_angle);
                ball_angle-=5;
                break;
            case GLFW_KEY_UP:
                zoom++;
                if(zoom>10)
                  zoom=10;
                display_x=zoom*0.1;
                display_y=zoom*0.1;
                break;
            default:
                break;
            case GLFW_KEY_DOWN:
                zoom--;
                if(zoom<-4)
                  zoom=-4;
                display_x=zoom*0.1;
                display_y=zoom*0.1;

        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;

	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, float(4.0), -4.0f, float(4.0), 0.1f, 500.0f);
}

VAO *triangle, *ball, *base, *Rotator, *platform,*Rectangle,*Target,*Obstacle , *score;
VAO* Objects[100];
VAO* scoElements[100];
//int no_objects=0;
int no_scoelements = 0;

// Creates the triangle object used in this sample code
void createRectangle(double length,double width,double x,double y,double velocity,int translate)
{
  GLfloat vertex_buffer_data [] = {
    0,0,0,
    length,0,0,
    length,width,0,


    0,0,0,
    length,width,0,
    0,width,0
  };

   GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1

  //  1,0,0,
//    0,0,1,
//    0,1,0

  };

  Rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  
  Rectangle->isRectangle = 1;
  Rectangle->length = length;
  Rectangle->width = width;
  Rectangle->isMovable = 1;
  Rectangle->radius = sqrt(length*length+width*width)/2;
  Rectangle->origin[0]=x;
  Rectangle->origin[1]=y;
  Rectangle->origin[2]=0;
  Rectangle ->isTranslateable = translate;
  Rectangle->velocity_x = velocity;
  Rectangle->velocity_angular = 0;
  Rectangle->rotation_angle = 0;

// cout << platform->length << "  " << platform->width << endl;
  Objects[no_objects]=Rectangle;
  no_objects++;


}
void createPlateform()
{
  static const GLfloat vertex_buffer_data [] = {
    0,0,0,
    7.5,0,0,
    7.5,0.1,0,


    0,0,0,
    7.5,0.1,0,
    0,0.1,0
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1

  //  1,0,0,
//    0,0,1,
//    0,1,0

  };

  platform = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  //cout << "hi " << endl;
//  cout << platform->NumVertices << endl;
  platform->isRectangle = 1;
  platform->length = 7.5;
  platform->width = 0.1;
  platform->origin[0]=-3.5;
  platform->origin[1]=-4;
  platform->origin[2]=0;
  platform->isMovable = 0;

// cout << platform->length << "  " << platform->width << endl;
  Objects[no_objects]=platform;
  no_objects++;

}
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
//  Objects[no_objects]=traingle;
//  no_objects++;
}


// Creates the rectangle object used in this sample code
void createBase()
{
  static const GLfloat vertex_buffer_data[] = {
    0,0,0,
    0.5,0,0,
    0.5,1,0,


    0,0,0,
    0,1,0,
    0.5,1,0

  };
  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };

  base = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

}
void createBall ()
{
  GLfloat vertex_buffer_data [1000];
int i=0,j=0;
for(i=0;i<72;i++){
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=(0.15f)*cos((i*5)*3.14/180);
  vertex_buffer_data[j++]=(0.15f)*sin((i*5)*3.14/180);
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=(0.15f)*cos(((i*5)+5)*3.14/180);
  vertex_buffer_data[j++]=(0.15f)*sin(((i*5)+5)*3.14/180);
  vertex_buffer_data[j++]=0;



}

j=0;
 GLfloat color_buffer_data [1000];
    for(i=0;i<72;i++)
    {
      color_buffer_data[j++]=1;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=1;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=1;


    }

  // create3DObject creates and returns a handle to a VAO that can be used later
  ball = create3DObject(GL_TRIANGLES, 216, vertex_buffer_data, color_buffer_data, GL_FILL);
  ball->isCircle = 1;
  ball->radius = 0.2;
  ball->origin[0] = ball_x;
  ball->origin[1] = ball_y;
  ball->origin[2] = 0;
  ball->isMovable = 1;
  Objects[no_objects]=ball;
  no_objects++;
}
void createRotator()
{
  static const GLfloat vertex_buffer_data[] = {
    0,0,0,
    1,0,0,
    1,0.1,0,


    0,0,0,
    0,0.1,0,
    1,0.1,0

  };
  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };

  Rotator = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
void createTarget(double x,double y,double radius,double velocity,int translate)
{

  GLfloat vertex_buffer_data [1000];
  int i=0,j=0;
  for(i=0;i<72;i++){
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=(radius)*cos((i*5)*3.14/180);
  vertex_buffer_data[j++]=(radius)*sin((i*5)*3.14/180);
  vertex_buffer_data[j++]=0;
  vertex_buffer_data[j++]=(radius)*cos(((i*5)+5)*3.14/180);
  vertex_buffer_data[j++]=(radius)*sin(((i*5)+5)*3.14/180);
  vertex_buffer_data[j++]=0;



  }

  j=0;
  GLfloat color_buffer_data [1000];
    for(i=0;i<72;i++)
    {
      color_buffer_data[j++]=1;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=1;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=0;
      color_buffer_data[j++]=1;


    }

  // create3DObject creates and returns a handle to a VAO that can be used later
  Target = create3DObject(GL_TRIANGLES, 216, vertex_buffer_data, color_buffer_data, GL_FILL);
  Target->isTarget = 1;
  Target->radius = radius;
  Target->origin[0] = x;
  Target->origin[1] = y;
  Target->origin[2] = 0;
  Target->isMovable = 1;
  Target->isMovable = 1;
  Target->isTranslateable = translate;
  Target->velocity_x = velocity;
  Objects[no_objects]=Target;
  no_objects++;


}
void createObstacles(double x,double y)
{

  static const GLfloat vertex_buffer_data[] = {
    -0.5,-0.05,0,
    0.5,-0.05,0,
    0.5,0.05,0,


    -0.5,-0.05,0,
    -0.5,0.05,0,
    0.5,0.05,0

  };
  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };

  Obstacle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  Obstacle->isObstacle = 1;
  //Target->radius = radius;
  Obstacle->length = 1;
  Obstacle->radius = 0.2;
  Obstacle->width = 0.1;
  Obstacle->origin[0] = x;
  Obstacle->origin[1] = y;
  Obstacle->origin[2] = 0;
  Obstacle->isMovable = 1;
  Obstacle->rotation_angle = 90;
  Objects[no_objects]=Obstacle;
  no_objects++;
}
void createScore(double x , double y,int digit ){


  if(digit==1)
  {
     GLfloat vertex_buffer_data [] = {
      x,y,0,
      x+0.5,y,0,
      x,y+0.1,0,


      x,y+0.1,0,
      x+0.5,y,0,
      x+0.5,y+0.1,0


    };
   GLfloat color_buffer_data [] = {
      1,0,0, // color 1
      0,0,1, // color 2
      0,1,0, // color 3



      0,1,0, // color 3
      0.3,0.3,0.3, // color 4
      1,0,0  // colo
    };
    scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

  }

else   if(digit==2)
  {
//    cout<<"QWERT "<<x<<' '<<y<<endl;

     GLfloat vertex_buffer_data[] = {
      x+0.5,y,0,
      x+0.5+0.05,y,0,
      x+0.5,y-0.5,0,

      x+0.5,y-0.5,0,
      x+0.5+0.05,y,0,
      x+0.5+0.05,y-0.5,0

    };
     GLfloat color_buffer_data [] = {
      1,0,0, // color 1
      0,0,1, // color 2
      0,1,0, // color 3


      0,1,0, // color 3
      0.3,0.3,0.3, // color 4
      1,0,0  // colo
    };
    scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

  }
else   if(digit==3)
  {
     GLfloat vertex_buffer_data[] = {
      x,y,0,
      x,y-0.5,0,
      x-0.05,y,0,

      x,y-0.5,0,
      x-0.05,y,0,
      x-0.05,y-0.5,0

  };
 GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3


    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // colo
  };
  scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);


}
else   if(digit==4)
  {
   GLfloat vertex_buffer_data[] = {
      x,y-0.5+0.05,0,
      x+0.5,y-0.5+0.05,0,
      x+0.5,y-0.5-0.1+0.05,0,

        x,y-0.5+0.05,0,
        x+0.5,y-0.5-0.1+0.05,0,
        x,y-0.5-0.1+0.05,0

  };
 GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3


    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // colo
  };
  scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);


}
else  if(digit==5)
{
   GLfloat vertex_buffer_data[] = {
    x,y-0.5,0,
    x-0.05,y-0.5,0,
    x,y-1,0,

    x-0.05,y-0.5,0,
    x,y-1,0,
    x-0.05,y-1,0,


};
GLfloat color_buffer_data [] = {
  1,0,0, // color 1
  0,0,1, // color 2
  0,1,0, // color 3

  0,1,0, // color 3
  0.3,0.3,0.3, // color 4
  1,0,0  // colo
};
scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);


}
else if(digit==6)
{
   GLfloat vertex_buffer_data[] = {
    x+0.5,y-0.5,0,
    x+0.5,y-1,0,
    x+0.5+0.05,y-0.5,0,

    x+0.5+0.05,y-0.5,0,
    x+0.5,y-1,0,
    x+0.5+0.05,y-1,0


};
 GLfloat color_buffer_data [] = {
  1,0,0, // color 1
  0,0,1, // color 2
  0,1,0, // color 3


    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // colo
};
scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);


}
else if(digit==7)
{
    GLfloat vertex_buffer_data[] = {
    x,y-1,0,
    x+0.5,y-1,0,
    x,y-1-0.1,0,

    x+0.5,y-1,0,
    x,y-1-0.1,0,
    x+0.5,y-1-0.1,0


};
 GLfloat color_buffer_data [] = {
  1,0,0, // color 1
  0,0,1, // color 2
  0,1,0, // color 3


  0,1,0, // color 3
  0.3,0.3,0.3, // color 4
  1,0,0  // colo
};
scoElements[no_scoelements] =  create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

}




}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void drawscore()
{

      int number = sco;
    //  number = 55;
      int dig,iteration=0;
      double xcor;
      no_scoelements = 0;
      do
      {
        dig=number%10;
        number=number/10;
        xcor=(double)3.3-iteration*0.8;
        if(dig==0)
        {
          createScore(xcor,3.8,1);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
        //  scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,3);
        //  scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,5);
        //  scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;

        }
        if(dig==1)
        {
          createScore(xcor,3.8,2);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==2)
        {
          createScore(xcor,3.8,1);
        //  scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,5);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==3)
        {
          createScore(xcor,3.8,1);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
      ///    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==4)
        {
          createScore(xcor,3.8,3);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
        //  scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==5)
        {
          createScore(xcor,3.8,1);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,3);
        //  scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==6)
        {
          createScore(xcor,3.8,1);
      //    scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,3);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,5);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==7)
        {
          createScore(xcor,3.8,1);
//          scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==8)
        {
          createScore(xcor,3.8,1);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,3);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,5);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
        }
        if(dig==9)
        {
          createScore(xcor,3.8,3);
//          scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,1);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,2);
    //      scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,6);
  //        scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,4);
//          scoElements[no_scoelements]=score;
          no_scoelements++;
          createScore(xcor,3.8,7);
//          scoElements[no_scoelements]=score;
          no_scoelements++;
        }

        iteration++;
      }
      while(number!=0);
}


void collision()
{

    int i=0;
      for(int j=1;j<no_objects;j++)
      {

        if(Objects[i]->isCircle)
        {
            if(j<18)
            {
            if(Objects[j]->isRectangle)
            {

                if(abs(Objects[j]->origin[1]-Objects[i]->origin[1])<=Objects[i]->radius+Objects[j]->width && Objects[i]->origin[0]+Objects[i]->radius>Objects[j]->origin[0] && Objects[i]->origin[0]-Objects[i]->radius <= Objects[j]->origin[0]+Objects[j]->length && Objects[i]->origin[1]>Objects[j]->origin[1])
                {
                    ball_y=Objects[j]->origin[1]+Objects[j]->width+Objects[i]->radius;
                    ball_velocity_y = -ball_velocity_y;

                    Objects[j]->isMoving = 1;
                //    Objectsy[j]->velocity_angular = 100;
                }
                if(Objects[j]->origin[1]>Objects[i]->origin[1] && abs(Objects[j]->origin[1]-Objects[i]->origin[1])<=Objects[i]->radius &&  Objects[i]->origin[0]+Objects[i]->radius>Objects[j]->origin[0] && Objects[i]->origin[0]-Objects[i]->radius <= Objects[j]->origin[0]+Objects[j]->length)
                {
                      ball_y = Objects[j]->origin[1]-Objects[i]->radius;
                      ball_velocity_y = -ball_velocity_y;
                    //  Objects[j]

                }
                if(Objects[i]->origin[0]<Objects[j]->origin[0] && Objects[j]->origin[0]-Objects[i]->origin[0]<=Objects[i]->radius && Objects[i]->origin[1] <= Objects[j]->origin[1]+Objects[j]->width && Objects[i]->origin[1]>=Objects[j]->origin[1])
                {
                    ball_velocity_x = -0.1*ball_velocity_x;


                }
                if(Objects[i]->origin[0]>Objects[j]->origin[0] && Objects[i]->origin[0]-Objects[j]->origin[0]<=Objects[i]->radius+Objects[j]->length && Objects[i]->origin[1] <= Objects[j]->origin[1]+Objects[j]->width && Objects[i]->origin[1]>=Objects[j]->origin[1])
                {
                    ball_velocity_x = -0.1*ball_velocity_x;



                }

            }
            if(Objects[j]->isTarget)
            {

                double x1 = Objects[i]->origin[0];
                double y1 = Objects[i]->origin[1];
                double x2 = Objects[j]->origin[0];
                double y2 = Objects[j]->origin[1];
                double dist = sqrt((y2-y1)*(y2-y1)+(x2-x1)*(x2-x1));
                if(dist<Objects[i]->radius+Objects[j]->radius)
                {
                     Objects[j]->origin[0]=5;
                     Objects[j]->origin[1]=5;
                     sco+=7-(7-chances-1);

                }




            }

            if(Objects[j]->isObstacle)
            {

                double x1 = Objects[i]->origin[0];
                double y1 = Objects[i]->origin[1];
                double x2 = Objects[j]->origin[0];
                double y2 = Objects[j]->origin[1];
                double dist = sqrt((y2-y1)*(y2-y1)+(x2-x1)*(x2-x1));
                if(dist<Objects[i]->radius+Objects[j]->radius)
                {
                     ball_velocity_x = -0.5*ball_velocity_x;
                     ball_velocity_y = -0.5*ball_velocity_y;

                }

            }
          }
      }


}
}

void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  Matrices.projection = glm::ortho(-4.0f, float(4.0-display_x), -4.0f, float(4.0-display_y), 0.1f, 500.0f);


  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  glfwGetCursorPos(window, &xpos, &ypos);
//  ball_angle = ypos/xpos;
  glfwGetFramebufferSize(window, &width, &height);
  xpos=-4+(float)8.0/width*xpos;
  ypos=-4+(float)8.0/height*ypos;
  ypos*=-1;
    //cout<<xpos<<" "<<ypos<<endl;
  ball_angle = atan2 (ypos+3,xpos+3.75) * 180 / M_PI;
  collision();
  drawscore();
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model




  // Load identity to model matrix
  for(int i=0;i<no_objects;i++)
  {
  Matrices.model = glm::mat4(1.0f);

  time_elapsed = 0.0004;
  if(Objects[i]->isMovable){
  //  cout << "hello " << endl;

  if(Objects[i]->isMoving){
  Objects[i]->rotation_angle+=Objects[i]->velocity_angular*time_elapsed*60;

}
  if(Objects[i]->origin[1]<-3.9)
{      Objects[i]->origin[1]=-3.9;
      Objects[i]->isMoving = 0;

  }
}  /* Render your scene */
if(i==12)
{
    if(Objects[i]->isTarget)
    {
    }
    Objects[i]->origin[0]+=Objects[i]->velocity_x*time_elapsed*60;
    Objects[i+1]->origin[0]+=Objects[i+1]->velocity_x*time_elapsed*60;
    if(Objects[i]->isRectangle)
    {
      if(Objects[i+1]->origin[0]==5)
      {
        Objects[i]->velocity_x = 0;
        Objects[i+1]->velocity_x = 0;
      }
    if(Objects[i]->origin[0]>-0.1 || Objects[i]->origin[0] < -1.55)
    {
      Objects[i]->velocity_x = -Objects[i]->velocity_x;
      Objects[i+1]->velocity_x = -Objects[i+1]->velocity_x;

    }
  }
}
if(i==14)
{
    if(Objects[i]->isTarget)
    {
    }
    Objects[i]->origin[0]+=Objects[i]->velocity_x*time_elapsed*60;
    Objects[i+1]->origin[0]+=Objects[i+1]->velocity_x*time_elapsed*60;
    if(Objects[i]->isRectangle)
    {
      if(Objects[i+1]->origin[0]==5)
      {
        Objects[i]->velocity_x = 0;
        Objects[i+1]->velocity_x = 0;
      }
    if(Objects[i]->origin[0]>2.2 || Objects[i]->origin[0] < 1 )
    {
      Objects[i]->velocity_x = -Objects[i]->velocity_x;
      Objects[i+1]->velocity_x = -Objects[i+1]->velocity_x;

    }
  }
}
if(i==16)
{
    if(Objects[i]->isTarget)
    {
    }
    Objects[i]->origin[1]+=Objects[i]->velocity_x*time_elapsed*60;
    Objects[i+1]->origin[1]+=Objects[i+1]->velocity_x*time_elapsed*60;
    if(Objects[i]->isRectangle)
    {
      if(Objects[i+1]->origin[1]==5)
      {
        Objects[i+1]->velocity_x = 0;
        if(Objects[i]->origin[1]>2.5)
            Objects[i]->velocity_x=0;

      }
    if(Objects[i]->origin[1]>=3 || Objects[i]->origin[1] < 0 )
    {
      Objects[i]->velocity_x = -Objects[i]->velocity_x;
      Objects[i+1]->velocity_x = -Objects[i+1]->velocity_x;

    }
  }
}
  glm::mat4 translateObject = glm::translate (glm::vec3(Objects[i]->origin[0],Objects[i]->origin[1], 0.0f));
  glm::mat4 rotateObject;
  if(!Objects[i]->isObstacle){
      rotateObject = glm::rotate((float)((Objects[i]->rotation_angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
}
  if(Objects[i]->isObstacle)
  {
    rotateObject = glm::rotate((float)((Objects[i]->rotation_angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)

//    glm::mat4 translateObject = glm::translate (glm::vec3(Objects[i]->origin[0],Objects[i]->origin[1], 0.0f));
  } // glTranslatef
  glm::mat4 ObjectTransform = translateObject*rotateObject;
  Matrices.model = ObjectTransform;
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix

  if(Objects[i]->isObstacle)
  {
      Objects[i]->rotation_angle+=5;

  }
  draw3DObject(Objects[i]);
}
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateBase = glm::translate (glm::vec3(-4.0f, -4.0f, 0.0f)); // glTranslatef
  //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 BaseTransform = translateBase;
  Matrices.model *= BaseTransform;
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(base);

/*  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translatePlateform = glm::translate (glm::vec3(-3.0f, -4.0f, 0.0f)); // glTranslatef
  //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 PlateformTransform = translatePlateform;
  Matrices.model *= PlateformTransform;
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  platform->origin[0]=-3;
  platform->origin[1]=-4;
  platform->origin[2]=0;
  draw3DObject(platform); */

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  Matrices.model = glm::mat4(1.0f);
  if(flag==1){
//  time_elapsed = current_time - last_update_time;
  time_elapsed = 0.0004;
  ball_velocity_y-=10*(time_elapsed*60);
  ball_x+=ball_velocity_x*(time_elapsed*60);
  ball_y+=ball_velocity_y*(time_elapsed*60)-5*time_elapsed*time_elapsed*3600 ;
  }
  if(ball_y<-4 || ball_x > 4 || ball_x<-4) 
  {
    flag=0;
    ball_x = -3.75;
    ball_y = -2.8;
    if(chances<=0)
      chances = -1;
  }
for(int i=0;i<no_scoelements;i++)
{
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateScore  = glm::translate (glm::vec3(0,0,0));        // glTranslatef
  Matrices.model *= translateScore;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(scoElements[i]);

}

  // draw3DObject draws the VAO given to it using current MVP matrix
  ball->origin[0] = ball_x;
  ball->origin[1] = ball_y;
  ball->origin[2] = 0;
  // Increment angles

  float increments = 1;
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRotator  = glm::translate (glm::vec3(-3.75,-3, 0));        // glTranslatef
  glm::mat4 rotateRotator = glm::rotate((float)(ball_angle*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model = translateRotator*rotateRotator;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(Rotator);



  //camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
//  ball_x+=(ball_velocity_x*(0.001f));/
//  ball_y+=ball_velocity_y*(0.001f)-5*(0.001f);
//  ball_x+=cos((ball_angle*M_PI)/180.0f)/10;
//  ball_y+=sin((ball_angle*M_PI)/180.0f)/10;
//    collision();

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);

    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createBall ();
  createBase();
  createRotator();
  createPlateform();
  createRectangle(1,0.5,-2,-2,0,0);
  createRectangle(1,0.5,0,-3,0,0);
  createTarget(3,-2,0.2,0,0);
  createTarget(3,1.5,0.2,0,0);
  createObstacles(-1.5,0);
  createObstacles(-1.5,2.5);
  createTarget(0.9,1.5,0.2,0,0);
  createRectangle(1,0.5,2,-1,0,0);
  createTarget(3.8,-0.25,0.2,0,0);
  createTarget(-1.5,1.25,0.2,0,0);
  createRectangle(1,0.4,-1,0.6,0.5,1);
  createTarget(-0.5,1.2,0.2,0.5,1);
  createRectangle(1,0.4,1,0.6,0.5,1);
  createTarget(1.5,1.2,0.2,0.5,1);
  createRectangle(1,0.4,-3.1,0,0.5,1);
  createTarget(-2.5,0.6,0.2,0.5,1);
  createTarget(-3.9,3.8,0.1,0,0);
  createTarget(-3.9,3.6,0.1,0,0);
  createTarget(-3.9,3.4,0.1,0,0);
  createTarget(-3.9,3.2,0.1,0,0);
  createTarget(-3.9,3,0.1,0,0);
  createTarget(-3.9,2.8,0.1,0,0);
  createTarget(-3.9,2.6,0.1,0,0);

//  createScore(3,3,1);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);
//  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  GLFWcursor* cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
  glfwSetCursor(window, cursor);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{


   window = initGLFW(width, height);

	initGL (window, width, height);

//    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
//        last_update_time = glfwGetTime();
        current_time = glfwGetTime();
        //if(chances<0)
        //  break;
        draw();
        if(chances==-1)
          break;

        //collision();
        last_update_time = glfwGetTime();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
    //    current_time = glfwGetTime(); // Time in seconds
  //      printf("%lf  %lf\n",current_time,last_update_time );
//        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
    //        last_update_time = current_time;
  //      }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

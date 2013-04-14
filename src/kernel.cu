/* Program to apply Laplacian of Guassian edge detection on BMP image */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//Open GL libraries
#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "stack.h"
#include "convolution.h"
#include "Timer.h"
#include<list>
const float  PI = 3.14159265358979f;
using namespace std;
struct ImageData{
    GLint   x;      // resolution X
    GLint   y;      // resolution Y
    GLenum  format; // data format (RGB or INDEX..)
    GLenum  type;   // data type (8bit, 16bit or 32bit..)
    GLvoid  *buf;   // image pixel bits
};

// Function declerations
void display (void);

void reshape(int nw, int nh);

void keybord(unsigned char key, int x, int y);

bool initSharedMem();
void clearSharedMem();
bool loadRawImage(char *fileName, int x, int y, unsigned char *data);
void drawString(const char *str, int x, int y, void *font);
void create_LoG();

// constants
const char *FILE_NAME = "lena.bmp";
const int  IMAGE_X = 512;
const int  IMAGE_Y = 512;
const int  MAX_NAME = 1024;
const int  THRESHOLD = 120;

// global variables ////////////////
ImageData       *image;
unsigned char   *inBuf;
unsigned char   *outBuf1;
unsigned char   *outBuf2;
float		*kernel;
unsigned char    *grayscale_Image;
char            fileName[MAX_NAME];
int             imageX;
int             imageY;
int 		kernel_size;
float		sigma_value;
int		threshold;
void            *font = GLUT_BITMAP_8_BY_13;
int             fontWidth = 8;
int             fontHeight = 13;
int             mainWin, subWin1, subWin2, subWin3, subWin4;
double          time1, time2;
int 		kernel_use;

//#define CPU_COMP 1

#define IDX_1D(col, row, stride) ((col * stride) + row)
#define COL_2D(index, stride) (index / stride)
#define ROW_2D(index, stride) (index % stride)
#define ROUNDUP32(integer) ( ((integer-1)/32 + 1) * 32 )

#define SHMEM 8192
#define FLOAT_SZ sizeof(float)


#define CREATE_CONVOLUTION_VARIABLES(psfColRad, psfRowRad) \
	\
const int cornerCol = blockDim.x*blockIdx.x; \
const int cornerRow = blockDim.y*blockIdx.y; \
const int globalCol = cornerCol + threadIdx.x; \
const int globalRow = cornerRow + threadIdx.y; \
const int globalIdx = IDX_1D(globalCol, globalRow, imgRows); \
\
const int localCol = threadIdx.x; \
const int localRow = threadIdx.y; \
const int localIdx = IDX_1D(localCol, localRow, blockDim.y); \
const int localPixels = blockDim.x*blockDim.y; \
\
const int padRectStride = blockDim.y + 2*psfRowRad; \
const int padRectCol = localCol + psfColRad; \
const int padRectRow = localRow + psfRowRad; \
/*const int padRectIdx = IDX_1D(padRectCol, padRectRow, padRectStride); */ \
const int padRectPixels = padRectStride * (blockDim.x + 2*psfColRad); \
\
__shared__ char sharedMem[SHMEM]; \
float* shmPadRect = (float*)sharedMem; \
float* shmOutput = (float*)&shmPadRect[ROUNDUP32(padRectPixels)]; \
int nLoop;

#define PREPARE_PADDED_RECTANGLE(psfColRad, psfRowRad) \
	\
nLoop = (padRectPixels/localPixels)+1; \
for(int loopIdx=0; loopIdx<nLoop; loopIdx++) \
{ \
	int prIndex = loopIdx*localPixels + localIdx; \
	if(prIndex < padRectPixels) \
	{ \
		int prCol = COL_2D(prIndex, padRectStride); \
		int prRow = ROW_2D(prIndex, padRectStride); \
		int glCol = cornerCol + prCol - psfColRad; \
		int glRow = cornerRow + prRow - psfRowRad; \
		int glIdx = IDX_1D(glCol, glRow, imgRows); \
		if(glRow >= 0 && \
				glRow < imgRows && \
				glCol >= 0 && \
				glCol < imgCols) \
		shmPadRect[prIndex] = imgInPtr[glIdx]; \
		else \
		shmPadRect[prIndex] = 0.0f; \
	} \
} \

#define COPY_LIN_ARRAY_TO_SHMEM(srcPtr, dstPtr, nValues) \
	nLoop = (nValues/localPixels)+1; \
for(int loopIdx=0; loopIdx<nLoop; loopIdx++) \
{ \
	int prIndex = loopIdx*localPixels + localIdx; \
	if(prIndex < nValues) \
	{ \
		dstPtr[prIndex] = srcPtr[prIndex]; \
	} \
}

// CUDA kernel to calculate 2d convolution
__global__ void convolve_GPU(unsigned char* imgInPtr,unsigned char* imgOutPtr,float* imgPsfPtr,int imgCols,
		int imgRows,int psfColRad,int psfRowRad)
{

	//Get row and column values
	CREATE_CONVOLUTION_VARIABLES(psfColRad, psfRowRad);

	//define shared memory variable
	shmOutput[localIdx] = 0.0f;

	//calculate image strides
	const int psfStride = psfRowRad*2+1;
	const int psfPixels = psfStride*(psfColRad*2+1);
	float* shmPsf = (float*)&shmOutput[ROUNDUP32(localPixels)];

	COPY_LIN_ARRAY_TO_SHMEM(imgPsfPtr, shmPsf, psfPixels);

	PREPARE_PADDED_RECTANGLE(psfColRad, psfRowRad);

	__syncthreads();

	float accumFloat = 0.0f;

	//compute convolution
	for(int coff=-psfColRad; coff<=psfColRad; coff++)
	{
		for(int roff=-psfRowRad; roff<=psfRowRad; roff++)
		{
			int psfCol = psfColRad - coff;
			int psfRow = psfRowRad - roff;
			int psfIdx = IDX_1D(psfCol, psfRow, psfStride);
			float psfVal = shmPsf[psfIdx];

			int shmPRCol = padRectCol + coff;
			int shmPRRow = padRectRow + roff;
			int shmPRIdx = IDX_1D(shmPRCol, shmPRRow, padRectStride);
			accumFloat += psfVal * shmPadRect[shmPRIdx];
		}
	}

	//apply zero-crossings
	if(accumFloat < 0)
		accumFloat = 0.0f;
	else if(accumFloat > 255.0f)
		accumFloat = 255.0f;
	
	shmOutput[localIdx] = accumFloat;
	__syncthreads();

	imgOutPtr[globalIdx] = shmOutput[localIdx];
}
/// Window Functions
short wire = TRUE;
 
float w, h, tip = 0, turn = 0;
 
float ORG[3] = {0,0,0};
 
float XP[3] = {1,0,0}, XN[3] = {-1,0,0},
      YP[3] = {0,1,0}, YN[3] = {0,-1,0},
      ZP[3] = {0,0,1}, ZN[3] = {0,0,-1};
//Screen constants
const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 900;
const double CUBESIZE = 1.0;
const double div_num = 1.0;
static GLuint textureID = 0;





 
void reshape (int nw, int nh)
{
    w = nw;
    h = nh;
}
 
void Keybord (unsigned char key, int x, int y)
{
    switch (key) {
       case  'w' : wire = !wire;  break;
       case   27 : exit (0);
       default   : printf ("   %c == %3d from Keybord\n", key, key);
    }
}
 
void Special (int key, int x, int y)
{
    switch (key) {
       case  GLUT_KEY_RIGHT: turn += 5;  break;
       case  GLUT_KEY_LEFT : turn -= 5;  break;
       case  GLUT_KEY_UP   : tip  -= 5;  break;
       case  GLUT_KEY_DOWN : tip  += 5;  break;
 
       default : printf ("   %c == %3d from Special\n", key, key);
    }
}
int clicked=0;
//MouseButton Callback function

void MouseMotion(int x, int y){
	static float lastx=0.0;
	static float lasty=0.0;
	printf("\n%d %d",x,y);
	lastx=(float)x-lastx;
	lasty=(float)y-lasty;
	if( (float)x>lastx)
		turn -=lastx;
	else
		turn+=lastx;
	if((float)y>lasty)
	tip +=lastx;
	else
	tip-=lastx;

	if(abs((int)lastx)>10||(abs((int)lasty)>10))
	{
	lastx=(float)x;
	lasty=(float)y;
	return;
	}
	//glutPostRedisplay();
}
void MouseButton (int btn, int state, int x, int y)
{
//	(state == GLUT_DOWN)?clicked=1:clicked=0;
    //mouse wheel events
    if (btn == 3 || btn == 4 )
    {
        //scroll up
        if(state == GLUT_UP)
           tip+=5;
        
        //scroll down
        else if(state == GLUT_DOWN)
            tip-=5;
    }
    
    //left mouse buttom click
   // else if (btn == GLUT_LEFT_BUTTON && clicked)
   //   (x!=0 && y!=0)?tip+=:  (x!=0 && y==0)?tip += x:((y!=0 && x==0)?tip+=y:tip=tip);
		
    
    //right mosue button click
   // else if (btn == GLUT_RIGHT_BUTTON && clicked)
   //     turn -= x;
    
    //middle mouse button click
   // else if (btn == GLUT_MIDDLE_BUTTON && state ==GLUT_DOWN)
    //    turn -= y;
        
    // Request display update
   // glutPostRedisplay();
    
}

void Draw_Axes (void)
{ 
    glPushMatrix ();
 
       glTranslatef (-2.4, -1.5, -5);
       glRotatef    (0 , 1,0,0);
       glRotatef    (0, 0,1,0);
       glScalef     (0.25, 0.25, 0.25);
 
       glLineWidth (2.0);
 
       glBegin (GL_LINES);
          glColor4f (1,0,0,0);  glVertex3fv (ORG);  glVertex3fv (XP);    // X axis is red.
          glColor4f (0,1,0,0);  glVertex3fv (ORG);  glVertex3fv (YP);    // Y axis is green.
          glColor4f (0,0,1,0);  glVertex3fv (ORG);  glVertex3fv (ZP);    // z axis is blue.
       glEnd();
 
   glPopMatrix ();
}
typedef struct {
unsigned int r:8,g:8,b:8,a:8;
}Color32;
  Color32* lpTex32 ;
  

  typedef struct {GLdouble vertex[3];}vertex3d_Array;
  std::list<vertex3d_Array> v;

void draw_stack ()
{
	int ii; 
	glPushMatrix ();
	 glTranslatef (0, 0, -5);
       glRotatef (tip , 1,0,0);
       glRotatef (turn, 0,1,0);
   float percent=25.0;
	for(ii=0;(CUBESIZE/div_num-((float)ii)*percent/100.0)>= -( CUBESIZE/div_num);ii++){
	//  // cube - TOP
    glBegin(GL_POLYGON);
    glColor4f( 1.0,  1.0,  1.0,  1.0 );
    glTexCoord2f(0.0f, 1.0f);
    glVertex3d(  CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*percent/100.0,  CUBESIZE/div_num );
    glTexCoord2f(1.0f, 1.0f);
    glVertex3d(  CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*percent/100.0, -CUBESIZE/div_num );
    glTexCoord2f(1.0f, 0.0f);
    glVertex3d( -CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*percent/100.0, -CUBESIZE/div_num );
    glTexCoord2f(0.0f, 0.0f);
    glVertex3d( -CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*percent/100.0,  CUBESIZE/div_num);
    glEnd();

	}
	GLfloat vertices[][3] = {{1.0,0.5,0.0},{0.4,1.0,0.0},
	{1.0,1.0,0.0}, {1.0,1.0,0.0}, {-1.0,-1.0,1.0}, 
	{1.0,-1.0,1.0}, {1.0,1.0,1.0}, {-1.0,1.0,1.0}};
	int a,b,c,d;
	a=0;b=3;c=2;d=1;
	list<vertex3d_Array> points=v;
	//list<vertex3d_Array>::iterator it;
		//for (it=v.begin(); it != sList.end(); ++it)
//	for(ii = 0; ii<points.size();ii++)
//{
//		
//		vertex3d_Array pt;
//		pt=points.front(); 
//		points.pop_front();
//		//glVertex3f(.0f, 100.0f, -25.0f);
//		//for(int f=0;f<20;f++)
//		{
//		glBegin(GL_LINES);
//		glColor4f( (GLfloat)ii/(float)points.size(),  (float)ii/(float)points.size(),  (float)ii/(float)points.size(),  1.0 );		      
//		glTexCoord2f(0.0f, 1.0f);
//		
//		glVertex3d(pt.vertex[0],pt.vertex[1],-1.0f);
//		glVertex3d(pt.vertex[0],pt.vertex[1],0.0f);
//		glVertex3d(pt.vertex[0],pt.vertex[1],1.0f);
//	
//		glEnd();
//		}
//	}

	
	//glBegin(GL_POINTS);
	//glColor4f( 1.0,  1.0,  1.0,  1.0 );		      
 //   glTexCoord2f(0.0f, 1.0f);
	///*	glColor4fv(colors[a]);*/
	///*	glTexCoord2f(0.0,0.0); */

	//	glVertex3fv(vertices[a]);
	///*	glColor4fv(colors[b]); */
	///*	glTexCoord2f(0.0,1.0); */
	//	glVertex3fv(vertices[b]);
	///*	glColor4fv(colors[c]); */
	///*	glTexCoord2f(1.0,1.0); */
	//	
	//	glVertex3fv(vertices[c]);
	///*	glColor4fv(colors[d]); */
	///*	glTexCoord2f(1.0,0.0); */
	//	
	//	glVertex3fv(vertices[d]);
	//glEnd();
     glPopMatrix (); 

}

void display (void)
{
    //glutSetWindow(subWin3);
 //   glClear(GL_COLOR_BUFFER_BIT); // clear canvas

	glViewport (0, 0, w, h);
    glClear    (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
     glEnable(GL_TEXTURE_2D);
  
 
    // Create OpenGL textures
    glGenTextures(1, &textureID);
    
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    
	 glTexEnvf(GL_TEXTURE_2D,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(1.0,1.0,1.0,0.5);//Replace this alpha for transparency
    // Generate texture
	//glTexImage2D(GL_TEXTURE_2D, 0, image->format, imageX, imageY, 0, image->format, image->type, outBuf2);
   
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageX, imageY, 0, GL_RGBA, GL_UNSIGNED_BYTE, lpTex32);
    
    //Set texture parameters
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    //Set texture ID
   // glEnable(GL_TEXTURE_2D);
   // glBindTexture( GL_TEXTURE_2D, textureID );
    
    draw_stack();
	Draw_Axes();
    //free texture
    glDeleteTextures( 1, &textureID );
    textureID = 0;
   // Draw_Teapot ();
	
 
    glutSwapBuffers ();
}

//End of window functions
// main program

/*
int main(int argc, char **argv)
{


	kernel_use = 0;
// use default image file if not specified
    if(argc == 7)
    {
        strcpy(fileName, argv[1]);
        imageX = atoi(argv[2]);
        imageY = atoi(argv[3]);
	threshold = atoi(argv[6]);
	kernel_size = atoi(argv[4]);
	sigma_value = atof(argv[5]);
    }
    else{
        printf("Usage: %s <image-file> <width> <height> <threshold>\n", argv[0]);
        strcpy(fileName, FILE_NAME);
        imageX = IMAGE_X;
        imageY = IMAGE_Y;
	threshold = THRESHOLD;
	kernel_use = 1;
        printf("\nUse default image \"%s\", (%d,%d) with threshold value %d\n", fileName, imageX, imageY, threshold);
    }
	   
    // allocate memory for global variables
    if(!initSharedMem()) 
	    return 0;
    
    // open raw image file
    if(!loadRawImage(fileName, imageX, imageY, grayscale_Image))
    {
        clearSharedMem();            
        return 0;
    } 
    
    
    int bmp_image_size = (imageX * imageY * 3) + 48;
    int gray_size = 0;
    
    for (int i=48;i <= bmp_image_size-1;i+=3) {
        unsigned char b = grayscale_Image[i];
        unsigned char g = grayscale_Image[i+1];
        unsigned char r = grayscale_Image[i+2];
        
        //calculate grayscale value
        int grayscale = ((float)r * 0.35) + ((float) g * 0.54) + ((float)b * 0.11);
            
        inBuf[gray_size] = grayscale;
        gray_size = gray_size + 1;
    }
        
    
    // define 5x5 Gaussian kernel
    //float kernel[25] = { 1/256.0f,  4/256.0f,  6/256.0f,  4/256.0f, 1/256.0f,
    //                     4/256.0f, 16/256.0f, 24/256.0f, 16/256.0f, 4/256.0f,
    //                     6/256.0f, 24/256.0f, 36/256.0f, 24/256.0f, 6/256.0f,
    //                     4/256.0f, 16/256.0f, 24/256.0f, 16/256.0f, 4/256.0f,
    //                     1/256.0f,  4/256.0f,  6/256.0f,  4/256.0f, 1/256.0f };
    
    
    
    //float kernel[25] = { 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    //                    0.0f, -1.0f, -2.0f, -1.0f, 0.0f,
    //                    -1.0f, -2.0f, 16.0f, -2.0f, -1.0f,
    //                    0.0f, -1.0f, -2.0f, -1.0f, 0.0f,
    //                    0.0f, 0.0f, -1.0f, 0.0f, 0.0f};
       
    
    //float kernel[9] = {1/9.0f, 1/9.0f, 1/9.0f, 1/9.0f, 1/9.0f, 1/9.0f, 1/9.0f, 1/9.0f, 1/9.0f};
   //float kernel_u[9] = {1.0f, 1.0f, 1.0f, 1.0f, -8.0f, 1.0f, 1.0f, 1.0f, 1.0f};  //-- works good filt  
   float kernel_u[9] = {-1.0f, -1.0f, -1.0f, -1.0f, 8.0f, -1.0f, -1.0f, -1.0f, -1.0f};  //-- works good filt
  
    //calculate LoG kernel
   // create_LoG();

    if(kernel_use == 0) {
    printf("LoG kernel values are:");

    for(int i =0; i < (kernel_size * kernel_size) ; i++) {
	    if ( (i % kernel_size) == 0)
		    printf("\n");
	    printf("%0.2f\t", kernel[i]);
    }

    printf("\n\n");
    }
    
    Timer t;

    // perform convolution on CPU
#ifdef CPU_COMP
    t.start();
    convolve2D(inBuf, outBuf1, imageX, imageY, kernel_u, 3, 3, 100);
    t.stop();
    time1 = t.getElapsedTimeInMilliSec();
    printf("Convolution on CPU: %f ms\n", time1);
#endif

    int image_size = imageX * imageY;
    unsigned char *dev_a, *dev_c;
    float *dev_b;

    //calculate the grid and block dimenions on device
    unsigned int blockDimX = 8; // X ~ COL
    unsigned int blockDimY = 32; // Y ~ ROW
    unsigned int gridDimX = imageX/blockDimX; // X ~ COL
    unsigned int gridDimY = imageY/blockDimY; 

    dim3 GRID( gridDimX, gridDimY, 1);
    dim3 BLOCK( blockDimX, blockDimY, 1);
    
    // allocate memory on the GPU
    cudaMalloc((void **) &dev_a, image_size * sizeof(char));
    if(kernel_use ==1)
    	cudaMalloc((void **) &dev_b, 9 * sizeof(float));
    else
	    cudaMalloc((void **) &dev_b, kernel_size * kernel_size * sizeof(float));
    cudaMalloc((void **) &dev_c,  image_size * sizeof(char));

    // copy the arrays to the GPU
    cudaMemcpy(dev_a, inBuf, image_size * sizeof(char),cudaMemcpyHostToDevice);
    if(kernel_use == 1)
	    cudaMemcpy(dev_b, kernel_u, 9 * sizeof(float) ,cudaMemcpyHostToDevice);
    else
	    cudaMemcpy(dev_b, kernel, kernel_size * kernel_size * sizeof(float) ,cudaMemcpyHostToDevice);

    // Launch kernels
    t.start(); 
    if(kernel_use == 1)
	    convolve_GPU<<<GRID, BLOCK>>>(dev_a, dev_c, dev_b, imageX, imageY, 3, 3);
    else
	    convolve_GPU<<<GRID, BLOCK>>>(dev_a, dev_c, dev_b, imageX, imageY, kernel_size, kernel_size);
    t.stop();
    time2 = t.getElapsedTimeInMilliSec();
    printf("Convolution on GPU: %f ms\n", time2);

    // copy the result array back from GPU to CPU
    cudaMemcpy(outBuf2, dev_c, image_size * sizeof(char) ,cudaMemcpyDeviceToHost);
	
	
	lpTex32=(Color32*)calloc(image_size,sizeof(Color32));
	
	vertex3d_Array vertex; int numberofvertex=1;
	double* vert=(double*)malloc(sizeof(double)*3);

	// Apply the threshold value to the output image and replace black with transparent pixels
	//
	//     8 Connected Neighbours for i,j
	//  (i-1,j) (i+1,j) (i+1,j+1) (i,j+1) (i,j-1) (i-1,j-1) (i+1,J-1) (i-1,j+1)
	//  Check threshold value of each connected neighbour
	//
    for(int i =20; i < imageX;i ++)
	for(int j=20; j<imageY;j++) {
	   
			int iplus,jplus,iminus,jminus;

			((i-1)<0 )?iminus=i:iminus=i-1;
			((i+1)>=imageX)?iplus=i:iplus=i+1;
			((j-1)<0 )?jminus=j:jminus=j-1;
			((j+1)>=imageY)?jplus=j:jplus=j+1;
			//if((abs(outBuf2[iminus*imageX+j])<threshold ) && 
			//	(abs(outBuf2[iplus*imageX+j])<threshold ) &&
			//	(abs(outBuf2[iplus*imageX+jplus])<threshold ) &&
			//	(abs(outBuf2[i*imageX+jplus])<threshold ) &&
			//	(abs(outBuf2[i*imageX+jminus])<threshold ) &&
			//	(abs(outBuf2[iminus*imageX+jminus])<threshold ) &&
			//	(abs(outBuf2[iplus*imageX+jminus])<threshold ) &&
			//	(abs(outBuf2[iminus*imageX+jplus])<threshold ) )
			//
			if(outBuf2[i*imageX+j] < threshold)
			 {

		outBuf2[i*imageX+j] = 0; // We are making the pixel black in grayscale.
		//Construct [ pixel
		lpTex32->a=0; // Pixel is transparent
		lpTex32->r=0;
        lpTex32->g=0;
        lpTex32->b=0;

		}else{
			// Construct opaque while pixel
			//realloc(vertex,sizeof(double)*3);
			//vertex.vertex[0]=((double)i/(double)imageX)-0.5f;
			//vertex.vertex[1]=((double)j/(double)imageY)-0.5f;
			//vertex.vertex[2]=0.0;
			//memcpy(vert,vertex,sizeof(double)*3);
			//v.push_back(vertex);

		 lpTex32->a=0xFF; // Opaque
		 // Specify color value[here its white]
		 lpTex32->r=0xFF;
         lpTex32->g=0xFF;
         lpTex32->b=0xFF;
		 numberofvertex++;
		 
		}

       lpTex32++;
    
	}

	//Take backe the memory location to start of image (Need to replace with formula like in line below)
	for(int i =0; i < image_size;i ++, lpTex32--){}
	
	//lpTex32=lpTex32-((image_size+1)*4);

    //clear CUDA memory
   cudaFree(dev_a);
    cudaFree(dev_b);
    cudaFree(dev_c);

    //Draw the image using OpenGL
  

    glutInitWindowSize     (600, 400);
    glutInitWindowPosition (400, 300);
    glutInitDisplayMode    (GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	
    glutCreateWindow       ("Corner Axes");
    glutDisplayFunc        (display);
	glutIdleFunc           (display);
    glutReshapeFunc        (reshape);
    glutKeyboardFunc       (Keybord);
    glutSpecialFunc        (Special);

	glutMouseFunc (MouseButton);
	glutMotionFunc(MouseMotion);
    glClearColor   (0.1, 0.2, 0.1, 1.0);
   glEnable       (GL_DEPTH_TEST);
   glMatrixMode   (GL_PROJECTION);
    gluPerspective (40.0, 1.5, 1.0, 10.0);
    glMatrixMode   (GL_MODELVIEW);


   
    glutMainLoop(); // Start GLUT event-processing loop 
	free(lpTex32);
    return 0;
}
*/



///////////////////////////////////////////////////////////////////////////////
// initialize global variables
///////////////////////////////////////////////////////////////////////////////
bool initSharedMem()
{
    image = new ImageData;
    if(!image)
    {
        printf("ERROR: Memory Allocation Failed.\n");
        return false;
    }

    // allocate input/output buffer
    inBuf = new unsigned char[imageX * imageY];
    grayscale_Image = (unsigned char *) malloc((imageX * imageY * 3) + 48);
    outBuf2 = new unsigned char[imageX * imageY];

    if(kernel_use == 0)
	    kernel = new float[kernel_size * kernel_size];

#ifdef CPU_COMP
    outBuf1 = new unsigned char[imageX * imageY];
    if(!outBuf2)
    {
        printf("ERROR: Memory Allocation Failed.\n");
        return false;
    }

#endif

    if(!inBuf || !outBuf2)
    {
        printf("ERROR: Memory Allocation Failed.\n");
        return false;
    }

    // set image data
    image->x = imageX;
    image->y = imageY;
    image->format = GL_LUMINANCE;
    image->type = GL_UNSIGNED_BYTE;
    image->buf = (GLvoid*)inBuf;

    return true;
}



///////////////////////////////////////////////////////////////////////////////
// clean up shared memory
///////////////////////////////////////////////////////////////////////////////
void clearSharedMem()
{
    delete image;
    delete [] inBuf;
#ifdef CPU_COMP
    delete [] outBuf1;
#endif
    delete [] outBuf2;
    delete [] grayscale_Image;
    delete [] kernel;
}



///////////////////////////////////////////////////////////////////////////////
// load 8-bit RAW image
///////////////////////////////////////////////////////////////////////////////
bool loadRawImage(char *fileName, int x, int y, unsigned char *data)
{
    // check params
    if(!fileName || !data)
        return false;

    FILE *fp;
    if((fp = fopen(fileName, "r")) == NULL)
    {
        printf("Cannot open %s.\n", fileName);
        return false;
    }

    // read pixel data
    fread(data, 1, x * y * 3, fp);
    fclose(fp);

    return true;
}






void create_LoG()
{
	int temp, col, dx, dy, dxSq, dySq;
	double sigma_sqr = sigma_value * sigma_value;
	double sigma_f = pow(sigma_value, 5);
	double firstTerm = 0.0f;
	double secondTerm = 0.0f;
	//double norm = 1/(sqrt(2 * PI));
	double norm = 1/(sqrt(2 * PI) * sigma_f);
	int size = kernel_size * kernel_size;

	//printf("sigma sqaure value size value %0.2f\n", sigma_sqr);

	//double const_val = norm / sigma_f;
	//printf("Const value %0.2f\n",const_val );

	for(int i=0;i< size; i++)
	{
		temp = (int)(i/kernel_size);
		col = i - (kernel_size * temp);
		dy = ((int) (kernel_size/2)) - temp;
		dx = col - ((int)(kernel_size/2));
		//printf("dx value value %d\n", dx);
		//printf("dy value value %d\n", dy);
		dxSq = dx*dx;
		dySq = dy*dy;    
		firstTerm  = (dxSq + dySq - (2*sigma_sqr)) * norm;
		secondTerm = exp(-0.5 * (dxSq + dySq) / sigma_sqr);
		//printf("firstTerm value value %0.2f\n", firstTerm);
		//printf("secondTerm value value %0.2f\n", secondTerm);
		kernel[i] = firstTerm * secondTerm;
		//printf("kernel value value %0.2f\n", kernel[i]);
	}

	return;
}



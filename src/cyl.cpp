#ifdef _WIN32
#pragma comment(lib,"Ws2_32.lib")
#include <Winsock2.h>
#include <ws2tcpip.h>
#include"libavutil/inttypes.h"
#include <tchar.h>
#include <strsafe.h>
#endif 

#define GL_GLEXT_PROTOTYPES
#define GLEW_STATIC

#include <GL/glut.h>
#include "glext.h"
#include"glInfo.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
//#include<fsrmpipeline.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#ifdef __cplusplus
//#define GL_GLEXT_PROTOTYPES
//#define GLEW_STATIC
extern "C"  {
#else
#define inline __inline
#endif

#ifdef __cplusplus
}
#endif

extern "C" {
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"	
#include "libavutil/opt.h"
}

#include "readBMP.h"
#include "sockfunctions.h"

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

// function pointers for PBO Extension
// Windows needs to get function pointers from ICD OpenGL drivers,
// because opengl32.dll does not support extensions higher than v1.1.
#ifdef _WIN32
PFNGLGENBUFFERSARBPROC pglGenBuffersARB = 0;                     // VBO Name Generation Procedure
PFNGLBINDBUFFERARBPROC pglBindBufferARB = 0;                     // VBO Bind Procedure
PFNGLBUFFERDATAARBPROC pglBufferDataARB = 0;                     // VBO Data Loading Procedure
PFNGLBUFFERSUBDATAARBPROC pglBufferSubDataARB = 0;               // VBO Sub Data Loading Procedure
PFNGLDELETEBUFFERSARBPROC pglDeleteBuffersARB = 0;               // VBO Deletion Procedure
PFNGLGETBUFFERPARAMETERIVARBPROC pglGetBufferParameterivARB = 0; // return various parameters of VBO
PFNGLMAPBUFFERARBPROC pglMapBufferARB = 0;                       // map VBO procedure
PFNGLUNMAPBUFFERARBPROC pglUnmapBufferARB = 0;                   // unmap VBO procedure
#define glGenBuffersARB           pglGenBuffersARB
#define glBindBufferARB           pglBindBufferARB
#define glBufferDataARB           pglBufferDataARB
#define glBufferSubDataARB        pglBufferSubDataARB
#define glDeleteBuffersARB        pglDeleteBuffersARB
#define glGetBufferParameterivARB pglGetBufferParameterivARB
#define glMapBufferARB            pglMapBufferARB
#define glUnmapBufferARB          pglUnmapBufferARB
#endif


#define	QLEN		 5 /* 32 maximum connection queue length */
#define	BUFSIZE		2048    /* buffer size */



//global parameters
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 240;
double rotate_y = 0;
double rotate_x = 20;
int WireFrameOn = 0;

Image *get_image;
long imagewidth, imageheight;
GLuint textureID = 0;
char *filename = "ct_image_cropped.bmp";
GLUquadricObj* myReusableQuadric = 0;

int index_t = 0;
int nextIndex;
struct sockaddr_in	fsin; 
//char*	readbuf;
#ifdef _WIN32
SOCKET
#else
int
#endif
listfd,streamfd;

//fucntion
void myKeyboardFunc( unsigned char key, int x, int y );
void mySpecialKeyFunc( int key, int x, int y );
void drawScene(void);
void initRendering();
void resizeWindow(int w, int h);
void drawGluCylinder( double height, double radiusBase, double radiusTop, int slices, int stacks );
void getImage();
void drawString(const char *str, int x, int y, void *font);
void	send_file(int fd);
//int __cdecl	errexit(const char *format, ...);
//int __cdecl passivesock(int portnumber, const char *transport, int qlen);
void exitMain(int _Code);
int     record=0;
//-1 exit
//0 Pause
// 1 Start/Continue
static int locked=0,tExit=0;
volatile static unsigned int updated=0;
static struct sockaddr_in clienttoSend; 
static int clisendsize;
static int sPlayer=0,sCtrl=0,streamer=0;
AVFrame *picture,*pictYUV;

#ifdef _WIN32
HANDLE hThread ;
#else
bool suspended;
pthread_mutex_t m_SuspendMutex;
pthread_cond_t m_ResumeCond;
#endif


void suspendThread() {
#ifdef _WIN32
	SuspendThread(hThread);
#else
	pthread_mutex_lock(&m_SuspendMutex);
	suspended = true;
	do {
		pthread_cond_wait(&m_ResumeCond, &m_SuspendMutex);
	} while (suspended);
	pthread_mutex_unlock(&m_SuspendMutex);

#endif
}

void resumeThread() {
#ifdef _WIN32
	ResumeThread(hThread);
#else
	pthread_mutex_lock(&m_SuspendMutex);
	suspended = false;
	pthread_cond_signal(&m_ResumeCond);
	pthread_mutex_unlock(&m_SuspendMutex);
#endif
}

time_t prevSec;
GLubyte *pixels,*pictYUV_buf;
GLuint pboIds[2];
static struct SwsContext *img_convert_ctx;

void aquirelock(volatile unsigned int* resource)
{(*resource)++; }
void releaseLock(volatile unsigned int* resource)
{(!((*resource)==0))?(*resource)--:0; }

typedef struct MyThreadData {
	const char * filename;
	int width;
	int height;
} MYDATA, *PMYDATA;


static void  video_encode_example(const char *filename,const int width, const int height)
{
	if(!record)return;
	AVCodec *codec;

	AVCodecContext *c= NULL;
	int i, out_size,out_size1=0, size, x, y, outbuf_size, got_output;
	FILE *f;
	//uint8_t *outbuf, *picture_buf;
	uint8_t *outbuf, *picture_buf;
	AVPacket pkt;
	uint8_t encode[] ={ 0, 0, 1, 0xb7 };
	outbuf_size = 100000;
	outbuf = (uint8_t*)malloc(outbuf_size);

	const uint8_t sps[] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2 };
	const uint8_t pps[] = { 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
	const uint8_t slice_header[] = { 0x00, 0x00, 0x00, 0x01, 0x05, 0x88, 0x84, 0x21, 0xa0 };

	printf("Video encoding\n");
	//AVDictionary* opts= AVDictionary();
	//av_dict_set(&opts, "b", "2.5M", 0);

	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if (!codec) {
		fprintf(stderr, "codec not found\n");
		goto thread_exit;
	}

	c = avcodec_alloc_context3(codec);

	//CODEC MPEG1VIDEO
	// put sample parameters 
	c->bit_rate = 1000*1000;
	//320p 	180 kbit/s | 360p	300 kbit/s |	480p	500 kbit/s	| 576p (SD)	850 kbit/s	| 720p (HD)	1000 kbit/s	
	// resolution must be a multiple of two 
	c->width = width;
	c->height = height;
	// frames per second 
	c->time_base.den= 25;
	c->time_base.num= 1;
	//c->me_range = 16;
	//c->max_qdiff = 4;
	//c->qmin = 10;
	//c->qmax = 51;
	//c->qcompress = 0.6;
	c->gop_size = 10; //emit one intra frame every ten frames 
	c->max_b_frames=1;
	c->pix_fmt = PIX_FMT_YUV420P;

	// av_opt_set(c->priv_data,"profile","baseline",AV_OPT_SEARCH_CHILDREN);
	// av_opt_set(c->priv_data,"preset","slow",0);

	//c->profile = FF_PROFILE_H264_BASELINE; 
	/* open it */

	if (avcodec_open2(c, codec,NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		goto thread_exit;
	}

	pictYUV->format=c->pix_fmt;
	/* alloc image and output buffer */
	size = c->width * c->height;

	img_convert_ctx = sws_getContext(c->width, c->height,PIX_FMT_BGR24, 
			c->width, c->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	if(img_convert_ctx == NULL) {
		fprintf(stderr,"Cannot initialize the conversion context!" );
		goto thread_exit;
	}
	/* H.264 bitstreams */

	//fwrite(sps, 1, sizeof(sps), f);
	//fwrite(pps, 1, sizeof(pps), f);


	/* encode 1 second of video */

	while(1){fflush(stdout);
		if(record<0) goto finalize;
		/*if(updated>0){
		  locked=1;
		  picture->data[0] = (uint8_t*)pixels+(c->width)*3*((c->height)-1);
		  sws_scale(img_convert_ctx, picture->data, 
		  picture->linesize, 0, 
		  c->height,
		  pictYUV->data, pictYUV->linesize);
		  locked=0; 
		//updated=0;

		releaseLock(&updated);
		}
		//av_init_packet(&pkt);
		//pkt.data=NULL;
		//pkt.size=0;
		 encode the image */
		//out_size = avcodec_encode_video2(c, &pkt, pictYUV,&got_output);

		out_size = avcodec_encode_video(c, outbuf, outbuf_size,pictYUV);

		if(out_size<0)
		{
			fprintf(stderr, "Problem encoding"); 
			goto thread_exit;
		}
		//printf("encoding frame %3d (size=%5d)\n", i, out_size);
		//printf("%d",st.wMilliseconds);


		//fwrite(slice_header, 1, sizeof(slice_header), f);

		//fwrite(pkt.data, 1, pkt.size, f);
		//send(sPlayer,(char*) pkt.data,pkt.size,0);
		send(sPlayer,(char*) outbuf,out_size,0);
		//sendto(streamer,(char*) pkt.data,pkt.size,0,(struct sockaddr*)&clienttoSend,clisendsize);

		printf("[%d] %s",errno,strerror(errno));
#ifdef _WIN32

		//if((errno == 10054)||(errno == 10038))
#else
		//if((errno==110) || (errno==111)|| (errno==112)||(errno==113))
#endif
		if(errno!=0)
			goto thread_exit;

#ifdef _WIN32
		Sleep(30);
#else
		usleep(30 * 1000);
#endif

	}
finalize:

	/* get the delayed frames */
	for(; out_size;) {
		fflush(stdout);
		//av_init_packet(&pkt);
		//pkt.data=NULL;
		//pkt.size=0;
		// out_size = avcodec_encode_video2(c, &pkt, NULL, &got_output);

		//fwrite(slice_header, 1, sizeof(slice_header), f);
		//send(sPlayer,(char*) pkt.data,pkt.size,0);
		//	av_free_packet(&pkt);
#ifdef _WIN32

		if(errno == 10054)
#else
			if((errno==110) || (errno==111)|| (errno==112)||(errno==113))
#endif
				goto thread_exit;
		//fwrie(pkt.data, 1, pkt.size, f);
	}



	send(sPlayer,(char*) encode,4,0);
	printf("[%d] %s", errno,strerror(errno));
	//fwrite(encode, 1, 4, f);
thread_exit:
	//fclose(f);
	free(pictYUV_buf);
	//av_free_packet(&pkt);
	free(outbuf);
	avcodec_close(c);
	av_free(c);

	printf("\n");
	tExit=1;

#ifdef _WIN32
	ExitThread(1);
#else
	pthread_exit(0);
#endif


}


#ifdef _WIN32
DWORD WINAPI MyThreadFunction( LPVOID lpParam )
#else
static void * MyThreadFunction(void* lpParam)
#endif	
{
	printf("Executing the video enocde thread\n");
	MYDATA *threadData;
	threadData= (MYDATA *) lpParam;
	video_encode_example(threadData->filename,threadData->width,threadData->height);
	printf("Width from main = %d\n", threadData->width);
#ifdef _WIN32
	return 0;
#endif
}

void pixelUpdate(){
	printf("Calling update fucntion() \n");

	// Read bits from color buffer
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

	// Get the current read buffer setting and save it. Switch to
	// the front buffer and do the read operation. Finally, restore
	// the read buffer state
	//GLenum lastBuffer;
	// glGetIntegerv(GL_READ_BUFFER, (GLint *)&lastBuffer);
	// glReadBuffer(GL_FRONT);
	// "index" is used to read pixels from framebuffer to a PBO
	// "nextIndex" is used to update pixels in the other PBO

	index_t = (index_t + 1) % 2;
	nextIndex = (index_t + 1) % 2;

	// set the target framebuffer to read
	glReadBuffer(GL_FRONT);

	// read pixels from framebuffer to PBO
	// glReadPixels() should return immediately.
	//glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[index]);
	glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_WIDTH,  GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// map the PBO to process its data by CPU
	//glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[nextIndex]);
	//pixels = (GLubyte*)glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB,
	//                                       GL_READ_ONLY_ARB);
	if((pixels) && (index_t)) {
		picture->data[0] = (uint8_t*)pixels+(SCREEN_WIDTH)*3*((SCREEN_HEIGHT)-1); 
		picture->data[1] = picture->data[0] + (SCREEN_WIDTH*SCREEN_HEIGHT);
		//picture->data[1] = picture->data[1] + (SCREEN_WIDTH*SCREEN_HEIGHT);
		picture->data[2] = picture->data[1] + (SCREEN_WIDTH*SCREEN_HEIGHT);

		sws_scale(img_convert_ctx, picture->data, picture->linesize, 0, SCREEN_HEIGHT,pictYUV->data, pictYUV->linesize);

		//updated=0;
		//glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
	}
	// back to conventional pixel operation
	//glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	/*
	   if((record)&&(!locked)){
	   glReadPixels( 0,0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels );
	   }
	//updated =1;
	aquirelock(&updated);
	// glReadBuffer(lastBuffer);

	 */		

}

//get image data
void getImage() {
	get_image = (Image *) malloc(sizeof(Image));
	if (get_image == NULL) {
		printf("Error allocating space for the image");
		exitMain(-1);
	}
	if (!ImageLoad(filename, get_image)) {
		exitMain(-2);
	}
}

void drawGluCylinder( double height, double radiusBase, double radiusTop, int slices, int stacks )
{
	// First draw the cylinder
	if ( ! myReusableQuadric ) {
		myReusableQuadric = gluNewQuadric();
	}

	if(WireFrameOn)
	{
		gluQuadricNormals( myReusableQuadric, GL_TRUE );
		glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );
	}
	else
	{
		gluQuadricNormals( myReusableQuadric, GL_FILL );
		gluQuadricTexture(myReusableQuadric, GL_TRUE);
	}

	// Draw the cylinder.
	gluCylinder( myReusableQuadric, radiusBase, radiusTop, height, slices, stacks );

	// Draw the top disk cap
	glPushMatrix();
	glTranslated(0.0, 0.0, height);
	gluDisk( myReusableQuadric, 0.0, radiusTop, slices, 1.0 );
	glPopMatrix();

	// Draw the bottom disk cap
	glPushMatrix();
	glRotated(180.0, 1.0, 0.0, 0.0);
	gluDisk( myReusableQuadric, 0.0, radiusBase, slices, 1.0 );
	glPopMatrix();

}

void drawString(const char *str, int x, int y, void *font)
{
	//glRasterPos2i(x, y);

	// loop all characters in the string
	while(*str)
	{
		glutBitmapCharacter(font, *str);
		++str;
	}
}

// Callback Function
void specialKeys( int key, int x, int y ) {

	//  Right arrow - increase rotation by 5 degree
	if (key == GLUT_KEY_RIGHT)
		rotate_y -= 5;

	//  Left arrow - decrease rotation by 5 degree
	else if (key == GLUT_KEY_LEFT)
		rotate_y += 5;

	else if (key == GLUT_KEY_UP)
		rotate_x -= 5;

	else if (key == GLUT_KEY_DOWN)
		rotate_x += 5;

	//Escape key
	else if (key == 27)
		exitMain(0);

	//  Request display update
	glutPostRedisplay();
	glFinish();
	//pixelUpdate();

}

void myKeyboardFunc( unsigned char key, int x, int y )
{
	switch ( key ) {
		case 'w':
			WireFrameOn = 1-WireFrameOn;
			if ( WireFrameOn ) {
				glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );		// Just show wireframes
			}
			else {
				glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );		// Show solid polygons
			}
			glutPostRedisplay();
			break;
		case 'r':

			//pixelUpdate();
			record=(!record)?1:0;
			if(record) resumeThread(); else  suspendThread();


			break;
		case 'e':
			suspendThread();
			record=-1;
			resumeThread();
		case 27:	// Escape key
			record=-1;
			resumeThread();
			while(!tExit){}
			exitMain(1);
	}
}

void drawScene(void)
{
	// Clear the rendering window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Get size of BMP image
	getImage();
	imagewidth = get_image->sizeX;
	imageheight = get_image->sizeY;

	// Rotate the image
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glTranslatef( 0.0, 0.0, -50.0 );


	if(!WireFrameOn) {
		// Create OpenGL textures
		glGenTextures(1, &textureID);

		// Bind the texture
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Generate texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imagewidth, imageheight, 0, GL_RGB, GL_UNSIGNED_BYTE, get_image->data);

		//Set texture parameters
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		//Set texture ID
		glEnable(GL_TEXTURE_2D);
		glBindTexture( GL_TEXTURE_2D, textureID );
	}


	glRotatef( rotate_x, 1.0, 0.0, 0.0 );
	glRotatef( rotate_y, 0.0, 1.0, 0.0 );

	glEnable( GL_CULL_FACE );
	glPushMatrix();
	glRotatef( -90.0, 1.0, 0.0, 0.0 );
	glTranslated(2, 2, 2);
	glColor3f( 1.0, 1.0, 1.0 );

	//draw cylinder
	drawGluCylinder( 3.0, 1.5, 1.5, 20, 20 );
	glPopMatrix();

	free(get_image->data);
	free(get_image);

	if(!WireFrameOn)
	{
		//free texture
		glDeleteTextures( 1, &textureID );
		textureID = 0;
	}

	// x, y and z axis
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(4.0, 0.0, 0.0);

	glColor3f(0, 1, 0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 4.0, 0.0);

	glColor3f(1, 0, 1);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, -4.0);
	glEnd();

	glFlush();
	glutSwapBuffers();
	glFinish();
	pixelUpdate();


}


void init()
{
	avcodec_register_all();
	glEnable( GL_DEPTH_TEST );
	glCullFace( GL_BACK );
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	//glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );


	glEnable (GL_LIGHTING); //enable the lighting
	glEnable (GL_LIGHT0); //enable LIGHT0, our Diffuse Light

	// Create light components
	// float ambientLight[]={0.2f,0.2f,0.2f,1.0f};
	// float diffuseLight[]={0.8f, 0.8f, 0.8, 1.0f};
	// float specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f};
	// float position[] = { -1.5f, 1.0f, -4.0f, 1.0f };

	// Assign created components to GL_LIGHT0
	// glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	// glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	// glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	// glLightfv(GL_LIGHT0, GL_POSITION, position);
}


void resizeWindow(int w, int h)
{
	double aspectRatio;
	printf("resize window called\n");

	glViewport( 0, 0, w, h );	    

	w = (w==0) ? 1 : w;
	h = (h==0) ? 1 : h;
	aspectRatio = (double)w / (double)h;
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	//gluPerspective( 15.0, aspectRatio, 25.0, 45.0 );
	gluPerspective( 15.0, aspectRatio, 25.0, 2000.0 );

}

void idle_func(void)
{
	if(tExit)exitMain(0);
	/* read filename sent by client */
	int alen = sizeof(fsin);
	char readbuf[BUFSIZE]="";
	int cc = recv(listfd,  readbuf, BUFSIZE, 0);

	//printf("recved: [%d]  %s\n", errno,strerror(errno));
	//if(cc < 0 )
	//printf("recv failed: %s\n", strerror(errno));

	if(cc > 0) {
		//printf("Data sent by client is: %s\n", (char*) &readbuf);
		//glutTimerFunc(30,Timer,0);

		//  Right arrow - increase rotation by 5 degree
		if (strcmp((char*) readbuf , "GLUT_KEY_RIGHT") == 0){
			rotate_y -= 5;
			//  Request display update
			glutPostRedisplay();
		}

		//  Left arrow - decrease rotation by 5 degree
		else if (strcmp((char*) readbuf, "GLUT_KEY_LEFT") == 0) {
			rotate_y += 5;
			//  Request display update
			glutPostRedisplay();
		}

		else if (strcmp((char*) &readbuf, "GLUT_KEY_UP") == 0) {
			rotate_x -= 5;
			//  Request display update
			glutPostRedisplay(); 
		}

		else if (strcmp((char*) &readbuf, "GLUT_KEY_DOWN") == 0) {
			rotate_x += 5;
			//  Request display update
			glutPostRedisplay();
		}

		//memset(readbuf,0, BUFSIZE-1);
		//glFinish();
		//aquirelock(&updated);
		//pixelUpdate();
	}

}

int main( int argc, char** argv )
{
	//  Initialize GLUT and process user parameters
	glutInit(&argc, argv);

	//readbuf=(char*)malloc(BUFSIZE*sizeof(char));
	//memset(&readbuf,0, BUFSIZE-1);

	//Video stream client. Ideally we start the program if the client comes, we need to create a new process for each new connection.    
	listfd = passivesock(2222, "udp", QLEN);
	clisendsize= sizeof(clienttoSend);

	streamfd = passivesock(4000,"tcp", QLEN);

	struct sockaddr_in cli; int clisize=sizeof(cli);

#ifdef _WIN32
	sPlayer = accept(streamfd,(struct sockaddr*)&clienttoSend,(int*)&clisendsize);
#else
	sPlayer = accept(streamfd,(struct sockaddr*)&clienttoSend,(socklen_t*)&clisendsize);
#endif
	//printf("Stream: [%d] [%d] %s ",sPlayer,errno,strerror(errno));
	printf("Stream Obtained\n");
	//streamer = passivesock(2238, "udp", QLEN);


	//sCtrl= accept(listfd,(struct sockaddr*)&cli,&clisize);
	//printf("Control Obtained\n");


	//printf("Control: [%d] [%d] %s",sPlayer,errno,strerror(errno));

#ifdef WIN32
	u_long nonblocking_enabled = TRUE;
	ioctlsocket(listfd, FIONBIO, &nonblocking_enabled) ;
#else
	fcntl(listfd, F_SETFL, fcntl(listfd, F_GETFL) | O_NONBLOCK);
#endif


	// OpenGL window settings
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
	glutInitWindowPosition( 80, 80 );
	glutInitWindowSize( SCREEN_WIDTH, SCREEN_HEIGHT );
	glutCreateWindow( "Cylinder  Red - X  GREEN -Y  PINK - Z" );

	// Initialize GLUT parameters
	init();
	//glClearColor(1.0, 1.0, 1.0, 1.0);

	// Callback functions
	glutDisplayFunc( drawScene );
	glutKeyboardFunc( myKeyboardFunc );
	glutSpecialFunc( specialKeys );		
	glutReshapeFunc( resizeWindow );
	glutIdleFunc(idle_func);
	//glutTimerFunc(0,Timer,0);

	pixels=(unsigned char*)calloc( SCREEN_WIDTH * SCREEN_HEIGHT *3,sizeof(unsigned char));
	//wglMakeCurrent(hDC,hRC);
	// Call this for background processing
	// glutIdleFunc( myIdleFunction );

	// glMatrixMode   (GL_MODELVIEW);
	printf("Render name is: %s\n", glGetString(GL_RENDER));

#ifdef _WIN32
	PMYDATA myThreadData = (PMYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(MYDATA));
	myThreadData->filename ="test.mp4";
	myThreadData->width=SCREEN_WIDTH;
	myThreadData->height=SCREEN_HEIGHT;
	DWORD threadID;
	// video_encode_example("test.mpg",SCREEN_WIDTH,SCREEN_HEIGHT);
	hThread = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			MyThreadFunction,       // thread function name
			myThreadData,          // argument to thread function 
			CREATE_SUSPENDED,                      // use default creation flags 
			&threadID);   // returns the thread identifier 
	if(hThread == NULL)
	{
		printf("Error cannot create thread");
	}	
/* #else
	PMYDATA myThreadData = (PMYDATA) malloc(sizeof(MYDATA));
	myThreadData->filename ="test.mp4";
	myThreadData->width=SCREEN_WIDTH;
	myThreadData->height=SCREEN_HEIGHT;
	pthread_t thread_id;
	pthread_attr_t        pta;
	int s = pthread_attr_init(&pta);
	//int s = pthread_attr_init(&attr);

	if (s != 0)
		perror( "pthread_attr_init");

	s=pthread_create (&thread_id, &pta,MyThreadFunction, &myThreadData);
	//suspend
*/
#endif				

	picture= avcodec_alloc_frame();
	pictYUV= avcodec_alloc_frame();

	//  Pass control to GLUT for events
	picture->linesize[0] =-3*SCREEN_WIDTH;
	picture->linesize[1] =-3*SCREEN_WIDTH;
	picture->linesize[2] =-3*SCREEN_WIDTH;



	pictYUV_buf = (uint8_t*)malloc(((SCREEN_WIDTH*SCREEN_HEIGHT) * 3) / 2); /* size for YUV 420 */
	pictYUV->data[0] = pictYUV_buf;
	pictYUV->data[1] = pictYUV->data[0] + (SCREEN_WIDTH*SCREEN_HEIGHT);
	pictYUV->data[2] = pictYUV->data[1] + (SCREEN_WIDTH*SCREEN_HEIGHT) / 4;
	pictYUV->linesize[0] = SCREEN_WIDTH;
	pictYUV->linesize[1] = SCREEN_WIDTH / 2;
	pictYUV->linesize[2] = SCREEN_WIDTH/ 2;

	glInfo glInfo;
	glInfo.getInfo();
	glInfo.printSelf();

#ifdef _WIN32
	// check PBO is supported by your video card
	if(glInfo.isExtensionSupported("GL_ARB_pixel_buffer_object"))  {
		// get pointers to GL functions
		glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffersARB");
		glBindBufferARB = (PFNGLBINDBUFFERARBPROC)wglGetProcAddress("glBindBufferARB");
		glBufferDataARB = (PFNGLBUFFERDATAARBPROC)wglGetProcAddress("glBufferDataARB");
		glBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)wglGetProcAddress("glBufferSubDataARB");
		glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)wglGetProcAddress("glDeleteBuffersARB");
		glGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)wglGetProcAddress("glGetBufferParameterivARB");
		glMapBufferARB = (PFNGLMAPBUFFERARBPROC)wglGetProcAddress("glMapBufferARB");
		glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)wglGetProcAddress("glUnmapBufferARB");

		// check once again PBO extension
		if(glGenBuffersARB && glBindBufferARB && glBufferDataARB && glBufferSubDataARB &&
				glMapBufferARB && glUnmapBufferARB && glDeleteBuffersARB && glGetBufferParameterivARB)
		{
			// pboSupported = pboUsed = true;
			printf(  "Video card supports GL_ARB_pixel_buffer_object.\n" );
		}
		else
		{
			//pboSupported = pboUsed = false;
			printf ( "Video card does NOT support GL_ARB_pixel_buffer_object.\n" );
		}
	}

#else // for linux, do not need to get function pointers, it is up-to-date
	if(glInfo.isExtensionSupported("GL_ARB_pixel_buffer_object"))
	{
		//pboSupported = pboUsed = true;
		printf( "Video card supports GL_ARB_pixel_buffer_object.\n" );
	}
	else
	{
		//pboSupported = pboUsed = false;
		printf( "Video card dprintf( support GL_ARB_pixel_buffer_object.\n" );
	}
#endif
	// create 2 pixel buffer objects, you need to delete them when program exits.
	// glBufferDataARB with NULL pointer reserves only memory space.
	/*
	glGenBuffersARB(2, pboIds);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[0]);
	glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, SCREEN_WIDTH * SCREEN_HEIGHT * 4, 0, GL_STREAM_READ);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[1]);
	glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, SCREEN_WIDTH * SCREEN_HEIGHT * 4, 0, GL_STREAM_READ);

	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	*/

	//pixelUpdate();
	record =1;
//#ifdef __LINUX__
//	PMYDATA linuxThreadData = (PMYDATA) malloc(sizeof(MYDATA));
	MYDATA linuxThreadData;
	linuxThreadData.filename ="test.mp4";
	linuxThreadData.width = SCREEN_WIDTH;
	linuxThreadData.height = SCREEN_HEIGHT;

	pthread_t thread_id;
	pthread_attr_t        pta;
	int s = pthread_attr_init(&pta);
	//int s = pthread_attr_init(&attr);

	if (s != 0)
		perror( "pthread_attr_init");

	s = pthread_create (&thread_id, &pta, MyThreadFunction, (void*) &linuxThreadData);
//#endif

#ifdef _WIN32
	resumeThread();
#endif

	glutMainLoop(  );

	return(0);
}

void exitMain(int _Code)
{
	//Freeup all Resources
	av_free(picture);
	av_free(pictYUV);
	free(pixels);
	//WSACleanup();
	//free(readbuf);
	exit(_Code);
}



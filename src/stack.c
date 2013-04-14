//#include <stdio.h>
//#include <stdlib.h>
//#include <GL/glut.h>
////#include"readBMP.h"
/////--3rd Window Functions
//short wire = TRUE;
// 
//float w, h, tip = 0, turn = 0;
// 
//float ORG[3] = {0,0,0};
// 
//float XP[3] = {1,0,0}, XN[3] = {-1,0,0},
//      YP[3] = {0,1,0}, YN[3] = {0,-1,0},
//      ZP[3] = {0,0,1}, ZN[3] = {0,0,-1};
////Screen constants
//const int SCREEN_WIDTH = 900;
//const int SCREEN_HEIGHT = 900;
//const double CUBESIZE = 1.0;
//const double div_num = 1.8;
//GLuint textureID = 0;
//
//
//
//
//
// 
//void reshape (int nw, int nh)
//{
//    w = nw;
//    h = nh;
//}
// 
//void Keybord (unsigned char key, int x, int y)
//{
//    switch (key) {
//       case  'w' : wire = !wire;  break;
//       case   27 : exit (0);
//       default   : printf ("   %c == %3d from Keybord\n", key, key);
//    }
//}
// 
//void Special (int key, int x, int y)
//{
//    switch (key) {
//       case  GLUT_KEY_RIGHT: turn += 5;  break;
//       case  GLUT_KEY_LEFT : turn -= 5;  break;
//       case  GLUT_KEY_UP   : tip  -= 5;  break;
//       case  GLUT_KEY_DOWN : tip  += 5;  break;
// 
//       default : printf ("   %c == %3d from Special\n", key, key);
//    }
//}
//int clicked=0;
////MouseButton Callback function
//
//void MouseMotion(int x, int y){
//	static float lastx=0.0;
//	static float lasty=0.0;
//	printf("\n%d %d",x,y);
//	lastx=(float)x-lastx;
//	lasty=(float)y-lasty;
//	if( (float)x>lastx)
//		turn -=lastx;
//	else
//		turn+=lastx;
//	if((float)y>lasty)
//	tip +=lastx;
//	else
//	tip-=lastx;
//
//	if(abs((int)lastx)>10||(abs((int)lasty)>10))
//	{
//	lastx=(float)x;
//	lasty=(float)y;
//	return;
//	}
//	//glutPostRedisplay();
//}
//void MouseButton (int btn, int state, int x, int y)
//{
////	(state == GLUT_DOWN)?clicked=1:clicked=0;
//    //mouse wheel events
//    if (btn == 3 || btn == 4 )
//    {
//        //scroll up
//        if(state == GLUT_UP)
//           tip+=5;
//        
//        //scroll down
//        else if(state == GLUT_DOWN)
//            tip-=5;
//    }
//    
//    //left mouse buttom click
//   // else if (btn == GLUT_LEFT_BUTTON && clicked)
//   //   (x!=0 && y!=0)?tip+=:  (x!=0 && y==0)?tip += x:((y!=0 && x==0)?tip+=y:tip=tip);
//		
//    
//    //right mosue button click
//   // else if (btn == GLUT_RIGHT_BUTTON && clicked)
//   //     turn -= x;
//    
//    //middle mouse button click
//   // else if (btn == GLUT_MIDDLE_BUTTON && state ==GLUT_DOWN)
//    //    turn -= y;
//        
//    // Request display update
//   // glutPostRedisplay();
//    
//}
//
//void Draw_Axes (void)
//{ 
//    glPushMatrix ();
// 
//       glTranslatef (-2.4, -1.5, -5);
//       glRotatef    (0 , 1,0,0);
//       glRotatef    (0, 0,1,0);
//       glScalef     (0.25, 0.25, 0.25);
// 
//       glLineWidth (2.0);
// 
//       glBegin (GL_LINES);
//          glColor3f (1,0,0);  glVertex3fv (ORG);  glVertex3fv (XP);    // X axis is red.
//          glColor3f (0,1,0);  glVertex3fv (ORG);  glVertex3fv (YP);    // Y axis is green.
//          glColor3f (0,0,1);  glVertex3fv (ORG);  glVertex3fv (ZP);    // z axis is blue.
//       glEnd();
// 
//   glPopMatrix ();
//}
//
//void draw_stack ()
//{
//	int ii; 
//	glPushMatrix ();
//	 glTranslatef (0, 0, -5);
//       glRotatef (tip , 1,0,0);
//       glRotatef (turn, 0,1,0);
//   
//	for(ii=0;(CUBESIZE/div_num-((float)ii)*5.0/100.0)>= -( CUBESIZE/div_num);ii++){
//	  // cube - TOP
//    glBegin(GL_POLYGON);
//    glColor3f(   1.0,  1.0,  1.0 );
//    glTexCoord2f(0.0f, 1.0f);
//    glVertex3d(  CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*5.0/100.0,  CUBESIZE/div_num );
//    glTexCoord2f(1.0f, 1.0f);
//    glVertex3d(  CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*5.0/100.0, -CUBESIZE/div_num );
//    glTexCoord2f(1.0f, 0.0f);
//    glVertex3d( -CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*5.0/100.0, -CUBESIZE/div_num );
//    glTexCoord2f(0.0f, 0.0f);
//    glVertex3d( -CUBESIZE/div_num,  CUBESIZE/div_num-((float)ii)*5.0/100.0,  CUBESIZE/div_num);
//    glEnd();
//	}
//     glPopMatrix (); 
//
//}
//
//void display (void)
//{
//   // glutSetWindow(window);
//   // glClear(GL_COLOR_BUFFER_BIT); // clear canvas
//
//	glViewport (0, 0, w, h);
//    glClear    (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	
//    
//    // Create OpenGL textures
//    glGenTextures(1, &textureID);
//    
//    // Bind the texture
//    glBindTexture(GL_TEXTURE_2D, textureID);
//    
//    // Generate texture
//	glTexImage2D(GL_TEXTURE_2D, 0, outputImage.format, imagewidth, imageheight, 0, outputImage.format, outputImage.type, outputImage.buf);
//    
//    
//    //Set texture parameters
//    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//    
//    //Set texture ID
//    glEnable(GL_TEXTURE_2D);
//    glBindTexture( GL_TEXTURE_2D, textureID );
//    
//    draw_stack();
//	Draw_Axes();
//    //free texture
//    glDeleteTextures( 1, &textureID );
//    textureID = 0;
//   // Draw_Teapot ();
//	
// 
//    glutSwapBuffers ();
//}
//// 
////void main (void)
////{
////	//filename = "sjsu_college.bmp";
////	//Get size of BMP image
////   // getImage();
////   // imagewidth = get_image->sizeX;
////   // imageheight = get_image->sizeY;
////
////    glutInitWindowSize     (600, 400);
////    glutInitWindowPosition (400, 300);
////    glutInitDisplayMode    (GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
////    glutCreateWindow       ("Corner Axes");
////    glutDisplayFunc        (display);
////    glutIdleFunc           (display);
////    glutReshapeFunc        (reshape);
////    glutKeyboardFunc       (Keybord);
////    glutSpecialFunc        (Special);
//// 
////	glutMouseFunc (MouseButton);
////	glutMotionFunc(MouseMotion);
////
////    glClearColor   (0.1, 0.2, 0.1, 1.0);
////    glEnable       (GL_DEPTH_TEST);
////    glMatrixMode   (GL_PROJECTION);
////    gluPerspective (40.0, 1.5, 1.0, 10.0);
////    glMatrixMode   (GL_MODELVIEW);
//// 
////    glutMainLoop ();
////	printf("Over");
////}
//
//

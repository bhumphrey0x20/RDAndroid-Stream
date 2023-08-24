/* 
	This code uses rawdrawandroid templet to create a socket client, receiving webcam video from a local server.
*/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "os_generic.h"
#include <GLES3/gl3.h>
#include <asset_manager.h>
#include <asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <android/sensor.h>
#include "CNFGAndroid.h"
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "parse.h"
#include "vars.h"

#include <sys/un.h>

#define CNFG_IMPLEMENTATION
#define CNFG3D

#include "CNFG.h"

#include <android/log.h>
#define  LOG_TAG    "NSocket"



ASensorManager * sm;
const ASensor * as;
ASensorEventQueue* aeq;
ALooper * l;


// Set up socket variables

int sock;
struct sockaddr_in server;
char message[1000];
uint8_t server_reply[maxMsgSize];
msg rx_msg;
msg_img mi;
int getSocketData=0;
int img_w = 800;
int img_h = 800;


// string arrays
char st[20];
char stm[50];
char stm2[50];

void alarm_handler(int signo);
void timer_function();
void *get_socket_data();


void SetupIMU()
{
	sm = ASensorManager_getInstance();
	as = ASensorManager_getDefaultSensor( sm, ASENSOR_TYPE_GYROSCOPE );

	l = ALooper_prepare( ALOOPER_PREPARE_ALLOW_NON_CALLBACKS );
	aeq = ASensorManager_createEventQueue( sm, (ALooper*)&l, 0, 0, 0 ); //XXX??!?! This looks wrong.

		ASensorEventQueue_enableSensor( aeq, as);
		printf( "setEvent Rate: %d\n", ASensorEventQueue_setEventRate( aeq, as, 10000 ) );
}

float accx, accy, accz;
int accs;

void AccCheck()
{

	ASensorEvent evt;
do
	{
		ssize_t s = ASensorEventQueue_getEvents( aeq, &evt, 1 );
		if( s <= 0 ) break;
		accx = evt.vector.v[0];
		accy = evt.vector.v[1];
		accz = evt.vector.v[2];
		
	} while( 1 );
}

unsigned frames = 0;
unsigned long iframeno = 0;

void AndroidDisplayKeyboard(int pShow);

int lastbuttonx = 0;
int lastbuttony = 0;
int lastmotionx = 0;
int lastmotiony = 0;
int lastbid = 0;
int lastmask = 0;
int lastkey, lastkeydown;

static int keyboard_up;

void HandleKey( int keycode, int bDown )
{
	lastkey = keycode;
	lastkeydown = bDown;
	//if( keycode == 10 && !bDown ) { keyboard_up = 0; AndroidDisplayKeyboard( keyboard_up );  }

	if( keycode == 4 ) { AndroidSendToBack( 1 ); } //Handle Physical Back Button.
}

void HandleButton( int x, int y, int button, int bDown )
{
	lastbid = button;
	lastbuttonx = x;
	lastbuttony = y;

	//if( bDown ) { keyboard_up = !keyboard_up; AndroidDisplayKeyboard( keyboard_up ); }
}

void HandleMotion( int x, int y, int mask )
{
	lastmask = mask;
	lastmotionx = x;
	lastmotiony = y;
}

#define HMX 162
#define HMY 162
short screenx, screeny;


extern struct android_app * gapp;


void HandleDestroy()
{
	printf( "Destroying\n" );
	exit(10);
}

volatile int suspended;

void HandleSuspend()
{
	suspended = 1;
}

void HandleResume()
{
	suspended = 0;
}


int main()
{
	int x, y;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();	int linesegs = 0;
 
	CNFGBGColor = 0x503060ff;
	
	CNFGSetupFullscreen( "Test Bench", 0 );
	//CNFGSetup( "Test Bench", 0, 0 );

	//set up thread for socket messages
	pthread_t socket_thread;

	//Create Socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		__android_log_print(ANDROID_LOG_INFO,LOG_TAG,"Socket Creation Error");
		sprintf(stm, "Socket Failed.");
		CNFGPenX = 10; CNFGPenY = 800;
		CNFGDrawTextbox(CNFGPenX, CNFGPenY,  st, 10 );
	}else{
		__android_log_print(ANDROID_LOG_INFO,LOG_TAG, "Socket Created");
		CNFGPenX = 10; CNFGPenY = 800;
		sprintf(stm, "Socket Created!");
		//CNFGDrawTextbox(CNFGPenX, CNFGPenY,  "Socket Created.", 10 );
	}	
	//TODO: add IP Address of local server
	server.sin_addr.s_addr = inet_addr("IP Add of server");
	//server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family = AF_INET;
	server.sin_port = htons( 8888 );


	int ret = 0; 


	ret = connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_un) );
	if( ret <0){
		__android_log_print(ANDROID_LOG_INFO,LOG_TAG,"Socket Connection Error");
		sprintf(stm2, "Error: Connecting Socket ");
		
	}else{
		__android_log_print(ANDROID_LOG_INFO,LOG_TAG, "Socket Connected");
		sprintf(stm2, "Socket Connected success");
		
	}

 	timer_function(); //sets a timer to contact server and receive data

	while(1)
	{
		int i, pos;
		iframeno++;
		
		CNFGHandleInput();
//		AccCheck();
		if( suspended ) { usleep(50000); continue; }

		CNFGClearFrame();
		
		CNFGColor( 0xFFFFFFFF );
		CNFGGetDimensions( &screenx, &screeny );

		// Comm with Server
	if(	getSocketData ==1){

			//Send some data
		if( pthread_create( &socket_thread, NULL, get_socket_data, NULL) ){
			//perror("could not create thread");
			__android_log_print(ANDROID_LOG_INFO,LOG_TAG, "Could Not Create Thread.");
			return 1;
		}
	


			getSocketData = 0;
	} //if getSocketData	

		
		//Change color to black.
		
		CNFGColor( 0xffffffff ); 

	//write incoming data to screen
		short w, h;
		int x, y; 
		CNFGClearFrame();
		CNFGGetDimensions( &w, &h );
	
		sprintf(st, "%hhu  ", mi.code);
		CNFGPenX = 10; CNFGPenY = 10;
		CNFGDrawText( text_code, 5 );		
		CNFGDrawTextbox(CNFGPenX+x+5, CNFGPenY,  st, 5 );

		CNFGPenX = 10; CNFGPenY = 75;
		CNFGGetTextExtents( text_w , &x, &y, 5 );
		sprintf(st, "%u  ", (mi.img_dim >> 16) );	
		CNFGDrawText( text_w, 5 );
		CNFGPenX+=x+5;
		CNFGDrawTextbox(CNFGPenX, CNFGPenY,  st, 5 );
		CNFGPenX += 10+x+5; 
		CNFGGetTextExtents( text_h , &x, &y, 5 );
		sprintf(st, "%u  ", (mi.img_dim & 0x0000ffff) );	
		CNFGDrawText( text_h, 5 );		
		CNFGDrawTextbox(CNFGPenX+x+5, CNFGPenY,  st, 5 );
		sprintf(st, "%u  ", mi.img_size);
		CNFGPenX = 10; CNFGPenY = 150;
		CNFGDrawTextbox(CNFGPenX, CNFGPenY,  st, 10 );


		CNFGPenX = 10; CNFGPenY = 250;
		CNFGDrawTextbox(CNFGPenX, CNFGPenY,  stm, 10 );
		
		//Draw image if available
		CNFGBlitImage( mi.img, 10, 500, img_w, img_h );


		CNFGFlushRender();


		frames++;
		//On Android, CNFGSwapBuffers must be called, and CNFGUpdateScreenWithBitmap does not have an implied framebuffer swap.
		CNFGSwapBuffers();

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			//printf( "FPS: %d\n", frames );
			frames = 0;
			linesegs = 0;
			LastFPSTime+=1;
		}

	} //while()

	close(sock);

	return(0);
}


/**********************************
FUNCTIONS 

***********************************/


void alarm_handler(int signo){
 message[0] = 32;
 getSocketData = 1;	// set socket flag
}


void timer_function(){
	struct itimerval delay;
	int ret; 
	
	signal(SIGALRM, alarm_handler);
	
	delay.it_value.tv_sec = 1; 
	delay.it_value.tv_usec= 0;
	delay.it_interval.tv_sec = 0; 
	delay.it_interval.tv_usec = 250000;
	ret = setitimer(ITIMER_REAL, &delay, NULL);
	if(ret){ perror("setitimer"); }

}


// receive and parse data received on buffer
void *get_socket_data(){

	int msg_cnt=0;
	int msg_buf_cnt=0; 
	
	
			if( msg_cnt = send(sock , message , strlen(message) , 0) < 0)
			{
				puts("Send failed");

			}
			printf("\nMsg Sent, %i bytes\n", msg_cnt); 
			
			
			//Receive a reply from the server
			msg_cnt = recv(sock , server_reply , maxMsgSize , 0);
			//printf("recv size: %i\n", msg_cnt);
			if(msg_cnt < 0 )
			{
				puts("recv failed");
			
			}else if(msg_cnt == 1){
				puts("recv 0 bytes");
			}else{

				uint8_t msgCode = server_reply[0]; 
				uint32_t data_size = (server_reply[1]<<24) | (server_reply[2] << 16) | (server_reply[3] << 8) | server_reply[4] ;
				int loopCnt = 0; //debug
				while( msg_cnt <  data_size){
					msg_cnt+= recv(sock, &server_reply[msg_cnt], maxMsgSize, 0); 
				}

	

				if(msgCode == 32){
					msg_buf_cnt = decode_img_msg(server_reply, &mi, msg_cnt, msg_buf_cnt);

					if (msg_buf_cnt >= mi.total_size){
						msg_buf_cnt = 0; 
					}
					

					
				}else{			
					decode(server_reply, &rx_msg);

					
				} //msg code
			}
			
}







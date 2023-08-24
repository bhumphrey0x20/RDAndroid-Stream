/***********************************************************
	server for android phone webcam streaming. Uses openCV 4 to 
	grab webcam video and sent to client via socket server. 
	
***********************************************************/


#include<stdio.h>
#include <string.h>
#include <stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include <unistd.h>
#include<pthread.h> //for threading , link with lpthread
		/*** MUST ADD "-pthread" as option in gcc during compile ***/
#include <thread>		
#include<iostream>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "parse.h"		
		
using namespace std; 
using namespace cv;


void *connection_handler(void *);
void *camera_handler(void *);
void encode_image_data(Mat *img, uint32_t *dataArray, int w, int h, int ch);


Mat frame0, frame1;
double width0, height0,width1, height1;
int rsz_w = 800; 
int rsz_h = 800; 
double fps=15;
bool isQuit = false;
uint32_t frame_num = 0;

VideoCapture cam0;


int main(int argc , char *argv[])
{
	
	//Create socket
	int socket_desc , new_socket , c, *new_sock;
	struct sockaddr_in server , client;
	char message[50];
	
	
	// Load the web cam
	cam0.open(200);		//creating webcam obj
	
	if(!cam0.isOpened()){
			cout << "Could Not open Camera 0" << endl;
			return -1;
	}

	width0 = cam0.get(CAP_PROP_FRAME_WIDTH);
	height0 = cam0.get(CAP_PROP_FRAME_HEIGHT);	
	
	cout << "Camera is Open. \n\nPress 'q' to quit!" << endl;
 
 //create camera thread
 pthread_t camera_thread;
 if( pthread_create( &camera_thread, NULL, camera_handler, NULL) ){
	perror("could not create thread");
	return 1;
 }
	
	
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY; //local host	
	server.sin_port = htons( 8888);
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return -1;
	}else{
		puts("bind done");
	}
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c) ){
		puts("Connection accepted");
		

//* C thread

	pthread_t sniffer_thread;
	new_sock = new int[1];
	*new_sock = new_socket;

		
		if( pthread_create( &sniffer_thread, NULL, connection_handler, (void*) new_sock) ){

			perror("could not create thread");
			return 1;
		}
		

/*
	// Create C++ thread
	thread socket_thread(connection_handler,&new_socket); 
*/
		puts("Handler assigned");
		
	}

	if (new_socket<0)
	{
		perror("accept failed");
		return 1;
	}


	return 0;
}



/* *
 * This will handle connection for each client
 * */ 

void *connection_handler(void *socket_desc)
{ 


	//Get the socket descriptor
	int sock = *(int*)socket_desc;

	int read_size;
	char client_message[2000];
	int len=0; 
	
	msg tx_msg  = {212,1020,305419896,-32.12345};
	msg_img mi; 
	

	//Receive a message from client and send data
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{
		// get code from message and choose next option
		uint8_t data = client_message[0];
		printf("\t%u \n", (unsigned int)data); 
		
		if(data == 32){
			// fill msg_img structure
			encode_image_data(&frame1, image_data, rsz_w, rsz_h, 3);
			mi.code=32;
			mi.total_size=sizeof(msg_img);  
			mi.msgNum = frame_num;
			mi.img_dim= ((uint32_t)(rsz_w) << 16) | ((uint32_t)(rsz_h) ); 
			mi.img_size=max_num_pix;

			for(int i = 0; i<max_num_pix; i++){
				mi.img[i] = image_data[i]; 
			}
	
			len = encode_img_msg(buf, &mi);
				
		}else if(data == 128){
			tx_msg.code = 128;
			tx_msg.msgNum = 2010;
			tx_msg.val2 = 2900128;
			tx_msg.val3 = 128.67890;
			encode(buf, &tx_msg); 
			len = sizeof(tx_msg); 
						
					//debug 
			for(int i = 0; i< 11; i++){
				printf("%X ", buf[i]); 		
			}	
		}else if(data == 250){
			
			tx_msg.code = 250;
			tx_msg.msgNum = 980;
			tx_msg.val2 = 1200255;
			tx_msg.val3 = -2.1379;
			encode(buf, &tx_msg); 
			len = sizeof(tx_msg); 			
		
			//debug 
			for(int i = 0; i< 11; i++){
				printf("%X ", buf[i]); 
			}
		}else{
			encode(buf, &tx_msg); 
			len = sizeof(tx_msg); 			
		
					//debug 
			for(int i = 0; i< 11; i++){
				printf("%X ", buf[i]); 
			}
		}
			printf("\n\n"); 
			
			//Send the data back to client
			//write(sock , buf , 20);
			send(sock, buf, len, 0); 

	}
	
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}


void *camera_handler(void *){
	int key;
	bool isQuit = false;
	
	do{
	cam0.grab();
	cam0.retrieve(frame0, 0); 
	resize(frame0, frame1, Size(rsz_w, rsz_h), 0, 0,INTER_AREA);
	frame_num++; 	
//	imshow(window,frame1);
	key = waitKey(fps);		
	if( ( key== 'q') || ( key == 'Q') ) isQuit = true;
	imshow("camera", frame1); 
	}while(!isQuit); //video loop

	cam0.release();
	return 0;


}


/* Formats image pixels for rawdrawandroid CNFGBlitImage(), that draws image
to screen. pixel format is 0xRRBBGGAA

*/

void encode_image_data(Mat *img, uint32_t *dataArray, int w, int h, int ch){
printf("w: %i, h: %i, ch: %i\n", img->cols, img->rows, img->channels()); 
	uint32_t px0,px1,px2;
	uint32_t pixel = 0; 
	int d = 0;
	//int k = 0;
	int shift = 24; 

	
	for(int i = 0; i< h; i++){	
		for(int j = 0; j< w; j++){
			pixel = 0; 
			px0 = img->at<Vec3b>(i, j)[0];	
			px1 =	img->at<Vec3b>(i, j)[1];	
			px2	=	img->at<Vec3b>(i, j)[2];	

			pixel |= (uint32_t)px2 << 24; 
			pixel |= (uint32_t)px1 << 16; 
			pixel |= (uint32_t)px0 << 8;
		 	pixel |= 0xff; //preset alpha chanel 	

			dataArray[d] = pixel;

			d++; 
		} //for j
	} //for i
	d=0; 


}


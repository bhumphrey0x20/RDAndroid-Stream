/*********************************
	parsing uchar data for server-client msg
*********************************/
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <inttypes.h>


#define maxMsgSize 2600200 //1000100
#define max_num_pix 800*800

int msgCounter=0; 
uint32_t image_data[max_num_pix];
uint8_t buf[maxMsgSize];

typedef struct {
	uint8_t code; 
	uint16_t msgNum; 
	uint32_t val2;
	float val3; 
}msg;


typedef struct __attribute__((__packed__)){
	uint8_t code;
	uint32_t total_size;  			//total size of data 
	uint32_t msgNum;						// sequential number of data sent to client
	uint32_t img_dim;						// stores OR'd values for image width and height 
	uint32_t img_size;					// size of image
	uint32_t img[max_num_pix];  // image sent
}msg_img;

/**************
	Functions
**************/

void encode32(uint8_t *_buf, uint32_t val){
	_buf[0] = (val >> 24); 
	_buf[1] = (val >> 16) & 0x00ff; 
	_buf[2] = (val >> 8) & 0x0000ff; 
	_buf[3] = (val ) & 0x000000ff; 

}

void encodeF(uint8_t *_buf, float f){
	//float f = _msg->val3;

	uint32_t sign=0;
	uint32_t p = 0; 
	if( f < 0){
		sign = 1;
		f = -f;
	}else{
		sign = 0;
	}
	
	p = ((((uint32_t)f)&0x7fff)<<16) | (sign<<31); // whole part and sign
	p |= (uint32_t)(((f - (int)f) * 65536.0f))&0xffff; // fraction

	_buf[0] = (p >> 24); 
	_buf[1] = (p >> 16) & 0x00ff; 
	_buf[2] = (p >> 8) & 0x0000ff; 
	_buf[3] = (p ) & 0x000000ff; 
	
}

int encode(uint8_t *_buf, msg* _msg){
	int msg_size = 0; 
	_buf[0] = _msg->code;
	_buf[1] = (_msg->msgNum>>8 & 0x00ff); 
	_buf[2] = (_msg->msgNum & 0x00ff);
	msg_size+= 3; 
	encode32(&_buf[3], _msg->val2);
	msg_size += 4; 
	encodeF(&_buf[7], _msg->val3); 
	msg_size += 4; 
//	buf[3] = (_msg->val2>>8); // & 0x00ff; 
//	buf[4] = (_msg->val2 & 0xff);

	printf("Msg_size: %i -- thread\n", msg_size); 
	return msg_size; 
}


/*
	Takes the data from struct msg_img _msg and writes it to 
	_buf as unsigned char. 
	returns the message length in bytes
*/

int encode_img_msg(uint8_t * _buf, msg_img* _msg){

	int msg_size = 0; 
	_buf[0] = _msg->code; //msg code
	msg_size+=1; 
	_buf[1] = (_msg->total_size & 0xff000000) >> 24;	// total_size
	_buf[2] = (_msg->total_size & 0x00ff0000) >> 16;
	_buf[3] = (_msg->total_size & 0x0000ff00) >> 8;
	_buf[4] = (_msg->total_size & 0x000000ff);
	msg_size+= 4;
	_buf[5] = (_msg->total_size & 0xff000000) >> 24;	// msgNum
	_buf[6] = (_msg->total_size & 0x00ff0000) >> 16;
	_buf[7] = (_msg->total_size & 0x0000ff00) >> 8;
	_buf[8] = (_msg->total_size & 0x000000ff);
	msg_size+= 4;
	_buf[9] = (_msg->img_dim & 0xff000000) >> 24;		// dim
	_buf[10] = (_msg->img_dim & 0x00ff0000) >> 16;
	_buf[11] = (_msg->img_dim & 0x0000ff00) >> 8;
	_buf[12] = (_msg->img_dim & 0x000000ff);
	msg_size+= 4;
	_buf[13] = (_msg->img_size & 0xff000000) >> 24;  //img_size
	_buf[14] = (_msg->img_size & 0x00ff0000) >> 16;
	_buf[15] = (_msg->img_size & 0x0000ff00) >> 8;
	_buf[16] = (_msg->img_size & 0x000000ff);
	msg_size+= 4;

	

	// encode the image to buffer
	int j= 17; 
	for( int i = 0; i< max_num_pix; i++){
		_buf[j++] = (_msg->img[i] & 0xff000000) >> 24;
		_buf[j++] = (_msg->img[i] & 0x00ff0000) >> 16;
		_buf[j++] = (_msg->img[i] & 0xff00) >> 8;
		_buf[j++] = (_msg->img[i] & 0xff);
		msg_size+= 4;
	}
	
	printf("Msg_size: %i --parse.h\n", msg_size); 
	puts("Decode 3"); 
	return msg_size; 

}


uint32_t decode32(uint8_t *_buf){
 	uint32_t val= 0; 
	val = (_buf[0]<<24) | (_buf[1] << 16) | (_buf[2] << 8) | _buf[3] ; 
	return val; 
}


float decodeF(uint8_t *_buf){
	uint32_t num = (_buf[0]<<24) | (_buf[1] << 16) | (_buf[2] << 8) | _buf[3] ; 
	
	
	float f = ((num>>16)&0x7fff); // whole part
	f += (num&0xffff) / 65536.0f; // fraction
	if (((num>>31)&0x1) == 0x1) { f = -f; } // sign bit set
 
	return f; 
}

void decode(uint8_t *_buf, msg* new_msg){

	new_msg->code = _buf[0];
	new_msg->msgNum = (_buf[1] << 8) | (_buf[2] & 0x00ff);
	new_msg->val2 = decode32(&_buf[3]); 
	new_msg->val3 = decodeF(&_buf[7]); 

}



/*
	Writes to data received from the server to struct img_msg. 
	uint8_t *_buf - pointer to the data
	img_msg *new_msg - struct to hold data
	uint32_t _buf_size - size of the message buffer received by client, from server.
	uint32_t msg_index - current index of messge to be parsed and written to struct *new_msg, 
										 - this is the index of message that holds the entire struct msg_image,
										 - sent from server.
										 - use in case msg_image is separated into multiple packets

	
	returns updated new_msg index of buf
*/

uint32_t decode_img_msg(uint8_t * _buf, msg_img* new_msg, uint32_t _buf_size, uint32_t msg_index){
	static int cnt=1; 
		printf("Debug 1\n"); 
		printf("msg_index: %i\n", msg_index);

	// parse initial values of data to struct
	if( msg_index < 17){
 
		new_msg->code = _buf[0]; 
		new_msg->total_size = decode32(&(_buf[1])); 
		new_msg->msgNum = decode32(&(_buf[5])); 
		new_msg->img_dim = decode32(&(_buf[9])); 
		new_msg->img_size = decode32(&(_buf[13])); 
		
		//copy img from buf to new_msg.img
		 printf("\tDebug: img_size %i \n", new_msg->img_size);
		printf("\tDebug: total_size %i\tcnt: %i\n", new_msg->total_size, cnt++);
		
		int j = 0; 
		int i;

		for(i=17 ; i< _buf_size; i+=4){
			new_msg->img[j++] = (_buf[i]<<24) |(_buf[i+1]<<16) |(_buf[i+2]<<8) | (_buf[i+3]) ;  
		}

		msg_index = _buf_size;  

	}
	else{
	// copy image
		int j = msg_index - 17 + 1;
		int i; 
		for(i=0 ; i < _buf_size; i+=4){
			new_msg->img[j++] = (_buf[i]<<24) |(_buf[i+1]<<16) |(_buf[i+2]<<8) | (_buf[i+3]) ;  
		} 
		
	}
	return _buf_size; 
}


/******************************************************************************
						Tandem Virtual Machine Simulator

								Sung Chul Lee


* Debug information should be generated when compiling .sysj file

* Exception handler source: xhandler.asm

* Usage:
	1. Copy directories under the assembler directory
	2. Compile all the files
	3. Create header using: javah -jni packagename.classname
	4. Correctly replace the packagename and classname inside m2.c file
	5. Build using: compile2
	6. Run as: java -Djava.library.path=. packagename.classname
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <jni.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "simulate.h"
#include "socket_io.h"
#include "control.h"
#include "data.h"
#include <pthread.h>


#define SRAM_SIZE 				0x40000
#define BANK_SIZE				0x10000

#define PROG_BASE				0x10000

#define EXCEPTION_DATA_BASE		0x20000
#define EXCEPTION_HNDL_BASE 	0x1FF00

#define HLT_INST				0x3900
#define BKP_INST				0x2D00
#define ESL_INST				0x2F00

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

static unsigned int g_memory[SRAM_SIZE];

static const unsigned short EXCEPTION_HANDLER[]={
	0x8200,0x0000,0x8201,0x0001,0x8202,0x0002,0x8203,0x0003,0x8204,0x0004,
	0x8205,0x0005,0x8206,0x0006,0x8207,0x0007,0x8208,0x0008,0x8209,0x0009,
	0x820A,0x000A,0x820B,0x000B,0x820C,0x000C,0x820D,0x000D,0x820E,0x000E,
	0x820F,0x000F,0xAA00,0x0010,0xAB00,0x0011,0xB000,0x0012,0xB100,0x0013,
	0x3900,0x80D0,0x0014,0x80C0,0x0015,0x1B00
};

unsigned int build_uint(char* s);
unsigned int build_instruction(INSTRUCTION *p, unsigned int *inst);

void transmit_8(unsigned char data, int sock);
void transmit_16(unsigned short data, int sock);

void *thread_func(void *);
JNIEnv *attach_to_current_thread();

void simulate(INSTRUCTION *p, JNIEnv *env, jobject obj) {

	// init memory //
	for(int i=0; i < SRAM_SIZE; i++)
		g_memory[i]=0;

	// upload program into memory //
	unsigned int addr=PROG_BASE,instruction=0,dword=0;
	while(p) {
		dword=build_instruction(p, &instruction);

		g_memory[addr]=(instruction >> 16);
		addr++;

		if(dword) {
			g_memory[addr]=(instruction & 0xFFFF);
			addr++;
		}
		p=p->next;
	}

	// upload exception handler //
	for(int i=0; i < sizeof(EXCEPTION_HANDLER); i++)
		g_memory[EXCEPTION_HNDL_BASE + i]=EXCEPTION_HANDLER[i];

	// init exception data memory //
	for(int i=0; i < BANK_SIZE; i++)
		g_memory[EXCEPTION_DATA_BASE + i]=0x0;

	int pause_at_eot=1;
	g_memory[EXCEPTION_DATA_BASE + 0x14]=0xFF;
	g_memory[EXCEPTION_DATA_BASE + 0x15]=0xFFFF;

	// setup communication //
	char buffer[320];
	unsigned char length=0;
	int serversock=0,clientsock=-1,port=9100;

	while(serversock==0)
		serversock=init_server(port++);
	port--;

	// create instance of control and data processors //
	ControlProcessor recop(g_memory);
	DataProcessor nios(obj);
	DataProcessor nios2(obj);

	// debugger,environment signals //
	struct {
		unsigned int *ready;
		unsigned int debug,start_edge,mem_control;
		unsigned int er,sip[2];
	} signal;
	
	memset(&signal,0,sizeof(signal));

	nios.signal.eot 		 = &recop.signal.eot;
	nios.signal.dpc 		 = &recop.signal.dpc_0;
	nios.signal.dpcr 		 = &recop.signal.dpcr_0;
	nios.signal.ccd 		 = &recop.signal.ccd;
	nios.signal.clr_irq 	 = &recop.signal.clr_irq_0;

	nios2.signal.eot 		 = &recop.signal.eot;
	nios2.signal.dpc 		 = &recop.signal.dpc_1;
	nios2.signal.dpcr 		 = &recop.signal.dpcr_1;
	nios2.signal.ccd 		 = &recop.signal.ccd;
	nios2.signal.clr_irq 	 = &recop.signal.clr_irq_1;

	recop.signal.dprr_0		 = &nios.signal.dprr;
	recop.signal.dprr_1		 = &nios2.signal.dprr;
	recop.signal.debug		 = &signal.debug;
	recop.signal.start_edge	 = &signal.start_edge;
	recop.signal.mem_control = &signal.mem_control;
	recop.signal.er			 = &signal.er;
	recop.signal.sip[0]		 = &signal.sip[0];
	recop.signal.sip[1]		 = &signal.sip[1];

	signal.ready 			 = &recop.signal.ready;
	signal.debug			 = 1;

	pthread_t thread1, thread2;
  if (pthread_create(&thread1, NULL, thread_func, (void *)&nios) != 0)
  {
		printf("could not create thread\n");
    return;
  }
  if (pthread_create(&thread2, NULL, thread_func, (void *)&nios2) != 0)
  {
		printf("could not create thread\n");
    return;
  }

	// signal.start_edge=1;

	while(1)
	{
		fflush(stdout);

		if(clientsock == -1)
			clientsock=accept_connection(serversock);

		if(clientsock != -1)
			clientsock=handle_client(clientsock,buffer,&length);

		if(length)
		{
			buffer[length]='\0';
			length=0;

			unsigned int arg0,arg1;
			arg0=build_uint(&buffer[1]);
			arg1=build_uint(&buffer[5]);

			if(buffer[0] == 'e')
				signal.er=(arg0 & 1);

			if(buffer[0] == 'i')
				signal.sip[arg0%2]=(arg1 & 0xFFFF);

			if(buffer[0] == 'z')
				signal.debug=(arg0 & 1);

			if(*signal.ready) {
				if(buffer[0] == 'm') {
					if( (arg0 & 0x00FF) & 0x4 )
						 pause_at_eot=1;
					else pause_at_eot=0;

					g_memory[EXCEPTION_DATA_BASE + 0x14]=( (arg0 & 0x00FF) | 0x008C );
				}

				if(buffer[0] == 't')
					g_memory[EXCEPTION_DATA_BASE + 0x15]=(arg0 & 0xFFFF);

				if(buffer[0] == 'w')
					g_memory[arg0]=(arg1 & 0xFFFF);

				if(buffer[0] == 'b') {
					unsigned char temp;
					if(	(ESL_INST==g_memory[arg0]) || (BKP_INST==g_memory[arg0]) ) {
						g_memory[arg0]=BKP_INST;
						transmit_8('o', clientsock);
					}
					else transmit_8('x', clientsock);
				}

				if(buffer[0] == 'c') {
					unsigned char temp;
					if(	(ESL_INST==g_memory[arg0]) || (BKP_INST==g_memory[arg0]) ) {
						g_memory[arg0]=ESL_INST;
						transmit_8('o', clientsock);
					}
					else transmit_8('x', clientsock);
				}

				if(buffer[0] == 'd') {
					for(int i=arg0; i < (arg0 + arg1); i++)
						transmit_16( g_memory[i], clientsock );
				}

				if(buffer[0] == 'p')
					transmit_8('o',clientsock);

				if(buffer[0] == 'D') {
					for(int i=0; i < BANK_SIZE; i++)
						transmit_16( g_memory[i], clientsock );
				}
				if(buffer[0] == 'r') {
					/* Clear reactions at EOT */
					if(g_memory[EXCEPTION_DATA_BASE + 0x13] & 0x400) {
						for(int i=0; i < 256; i++)
							g_memory[EXCEPTION_DATA_BASE + 0x18 + i]=0;
					}
					signal.start_edge=1;
				}
			}
			else {
				if(buffer[0] == 's')
					signal.start_edge=-1;
			}
		}
		int prev_signal_ready=*signal.ready;
		if(rand()%100 < 100)
			recop.Run();

		signal.start_edge=0;

		// rising edge of ready signal (isr).
		if(!prev_signal_ready && (*signal.ready)) {
			unsigned int cause=g_memory[EXCEPTION_DATA_BASE + 0x13] >> 8;

			/* Clear TPC */
			if(cause & 0x80)
				g_memory[EXCEPTION_DATA_BASE + 0x15]=0xFFFF;

			/* If EOT was the only cause and if pausing at EOT is not enabled */
			if(!pause_at_eot && (cause == 0x4)) {
				/* Clear reactions at EOT */
				for(int i=0; i < 256; i++)
					g_memory[EXCEPTION_DATA_BASE + 0x18 + i]=0;

				/* Auto resume */
				signal.start_edge=1;
			}
			else
				transmit_8('o',clientsock);
		}
	}
	shut_server(serversock, clientsock);
}

void *thread_func(void *ptr)
{
	DataProcessor *nios = (DataProcessor *)ptr;
	JNIEnv *env = attach_to_current_thread();
	nios->setJNIEnv(env);
	printf("in thread_func()\n");
	while(1)
	{
		nios->Run();
	}
	return NULL;
}

JNIEnv *attach_to_current_thread()
{
	jsize nVMs;
	JavaVM *javaVM;
	if (JNI_GetCreatedJavaVMs(&javaVM, 1, &nVMs) < 0) {
		printf("error getting created jvms\n");
	}
	JNIEnv *currentEnv;
	if (javaVM->AttachCurrentThread((void **) &currentEnv, NULL) != 0) {
		printf("error attaching current thread\n");
	}
	return currentEnv;
}

unsigned int build_uint(char* s)
{
	unsigned int i=(unsigned char)s[0];
	i=(i<<8) + (unsigned char)s[1];
	i=(i<<8) + (unsigned char)s[2];
	i=(i<<8) + (unsigned char)s[3];
	return i;
}

unsigned int build_instruction(INSTRUCTION *p, unsigned int *inst) {
	unsigned int dword=0;

	if( (p->addrMode=='I') && (p->opcode==0xbc000000) )
		(*inst)=0x2f000000;
	else {
		switch(p->addrMode) {
			case 'I': (*inst)=0x4000; dword=1; break;
			case 'D': (*inst)=0x8000; dword=1; break;
			case 'R': (*inst)=0xc000; dword=0; break;
			case 'S': (*inst)=0x0000; dword=0; break;
			default	: printf("conversion error: invalid instruction\n");
					  exit(1);
					  break;
		}
		(*inst) = (*inst) + (p->opcode >> 18) + (p->zReg << 4) + p->xReg;
		(*inst) = ((*inst) << 16) + p->operand;
	}
	return dword;
}

void transmit_8(unsigned char data, int sock) {
	send(sock, &data, 1, 0);
}

void transmit_16(unsigned short data, int sock) {
	transmit_8(data >> 8, sock);
	transmit_8(data & 0xFF, sock);
}


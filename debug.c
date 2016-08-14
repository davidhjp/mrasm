#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

/*
	uint_vector v1,v2;
	v1=42345;
	v2=204;
	printf("v1 (%06d): ", (unsigned int)v1);
	print_binary(v1);
	printf("v2 (%06d): ", (unsigned int)v2);
	print_binary(v2);

	printf("v1 greater? %d\n", v1 > v2);

	uint_vector v3;
	v3=v1+v2;
	printf("v3 (%06d): ", (unsigned int)v3);
	print_binary(v3);

	uint_vector v4;
	// copies all data.
	v4=v1;
	v4(19,16)=v2(3,0);
	printf("v4 (%06d): ", (unsigned int)v4);
	print_binary(v4);

	printf("v4 (8 to 1): ");
	print_binary(v4(8,1));

	printf("v1 (%06d): ", (unsigned int)v1);
	print_binary(v1);

	uint abc=42307;
	print_binary(abc);
	print_binary(uint_vector(abc)(10,8));

	uint_vector v5;
	v5(21,5) = ( v1(15,8), v2(7,0) );
	print_binary(v5);
	print_binary(abc & v5);
	exit(1);*/
/*
	int ival=342234239;
	print_binary(ival);

	char buf[8];
	buf[0]=(ival & 0xFF000000) >> 24;
	buf[1]=(ival & 0x00FF0000) >> 16;
	buf[2]=(ival & 0x0000FF00) >> 8;
	buf[3]=(ival & 0x000000FF);
	printf("%08d,", pseudo_binary(buf[0]));	
	printf("%08d,", pseudo_binary(buf[1]));
	printf("%08d,", pseudo_binary(buf[2]));
	printf("%08d\n", pseudo_binary(buf[3]));
	ival=buf[0];
	ival=(ival << 8) + buf[1];
	ival=(ival << 8) + buf[2];
	ival=(ival << 8) + buf[3];
	print_binary(ival);

					unsigned int pc=g_memory[E_DATA_BASE+16];
					printf("%05x: 0x%04x\n", pc, (unsigned int)g_memory[PRG_START+pc]);
			
					unsigned int base=E_DATA_BASE;
					printf("> status (0x%08x)\n", target_addr);
					for(int k=0; k < 3; k++) {
						printf("    ");
						for(int i=0; i < 8; i++) {
							printf("%04x ",(unsigned int)g_memory[base]);
							base++;
						}
						printf("\n");
					}

	unsigned int i=0x10000,instruction,dword;
	unsigned short tst[256]={ 
		0x3400,				// noop 1
		0x4100, 0xcc20,		// init 3
		0x4020, 0x4321,		// ldr 5
		0x4030, 0xcc10,		// ldr 7
		0x2D00, 			// bkp 8
		0xe802, 			// senddata 9
		0x4090, 0xcc20,		// ldr ffmr 11
		0x40a0, 0xcc30,		// ldr flmr 13
		0x4040, 0xcc21,		// ldr 15
		0x5800, 0x0000,		// jump 17
		0x4230, 0x1234,
		0x4030, 0x0003,
		0x5800, 0x0017,
		0x7831, 0x0001,
		0x4000, 0x000e,
		0x43b1, 0x0005,
		0x43b1, 0x0004,
		0x3400, 
		0x4090, 0xee00,
		0x40a0, 0xeeff,
		0x4100, 0xee00,
		0x4000, 0x3021,
		0xe400, 0x3400,
		0x7844, 0x0001,
		0x4010, 0x0003,
		0x4030, 0x1000
	};
	int tst_size = 37;
	for(int j=0; j < tst_size; j++) {
		g_memory[i]=tst[j];
		i++;
	}*/
/*
#define c_msk_lpc	128
#define c_msk_asm	64
#define c_msk_dpc 	32
#define c_msk_stp	16
#define c_msk_eot 	8
#define c_msk_bkp 	4
#define c_msk_esl 	2

#define c_max_bkp 	256

static int g_exception=1,g_cause=0,g_mask=0,g_lpc=-1,g_epc=0,g_pc=0;
static int g_bkp_list[c_max_bkp];

static int *g_dm_ptr=0;
static int g_dm_size=0;

static void remove_bkp(int pc)
{
	int i;
	for(i=0; i < c_max_bkp; i++)
	{
		if(g_bkp_list[i]==pc)
			g_bkp_list[i]=-1;
	}
}

static void insert_bkp(int pc)
{
	int i;
	for(i=0; i < c_max_bkp; i++)
	{
		if(g_bkp_list[i]==-1)
		{
			g_bkp_list[i]=pc;
			break;
		}
	}
}

static void send_memory(int sock)
{
	int i;
	for(i=0; i < g_dm_size; i++)
		send(sock,&g_dm_ptr[i],sizeof(int),0);
}

static void send_status(int sock)
{
	int i,j;
	unsigned char buffer[512];

	sprintf(buffer,"cause=0x%02x,mask=0x%02x\n", g_cause,g_mask);
	send(sock, buffer, strlen(buffer), 0);

	sprintf(buffer,"lpc=%05d,epc=%05d,pc=%05d,e=%d\n", g_lpc,g_epc,g_pc,g_exception);
	send(sock, buffer, strlen(buffer), 0);

	sprintf(buffer,"breakpoints=");
	send(sock, buffer, strlen(buffer), 0);
	for(i=0,j=0; i < c_max_bkp; i++)
	{
		if(g_bkp_list[i] > 0)
		{
			sprintf(buffer,"%05d ", g_bkp_list[i]);
			send(sock, buffer, strlen(buffer), 0);
			j++;
		}
	}
	sprintf(buffer,"(%d/%d)\n",j,c_max_bkp);
	send(sock, buffer, strlen(buffer), 0);
}

int program_counter(int pc)
{
	if(pc >= 0)
		g_pc=pc;
	return g_pc;
}

int is_exception()
{
	if(g_cause & g_mask)
	{
		g_exception=1;
		g_epc=g_pc;
	}
	return g_exception;
}

void set_cause(int n)
{
	int i;
	if(n == 7)
	{
		if(g_lpc==g_pc)
			g_cause|=c_msk_lpc;
	}
	if(n == 6)
		g_cause|=c_msk_asm;
	if(n == 5)
		g_cause|=c_msk_dpc;
	if(n == 4)
		g_cause|=c_msk_stp;
	if(n == 3)
		g_cause|=c_msk_eot;
	if(n == 2)
	{
		g_cause|=c_msk_bkp;
		g_cause|=c_msk_esl;
	}
	if(n == 1)
	{
		for(i=0; i < c_max_bkp; i++)
		{
			if(g_bkp_list[i]==g_pc)
				g_cause|=c_msk_bkp;
		}
		g_cause|=c_msk_esl;
	}
}

void init_debug(int* dm_ptr, int dm_size)
{
	int i;
	for(i=0; i < c_max_bkp; i++)
		g_bkp_list[i] = -1;

	g_dm_ptr=dm_ptr;
	g_dm_size=dm_size;
}

int run_debug(int sock, unsigned char* msg, unsigned char length)
{
	msg[length]='\0';

	if( msg[0] == 'g' ) // go
	{
		g_exception=0;
		g_cause=0;
	}
	if( msg[0] == 'x' ) // stop
		set_cause(4);

	if( msg[0] == 'a' ) // insert break point
		insert_bkp(atoi(&msg[1]));

	if( msg[0] == 'b' ) // remove break point
		remove_bkp(atoi(&msg[1]));

	if( msg[0] == 'm' ) // set mask
		g_mask=atoi(&msg[1]);

	if( msg[0] == 'l' ) // set lpc
		g_lpc=atoi(&msg[1]);

	if( msg[0] == 's' ) // send status
		send_status(sock);

	if( msg[0] == 'd' ) // dump data memory
		send_memory(sock);

	return 0;
}*/

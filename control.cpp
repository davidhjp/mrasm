#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "control.h"

ControlProcessor::ControlProcessor(unsigned int *external_memory) {
	next_state=Ini;

	memset(&signal,0,sizeof(signal));
	memset(&reg,0,sizeof(reg));
	memset(&flag,0,sizeof(flag));

	memory.instance=external_memory;
	memory.ext.e=&flag.e;
}

void ControlProcessor::Run() {

	CaptureSignals();

	Always();

	if( PulseDistributor() )
		OperationDecoder();

	WriteSignals();
}

void ControlProcessor::Always() {

	if((int)(*signal.start_edge) > 0)
		reg.start=1;

	if((int)(*signal.start_edge) < 0)
		reg.stop=1;

	reg.debug=(*signal.debug);

	if(reg.stop)
		SetExceptionCause(STP_X);

	if(reg.pc == reg.file[TPC])
		SetExceptionCause(TPC_X);

	if( (flag.e==0) && (reg.eot==0) )
		reg.cyc++;
}

bool ControlProcessor::PulseDistributor() {

	if( ((*signal.mem_control) == 1) && ((state == T0) || (state == E0)) )
		return 0;

	state=next_state;
	return 1;
}

void ControlProcessor::OperationDecoder() {
	unsigned int i,
		am		= pseudo_binary( bits(reg.ir,31,30) ),
		opcode	= pseudo_binary( bits(reg.ir,29,24) ),
		rz		= bits(reg.ir,23,20),
		rx		= bits(reg.ir,19,16),
		operand = bits(reg.ir,15,0),
		ccd		= bits(reg.file[CCD],3,0) == binary(0010),
		pcd		= bits(reg.file[PCD],3,0) == binary(0010),
		irq_0		= bits(reg.dprr_0,1,1),
		irq_1		= bits(reg.dprr_1,1,1);

	switch(state) {
		case Ini:
			next_state		 = E0;
			reg.file[TPC]	 = 0xFFFF;
			reg.file[MODE]	 = 0xFF;
	//		reg.ld_dprr_done = 1;
			reg.cyc			 = 0;
			break;

		case TestA:
			if(reg.start) {
				reg.start	= 0;
				reg.stop	= 0;
				reg.ready	= 0;
				next_state	= E0;
			}
			else next_state	= TestB;
			break;

		case TestB:
			if(reg.start) {
				reg.start	= 0;
				reg.stop	= 0;
				reg.ready	= 0;
				next_state	= E0;
			}
			else next_state = TestA;
			break;

		case E0:
			if(flag.e)
				next_state = T0;
			else if( (reg.debug==0) || (flag.cause==0) ) {
				if(reg.dpc_0 && irq_0) {
					/*memory.sel	 = 0;
					memory.addr	 = reg.file[HP];
					memory.we 	 = 0;*/
					reg.file[15] = reg.res_0;//bits(memory.out,7,0);
					next_state	 = E1;
				}
				else if( (reg.dpc_0==0) && (irq_0==0) && (reg.file[HP] != reg.file[TP])) {
				//	if(reg.ld_dprr_done)
						reg.clr_irq_0 = 0;

				//	flag.ztmp  = (reg.file[HP] == reg.file[TP]);
					next_state = E2;
				}
				else {
					if(/*reg.ld_dprr_done &&*/ (irq_0==0) )
						reg.clr_irq_0 = 0;

					if(reg.dpc_1 && irq_1) {
						/*memory.sel	 = 0;
						memory.addr	 = reg.file[HP];
						memory.we 	 = 0;*/
						reg.file[15] = reg.res_1;//bits(memory.out,7,0);
						next_state	 = E1_1;
					}
					else if( (reg.dpc_1==0) && (irq_1==0) && (reg.file[HP] != reg.file[TP])) {
					//	if(reg.ld_dprr_done)
							reg.clr_irq_1 = 0;

//						flag.ztmp  = (reg.file[HP] == reg.file[TP]);
						next_state = E2_1;
					}
					else {
						if(/*reg.ld_dprr_done &&*/ (irq_1==0) )
							reg.clr_irq_1 = 0;
						next_state = T0;
					}
				}
			}
			else {
				flag.e		= 1;
				reg.epc		= reg.pc;
				reg.pc		= 0xFF00;
				next_state	= T0;
			}
			break;

		case E1:
			memory.sel	= 0;
			memory.addr	= reg.file[15];
			memory.we	= 0;
			memory.in	= (bits(memory.out,15,2) << 2) + bits(reg.dprr_0,1,0);
			memory.we	= 1;
			flag.ztmp	= (reg.file[HP] == reg.file[FLMR]);
			next_state	= E1bis;
			break;

		case E1_1:
			memory.sel	= 0;
			memory.addr	= reg.file[15];
			memory.we	= 0;
			memory.in	= (bits(memory.out,15,2) << 2) + bits(reg.dprr_1,1,0);
			memory.we	= 1;
			flag.ztmp	= (reg.file[HP] == reg.file[FLMR]);
			next_state	= E1bis_1;
			break;

		case E1bis:
			reg.clr_irq_0		 = 1;
//			reg.ld_dprr_done = 0;
			reg.dpc_0 		 = 0;
			/*if(flag.ztmp)
				 reg.file[HP]	= reg.file[FFMR];
			else {
				 reg.file[HP]	= bits(reg.file[HP] + 1,15,0);
				 flag.ztmp		= (reg.file[HP] == 0);
			}*/
			next_state = E0;

			SetExceptionCause(IRQ_X);
			break;

		case E1bis_1:
			reg.clr_irq_1		 = 1;
//			reg.ld_dprr_done = 0;
			reg.dpc_1 		 = 0;
			/*if(flag.ztmp)
				 reg.file[HP]	= reg.file[FFMR];
			else {
				 reg.file[HP]	= bits(reg.file[HP] + 1,15,0);
				 flag.ztmp		= (reg.file[HP] == 0);
			}*/
			next_state = E0;

			SetExceptionCause(IRQ_X);
			break;

		case E2:
			/*if(flag.ztmp)
				next_state = T0;
			else {*/
	
				memory.sel 	= 0;
				memory.addr = reg.file[HP];
				memory.we 	= 0;
				reg.dpcr_0 	= bits(memory.out,15,8);
				reg.res_0		= bits(memory.out,7,0);
				reg.dpc_0		= 1;
				reg.dprr_0	= (*signal.dprr_0);
			//	reg.ld_dprr_done = 1;
				next_state		 = E0;

				printf("res_0:0x%x, case:%d\n", reg.res_0, reg.dpcr_0);

				SetExceptionCause(DPC_X);
		
				if(reg.file[HP]	== reg.file[FLMR])
					 reg.file[HP]	= reg.file[FFMR];
				else {
					 reg.file[HP]	= bits(reg.file[HP] + 1,15,0);
	//				 flag.ztmp		= (reg.file[HP] == 0);
				}
			//}
			break;

		case E2_1:
			/*if(flag.ztmp)
				next_state = T0;
			else {*/

				memory.sel 	= 0;
				memory.addr = reg.file[HP];
				memory.we 	= 0;
				reg.dpcr_1 	= bits(memory.out,15,8);
				reg.res_1		= bits(memory.out,7,0);
				reg.dpc_1		= 1;
				reg.dprr_1	= (*signal.dprr_1);
			//	reg.ld_dprr_done = 1;
				next_state		 = E0;

				printf("res_1:0x%x, case:%d\n", reg.res_1, reg.dpcr_1);

				SetExceptionCause(DPC_X);

				if(reg.file[HP]	== reg.file[FLMR])
					 reg.file[HP]	= reg.file[FFMR];
				else {
					 reg.file[HP]	= bits(reg.file[HP] + 1,15,0);
	//				 flag.ztmp		= (reg.file[HP] == 0);
				}
			//}
			break;

		case T0:
			memory.sel	= 1;
			memory.addr	= reg.pc;
			memory.we 	= 0;
			reg.ir		= memory.out << 16;
			reg.pc 		= bits(reg.pc + 1,15,0);
			next_state 	= T1;

			SetExceptionCause(IRF_X);
			break;

		case T1:
			next_state = T2;
			if( (am == IMMEDIATE) || (am == DIRECT) ) {
				memory.sel	= 1;
				memory.addr	= reg.pc;
				memory.we	= 0;
				reg.ir		= reg.ir + bits(memory.out,15,0);
				reg.pc		= bits(reg.pc + 1,15,0);
			}
			else if(opcode == SWITCH) {
				memory.sel	 = 0;
				memory.addr	 = reg.file[rx];
				memory.we	 = 0;
				reg.file[rz] = memory.out;
			}
			else if(opcode == LER) {
				reg.er = (*signal.er);
			}
			else if(opcode == LSIP) {
				reg.sip[ccd] = *(signal.sip[ccd]);
			}
			break;

		case T2:
//			printf("0x%05x: opcode=%06d rz=%02d rx=%02d, operand=%05d, z=%d\n", reg.pc, opcode, rz,rx, operand, flag.z);
			if( (opcode == INIT) || (opcode == SWITCH) )
				next_state = T3;
			else {
				if( flag.e && (opcode == HLT) ) {
					reg.start	= 0;
					reg.stop	= 0;
					reg.ready	= 1;
					next_state	= TestA;
				}
				else next_state = E0;

				reg.dprr_0 		 = (*signal.dprr_0);
				reg.dprr_1 		 = (*signal.dprr_1);
				//reg.ld_dprr_done = 1;
			}

			if( am == DIRECT ) {
				switch(opcode) {
					case LDR:
						memory.sel 	 = 0;
						memory.addr  = operand;
						memory.we 	 = 0;
						reg.file[rz] = memory.out;
						break;

					case STR:
						memory.sel 	= 0;
						memory.addr = operand;
						memory.in	= reg.file[rx];
						memory.we 	= 1;
						break;

					case STCH:
						memory.sel	= 0;
						memory.addr	= operand;
						memory.in	= bits(reg.cyc,31,16);
						memory.we	= 1;
						break;

					case STCL:
						memory.sel	= 0;
						memory.addr	= operand;
						memory.in	= bits(reg.cyc,15,0);
						memory.we	= 1;
						break;

					case STE:
						memory.sel 	= 0;
						memory.addr = operand;
						memory.in	= reg.epc;
						memory.we 	= 1;
						break;

					case STF:
						memory.sel	= 0;
						memory.addr	= operand;
						memory.in	=
							(flag.cause << 8) +
							(bits(reg.dprr_0,1,1) << 3) +
							(flag.e << 2) +
							(flag.z << 1) +
							(flag.ztmp);
						memory.we	= 1;
						break;

					default:break;
				}
			}
			else if( am == IMMEDIATE ) {
				switch(opcode) {

					case LDR:
						reg.file[rz] = operand;
						break;

					case INIT:
						reg.file[HP] = operand;
						break;

					case STR:
						memory.sel 	= 0;
						memory.addr = reg.file[rz];
						memory.in	= operand;
						memory.we 	= 1;
						break;

					case JMP:
						reg.pc = operand;
						break;

					case PRESENT:
						if( 0==bits(reg.file[rz],15,0) )
							reg.pc = operand;
						break;

					case SZ:
						if(flag.z)
							reg.pc = operand;
						break;

					case ADDR:
						reg.file[rz] = bits(reg.file[rx] + operand,15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case SUBVR:
						reg.file[rz] = bits(reg.file[rx] - operand,15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case SUBR:
						flag.z 		 = (bits(reg.file[rz] - operand,15,0) == 0);
						break;

					case ANDR:
						reg.file[rz] = bits(reg.file[rx] & operand,15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case ORR:
						reg.file[rz] = bits(reg.file[rx] | operand,15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					default:break;
				}
			}
			else if( am == INDIRECT ) {
				switch(opcode) {
					case SWITCH:
						reg.file[rz] = bits(reg.file[rz] + reg.file[rx] + 1,15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case SENDATA:
						memory.sel 	= 0;
						memory.addr	= reg.file[TP];
						memory.in	= reg.file[rx];
						memory.we	= 1;
						break;

					case JMP:
						reg.pc = reg.file[rx];
						break;

					case CHKEND:
						reg.file[rz] = MaxV(
							bits(reg.file[rx],15,12),
							bits(reg.file[rx],11,8),
							bits(reg.file[rx],7,4),
							bits(reg.file[rx],3,0),
							bits(reg.file[rz],3,0));
						break;

					case LDR:
						memory.sel	 = 0;
						memory.addr	 = reg.file[rx];
						memory.we	 = 0;
						reg.file[rz] = memory.out;
						break;
					
					case STR:
						memory.sel	= 0;
						memory.addr	= reg.file[rz];
						memory.in	= reg.file[rx];
						memory.we	= 1;
						break;

					case ADDR:
						reg.file[rz] = bits(reg.file[rz] + reg.file[rx],15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case ANDR:
						reg.file[rz] = bits(reg.file[rz] & reg.file[rx],15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case ORR:
						reg.file[rz] = bits(reg.file[rz] | reg.file[rx],15,0);
						flag.z 		 = (reg.file[rz] == 0);
						break;

					case SSVOP:
						reg.svop[pcd] = reg.file[rx];
						break;

					case SSOP:
						reg.sop[pcd] = reg.file[rx];
						break;

					case LSIP:
						reg.file[rz] = reg.sip[ccd];
						break;

					case LER:
						reg.file[rz] = reg.er;
						break;

					default:break;
				}

			}
			else if( am == INHERENT ) {
				switch(opcode) {
					case CLFZ:
						flag.z = 0;
						break;

					case CER:
						reg.er = 0;
						break;

					case CEOT:
						reg.eot = 0;
						reg.cyc = 0;
						break;

					case SEOT:
						reg.eot = 1;
						SetExceptionCause(EOT_X);
						break;

					case RTE:
						reg.pc 		= reg.epc;
						flag.cause 	= 0;
						flag.e	 	= 0;
						break;

					case BKP:
						memory.ext 	= 1;
						memory.sel 	= 0;
						memory.addr	= reg.file[CURR];
						memory.in	= reg.pc;
						memory.we	= 1;
						SetExceptionCause(BKP_X);
						SetExceptionCause(ESL_X);
						break;

					case ESL:
						memory.ext 	= 1;
						memory.sel 	= 0;
						memory.addr	= reg.file[CURR];
						memory.in	= reg.pc;
						memory.we	= 1;
						SetExceptionCause(ESL_X);
						break;

					case NOOP:
						break;

					default:break;
				}
			}
			break;

		case T3:
			next_state 			= E0;
			reg.dprr_0 			= (*signal.dprr_0);
			reg.dprr_1 		 = (*signal.dprr_1);
			//reg.ld_dprr_done	= 1;

			switch(opcode) {
				case INIT:
					reg.file[TP] = operand;
					break;

				case SWITCH:
					memory.sel	= 0;
					memory.addr	= reg.file[rz];
					memory.we	= 0;
					reg.pc 		= memory.out;
					break;

				default:break;
			}
			break;

		default:break;
	}
}

extern pthread_mutex_t mutex1;
extern pthread_mutex_t mutex2;

void ControlProcessor::CaptureSignals() {
	//printf("ControlProcessor::CaptureSignals()\n");
	pthread_mutex_lock(&mutex2);
	signal.mem_control();
	signal.start_edge();
	signal.debug();
	signal.dprr_0();
	signal.dprr_1();
	signal.er();
	for(int i=0; i < CLOCK_DOMAINS; i++)
		signal.sip[i]();
	pthread_mutex_unlock(&mutex2);
}

void ControlProcessor::WriteSignals() {
	//printf("ControlProcessor::WriteSignals()\n");
	pthread_mutex_lock(&mutex1);
	signal.eot		= reg.eot;
	signal.dpc_0		= reg.dpc_0;
	signal.dpcr_0		= reg.dpcr_0;
	signal.clr_irq_0	= reg.clr_irq_0;

	signal.dpc_1		= reg.dpc_1;
	signal.dpcr_1		= reg.dpcr_1;
	signal.clr_irq_1	= reg.clr_irq_1;
	signal.ready 	= reg.ready;
	signal.ccd		= bits(reg.file[CCD],3,0);
	signal.pcd		= bits(reg.file[PCD],3,0);
	for(int i=0; i < CLOCK_DOMAINS; i++) {
		signal.sop[i]	= reg.sop[i];
		signal.svop[i]	= reg.svop[i];
	}
	pthread_mutex_unlock(&mutex1);
}

unsigned int binary(unsigned int value) {
	static const int N=8;
	static const unsigned int LUT[N]={
	 	1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000
	};
	unsigned int converted=0;
	for(int i=0; i < N; i++)
		converted += ((value/LUT[i]) & 1) << i;

	return converted;
}

unsigned int pseudo_binary(unsigned int value) {
	static const int N=8;
	static const unsigned int LUT[N]={
	 	1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000
	};
	unsigned int converted=0;
	for(int i=0; i < N; i++)
		if( (value & (1 << i)) > 0)
			converted += LUT[i];

	return converted;
}

unsigned int bits(unsigned int value, int top, int down) {
	unsigned long
		mask	= (-1),
		length	= (top-down)+1;
	return ~(mask << length) & (value >> down);
}

unsigned int bits(unsigned int value, int top, int down, bool old_version) {
	unsigned int result=0;
	for(int i=top; i >= down; i--)
		result = (result << 1) + (( value & (1 << i) ) > 0);
	return result;
}

void print_binary(unsigned int value) {
	for(int i=31,j=0; i >= 0; i--,j++) {
		if(j && !(j%8))
			 printf(",");
		if( (value & (1 << i)) > 0 )
			 printf("1");
		else printf("0");
	}
	printf("\n");
}

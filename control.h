/*
 * Control Processor Simulator, Sung Chul Lee
 */
#ifndef _ControlProcessor
#define _ControlProcessor

unsigned int binary(unsigned int value);
unsigned int pseudo_binary(unsigned int value);
unsigned int bits(unsigned int value, int top, int down);
unsigned int bits(unsigned int value, int top, int down, bool old_version);
void print_binary(unsigned int value);

class ControlProcessor {
public:
	static const int CLOCK_DOMAINS=2;

private:
	static const unsigned int
		INHERENT 	=  0,
		IMMEDIATE	=  1,
		DIRECT 		= 10,
		INDIRECT	= 11;

	static const unsigned int
		LDR		=      0,		INIT	=      1,		STR		=     10,
		STCH	= 101010,		STCL	= 101011,		STE		= 110000,
		STF		= 110001,		RTE		=  11011,		HLT		= 111001,		
		BKP		= 101101,		ESL		= 101111,		JMP		=  11000,
		PRESENT	=  11100,		ANDR	=   1000,		ORR		=   1100,
		ADDR	= 111000, 		SUBR	=    100,		SUBVR	=     11,
		CLFZ	=  10000,		CER		= 111100, 		CIRQ	= 111101,
		CEOT	= 111110,		SEOT	= 111111,		NOOP	= 110100,
		SZ		=  10100,		LER		= 110110,		SENDATA	= 101000,
		CHKEND	= 101100, 		SWITCH	= 100000,		SSVOP	= 111011,
		SSOP	= 111010,	 	LSIP	= 110111;

	static const unsigned int
		TPC_X	= 10000000,
		IRF_X	=  1000000,
		DPC_X 	=   100000,
		IRQ_X 	=    10000,
		STP_X	=     1000,
		EOT_X	=      100,
		BKP_X 	=       10,
		ESL_X 	=        1;

	static const unsigned int
		TP	 	= 4,
		HP	 	= 5,
		CCD	 	= 7,
		PCD	 	= 8,
		FFMR 	= 9,
		FLMR 	= 10,
		TPC		= 12,
		MODE	= 13,
		CURR	= 14;

	enum ProcessorState {
		Ini,
		TestA,
		TestB,
		E0,
		E1,E1_1,
		E1bis,E1bis_1,
		E2,E2_1,
		T0,
		T1,
		T2,
		T3
	} state, next_state;

public:
	struct ExternalSignal {
		unsigned int
			eot,
			dpc_0,
			dpcr_0,
			clr_irq_0,
			dpc_1,
			dpcr_1,
			clr_irq_1,
			ccd,
			pcd,
			ready,
			sop[CLOCK_DOMAINS],
			svop[CLOCK_DOMAINS];

		struct BufferedSignal {
			unsigned int value,*ptr;
			
			unsigned int operator*(void) {
				return value;
			}
			void operator()(void) {
				value=(*ptr);
			}
			void operator=(unsigned int *ptr) {
				this->ptr=ptr;
			}
		} 	mem_control,
			dprr_0,
			dprr_1,
			er,
			debug,
			start_edge,
			sip[CLOCK_DOMAINS];
	} signal;

private:
	struct MemoryInterface {
		unsigned int
			*instance,
			in,
			out,
			addr,
			sel;

		struct AddressExtension {
			unsigned int *e,*sel,enable;
			unsigned int generate() {
				if(enable) {
					enable=0;
					return 1;
				}
				return (*e) & bits(~(*sel),0,0);
			}
			void operator=(unsigned int enable) {
				this->enable=enable;
			}
		} ext;

		struct LiveControl {
			MemoryInterface *interface;
			unsigned int value;

			void operator=(unsigned int value) {
				this->value=value;
				interface->Update();
			}
		} we;

		MemoryInterface() {
			ext.enable=0;
			ext.sel=&sel;
			we.interface=this;
		}

		void Update() {
			unsigned int address=(bits(sel,0,0)<<16) + bits(addr,15,0);
			address |= ext.generate() << 17;
			if(we.value)
				instance[address]=in;
			out=instance[address];
		}
	} memory;

	struct InternalRegister {
		static const int FILE_SIZE=16;

		unsigned int
			ir,				pc,				eot,			er,
			dpc_0,		dpcr_0,		dprr_0,			clr_irq_0,
			dpc_1,		dpcr_1,		dprr_1,			clr_irq_1,
			epc,			debug,			start,			stop,			
			cyc,			ready, res_0, res_1;//,			ld_dprr_done;

		unsigned int
			file[FILE_SIZE],
			svop[CLOCK_DOMAINS],
			sip[CLOCK_DOMAINS],
			sop[CLOCK_DOMAINS];
	} reg;

	struct InternalFlag {
		unsigned int
			z,
			ztmp,
			e,
			cause;
	} flag;

public:
	ControlProcessor(unsigned int *external_memory);
	void Run();

private:
	void Always();

	bool PulseDistributor();
	void OperationDecoder();

	void CaptureSignals();
	void WriteSignals();

	void SetExceptionCause(unsigned int c) {
		if( (flag.e==0) && (reg.file[MODE] & binary(c)) )
			flag.cause |= binary(c);
	}

	unsigned int MaxV(
		unsigned int a0,
		unsigned int a1,
		unsigned int a2,
		unsigned int a3,
		unsigned int a4) {
		unsigned int result=0;
		if(a0 > result) result=a0;
		if(a1 > result) result=a1;
		if(a2 > result) result=a2;
		if(a3 > result) result=a3;
		if(a4 > result) result=a4;
		return result;
	}
};

#endif

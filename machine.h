// typedef enum addressingModes { immediate, inherent, direct, regind, stack } ADDRMODE;

// I, H, D, R, S

typedef struct _instr INSTRUCTION;
struct _instr
{
	int instrCount;
	char addrMode;	
	unsigned long opcode;
	int operand; //This is like the value in there
	char zReg;
	char yReg;
	char xReg;
	// reactive stuff
	char aSig;
	char bSig;
	INSTRUCTION *next;
};

int prog_add(int instrCount, char addrMode, unsigned long opcode, int operand, char zReg, char yReg, char xReg, char aSig, char bSig);


typedef enum entryType { label, symbol, opcode, directive, macro } TYPE;

typedef struct _operation OPERATION;
struct _operation
{
	unsigned long opcode;
	char *format[10];
	
};

typedef struct _entry ENTRY;
struct _entry
{
	char *text;
	TYPE type;
	int value;
	OPERATION *instr;
	ENTRY *next;
};

int table_add(char *text, TYPE type, int value);
int table_mod(char *text, TYPE type, int value);
ENTRY* table_find(char *text);

int populate_table(char *inifile);
void show_table(void);

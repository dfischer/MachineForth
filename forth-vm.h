// ------------------------------------------------------------------------------------------
// The VM
// ------------------------------------------------------------------------------------------

extern CELL *RSP; // the return stack pointer
extern CELL *DSP; // the data stack pointer

extern CELL PC;		// The "program counter"
extern BYTE IR;		// The "instruction register"

// extern CELL *dsp_init;
// extern CELL *rsp_init;
extern CELL reg1, reg2, reg3;

extern BYTE the_memory[];
extern long memory_size;

extern CELL BASE, STATE;
extern CELL HERE, LAST;

// ------------------------------------------------------------------------------------------
extern void push(CELL);
extern CELL pop();
extern void rpush(CELL);
extern CELL rpop();
extern void run_program(CELL);
extern int StrLen(char *);
extern DICT_T the_words[];
extern int num_words;
extern HANDLE hStdout, hStdin;

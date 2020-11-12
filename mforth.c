#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serial.h"
#include "Shared.h"
#include "forth-vm.h"

char base_fn[32];
bool run_saved = true;
bool is_temp = false;
bool auto_run = false;

#define BUF_SZ 1024
char input_buf[BUF_SZ];
FILE *input_fp = NULL;
FILE *input_stack[16];
int input_SP = 0;
int isBye = 0;

SERIAL_T SerialPort;
#define FORTH_PORT 1
#define USER_PORT  0

extern int txToForth(char);
extern char rxFromForth();
extern char rxFromUser();
extern void txToUser_String(char *);

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
OPCODE_T opcodes[] = {
          { NOP,           "nop",           }
        , { LITERAL,       "LITERAL",       }
        , { CLITERAL,      "CLITERAL",      }
        , { FETCH,         "@",             }
        , { STORE,         "!",             }
        , { CFETCH,        "c@",            }
        , { CSTORE,        "c!",            }
        , { SWAP,          "SWAP",          }
        , { DROP,          "DROP",          }
        , { DUP,           "DUP",           }
        , { LT,            "<",             }
        , { EQ,            "=",             }
        , { GT,            ">",             }
        , { JMP,           "JMP",           }
        , { JMPZ,          "JMPZ",          }
        , { JMPNZ,         "JMPNZ",         }
        , { CALL,          "CALL",          }
        , { RET,           "RET",           }
        , { OVER,          "over",          }
        , { AND,           "AND",           }
        , { OR,            "OR",            }
        , { XOR,           "XOR",           }
        , { COM,           "COM",           }
        , { ADD,           "+",             }
        , { SUB,           "-",             }
        , { MUL,           "*",             }
        , { DIV,           "/",             }
        , { DTOR,          ">r",            }
        , { RTOD,          "r>",            }
        , { HA,            "(h)",           }
        , { BA,            "base",          }
        , { SA,            "state",         }
        , { LA,            "last",          }
        , { SLASHMOD,      "/mod",          }
        , { NOT,           "NOT",           }
        , { RFETCH,        "RFETCH",        }
        , { INC,           "1+",            }
        , { DEC,           "1-",            }
        , { GETTICK,       "GETTICK",       }
        , { SHIFTLEFT,     "2*",            }
        , { SHIFTRIGHT,    "2/",            }
        , { PLUSSTORE,     "+!",            }
        , { COMMA,         ",",             }
        , { CCOMMA,        "c,",            }
        , { IMMEDIATE,     "immediate",     }
        , { INLINE,        "inline",        }
        , { TOSRC,         ">src",          }
        , { SRC,           "src",           }
        , { TODST,         ">dst",          }
        , { DST,           "dst",           }
        , { EMIT,          "emit",          }
        , { GOTORC,        "gotorc",        }
        , { CLS,           "cls",           }
        , { GETS,          "gets",          }
        , { GETCH,         "GETCH",         }
        , { DOT,           "(.)",           }
        , { BYE,           "BYE",           }
		, { 0,             0,               }
};

// ---------------------------------------------------------------------
void StrCpy(char *dst, char *src) 
{
	while (*src)
	{
		*(dst++) = *(src++);
	}
	*dst = (char)0;
}

// ---------------------------------------------------------------------
void StrCat(char *dst, char *src) 
{
	while (*dst)
		dst++;
	StrCpy(dst, src);
}

// ---------------------------------------------------------------------
int StrLen(char *src) 
{
	int i = 0;
	while (*src)
	{
		src++;
		i++;
	}
	return i;
}

// ---------------------------------------------------------------------
char *get_word(char *word)
{
	*word = (char)0;

	// skip any leading WS
	char c = rxFromUser();
	while (c < 33)
	{
		// NULL means end of stream
		if (!c) return (char *)0;
		c = rxFromUser();
	}

	while (c > 32)
	{
		*(word++) = c;
		c = rxFromUser();
	}
	*word = (char)0;

	return word;
}

// ---------------------------------------------------------------------
DICT_T *define_word(char *word)
{
	DICT_T *e = (&the_words[++num_words]);
	e->flags = 0;
	e->XT = HERE;
	e->len = StrLen(word);
	StrCpy(e->name, word);
	return e;
}

// ---------------------------------------------------------------------
DICT_T *rfind_word(CELL xt)
{
	for (int i = num_words; i > 0; i--)
	{
		DICT_T *e = (&the_words[i]);
		if (e->XT == xt) return e;
	}
	return NULL;
}

// ---------------------------------------------------------------------
DICT_T *find_word(char *name)
{
	for (int i = num_words; i > 0; i--)
	{
		DICT_T *e = (&the_words[i]);
		if (strcmpi(e->name, name) == 0) return e;
	}
	return NULL;
}

// ---------------------------------------------------------------------
OPCODE_T *find_opcode(char *name)
{
	for (int i = 0;; i++)
	{
		OPCODE_T *e = (&opcodes[i]);
		if (e->forth_prim == NULL) return NULL;
		if (strcmpi(e->forth_prim, name) == 0) return e;
	}
	return NULL;
}

// ---------------------------------------------------------------------
bool is_number(char *word, CELL *num)
{
	int base = BASE;
	bool is_neg = false;
	CELL my_num = 0;

	if ((word[0] == '\'') && (word[2] == '\'') && (word[3] == (char)0))
	{
		*num = word[1];
		return true;
	}

	if (*word == '%') { base =  2; word++; }
	if (*word == '#') { base = 10; word++; }
	if (*word == '$') { base = 16; word++; }

	if ((*word == '-') && (base == 10))
	{
		++word;
		is_neg = true;
	}

	*num = 0;
	while (*word)
	{
		char c = (*word++);
		int i = c - '0';
		if ((i >= 0) && (i < base))
		{
			my_num = ((my_num) * base) + i;
			continue;
		}
		if (base > 10)
		{
			c = ((c >= 'a') && (c <= 'z')) ? c - 32 : c;
			i = c - 'A';
			if ((i >= 0) && (i < (base-10)))
			{
				my_num = ((my_num) * base) + (i + 10);
				continue;
			}
		}
		return false;
	}

	*num = is_neg ? -my_num : my_num;
	return true;
}

// ---------------------------------------------------------------------
void parse_word(char *word)
{
	if ((*word == '\\') && (*(word+1) == 0))
	{
		while (rxFromUser()) { }
		return;
	}

	if ((*word == '(') && (*(word+1) == 0))
	{
		while (rxFromUser() != ')') ;
		return;
	}

	if ((*word == ':') && (*(word+1) == 0))
	{
		get_word(word);
		define_word(word);
		STATE = 1;
		return;
	}

	if ((*word == ';') && (*(word+1) == 0))
	{
		// Simple tail-call optimization
		if ((BYTE_AT(HERE-5) == CALL) && rfind_word(CELL_AT(HERE-4)))
			BYTE_AT(HERE-5) = JMP;
		else
			ccomma(RET);
		
		STATE = 0;
		return;
	}

	if (strcmpi(word, "dw") == 0)
	{
		get_word(word);
		define_word(word);
		return;
	}

	if (strcmpi(word, "forget") == 0)
	{
		HERE = the_words[num_words].XT;
		num_words--;
		return;
	}

	if (strcmpi(word, "load") == 0) {
		char fn[24];
		sprintf(fn, "block-%05d.fs", pop());
		FILE *fp = fopen(fn, "rt");
		if (!fp) {
			char buf[64];
			sprintf("File '%s' not found", fn);
			txToUser_String(buf);
		} else {
			input_stack[input_SP++] = input_fp;
			input_fp = fp;
		}
		return;
	}

	if (strcmpi(word, "edit") == 0) {
		char fn[64];
		sprintf(fn, "notepad .\\block-%05d.fs", pop());
		system(fn);
		return;
	}

	if (strcmpi(word, "bye") == 0) {
		isBye = 1;
		return;
	}

	DICT_T *ep = find_word(word);
	if (ep)
	{
		if ((STATE == 0) || (ep->flags & IS_IMMEDIATE))
		{
			run_program(ep->XT);
		}
		else
		{
			ccomma(CALL);
			comma(ep->XT);
		}
		
		return;
	}

	OPCODE_T *op = find_opcode(word);
	if (op)
	{
		if (STATE == 0)
		{
			BYTE_AT(HERE + 0x0100) = op->opcode;
			BYTE_AT(HERE + 0x0101) = RET;
			run_program(HERE + 0x0100);
		}
		else
		{
			ccomma(op->opcode);
		}
		
		return;
	}

	CELL the_num;
	if (is_number(word, &the_num))
	{
		push(the_num);
		if (STATE == 1)
		{
			if ((0 <= the_num) && (the_num < 0x0100))
			{
				ccomma(CLITERAL);
				ccomma(pop());
			}
			else
			{
				ccomma(LITERAL);
				comma(pop());
			}
		}
		return;
	}

	txToUser_String(word);
	txToUser_String(" ??");
	return;
}

// ---------------------------------------------------------------------
void forthOuterInterpreter()
{
	char word[64];
	while (true)
	{
		get_word(word);
		if (word[0] == 0) return;
		parse_word(word);
		if (isBye) return;
	}
	return;
}

// ---------------------------------------------------------------------
// The REPL (Read, Execute, Print, Loop) loop ...
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
void execute(char *line)
{
	// Serial_status(&SerialPort);
	while (*line) txToForth(*(line++));
	// Serial_status(&SerialPort);

	forthOuterInterpreter();
}

// ---------------------------------------------------------------------
bool open_file(char *ext, char *mode, FILE **pfp)
{
	char fn[64];
	StrCpy(fn, base_fn);
	StrCat(fn, ext);
	FILE *fp = fopen(fn, mode);
	if (!fp)
	{
		printf("\nUnable to open '%s'", fn);
		*pfp = NULL;
		return false;
	}
	*pfp = fp;
	return true;
}

// ---------------------------------------------------------------------
bool read_binaries() 
{
	FILE *fp = NULL;
	if (!open_file(".BIN", "rb", &fp)) return false;
	fread(the_memory, MEM_SZ, 1, fp);
	fclose(fp);

	if (!open_file(".INF", "rb", &fp)) return false;
	fread(&HERE, sizeof(CELL), 1, fp);
	fread(&num_words, sizeof(num_words), 1, fp);
	int num = fread(the_words, sizeof(DICT_T), MAX_WORDS, fp);
	fclose(fp);

	return true;
}

void dump_memory(FILE *fp)
{
	fprintf(fp, "\nMemory:\n----------------------------------------------------------\n");
	BYTE *addr = the_memory;
	int jmpBy = 0x10;
	while ((CELL)addr < HERE)
	{
		fprintf(fp, "%08lX:", addr);
		for (int i = 1; i <= jmpBy; i++)
		{
			fprintf(fp, " %02lX", addr[i-1]);
			if ((i % 8) == 0)
			 	fprintf(fp, " ");
		}
		addr += jmpBy;
		fprintf(fp, "\n");
	}
}

// ---------------------------------------------------------------------
void write_output()
{
	FILE *fp = NULL;
	CELL bin_sz = 0, start = (CELL)(&the_memory[0]);

	while ((start + bin_sz) < (HERE + 0x0400) ) bin_sz += 0x0800;

	if (!open_file(".BIN", "wb", &fp)) return;
	fwrite(the_memory, 1, bin_sz, fp);
	fclose(fp);

	if (!open_file(".TXT", "wt", &fp)) return;

	fprintf(fp, "HERE: 0x%08lx, the_memory: 0x%08lx, bytes: %d\n", 
		HERE, the_memory, HERE - (CELL)the_memory);

	fprintf(fp, "\n%d Words:\n-------------------------------\n", num_words);
	for (int i = num_words; i > 0; i--)
	{
		DICT_T *e = &the_words[i];
		fprintf(fp, "%4d: %02x %08lx %d %s\n", i, e->flags, e->XT, e->len, e->name);
	}

	fprintf(fp, "\nOpcodes:\n---------------------------\n");
	for (int i = 0; ; i++)
	{
		OPCODE_T *op = &opcodes[i];
		if (op->forth_prim == NULL)
			break;
		fprintf(fp, "#%02d ($%02x): %s\n", op->opcode, op->opcode, op->forth_prim);
	}

	dump_memory(fp);

	fclose(fp);

	if (!open_file(".INF", "wb", &fp)) return;
	fwrite(&HERE, sizeof(CELL), 1, fp);
	fwrite(&num_words, sizeof(num_words), 1, fp);
	fwrite(the_words, sizeof(DICT_T), num_words+4, fp);
	fclose(fp);
}

// ---------------------------------------------------------------------
int read()
{
	if (input_fp)
	{
		if (fgets(input_buf, BUF_SZ, input_fp) == input_buf)  return false;
		fclose(input_fp);
		input_fp = NULL;
		if (input_SP > 0) {
			input_fp = input_stack[--input_SP];
		}
		StrCpy(input_buf, "");
		return 1;
	}
	gets(input_buf);
	return 0;
}

void doHist(char *line, int addTS) {
	FILE *fp = NULL;
	open_file(".log", "at", &fp);

	fprintf(fp, "%s", line);
	if (addTS) {
		time_t ct = time(NULL);
		struct tm *t = localtime(&ct);
		fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d", t->tm_year + 1900, 
				t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	}
	fprintf(fp, "\n");
	fclose(fp);
}

// ---------------------------------------------------------------------
void REPL()
{
	FILE *fp = NULL;
	open_file(".log", "at", &fp);
	doHist("\\ new session: ", 1);
	while (! isBye)
	{
		if (!input_fp) printf(" ok\n");
		read();
		if (input_buf[0] && (!input_fp)) {
			doHist(input_buf, 0);
		}
		execute(input_buf);
		char c = rxFromForth();
		while (c) {
			if (c == '\n') {
				printf("\n");
			} else {
				printf("%c", (c < 32) ?  '.' : c);
			}
			c = rxFromForth();
		}
	}
}

// ---------------------------------------------------------------------
void parse_arg(char *arg) 
{
	// -f:baseFn
	if (*arg == 'f') StrCpy(base_fn, arg+2);

	// -b (bootstrap)
	if (*arg == 'b') run_saved = false;

	// -t (is-temp)
	if (*arg == 't') is_temp = true;

	// -a (auto-run)
	if (*arg == 'a') auto_run = true;

	if (*arg == '?')
	{
		printf("usage: mforth [options]\n");
		printf("\t-f:baseFn (default: 'mforth')\n");
		printf("\t-b        (bootstrap, default: false)\n");
		printf("\t-t        (is-temp,   default: false)\n");
		printf("\t-a        (auto-run,  default: false)\n");
		printf("\nNotes ...");
		
		printf("\n\n    -f:baseFn defines the base filename for the files in the working set.");
		printf("\nThis is used by the other options.");

		printf("\n\n    -b tells mforth to start with an empty dictionary. Then mforth");
		printf("\nreads the <baseFn>.SRC file as initial input. You can define an empty");
		printf("\n<baseFn>.SRC file to drop into the REPL with no words at all. I have");
		printf("\nprovided the empty.SRC file for that purpose. Use 'mforth -f:empty -b'");

		printf("\n\n    -t tells mforth that it should not save its state when closing. By");
		printf("\ndefault, on exit (bye), mforth will create a set of files <baseFn>.[TXT|INF|BIN]");
		printf("\nthat allow mforth to remember the current dictionary the next time it is run.");

		printf("\n\n    -a tells mforth to automatically run the last word in the working set");
		printf("\ndictionary and then exit. In this case, -t is automatically set to true. One would");
		printf("\nuse this option to run a stand-alone program.");
		exit(0);
	}
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------


// ---------------------------------------------------------------------
//  ... Forth side
// ---------------------------------------------------------------------
char rxFromUser() {
	char c = Serial_read(&SerialPort, FORTH_PORT);
	// printf("(fromUser[%d])", c);
	// return Serial_read(&SerialPort, FORTH_PORT);
	return c;
}

int txToUser(char c) {
	// Serial_write(&SerialPort, c, SERIAL_LTOR);
	if (c) printf("%c", c);
}

void txToUser_String(char *str)
{
	char *cp = str;
	while (*cp) txToUser(*(cp++));
}

// ---------------------------------------------------------------------
//  ... User side
// ---------------------------------------------------------------------
int txToForth(char c) {
	// printf("(to4th[%d])", c);
	return Serial_write(&SerialPort, c, SERIAL_RTOL);
}

int txToForth_String(char *str) {
	char *cp = str;
	while (*cp) txToForth(*(cp++));
}

char rxFromForth() {
	char c = Serial_read(&SerialPort, USER_PORT);
	// printf("(from4th[%d])", c);
	return c;
	// return Serial_read(&SerialPort, USER_PORT);
}

// ---------------------------------------------------------------------
//  ... User side
// ---------------------------------------------------------------------
void testSerial() {
	RINGBUF_T rb;
	ringbuf_init(&rb);
	ringbuf_dump(&rb);
	ringbuf_in(&rb, '9');
	ringbuf_dump(&rb);
	char c1 = ringbuf_out(&rb);
	printf("%s: c=%d\n", (c1 == '9') ? "ok" : "fail", c1);
	ringbuf_dump(&rb);

	Serial_status(&SerialPort);
	txToForth_String("123");
	Serial_status(&SerialPort);
	txToUser_String("456");
	Serial_status(&SerialPort);

	printf("Test rxFromUser() ...");
	char c = rxFromUser();
	while (c) {
		printf("%c", c);
		c = rxFromUser();
	}
	Serial_status(&SerialPort);

	printf("\nTest rxFromForth() ...");
	c = rxFromForth();
	while (c) {
		printf("%c", c);
		c = rxFromForth();
	}
	Serial_status(&SerialPort);
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
int main (int argc, char **argv)
{
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	StrCpy(base_fn, "mforth");
	HERE = (CELL)the_memory;
	BASE = 10;

	Serial_init(&SerialPort);

	// testSerial();
	// exit(0);

    for (int i = 1; i < argc; i++)
    {
        char *cp = argv[i];
        if (*cp == '-') parse_arg(++cp);
    }

	// run existing bin file?
	if (run_saved)
	{
		if (!read_binaries()) return 1;
		if (auto_run) 
		{
			run_program(0);
			return 0;
		}
	}
	else
	{
		ccomma(BYE);
		comma(0);
		define_word("the-memory");
		ccomma(LITERAL);
		comma((CELL)&the_memory[0]);
		ccomma(RET);
		execute("1 load");
	}

	REPL();

	if (!is_temp)
	{
		BYTE_AT((CELL)&the_memory[0]) = JMP;
		CELL_AT((CELL)&the_memory[1]) = the_words[num_words].XT;
		write_output();
	}

    return 0;
}

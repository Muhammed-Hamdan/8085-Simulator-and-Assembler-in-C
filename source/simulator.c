#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>

// Flag register bits
#define S_bit 0X80
#define Z_bit 0X40
#define AC_bit 0X10
#define P_bit 0X04
#define CY_bit 0X01

// Console Modes
#define DEFAULT_MODE 0
#define MEM_MODE 1
#define SIG_MODE 2
#define EXIT_MODE 3

// Execution halt flags
#define HLT 0x76
#define NO_ERROR 0

// CPU status values
#define HLD 0
#define ACTIVE 1

// Array size constants
#define INS_SET_SIZE 246
#define MAX_BUFFER_LENGTH 500

// Op_code available on data-bus when INTR interrupt serviced
#define INTR_OP_CODE 0XFF

struct intel_8085{
	unsigned char A,F,B,C,D,E,H,L; // 8-bit registers
	unsigned short int PC, SP; 	   // 16-bit registers
	unsigned char RIM_DATA, SIM_DATA, ITR_MASK; // 8-bit data affected by RIM, SIM, EI, DI and hardware	
	// RIM_DATA -> SDI-X-RST7.5(F/F)-RST6.5-RST5.5-IE-M7.5-M6.5-M5.5
	// SIM_DATA -> SDO-SDE-X-RST7.5(RST)-ME-M7.5-M6.5-M5.5
}REG_8085;

struct pins{
	char INTR, HOLD, RESET_IN, SDI; 
	char TRAP[2];					// Pins RST7P5[2] and TRAP[2] contain two values to mimic pos-edge triggering 
	char RST7P5[2], RST6P5, RST5P5;
	
}IN_PIN_8085;

const char INSTRUCTION_MNEMONIC_TABLE[INS_SET_SIZE][9] = {
	"NOP", "LXI B,", "STAX B", "INX B", "INR B", "DCR B", "MVI B,", "RLC", "DAD B", "LDAX B", 
	"DCX B", "INR C", "DCR C", "MVI C,", "RRC", "LXI D,", "STAX D", "INX D", "INR D", "DCR D", 
	"MVI D,", "RAL", "DAD D", "LDAX D", "DCX D", "INR E", "DCR E", "MVI E,", "RAR", "RIM", 
	"LXI H,", "SHLD ", "INX H", "INR H", "DCR H", "MVI H,", "DAA", "DAD H", "LHLD ", "DCX H", 
	"INR L", "DCR L", "MVI L,", "CMA", "SIM", "LXI SP,", "STA ", "INX SP", "INR M", "DCR M", 
	"MVI M,", "STC", "DAD SP", "LDA ", "DCX SP", "INR A", "DCR A", "MVI A,", "CMC", "MOV B,B", 
	"MOV B,C", "MOV B,D", "MOV B,E", "MOV B,H", "MOV B,L", "MOV B,M", "MOV B,A", "MOV C,B", "MOV C,C", "MOV C,D", 
	"MOV C,E", "MOV C,H", "MOV C,L", "MOV C,M", "MOV C,A", "MOV D,B", "MOV D,C", "MOV D,D", "MOV D,E", "MOV D,H", 
	"MOV D,L", "MOV D,M", "MOV D,A", "MOV E,B", "MOV E,C", "MOV E,D", "MOV E,E", "MOV E,H", "MOV E,L", "MOV E,M", 
	"MOV E,A", "MOV H,B", "MOV H,C", "MOV H,D", "MOV H,E", "MOV H,H", "MOV H,L", "MOV H,M", "MOV H,A", "MOV L,B", 
	"MOV L,C", "MOV L,D", "MOV L,E", "MOV L,H", "MOV L,L", "MOV L,M", "MOV L,A", "MOV M,B", "MOV M,C", "MOV M,D", 
	"MOV M,E", "MOV M,H", "MOV M,L", "HLT", "MOV M,A", "MOV A,B", "MOV A,C", "MOV A,D", "MOV A,E", "MOV A,H", 
	"MOV A,L", "MOV A,M", "MOV A,A", "ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", 
	"ADD A", "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A", "SUB B", 
	"SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A", "SBB B", "SBB C", "SBB D", 
	"SBB E", "SBB H", "SBB L", "SBB M", "SBB A", "ANA B", "ANA C", "ANA D", "ANA E", "ANA H", 
	"ANA L", "ANA M", "ANA A", "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", 
	"XRA A", "ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A", "CMP B", 
	"CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A", "RNZ", "POP B", "JNZ ", 
	"JMP ", "CNZ ", "PUSH B", "ADI ", "RST 0", "RZ", "RET", "JZ ", "CZ ", "CALL ", 
	"ACI ", "RST 1", "RNC", "POP D", "JNC ", "OUT ", "CNC ", "PUSH D", "SUI ", "RST 2", 
	"RC", "JC ", "IN ", "CC ", "SBI ", "RST 3", "RPO", "POP H", "JPO ", "XTHL", 
	"CPO ", "PUSH H", "ANI ", "RST 4", "RPE", "PCHL", "JPE ", "XCHG", "CPE ", "XRI ", 
	"RST 5", "RP", "POP PSW", "JP ", "DI", "CP ", "PUSH PSW", "ORI ", "RST 6", "RM", 
	"SPHL", "JM ", "EI", "CM ", "CPI ", "RST 7"
};

const unsigned char OPCODE_TABLE[INS_SET_SIZE] = {
	0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07, 0X09, 0X0A, 
	0X0B, 0X0C, 0X0D, 0X0E, 0X0F, 0X11, 0X12, 0X13, 0X14, 0X15, 
	0X16, 0X17, 0X19, 0X1A, 0X1B, 0X1C, 0X1D, 0X1E, 0X1F, 0X20, 
	0X21, 0X22, 0X23, 0X24, 0X25, 0X26, 0X27, 0X29, 0X2A, 0X2B, 
	0X2C, 0X2D, 0X2E, 0X2F, 0X30, 0X31, 0X32, 0X33, 0X34, 0X35, 
	0X36, 0X37, 0X39, 0X3A, 0X3B, 0X3C, 0X3D, 0X3E, 0X3F, 0X40, 
	0X41, 0X42, 0X43, 0X44, 0X45, 0X46, 0X47, 0X48, 0X49, 0X4A, 
	0X4B, 0X4C, 0X4D, 0X4E, 0X4F, 0X50, 0X51, 0X52, 0X53, 0X54, 
	0X55, 0X56, 0X57, 0X58, 0X59, 0X5A, 0X5B, 0X5C, 0X5D, 0X5E, 
	0X5F, 0X60, 0X61, 0X62, 0X63, 0X64, 0X65, 0X66, 0X67, 0X68, 
	0X69, 0X6A, 0X6B, 0X6C, 0X6D, 0X6E, 0X6F, 0X70, 0X71, 0X72, 
	0X73, 0X74, 0X75, 0X76, 0X77, 0X78, 0X79, 0X7A, 0X7B, 0X7C, 
	0X7D, 0X7E, 0X7F, 0X80, 0X81, 0X82, 0X83, 0X84, 0X85, 0X86, 
	0X87, 0X88, 0X89, 0X8A, 0X8B, 0X8C, 0X8D, 0X8E, 0X8F, 0X90, 
	0X91, 0X92, 0X93, 0X94, 0X95, 0X96, 0X97, 0X98, 0X99, 0X9A, 
	0X9B, 0X9C, 0X9D, 0X9E, 0X9F, 0XA0, 0XA1, 0XA2, 0XA3, 0XA4, 
	0XA5, 0XA6, 0XA7, 0XA8, 0XA9, 0XAA, 0XAB, 0XAC, 0XAD, 0XAE, 
	0XAF, 0XB0, 0XB1, 0XB2, 0XB3, 0XB4, 0XB5, 0XB6, 0XB7, 0XB8, 
	0XB9, 0XBA, 0XBB, 0XBC, 0XBD, 0XBE, 0XBF, 0XC0, 0XC1, 0XC2, 
	0XC3, 0XC4, 0XC5, 0XC6, 0XC7, 0XC8, 0XC9, 0XCA, 0XCC, 0XCD, 
	0XCE, 0XCF, 0XD0, 0XD1, 0XD2, 0XD3, 0XD4, 0XD5, 0XD6, 0XD7, 
	0XD8, 0XDA, 0XDB, 0XDC, 0XDE, 0XDF, 0XE0, 0XE1, 0XE2, 0XE3, 
	0XE4, 0XE5, 0XE6, 0XE7, 0XE8, 0XE9, 0XEA, 0XEB, 0XEC, 0XEE, 
	0XEF, 0XF0, 0XF1, 0XF2, 0XF3, 0XF4, 0XF5, 0XF6, 0XF7, 0XF8, 
	0XF9, 0XFA, 0XFB, 0XFC, 0XFE, 0XFF
};

unsigned char *REG_TABLE[8] = { &REG_8085.B, &REG_8085.C, &REG_8085.D, &REG_8085.E,
	&REG_8085.H, &REG_8085.L, NULL, &REG_8085.A};

const unsigned short int ROM_begin = 0X0800;		// Program execution starts from this address on Reset
const unsigned short int stack_bottom = 0XFFFF;		// Stack Pointer loaded with this address on Reset
unsigned short int break_point = 0XFFFF;			// Simulator stops continuous execution once PC = breakpoint 

const char memory_path[] = "D://ASM-8085/mem.txt";			
const char program_path[] = "D://ASM-8085/short_div_fast_final.txt";

char console_prompt[50] = "MewP-8085-SIM> ";
char console_mode = DEFAULT_MODE;
char error_code = NO_ERROR;
char io_access = 0;
char cpu_status = ACTIVE;

void reg_initialize(void);
void mem_initialize(void);
void pin_initialize(void);
void burn_program(void);
char fetch_decode_execute(char instruction[]);
void handle_interrupt(void);
void execute_instruction(unsigned char ins_no, char instruction[]);
void print_status(char instruction[]);
unsigned char ALU_execute_flag_update(unsigned char data1, unsigned char data2, char operation, char affected_flags);
char check_branch_condition(char op_code);
unsigned char find_source_data(char op_code);
unsigned char * find_destination(char op_code);

unsigned short int read_memory_16_bit(unsigned short int address);
unsigned char read_memory_8_bit(unsigned short int address);
void write_memory_8_bit(unsigned short int address, unsigned char data);

short int search_op_code(unsigned char op_code);
unsigned char hex_char_to_num(unsigned char ch);
unsigned char num_char_to_hex(unsigned char num);
int str2num(char operand[]);
char ishexdigit(char ch);
char isregister(char ch[]);

char handle_common_commands(char buffer[]);
void handle_default_mode(char buffer[]);
void handle_mem_mode(char buffer[]);
void handle_sig_mode(char buffer[]);
void tokenize(char buffer[], char token[][MAX_BUFFER_LENGTH/2]);
unsigned short int get_num_arg(char data[], unsigned short int *loc);
void user_write_memory(char data[], char address[], char choice);
void user_write_register(char dest_reg,char data[]);
void user_read_memory(char num_byte[], char address[], char choice);
void search_error(void);

int main(){
	char buffer[MAX_BUFFER_LENGTH];
	reg_initialize();
	mem_initialize();
	pin_initialize();
	burn_program();
	while(console_mode!=EXIT_MODE){
		printf("%s",console_prompt);
		gets(buffer);
		strupr(buffer);
		switch(console_mode){
			case DEFAULT_MODE:
				handle_default_mode(buffer);
				break;
			case MEM_MODE:
				handle_mem_mode(buffer);
				break;
			case SIG_MODE:
				handle_sig_mode(buffer);
				break;
			default:
				printf("Invalid Console Mode!\n");
				break;
		}
		if(error_code!=NO_ERROR){
			search_error();
			error_code = 0;
		}
	}
	return 0;
}

void reg_initialize(){
	REG_8085.A = 0;
	REG_8085.F = 0;
	REG_8085.B = 0;
	REG_8085.C = 0;
	REG_8085.D = 0;
	REG_8085.E = 0;
	REG_8085.H = 0;
	REG_8085.L = 0;
	REG_8085.PC = ROM_begin;
	REG_8085.SP = stack_bottom;
	// RIM and SIM data initialized such that all interrupts are enabled and unmasked
	REG_8085.RIM_DATA = 0x08;
	REG_8085.SIM_DATA = 0x08;
	REG_8085.ITR_MASK = 0x00;
}

void mem_initialize(){
	unsigned char buff[71];
	int mem_row = 0, mem_col = 0;
	FILE *MEMORY;
	MEMORY = fopen(memory_path,"w");
	strcpy(buff,"        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");   
	fputs(buff,MEMORY);
	strcpy(buff,"0x000  00  00  00  00  00  00  00  00  00  00  00  00  00  00  00  00\n");   
	while(mem_row<0X1000){
		if(mem_row<16) sprintf(&buff[4],"%X",mem_row);
		else if(mem_row<256) sprintf(&buff[3],"%X",mem_row);
		else sprintf(&buff[2],"%X",mem_row);
		buff[5] = ' ';																		  	
		fputs(buff,MEMORY);
		mem_row++;
	}
	strcpy(buff,"---------------------------------I/O---------------------------------\n");
	fputs(buff,MEMORY);
	mem_row = 0; mem_col = 0;
	strcpy(buff,"0x000  00  00  00  00  00  00  00  00  00  00  00  00  00  00  00  00\n");   
	while(mem_row<0X10){
		sprintf(&buff[4],"%X",mem_row);
		buff[5] = ' ';																		  	
		fputs(buff,MEMORY);
		mem_row++;
	}
	fclose(MEMORY);
}

void pin_initialize(void){ // Active low signals set to 1, Active high signals reset to 0
	IN_PIN_8085.HOLD = 1;		
	IN_PIN_8085.RESET_IN = 1;
	IN_PIN_8085.TRAP[0] = 0;
	IN_PIN_8085.TRAP[1] = 0;
	IN_PIN_8085.RST7P5[0] = 0;
	IN_PIN_8085.RST7P5[1] = 0;
	IN_PIN_8085.RST6P5 = 0;
	IN_PIN_8085.RST5P5 = 0;
	IN_PIN_8085.INTR = 0;
}

void burn_program(){
	unsigned short int mem_row, mem_col, address;
    char nibble, i=0, num_str[6];
	FILE *MEMORY, *PROGRAM;
	MEMORY = fopen(memory_path,"r+");
	PROGRAM = fopen(program_path,"r");
	mem_row = (ROM_begin>>4)&0X0FFF;
	mem_col = ROM_begin&0X000F;
	fseek(MEMORY,71+mem_row*71+7+mem_col*4,SEEK_SET);
	while((nibble = fgetc(PROGRAM))!=EOF){
		if(nibble=='@'){
			fgets(num_str,6,PROGRAM);
			address = str2num(num_str);
			mem_row = (address>>4)&0X0FFF;
			mem_col = address&0X000F;
			fseek(MEMORY,71+mem_row*71+7+mem_col*4,SEEK_SET);
			continue;
		}   
		printf("%c",nibble);  
		if(nibble==' '||nibble=='\n'){
			nibble = fgetc(MEMORY);
			if(nibble=='\n') fseek(MEMORY,7,SEEK_CUR);  
			else if(nibble==' ') fseek(MEMORY,1,SEEK_CUR);
			else printf("Unrecognized character in memory\n");
		}
		else fputc(nibble,MEMORY);
		fflush(MEMORY);               // pg-242 K&R (use fflush b/w read and write of same file) [ Same buffer cannot be used for input and output. So user has to explicitly clear the buffer
	}
	printf("\n");
	fclose(MEMORY);
	fclose(PROGRAM);
}

char fetch_decode_execute(char instruction[]){
	unsigned short int mem_row, mem_col, ins_no;
	unsigned char op_code = 0X00, lower_nibble, upper_nibble;
    FILE *MEMORY;
    if(cpu_status == HLD) return HLD;
	MEMORY = fopen(memory_path,"r");
	mem_row = (REG_8085.PC>>4)&0X0FFF;
	mem_col = REG_8085.PC&0X000F;
	fseek(MEMORY,71+mem_row*71+7+mem_col*4,SEEK_SET);
	// fetch()
	upper_nibble = hex_char_to_num(fgetc(MEMORY));
	lower_nibble = hex_char_to_num(fgetc(MEMORY));
	fclose(MEMORY);
	op_code = upper_nibble<<4 | lower_nibble;
	// decode()
	ins_no =  search_op_code(op_code);
	// execute() if no error
	if(error_code==NO_ERROR){
		REG_8085.PC++;
		execute_instruction(ins_no,instruction);
	}
	else instruction[0] = '\0';
	return op_code;
}

void handle_interrupt(void){
	unsigned char interrupt = 0, *mask_check_reg;
	unsigned short int temp;
	
	if(cpu_status == ACTIVE){
	
		if (IN_PIN_8085.RESET_IN==0){												// RESET_SIGNAL
			printf("Mew-P 8085 is now RESET\n");
			reg_initialize();
			return;
		}
		else if(IN_PIN_8085.HOLD==0) {												// HOLD signal highest priority
			printf("HOLD acknowledged! Mew-P 8085 in hold\n");
			cpu_status = HLD;
			return;
		}
		else if(IN_PIN_8085.TRAP[0]==0 & IN_PIN_8085.TRAP[1]==1) { 					// TRAP signal more priority
			interrupt = 1;
			temp = 0x24;
		}
		else if(REG_8085.RIM_DATA & 0x08){											// If interrupts enabled
			
			
			if((REG_8085.RIM_DATA & 0x40) && !(REG_8085.ITR_MASK & 0x04)) {  	 	// RST7.5 ACTIVE
				interrupt = 1;
				temp = 0x3C;
				REG_8085.RIM_DATA &= ~0x40;
			}
			else if((IN_PIN_8085.RST6P5) && !(REG_8085.ITR_MASK & 0x02)) {			// RST6.5 ACTIVE
				interrupt = 1;
				temp = 0x34;
			}
			else if((IN_PIN_8085.RST5P5) && !(REG_8085.ITR_MASK & 0x01)){			// RST5.5 ACTIVE
				interrupt = 1;
				temp = 0x2C;
			}
			else if(IN_PIN_8085.INTR) {												// INTR ACTIVE
				printf("INTR acknowledged! Data-bus contains instruction RST %d\n",(INTR_OP_CODE&0x38)>>3);	
				interrupt = 1;
				temp = INTR_OP_CODE;
			}
			
			if(interrupt) REG_8085.RIM_DATA &= ~0x08;								// If a maskable interrupt is serviced, disable interrupts 
		}
		
		if(interrupt){
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.PC>>8);
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.PC&0X0FF);
			REG_8085.PC = temp;
		}
	}
}

void execute_instruction(unsigned char ins_no, char instruction[]){
	char operand[6] = ""; // operand[0] = 0 => No operand , =1 => operand present
	unsigned char byte,flag_update = 0;
	unsigned char *dest;
	unsigned short int temp;
	
	if(error_code!=NO_ERROR) return;
	
	switch(ins_no){
		case 1:		// NOP
			break;
		case 2:		// LXI B,#
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.B = (temp>>8)&0X00FF;
			REG_8085.C = temp&0X00FF;
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 3:		// STAX B
			temp = REG_8085.B;
			temp = temp<<8 | REG_8085.C;
			write_memory_8_bit(temp,REG_8085.A);
			break;
		case 4:		// INX B
			REG_8085.C++;
			if(REG_8085.C==0) REG_8085.B++;
			break;
		case 5: 	// INR R
		case 12:
		case 19:
		case 26:
		case 34:
		case 41:
		case 49:
		case 56:
			dest = find_destination(OPCODE_TABLE[ins_no-1]);
			flag_update = Z_bit|P_bit|AC_bit|S_bit;
			if(dest==NULL){
				temp = REG_8085.H;
				temp = temp<<=8 | REG_8085.L;
				byte = read_memory_8_bit(temp);
				byte = ALU_execute_flag_update(byte,1,'+',flag_update);
				write_memory_8_bit(temp,byte);
			}
			else *dest = ALU_execute_flag_update(*dest,1,'+',flag_update);
			break;	
		case 6: 	// DCR R
		case 13:
		case 20:
		case 27:
		case 35:
		case 42:
		case 50:
		case 57:
			dest = find_destination(OPCODE_TABLE[ins_no-1]);
			flag_update = Z_bit|P_bit|AC_bit|S_bit;
			if(dest==NULL){
				temp = REG_8085.H;
				temp = temp<<=8 | REG_8085.L;
				byte = read_memory_8_bit(temp);
				byte = ALU_execute_flag_update(byte,1,'-',flag_update);
				write_memory_8_bit(temp,byte);
			}
			else *dest = ALU_execute_flag_update(*dest,1,'-',flag_update);
			break;
		case 7:		// MVI R,#
		case 14:
		case 21:
		case 28:
		case 36:
		case 43:
		case 51:
		case 58:
			byte = read_memory_8_bit(REG_8085.PC);
			dest = find_destination(OPCODE_TABLE[ins_no-1]);
			if(dest==NULL){
				temp = REG_8085.H;
				temp = temp<<8 | REG_8085.L;
				write_memory_8_bit(temp,byte);
			}
			else *dest = byte;
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 8:		// RLC
			if(REG_8085.A&0X80) {
				REG_8085.F |= CY_bit;
				REG_8085.A <<=1;
				REG_8085.A |= 0X01;
			}
			else{
				REG_8085.F &= ~CY_bit;
				REG_8085.A <<=1;
			}
			break;
		case 9:	 	// DAD B <>
			flag_update = CY_bit;
			REG_8085.L = ALU_execute_flag_update(REG_8085.L,REG_8085.C,'+',flag_update);
			if(REG_8085.F & CY_bit){
				REG_8085.B++;
				REG_8085.F &= ~CY_bit;
			}
			REG_8085.H = ALU_execute_flag_update(REG_8085.H,REG_8085.B,'+',flag_update);
			break;
		case 10:	// LDAX B
			temp = REG_8085.B;
			temp = temp<<8 | REG_8085.C; 
			REG_8085.A = read_memory_16_bit(temp);
			break;
		case 11:	// DCX B
			REG_8085.C--;
			if(REG_8085.C==0XFF) REG_8085.B--;
			break; 
		case 15:    // RRC
			if(REG_8085.A&0X01) {
				REG_8085.F |= CY_bit;
				REG_8085.A >>=1;
				REG_8085.A |= 0X80;
			}
			else{
				REG_8085.F &= ~CY_bit;
				REG_8085.A >>=1;
			}
			break;
		case 16:		// LXI D,#
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.D = (temp>>8)&0X00FF;
			REG_8085.E = temp&0X00FF;
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 17:		// STAX D
			temp = REG_8085.D;
			temp = temp<<8 | REG_8085.E;
			write_memory_8_bit(temp,REG_8085.A);
			break;
		case 18:		// INX D
			REG_8085.E++;
			if(REG_8085.E==0) REG_8085.D++;
			break;
		case 22:		// RAL
			temp = REG_8085.F & CY_bit;
			
			if(REG_8085.A&0X80) REG_8085.F |= CY_bit;
			else REG_8085.F &= ~CY_bit;
			
			REG_8085.A <<= 1;
			
			if(temp==0) REG_8085.F &= ~0X01;
			else REG_8085.F |= 0X01;
			
			break;
		case 23:	 	// DAD D <>
			flag_update = CY_bit;
			REG_8085.L = ALU_execute_flag_update(REG_8085.L,REG_8085.E,'+',flag_update);
			if(REG_8085.F & CY_bit){
				REG_8085.D++;
				REG_8085.F &= ~CY_bit;
			}
			REG_8085.H = ALU_execute_flag_update(REG_8085.H,REG_8085.D,'+',flag_update);
			break;
		case 24:	// LDAX D
			temp = REG_8085.D;
			temp = temp<<8 | REG_8085.E;
			REG_8085.A = read_memory_16_bit(temp);
			break;
		case 25:	// DCX D
			REG_8085.E--;
			if(REG_8085.E==0XFF) REG_8085.D--;
			break;
		case 29:	// RAR
			temp = REG_8085.F & CY_bit;
			
			if(REG_8085.A&0X01) REG_8085.F |= CY_bit;
			else REG_8085.F &= ~CY_bit;
			
			REG_8085.A >>= 1;
			
			if(temp==0) REG_8085.F &= ~0X80;
			else REG_8085.F |= 0X80;
			
			break;  
		case 30:	// RIM 
			REG_8085.A = REG_8085.RIM_DATA;
			break;
		case 31:	// LXI H,#
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.H = (temp>>8)&0X00FF;
			REG_8085.L = temp&0X00FF;
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 32:	// SHLD #
			temp = read_memory_16_bit(REG_8085.PC);
			write_memory_8_bit(temp,REG_8085.L);
			temp++;
			write_memory_8_bit(temp,REG_8085.H);
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 33:	// INX H
			REG_8085.L++;
			if(REG_8085.L==0) REG_8085.H++;
			break;
		case 37:	// DAA
			flag_update = S_bit|Z_bit|P_bit|CY_bit|AC_bit;
			if((REG_8085.A&0X0F)>9 || (REG_8085.F&AC_bit))  REG_8085.A = ALU_execute_flag_update(REG_8085.A,0X06,'+',flag_update);
			if((REG_8085.A&0XF0)>0X90 || (REG_8085.F&CY_bit))  REG_8085.A = ALU_execute_flag_update(REG_8085.A,0X60,'+',flag_update);
			break;
		case 38:	// DAD H
			flag_update = CY_bit;
			REG_8085.L = ALU_execute_flag_update(REG_8085.L,REG_8085.L,'+',flag_update);
			if(REG_8085.F & CY_bit){
				REG_8085.H++;
				REG_8085.F &= ~CY_bit;
			}
			REG_8085.H = ALU_execute_flag_update(REG_8085.H,REG_8085.H,'+',flag_update);
			break;
		case 39:	// LHLD #
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.H = temp>>8;
			REG_8085.L = temp&0X0F;
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 40:	// DCX H
			REG_8085.L--;
			if(REG_8085.L==0XFF) REG_8085.H--;
			break;
		case 44:	// CMA
			REG_8085.A = ~REG_8085.A;
			break;
		case 45:	// SIM 
			REG_8085.SIM_DATA = REG_8085.A;
			REG_8085.RIM_DATA = (REG_8085.RIM_DATA&0xf8) | (REG_8085.SIM_DATA&0x07);	// 	update masks in rim_data
			if(REG_8085.SIM_DATA & 0x08) REG_8085.ITR_MASK = REG_8085.SIM_DATA & 0x07; // if MSE bit low, use previous mask setting, else update new mask setting 
			if(REG_8085.SIM_DATA & 0x10) REG_8085.RIM_DATA &= ~0x40;	// if RST7.5 flip-flop is reset, make RST7.5 pending interrupt 0
			break;
		case 46:	// LXI SP,#
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.SP = temp;
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 47:	// STA #
			temp = read_memory_16_bit(REG_8085.PC);
			write_memory_8_bit(temp,REG_8085.A);
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 48:	// INX SP
			REG_8085.SP++;
			break;
		case 52:	// STC
			REG_8085.F |= CY_bit;
			break;
		case 53:	// DAD SP
			flag_update = CY_bit;
			REG_8085.L = ALU_execute_flag_update(REG_8085.L,REG_8085.SP&0XFF,'+',flag_update);
			if(REG_8085.F & CY_bit){
				REG_8085.H++;
				REG_8085.F &= ~CY_bit;
			}
			REG_8085.H = ALU_execute_flag_update(REG_8085.H,(REG_8085.SP>>8)&0XFF,'+',flag_update);
			break;
		case 54:	// LDA #
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.A = read_memory_8_bit(temp);
			operand[0] = 1;
			REG_8085.PC++; REG_8085.PC++;
			break;
		case 55:	// DCX SP
			REG_8085.SP--;
			break;
		case 59:	// CMC
			REG_8085.F ^= CY_bit;
			break;
		case 60:	// MOV R1,R2
		case 61:
		case 62:
		case 63:
		case 64:
		case 65:
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 71:
		case 72:
		case 73:
		case 74:
		case 75:
		case 76:
		case 77:
		case 78:
		case 79:
		case 80:
		case 81:
		case 82:
		case 83:
		case 84:
		case 85:
		case 86:
		case 87:
		case 88:
		case 89:
		case 90:
		case 91:
		case 92:
		case 93:
		case 94:
		case 95:
		case 96:
		case 97:
		case 98:
		case 99:
		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
		case 108:
		case 109:
		case 110:
		case 111:
		case 112:
		case 113:
		case 115:
		case 116:
		case 117:
		case 118:
		case 119:
		case 120:
		case 121:
		case 122:
		case 123:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			dest = find_destination(OPCODE_TABLE[ins_no-1]);
			if(dest==NULL){
				temp = REG_8085.H;
				temp <<= 8;
				temp += REG_8085.L;
				write_memory_8_bit(temp,byte);
			}
			else *dest = byte;
			break;
		case 114:	// HLT	------------------->
			REG_8085.PC--;
			break;
		case 124:	// ADD R
		case 125:
		case 126:
		case 127:
		case 128:
		case 129:
		case 130:
		case 131:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'+',flag_update);
			break;
		case 132:	// ADC R
		case 133:
		case 134:
		case 135:
		case 136:
		case 137:
		case 138:
		case 139:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			if(REG_8085.F & CY_bit){
				byte = ALU_execute_flag_update(byte,1,'+',flag_update);
				if((REG_8085.A&0X0F)+(byte&0X0F)<=0X0F) flag_update &= ~AC_bit;
				if(byte==0) flag_update &= ~CY_bit;
			}
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'+',flag_update);
			break;
		case 140:	// SUB R
		case 141:
		case 142:
		case 143:
		case 144:
		case 145:
		case 146:
		case 147:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'-',flag_update);
			break;
		case 148:	// SBB R 
		case 149:
		case 150:
		case 151:
		case 152:
		case 153:
		case 154:
		case 155:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			
			if(REG_8085.F & CY_bit) byte++;			
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'-',flag_update);
			break;
		case 156:	// ANA R
		case 157:
		case 158:
		case 159:
		case 160:
		case 161:
		case 162:
		case 163:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'&',flag_update);
			break;
		case 164:	// XRA R
		case 165:
		case 166:
		case 167:
		case 168:
		case 169:
		case 170:
		case 171:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'^',flag_update);
			break;
		case 172:	// ORA R
		case 173:
		case 174:
		case 175:
		case 176:
		case 177:
		case 178:
		case 179:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'|',flag_update);
			break;
		case 180:	// CMP R
		case 181:
		case 182:
		case 183:
		case 184:
		case 185:
		case 186:
		case 187:
			byte = find_source_data(OPCODE_TABLE[ins_no-1]);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			byte = ALU_execute_flag_update(REG_8085.A,byte,'-',flag_update);
			break;
		case 188:	// RET
		case 196:
		case 197:
		case 203:
		case 211:
		case 217:
		case 225:
		case 232:
		case 240:
			if(check_branch_condition(OPCODE_TABLE[ins_no-1])){
				REG_8085.PC = read_memory_16_bit(REG_8085.SP);
				REG_8085.SP++; REG_8085.SP++;
			}
			break;
		case 189:	// POP B
			REG_8085.C = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			REG_8085.B = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			break;
		case 190:	// JMP #
		case 191:
		case 198:
		case 205:
		case 212:
		case 219:
		case 227:
		case 234:
		case 242:
			operand[0]=1;
			temp = read_memory_16_bit(REG_8085.PC);
			if(check_branch_condition(OPCODE_TABLE[ins_no-1])) REG_8085.PC = temp;
			else {
				REG_8085.PC++; REG_8085.PC++;
			}
			break;
		case 192:	// CALL # 
		case 199:
		case 200:
		case 207:
		case 214:
		case 221:
		case 229:
		case 236:
		case 244:
			operand[0]=1;
			temp = read_memory_16_bit(REG_8085.PC);
			REG_8085.PC++; REG_8085.PC++;
			if(check_branch_condition(OPCODE_TABLE[ins_no-1])){
				printf("Condition pass, calling to %X from %X\n",temp,REG_8085.PC);
				REG_8085.SP--;
				write_memory_8_bit(REG_8085.SP,(REG_8085.PC>>8)&0XFF);	
				REG_8085.SP--;
				write_memory_8_bit(REG_8085.SP,REG_8085.PC&0XFF);
				REG_8085.PC = temp;
			}
			break;
		case 193:	// PUSH B
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.B);
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.C);
			break;
		case 194:	// ADI #
			byte = read_memory_8_bit(REG_8085.PC);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'+',flag_update);
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 195:	// RST X
		case 202:
		case 210:
		case 216:
		case 224:
		case 231:
		case 239:
		case 246:
			write_memory_8_bit(REG_8085.SP,(REG_8085.PC>>8)&0X0FF);
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.PC&0X0FF);
			REG_8085.SP--;
			REG_8085.PC = OPCODE_TABLE[ins_no-1] & 0X38;  // bits 3,4 and 5 of op-code into corresponding bits of PC
			break;
		case 201: // ACI #
			byte = read_memory_8_bit(REG_8085.PC);
			temp = byte;
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			if(REG_8085.F & CY_bit){
				byte = ALU_execute_flag_update(byte,1,'+',flag_update);
				if((REG_8085.A&0X0F)+(byte&0X0F)<=0X0F) flag_update &= ~AC_bit;
				if(byte==0) flag_update &= ~CY_bit;
			}
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'+',flag_update);
			operand[0] = 1;
			REG_8085.PC++;
		case 204:	// POP D
			REG_8085.E = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			REG_8085.D = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			break;
		case 206:	// OUT #
			byte = read_memory_8_bit(REG_8085.PC);
			io_access = 1;
			write_memory_8_bit(byte,REG_8085.A);
			operand[0]=1;
			temp = byte;
			REG_8085.PC++;
			break;
		case 208:	// PUSH D
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.D);
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.E);
			break;
		case 209:	// SUI #
			byte = read_memory_8_bit(REG_8085.PC);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'-',flag_update);
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 213:	// IN # 
			byte = read_memory_8_bit(REG_8085.PC);
			io_access = 1;
			REG_8085.A = read_memory_8_bit(byte);
			operand[0]=1;
			temp = byte;
			REG_8085.PC++;
			break;
		case 215:	// SBI #
			byte = read_memory_8_bit(REG_8085.PC);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			if(REG_8085.F & CY_bit) byte++;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'-',flag_update);
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 218:	// POP H 
			REG_8085.L = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			REG_8085.H = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			break;
		case 220:	// XTHL
			byte = read_memory_8_bit(REG_8085.SP);
			write_memory_8_bit(REG_8085.SP,REG_8085.L);
			REG_8085.L = byte;
			byte = read_memory_8_bit(REG_8085.SP+1);
			write_memory_8_bit(REG_8085.SP+1,REG_8085.H);
			REG_8085.H = byte;
			break;
		case 222:	// PUSH H
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.H);
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.L);
			break; 
		case 223:	// ANI #
			byte = read_memory_8_bit(REG_8085.PC);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'&',flag_update);
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 226:	// PCHL
			REG_8085.PC = REG_8085.H;
			REG_8085.PC <<= 8;
			REG_8085.PC += REG_8085.L;
			break;
		case 228:	// XCHG
			byte = REG_8085.H;
			REG_8085.H = REG_8085.D;
			REG_8085.D = byte;
			byte = REG_8085.L;
			REG_8085.L = REG_8085.E;
			REG_8085.E = byte;
			break;
		case 230:	// XRI #
			byte = read_memory_8_bit(REG_8085.PC);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'^',flag_update);
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 233:	// POP PSW
			REG_8085.F = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			REG_8085.A = read_memory_8_bit(REG_8085.SP);
			REG_8085.SP++;
			break;
		case 235:	// DI
			REG_8085.RIM_DATA &= ~0x08;
			break;
		case 237:	// PUSH PSW
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.A);
			REG_8085.SP--;
			write_memory_8_bit(REG_8085.SP,REG_8085.F);
			break;
		case 238:	// ORI #
			byte = read_memory_8_bit(REG_8085.PC);
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			REG_8085.A = ALU_execute_flag_update(REG_8085.A,byte,'|',flag_update);
			temp = byte;
			operand[0] = 1;
			REG_8085.PC++;
			break;
		case 241:	// SPHL
			REG_8085.SP = REG_8085.H;
			REG_8085.SP <<= 8;
			REG_8085.SP += REG_8085.L;
			break;
		case 243:	// EI
			REG_8085.RIM_DATA |= 0x08;
			break;
		case 245: // CPI #
			byte = read_memory_8_bit(REG_8085.PC);
			temp = byte;
			flag_update = S_bit|Z_bit|AC_bit|P_bit|CY_bit;
			byte = ALU_execute_flag_update(REG_8085.A,byte,'-',flag_update);
			operand[0] = 1;
			REG_8085.PC++;
			break;
		default:
			break;
	}
	strcpy(instruction,INSTRUCTION_MNEMONIC_TABLE[ins_no-1]);
	if(operand[0]!='\0'){
		sprintf(operand,"%XH",temp);
		strcat(instruction,operand);
	}
}
	

void print_status(char instruction[]){
	if(error_code!=NO_ERROR) return;
	if(cpu_status==HLD){
		printf("CPU-IN-HOLD!\n");
		instruction[0] = '\0';
	}
	printf("----------------------------------------------------------------\n");
	if(instruction[0]!='\0') printf("Instruction: %s\n",instruction);
	printf("Registers:\n");
	printf("  A:0x%.2X        Flag:0x%.2X\n",REG_8085.A,REG_8085.F);
	printf("  B:0x%.2X           C:0x%.2X\n",REG_8085.B,REG_8085.C);
	printf("  D:0x%.2X           E:0x%.2X\n",REG_8085.D,REG_8085.E);
	printf("  H:0x%.2X           L:0x%.2X\n",REG_8085.H,REG_8085.L);
	printf(" PC:0x%.4X        SP:0x%.4X\n",REG_8085.PC,REG_8085.SP);
	printf("RIM:0x%.2X         SIM:0x%.2X\n",REG_8085.RIM_DATA,REG_8085.SIM_DATA);
	printf("Flags:\n");
	printf("S:%d   ",(REG_8085.F&S_bit)!=0);
	printf("Z:%d   ",(REG_8085.F&Z_bit)!=0);
	printf("AC:%d   ",(REG_8085.F&AC_bit)!=0);
	printf("P:%d   ",(REG_8085.F&P_bit)!=0);
	printf("CY:%d   \n",(REG_8085.F&CY_bit)!=0);
	printf("----------------------------------------------------------------\n");
}

unsigned char ALU_execute_flag_update(unsigned char data1, unsigned char data2, char operation, char affected_flags){
	short int result;
	switch(operation){
		case '-':
			if(affected_flags&CY_bit){
				if(data1<data2) REG_8085.F |= CY_bit;
				else REG_8085.F &= ~CY_bit;
				affected_flags &= ~CY_bit;
			}
			data2 = ~data2 + 1;
		case '+':
			if(affected_flags&AC_bit){
				if((data1&0X0F)+(data2&0X0F)>0X0F) REG_8085.F |= AC_bit;
				else REG_8085.F &= ~AC_bit;
			}
			result = data1+data2;
			if(affected_flags&CY_bit){
				if(result&0X100) REG_8085.F |= CY_bit;
				else REG_8085.F &= ~CY_bit;
			}
			if(affected_flags&S_bit){
				if(result&0X80) REG_8085.F |= S_bit;
				else REG_8085.F &= ~S_bit;
			}
			if(affected_flags&Z_bit){
				if((result&0XFF)==0) REG_8085.F |= Z_bit;
				else REG_8085.F &= ~Z_bit;
			}
			if(affected_flags&P_bit){
				result &= ~0X100;
				REG_8085.F |= P_bit;
				while(result!=0){
					if(result&0X01) REG_8085.F ^= P_bit;
					result >>= 1;
				}
			}
			return data1+data2;
			break;
		case '&':
		case '|':	
		case '^':
			if(affected_flags&AC_bit) REG_8085.F &= ~AC_bit;
			if(affected_flags&CY_bit) REG_8085.F &= ~CY_bit;
			
			if(operation=='&') result = data1 & data2;
			else if(operation=='|') result = data1 | data2;
			else result = data1 ^ data2;
			
			if(affected_flags&S_bit){
				if(result&0X80) REG_8085.F |= S_bit;
				else REG_8085.F &= ~S_bit;
			}
			if(affected_flags&Z_bit){
				if(result&0XFF==0) REG_8085.F |= Z_bit;
				else REG_8085.F &= ~Z_bit;
			}
			
			data1 = result;
			
			if(affected_flags&P_bit){
				result &= ~0X100;
				REG_8085.F |= P_bit;
				while(result!=0){
					if(result&0X01) REG_8085.F ^= P_bit;
					result >>= 1;
				}
				result = data1;
			}
			
			return result;
			break;
		default:
			break;
	}
	return 0;
}

char check_branch_condition(char op_code){
	char branch_type = (op_code & 0X38)>>3;
	switch(branch_type){
		case 0:	// J/C/R-NZ or JMP
			if(op_code & 0X01) return 1;
			else{
				if((REG_8085.F & Z_bit)==0) return 1;
				else return 0;
			}
			break;
		case 1:	// J/C/R-Z or CALL or RET
			if(op_code & 0X01) return 1;
			else{
				if(REG_8085.F & Z_bit) return 1;
				else return 0;
			}
			break;
		case 2:	// J/C/R-NC
			if((REG_8085.F & CY_bit)==0) return 1;
			else return 0;
			break;
		case 3:	// J/C/R-C
			if(REG_8085.F & CY_bit) return 1;
			else return 0;
			break;
		case 4:	// J/C/R-PO
			if((REG_8085.F & P_bit)==0) return 1;
			else return 0;
			break;
		case 5:	// J/C/R-PE
			if(REG_8085.F & P_bit) return 1;
			else return 0;
			break;
		case 6:	// J/C/R-P
			if((REG_8085.F & S_bit)==0) return 1;
			else return 0;
			break;
		case 7:	// J/C/R-M
			if(REG_8085.F & S_bit) return 1;
			else return 0;
			break;
		default:
			return 0;
	}
}

unsigned char find_source_data(char op_code){
	unsigned short int temp;
	if((op_code&0X07)==0X06){
		temp = REG_8085.H;
		temp <<= 8;
		temp += REG_8085.L;
		return read_memory_8_bit(temp);
	}
	else return *REG_TABLE[op_code&0X07];
}

unsigned char * find_destination(char op_code){
	if((op_code&0X38)==0X30) return NULL;
	else return REG_TABLE[(op_code&0X38)>>3];
}

unsigned short int read_memory_16_bit(unsigned short int address){
	unsigned short int mem_row, mem_col;
	unsigned short int temp_16;
	unsigned char upper_nibble, lower_nibble;
	FILE *MEMORY;
	MEMORY = fopen(memory_path,"r");
	mem_row = (address>>4)&0X0FFF;
	mem_col = address&0X000F;
	fseek(MEMORY,71+mem_row*71+7+mem_col*4,SEEK_SET);
	upper_nibble = hex_char_to_num(fgetc(MEMORY));
	lower_nibble = hex_char_to_num(fgetc(MEMORY));
	temp_16 = upper_nibble<<4 | lower_nibble;
	if(fgetc(MEMORY)=='\n') fseek(MEMORY,7,SEEK_CUR);
	else fseek(MEMORY,1,SEEK_CUR);
	upper_nibble = hex_char_to_num(fgetc(MEMORY));
	lower_nibble = hex_char_to_num(fgetc(MEMORY));
	temp_16 |= (upper_nibble<<4 | lower_nibble)<<8;
	fclose(MEMORY);
	return temp_16; 
}

unsigned char read_memory_8_bit(unsigned short int address){
	unsigned short int mem_row, mem_col;
	unsigned char temp_8;
	unsigned char upper_nibble, lower_nibble;
	FILE *MEMORY;
	MEMORY = fopen(memory_path,"r");
	mem_row = (address>>4)&0X0FFF;
	mem_col = address&0X000F;
	if(io_access==0) fseek(MEMORY,71+mem_row*71+7+mem_col*4,SEEK_SET);
	else{
		fseek(MEMORY,290958+mem_row*71+7+mem_col*4,SEEK_SET);
		io_access = 0;
	}
	upper_nibble = hex_char_to_num(fgetc(MEMORY));
	lower_nibble = hex_char_to_num(fgetc(MEMORY));
	temp_8 = upper_nibble<<4 | lower_nibble;
	fclose(MEMORY);
	return temp_8;
}

void write_memory_8_bit(unsigned short int address, unsigned char data){
	unsigned short int mem_row, mem_col;
	unsigned char upper_nibble, lower_nibble;
	FILE *MEMORY;
	MEMORY = fopen(memory_path,"r+");
	mem_row = (address>>4)&0X0FFF;
	mem_col = address&0X000F;
	if(io_access==0) fseek(MEMORY,71+mem_row*71+7+mem_col*4,SEEK_SET);
	else{
		fseek(MEMORY,290958+mem_row*71+7+mem_col*4,SEEK_SET);
		io_access = 0;
	}
	upper_nibble = num_char_to_hex(data>>4);
	lower_nibble = num_char_to_hex(data&0X0F);
	fputc(upper_nibble,MEMORY);
	fputc(lower_nibble,MEMORY);
	fclose(MEMORY);
}

short int search_op_code(unsigned char op_code){
	short int beg=0,last=INS_SET_SIZE-1,mid;
	
	if(error_code!=NO_ERROR) return 0;
	
	while(beg<=last){
		mid = (beg+last)>>1;
		if(OPCODE_TABLE[mid]==op_code) return mid+1;
		else if(op_code>OPCODE_TABLE[mid]) beg = mid+1;
		else last = mid-1;
	}
	error_code = 10;
	return 0;
}

unsigned char hex_char_to_num(unsigned char ch){
	if(isdigit(ch)) return ch-'0';
	else{
		switch(ch){
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				return ch-'A'+10;
				break;
			default:
				if(error_code==NO_ERROR) error_code = 5;
		}
	}
	return 0;
}

unsigned char num_char_to_hex(unsigned char num){
	if(num>=0 && num<10) return '0'+num;
	else{
		switch(num){
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				return 'A'+num-10;
				break;
			default:
				if(error_code==NO_ERROR) error_code = 15;
				break;
		}
	}
	return 0;
}

int str2num(char operand[]){
	int i, j, k, num = 0;
	i = 0;
	while(operand[i+1]!='\0'&&operand[i+1]!='\n') i++;
	j = i;
	while(i>=0){
		if(isdigit(operand[i])) k = operand[i]-'0';
		else k = operand[i]-'A'+10;
		num|=k<<((j-i)*4);
	    i--;
	}
	return num;
}

char ishexdigit(char ch){
	if(isdigit(ch)||ch=='A'||ch=='B'||ch=='C'||ch=='D'||ch=='E'||ch=='F') return 1;
	else return 0;
}

char isregister(char ch[]){
	if(ch[1]=='\0'){
		if(ch[0]=='A') return 1;
		else if(ch[0]=='B') return 2;
		else if(ch[0]=='C') return 3;
		else if(ch[0]=='D') return 4;
		else if(ch[0]=='E') return 5;
		else if(ch[0]=='H') return 6;
		else if(ch[0]=='L') return 7;
		else return 0;
	}
	else if(strcmp(ch,"PC")==0) return 8;
	else if(strcmp(ch,"SP")==0) return 9;
	else if(strcmp(ch,"FLAG")==0) return 10;
	else if(strcmp(ch,"RIM")==0) return 11;
	else if(strcmp(ch,"SIM")==0) return 12;
	else return 0;
}

char handle_common_commands(char buffer[]){
	char instruction[10];
	char op_code = 0x00;
	if(strcmp(buffer,"STEP")==0){
		op_code = fetch_decode_execute(instruction);
		handle_interrupt();
		print_status(instruction);
		return 0;	
	}
	else if(strcmp(buffer,"STAT")==0){
		instruction[0] = '\0';
		print_status(instruction);
		return 0;
	}
	else if(strcmp(buffer,"EXEC")==0){
		do{
			op_code = fetch_decode_execute(instruction);
		}while(op_code!=HLT && REG_8085.PC!=break_point && error_code==NO_ERROR);
		if(error_code==NO_ERROR){
			printf("EXECUTION completed\n");
			if(REG_8085.PC==break_point){
				printf("Break-point reached at 0X%.4X. Break-point set to 0XFFFF\n", break_point);
				break_point = 0xFFFF;
			}
			else{
				printf("Break-point not reached at 0X%.4X. Execution halted prior to Break-point\n",break_point);
			}
		}
		instruction[0] = '\0';
		print_status(instruction);
		return 0;
	}
	else if(strcmp(buffer,"EXIT")==0){
		console_mode = EXIT_MODE;
		printf("EXIT simulator!\n");
		return 0;
	}
	else return 1;
}

void handle_default_mode(char buffer[]){
	char instruction[10];
	char op_code;
	unsigned short int address = 0,i;
	if(strcmp(buffer,"")==0){
		op_code = fetch_decode_execute(instruction);
		handle_interrupt();
		print_status(instruction);
	}
	else if(handle_common_commands(buffer)==0) return;
	else if(strcmp(buffer,"MEM")==0){
		console_mode = MEM_MODE;
		strcat(console_prompt,"MEM> ");
	}
	else if(strcmp(buffer,"SIG")==0){
		console_mode = SIG_MODE;
		strcat(console_prompt,"SIG> ");
	}
	else error_code = 20;
	return;
}

void handle_mem_mode(char buffer[]){
	char token[3][MAX_BUFFER_LENGTH/2];
	unsigned char dest_reg;
	unsigned short int address, i;
	tokenize(buffer,token);
	if(handle_common_commands(buffer)==0) return;
	else if((dest_reg = isregister(token[0]))!=0) user_write_register(dest_reg,token[1]);
	else if(strcmp(token[0],"BRKPNT")==0){
		i = 0;
		address = get_num_arg(token[1],&i);
		if(token[1][i]!='\0') error_code = 45;
		else break_point = address;
	}
	else if(strcmp(token[0],"WRITE")==0)   user_write_memory(token[1],token[2],0);
	else if(strcmp(token[0],"VIEW")==0)    user_read_memory(token[1],token[2],0);
	else if(strcmp(token[0],"WRITEIO")==0) user_write_memory(token[1],token[2],1);
	else if(strcmp(token[0],"VIEWIO")==0)  user_read_memory(token[1],token[2],1);
	else if(strcmp(buffer,"..")==0){
		console_mode = DEFAULT_MODE;
		console_prompt[15] = '\0';
	}
	else error_code = 20;
	return;
}

void handle_sig_mode(char buffer[]){
	char token[3][MAX_BUFFER_LENGTH/2];
	char pin_val;
	tokenize(buffer,token);
	if(handle_common_commands(buffer)==0) return;
	else if(strcmp(buffer,"..")==0){
		console_mode = DEFAULT_MODE;
		console_prompt[15] = '\0';
	}
	else {
		if(token[1][0]=='0'&&token[1][1]=='\0') pin_val = 0;
		else if(token[1][0]=='1'&&token[1][1]=='\0') pin_val = 1;
		else {
			error_code = 25;
			return;
		}
		if(token[0][0]=='R'){
			if(strcmp(token[0],"RST7.5")==0){
				IN_PIN_8085.RST7P5[0] = IN_PIN_8085.RST7P5[1];
				IN_PIN_8085.RST7P5[1] = pin_val;
				if(IN_PIN_8085.RST7P5[0] == 0 && IN_PIN_8085.RST7P5[1]) REG_8085.RIM_DATA |= 0x40;
			}
			else if(strcmp(token[0],"RST6.5")==0){
				IN_PIN_8085.RST6P5 = pin_val;
				if(pin_val) REG_8085.RIM_DATA |= 0x20;
				else REG_8085.RIM_DATA &= ~0x20;
			}
			else if(strcmp(token[0],"RST5.5")==0){
				IN_PIN_8085.RST5P5 = pin_val;
				if(pin_val) REG_8085.RIM_DATA |= 0x10;
				else REG_8085.RIM_DATA &= ~0x10;
			}
			else if(strcmp(token[0],"RESET_IN")==0){
				IN_PIN_8085.RESET_IN = pin_val;
			}
			else{
				error_code = 20;
				return;
			}
		}
		else if(strcmp(token[0],"INTR")==0){
			IN_PIN_8085.INTR = pin_val;
		}
		else if(strcmp(token[0],"TRAP")==0){
			IN_PIN_8085.TRAP[0] = IN_PIN_8085.TRAP[1];
			IN_PIN_8085.TRAP[1] = pin_val;
		}
		else if(strcmp(token[0],"HOLD")==0){
			IN_PIN_8085.HOLD = pin_val;
			if(pin_val==1 && cpu_status==HLD) cpu_status = ACTIVE;
		}
		else if(strcmp(token[0],"SDI")==0){
			IN_PIN_8085.SDI = pin_val;
			if(pin_val) REG_8085.RIM_DATA |= 0x80;
			else REG_8085.RIM_DATA &= ~0x80;
		}
		else error_code = 20;
	}
	return;
}

void user_write_register(char dest_reg,char data[]){
	unsigned short int num, i=0;
	if(!isdigit(data[0])){
		if(error_code==NO_ERROR) error_code = 45;
		return;
	}
	num = get_num_arg(data,&i);
	if(data[i]!='\0') error_code = 45;
	switch(dest_reg){
		case 1:
			REG_8085.A = num;
			break;
		case 2:
			REG_8085.B = num;
			break;
		case 3:
			REG_8085.C = num;
			break;
		case 4:
			REG_8085.D = num;
			break;
		case 5:
			REG_8085.E = num;
			break;
		case 6:
			REG_8085.H = num;
			break;
		case 7:
			REG_8085.L = num;
			break;
		case 8:
			REG_8085.PC = num;
			break;
		case 9:
			REG_8085.SP = num;
			break;
		case 10:
			REG_8085.F = num;
			break;
		case 11:
			REG_8085.RIM_DATA = num;
			break;
		case 12:
			REG_8085.SIM_DATA = num;
			REG_8085.RIM_DATA = (REG_8085.RIM_DATA&0xf8) | (REG_8085.SIM_DATA&0x07);	// 	update masks in rim_data
			if(REG_8085.SIM_DATA & 0x08) REG_8085.ITR_MASK = REG_8085.SIM_DATA & 0x07; // if MSE bit low, use previous mask setting, else update new mask setting 
			if(REG_8085.SIM_DATA & 0x10) REG_8085.RIM_DATA &= ~0x40;	// if RST7.5 flip-flop is reset, make RST7.5 pending interrupt 0
			break;
		default:
			break;
	}
}

void user_write_memory(char data[], char address[], char choice){
	unsigned char buffer[25];
	unsigned short int temp, i;
	unsigned char seperation, buff_cnt;
	if(data[0]=='\0') error_code = 10;
	else if(data[0]=='{'){
		i=1; buff_cnt=0; seperation = 1;
		while(data[i]!='}'&&data[i]!='\0'){
			if(isdigit(data[i])&&seperation){
				temp = get_num_arg(data,&i);
				buffer[buff_cnt++] = temp&0x00ff;
				if(temp&0xff00) buffer[buff_cnt++] = (temp&0xff00)>>8; 
				seperation = 0;
			}
			else if(data[i]==' ') i++;
			else if(data[i]==','&&seperation==0){
				seperation = 1;
				i++;
			}
			else{
				error_code = 30;
				break;
			}
		}
		if((data[i]!='}'||data[i+1]!='\0'||seperation)&&error_code==0) error_code = 35;	// operand_list_not terminated;
	}
	else if(data[0]=='\"'){
		i = 1; buff_cnt = 0;
		while(data[i]!='\"'&&data[i]!='\0'){
			buffer[buff_cnt++] = data[i++];
		}
		if(data[i]!='\"'||data[i+1]!='\0') error_code = 40;				// string_not terminated;
	}
	else if(isdigit(data[0])){
		i=0; buff_cnt = 0;
		temp = get_num_arg(data,&i);
		buffer[buff_cnt++] = temp&0x00ff;
		if(temp&0xff00) buffer[buff_cnt++] = (temp&0xff00)>>8; 
		if(data[i]!='\0') error_code = 45;
	}
	if(error_code!=NO_ERROR) return;
	
	i = 0;
	temp = get_num_arg(address,&i);
	if(address[i]!='\0'){
		error_code = 45;
		return;
	}
	
	for(i=0;i<buff_cnt;i++){
		io_access = choice;
		write_memory_8_bit(temp++,buffer[i]);
	}
	return;
}

void user_read_memory(char num_byte[], char address[], char choice){ 
	unsigned short int bytes, location, i;
	unsigned char buffer[MAX_BUFFER_LENGTH], ch;
	FILE *MEMORY;
	
	i = 0;
	location = get_num_arg(address,&i);
	if(address[i]!='\0'){
		error_code = 45;
		return;
	}
	if(location>0xFF && choice){
		error_code = 50;
		return;
	}
	
	if(num_byte[0]=='\0') bytes = 1;
	else{
		i = 0;
		bytes = get_num_arg(num_byte,&i);
		if(num_byte[i]!='\0'){
			error_code = 45;
			return;
		}
	}
	
	printf("        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");
	MEMORY = fopen(memory_path,"r");
	
	if(location&0x000f){
		i = location;
		while((i&0x000f) && bytes!=0){
			i++;
			bytes--;
		}
		i = (i&0x000f)?0:1;
		if(bytes==0) bytes = 16;
	}
	else i = 0;
	
	if(bytes & 0X0F) bytes+=16;
	bytes >>= 4;
	if(i) bytes++;
	
	location = (location>>4)&0XFFF;
	if(choice==0) fseek(MEMORY,71+location*71,SEEK_CUR);
	else if(location<0x1f) fseek(MEMORY,290958+location*71,SEEK_CUR);
	ch = fgetc(MEMORY);
	for(i=0;i<bytes && ch!=EOF && ch!='-' ;i++){
		fseek(MEMORY,-1,SEEK_CUR);
		fgets(buffer,MAX_BUFFER_LENGTH,MEMORY);
		printf("%s",buffer);
		ch = fgetc(MEMORY);
	}
	fclose(MEMORY);
	return;
}


void tokenize(char buffer[], char token[][MAX_BUFFER_LENGTH/2]){
	int i = 0, j;
	token[0][0] = '\0'; token[1][0] = '\0'; token[2][0] = '\0';
	j = 0;
	while(buffer[i]!='\0'&&buffer[i]!=' ') token[0][j++] = buffer[i++];
	token[0][j] = '\0';
	if(buffer[i]=='\0') return;
	while(buffer[i]==' ') i++;
	j = 0;
	while(buffer[i]!='\0'&&buffer[i]!='@'){
		if(buffer[i]=='\"'){
			token[1][j++] = buffer[i++];
			while(buffer[i]!='\0'){
				token[1][j++] = buffer[i++];
				if(buffer[i-1]=='\"') break;
			}
		}
		else if(buffer[i++]!=' ') token[1][j++] = buffer[i-1];
	}
	token[1][j] = '\0';
	if(buffer[i]=='\0') return;
	i++;
	while(buffer[i]==' ') i++;
	j = 0;
	while(buffer[i]!='\0')  token[2][j++] = buffer[i++];
	token[2][j] = '\0';
	return;
}

unsigned short int get_num_arg(char data[],unsigned short int *loc){
	unsigned short int num = 0, start;
	if(data[*loc]=='0'&&data[*loc+1]=='X'){
		*loc+=2;
		while(ishexdigit(data[*loc])){
			num = (num<<4) | hex_char_to_num(data[*loc]);
			(*loc)++;
		}
	}
	else{
		start = *loc;
		while(isdigit(data[*loc])) (*loc)++;
		num = atoi(&data[start]);
	}
	return num;
}

void search_error(void){
	printf("ERROR: ");
	switch(error_code){
		case 5: printf("Invalid hexadecimal literal found enconutered\n"); break;
		case 10:printf("Invalid op_code found in ROM\n"); break;
		case 15:printf("Invalid character write to memory\n"); break;
		case 20:printf("Invalid command\n"); break;
		case 25:printf("Invalid operand to command\n"); break;
		case 30:printf("Invalid character/delimiter in operand to command\n"); break;
		case 35:printf("Operand list not terminated\n"); break;
		case 40:printf("String not terminated\n"); break;
		case 45:printf("Invalid character in numeric operand\n"); break;
		case 50:printf("Address not in range of I/O map\n"); break;
		default: break;
	}
}

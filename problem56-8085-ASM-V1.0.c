#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>

#define MAX_TOKEN_SIZE 50
#define MAX_CHAR 150
#define NO_ISA_MNEMONICS 80
#define INS_SET_SIZE 247
#define NO_DIRECTIVES 6

#define LABEL 1
#define SYMBOL 2

#define NO_ERROR 0
#define END_ASSEMBLY -1

// Symbol table contains labels stored as a Linked list
struct label{
	char name[MAX_TOKEN_SIZE/4];
	int data;
	struct label *link; 
};
struct label *symbol_table = NULL; // Pointer to beginning of symbol table



const char ISA_MNEMONIC[NO_ISA_MNEMONICS][5] = {
	"ACI", "ADC", "ADD", "ADI", "ANA", "ANI", "CALL", "CC", "CM", "CMA", 
	"CMC", "CMP", "CNC", "CNZ", "CP", "CPE", "CPI", "CPO", "CZ", "DAA", "DAD", "DCR", "DCX", "DI", "EI", 
	"HLT", "IN", "INR", "INX", "JC", "JM", "JMP", "JNC", "JNZ", "JP", "JPE", "JPO", "JZ", "LDA", 
	"LDAX", "LHLD", "LXI", "MOV", "MVI", "NOP", "ORA", "ORI", "OUT", "PCHL", "POP", "PUSH", "RAL", "RAR",
	"RC", "RET", "RIM", "RLC", "RM", "RNC", "RNZ", "RP", "RPE", "RPO", "RRC", "RST", "RZ", "SBB", "SBI", "SHLD", "SIM",
	"SPHL", "STA", "STAX", "STC", "SUB", "SUI", "XCHG", "XRA", "XRI", "XTHL"
	};
	
const char INSTRUCTION[INS_SET_SIZE][9] = {
	"ACI ","ADC A","ADC B","ADC C","ADC D","ADC E","ADC H","ADC L","ADC M",
	"ADD A","ADD B","ADD C","ADD D","ADD E","ADD H","ADD L","ADD M",
	"ADI ","ANA A","ANA B","ANA C","ANA D","ANA E","ANA H","ANA L","ANA M",
	"ANI ","CALL ","CC ","CM ","CMA","CMC",
	"CMP A","CMP B","CMP C","CMP D","CMP E","CMP H","CMP L","CMP M",
	"CNC ","CNZ ","CP ","CPE ","CPI ","CPO ","CZ ","DAA","DAD B","DAD D","DAD H",
	"DAD SP","DCR A","DCR B","DCR C","DCR D","DCR E","DCR H","DCR L","DCR M",
	"DCX B","DCX D","DCX H","DCX SP","DI","EI","HLT","IN ",
	"INR A","INR B","INR C","INR D","INR E","INR H","INR L","INR M",
	"INX B","INX D","INX H","INX SP",
	"JC ","JM ","JMP ","JNC ","JNZ ","JP ","JPE ","JPO ","JZ ",
	"LDA ","LDAX B","LDAX D","LHLD ","LXI B,","LXI D,","LXI H,","LXI SP,",
	"MOV A,A","MOV A,B","MOV A,C","MOV A,D","MOV A,E","MOV A,H","MOV A,L","MOV A,M",
	"MOV B,A","MOV B,B","MOV B,C","MOV B,D","MOV B,E","MOV B,H","MOV B,L","MOV B,M",
	"MOV C,A","MOV C,B","MOV C,C","MOV C,D","MOV C,E","MOV C,H","MOV C,L","MOV C,M",
	"MOV D,A","MOV D,B","MOV D,C","MOV D,D","MOV D,E","MOV D,H","MOV D,L","MOV D,M",
	"MOV E,A","MOV E,B","MOV E,C","MOV E,D","MOV E,E","MOV E,H","MOV E,L","MOV E,M",
	"MOV H,A","MOV H,B","MOV H,C","MOV H,D","MOV H,E","MOV H,H","MOV H,L","MOV H,M",
	"MOV L,A","MOV L,B","MOV L,C","MOV L,D","MOV L,E","MOV L,H","MOV L,L","MOV L,M",
	"MOV M,A","MOV M,B","MOV M,C","MOV M,D","MOV M,E","MOV M,H","MOV M,L","MOV M,M",
	"MVI A,","MVI B,","MVI C,","MVI D,","MVI E,","MVI H,","MVI L,","MVI M,",
	"NOP","ORA A","ORA B","ORA C","ORA D","ORA E","ORA H","ORA L","ORA M",
	"ORI ","OUT ","PCHL","POP B","POP D","POP H","POP PSW",
	"PUSH B","PUSH D","PUSH H","PUSH PSW",
	"RAL","RAR","RC","RET","RIM","RLC","RM","RNC","RNZ","RP","RPE","RPO","RRC",
	"RST 0","RST 1","RST 2","RST 3","RST 4","RST 5","RST 6","RST 7","RZ",
	"SBB A","SBB B","SBB C","SBB D","SBB E","SBB H","SBB L","SBB M",
	"SBI ","SHLD ","SIM","SPHL","STA ","STAX B","STAX D","STC",
	"SUB A","SUB B","SUB C","SUB D","SUB E","SUB H","SUB L","SUB M",
	"SUI ","XCHG","XRA A","XRA B","XRA C","XRA D","XRA E","XRA H","XRA L","XRA M",
	"XRI ","XTHL"
};

const char OPCODE[INS_SET_SIZE][4] = {
	"CE ","8F","88","89","8A","8B","8C","8D","8E",
	"87","80","81","82","83","84","85","86",
	"C6 ","A7","A0","A1","A2","A3","A4","A5","A6",
	"E6 ","CD ","DC ","FC ","2F","3F",
	"BF","B8","B9","BA","BB","BC","BD","BE",
	"D4 ","C4 ","F4 ","EC ","FE ","E4 ","CC ","27","09","19","29",
	"39","3D","05","0D","15","1D","25","2D","35",
	"0B","1B","2B","3B","F3","FB","76","DB ",
	"3C","04","0C","14","1C","24","2C","34",
	"03","13","23","33",
	"DA ","FA ","C3 ","D2 ","C2 ","F2 ","EA ","E2 ","CA ",
	"3A ","0A","1A","2A ","01 ","11 ","21 ","31 ",
	"7F","78","79","7A","7B","7C","7D","7E",
	"47","40","41","42","43","44","45","46",
	"4F","48","49","4A","4B","4C","4D","4E",
	"57","50","51","52","53","54","55","56",
	"5F","58","59","5A","5B","5C","5D","5E",
	"67","60","61","62","63","64","65","66",
	"6F","68","69","6A","6B","6C","6D","6E",
	"77","70","71","72","73","74","75","76",
	"3E ","06 ","0E ","16 ","1E ","26 ","2E ","36 ",
	"00","B7","B0","B1","B2","B3","B4","B5","B6",
	"F6 ","D3 ","E9","C1","D1","E1","F1",
	"C5","D5","E5","F5",
	"17","1F","D8","C9","20","07","F8","D0","C0","F0","E8","E0","0F",
	"C7","CF","D7","DF","E7","EF","F7","FF","C8",
	"9F","98","99","9A","9B","9C","9D","9E",
	"DE ","22 ","30","F9","32 ","02","12","37",
	"97","90","91","92","93","94","95","96",
	"D6 ","EB","AF","A8","A9","AA","AB","AC","AD","AE",
	"EE ","E3"
};

const char INS_SIZE[INS_SET_SIZE] = {
	2,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	2,1,1,1,1,1,1,1,1,
	2,3,3,3,1,1,
	1,1,1,1,1,1,1,1,
	3,3,3,3,2,3,3,1,1,1,1,
	1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,2,
	1,1,1,1,1,1,1,1,
	1,1,1,1,
	3,3,3,3,3,3,3,3,3,
	3,1,1,3,3,3,3,3,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,
	1,1,1,1,1,1,1,1,1,
	2,2,1,1,1,1,1,
	1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,
	2,3,1,1,3,1,1,1,
	1,1,1,1,1,1,1,1,
	2,1,1,1,1,1,1,1,1,1,
	2,1
};


const char ISA_DIRECTIVE[NO_DIRECTIVES][6] = {"DB","DS","DW","END","EQU","ORG"};

int error_code  = 0;
int program_counter = 0x0800;
int RAM_counter = 0x0200;

void pass1(char path_orig[],char path_inter[]);										   // Pass 1 of assembler 
void pass2(char path_inter[],char path_final[]); 									   // Pass 2 of assembler

char tokenize(char buff[], char token[][MAX_TOKEN_SIZE]);                              // Convert Raw text to tokens ie expression_mnemonic_operands 
void parse_tokens(char buff[],char token[][MAX_TOKEN_SIZE], char no_token);            // Read tokens to find type of token and perform appropriate action
char search_instruction(char token[]);												   // Search if token is a valid mnemonic
char search_directive(char token[]);												   // Search if token is a valid directive
void update_symbol_table(char token[],int program_counter);							   // Update symbol table with label and address
void generate_opcode(char buff[],char token[],char ins_no);							   // For valid instruction, check if operands correct and provide op-code
void handle_directive(char token[][MAX_TOKEN_SIZE],char dir_no,char start_token, char buff[]);  // Take necessary action according to directive                                       
int find_label(char label[]);                                                          // Locate and return address of label. If not found, return -1
void search_error(int error_code,int line);											   // Print Error message with line number

int binary_search(char str[],const char *master, int len_master, int max_str_len);	   // Searches for the given string in master array and returns index+1 if found. If not found, return 0 	
char strcmp_mod(char str[],const char instruction[]);								   // Compares two strings and returns 0 if equal. strcmp modified to Use for binary search
void hexa_arg(char op_code[],char operand[],int end,int nibble);					   // Coverts hexadecimal operands to 8-bit chunk to store in memory   
void dec_arg(char op_code[],char operand[],int end,int nibble);						   // Coverts decimal operands to 8-byte chunk to store in memory
int str2num(char operand[]);    													   // Convert string of hexadecimal or decimal characters to integer
void copy_string_operand(char dest[],char source[], int *i, int *j);				   // Copy the string operand (ex: 'HELLO') from source to destination string
char check_syntax(char token[],char operand_type);									   // Check syntax of Label/Symbol
char ishexdigit(char ch);															   // Check if ch is 0-F
char isregister(char ch[]);															   // Check if ch is A,B,C,D,E,H,L,PSW,SP

int main(){
	const char directory[] = "D://ASM-8085/"; 
	const char file_orig[] = "short_div_fast.txt";
	char file_inter[strlen(file_orig)+15];
	char file_final[strlen(file_orig)+15];
	char path_orig[strlen(directory)+strlen(file_orig)];
	char path_inter[strlen(directory)+strlen(file_orig)+15];
	char path_final[strlen(directory)+strlen(file_orig)+15];
	
	int i = 0;
	while(file_orig[i]!='.'){
		file_inter[i]=file_orig[i];
		file_final[i]=file_orig[i];
		i++;
	}
	strcat(file_inter,"_inter"); strcat(file_inter,&file_orig[i]);
	strcat(file_final,"_final"); strcat(file_final,&file_orig[i]);
	
	// Construct complete paths => directory + file_name
	strcpy(path_orig,directory); strcat(path_orig,file_orig);
	strcpy(path_inter,directory); strcat(path_inter,file_inter);
	strcpy(path_final,directory); strcat(path_final,file_final);
	
	// Two-Pass Assembler
	pass1(path_orig,path_inter);
	if(error_code==NO_ERROR) pass2(path_inter,path_final);
	
	return 0;
}

void pass1(char path_orig[],char path_inter[]){
	char token[3][MAX_TOKEN_SIZE], buff[MAX_CHAR];
	char no_token;
	int line = 0;
	FILE *ORIG,*INTER;
	ORIG = fopen(path_orig,"r");
	INTER = fopen(path_inter,"w");
	
	while(fgetc(ORIG)!=EOF&&error_code==NO_ERROR){
		fseek(ORIG,-1,SEEK_CUR);
		fgets(buff,MAX_CHAR,ORIG);
		no_token = tokenize(buff,token);
		parse_tokens(buff,token,no_token);
		fputs(buff,INTER);
		line++;
	}
	
	if(error_code!=NO_ERROR){
		if(error_code==END_ASSEMBLY){
			error_code=NO_ERROR;
		}
		else search_error(error_code,line);
	}
	
	fclose(ORIG);
	fclose(INTER);
}


void pass2(char path_inter[],char path_final[]){
	char buff[MAX_CHAR], operand[MAX_TOKEN_SIZE];
	int line = 0;
	int i,j,data,nibble;
	FILE *INTER,*FINAL;
	
	INTER = fopen(path_inter,"r");
	FINAL = fopen(path_final,"w");
	
	while(fgetc(INTER)!=EOF&&error_code==NO_ERROR){ 
		fseek(INTER,-1,SEEK_CUR);
		fgets(buff,MAX_CHAR,INTER);
		if(buff[0]=='\n'){
			line++;
			continue;
		}
		for(i=0;buff[i]!='&'&&buff[i]!='?'&&buff[i]!='\n';i++);
		if(buff[i]=='&'||buff[i]=='?'){
			nibble = (buff[i]=='&')?4:2;
			buff[i++]='\0';
			for(j=0;buff[i]!='\n';i++) operand[j++] = buff[i];
			operand[j]='\0';
			data = find_label(operand);
			if(data>=0){
				sprintf(operand,"%XH",data);
				hexa_arg(buff,operand,strlen(operand)-2,nibble);
			}
		}
		printf("%s",buff);
		fputs(buff,FINAL);
		line++;
	}
	
	if(error_code!=NO_ERROR) search_error(error_code,line);
	
	
	fclose(INTER);
	fclose(FINAL);
}


char tokenize(char buff[], char token[][MAX_TOKEN_SIZE]){
	char t = 0;
	int i = 0, j;
	if(buff[i]==' '){											                                     // Remove leading spaces for every token
		while(buff[i]==' ') i++;
	}
	while(t<3){
		j = 0;
		if(buff[i]=='\n'||buff[i]==';'||buff[i]=='\0'){                                              // If end of line or comment, exit
			token[t][j] = '\0';
			break;
		}
		if(buff[i]=='\''){                                                                           // If string encountered
			copy_string_operand(token[t],buff,&i,&j);
		}
		while(buff[i]!=' '&&buff[i]!=','&&buff[i]!='\n'&&buff[i]!=';'&&buff[i]!='\0'){               // Scan token
			token[t][j++] = buff[i++];
		}
		if(buff[i]==' '){											                                 // Remove additional spaces between tokens
			while(buff[i]==' ') i++;
		}
		if(buff[i]==','){                                                                            // If the token contains multiple operands, copy them
			while(buff[i]!='\n'&&buff[i]!=';'){
				if(buff[i]=='\'') copy_string_operand(token[t],buff,&i,&j);                          //If string encountered
				else if(buff[i]!=' ') token[t][j++] = buff[i++];                                     // Remove spaces in between operands
				else i++;
			}		                                                                                  
		}
		token[t][j] = '\0';                              			                                 // Convert token to string 
		t++;                                            			                                 // Move to next token
	}
	if(t<3) i = t;
	while(i<3) token[i++][0] = '\0';
	strupr(token[0]);
	strupr(token[1]);
	strupr(token[2]);
	return t;                                         				                                 // Return number of tokens
}

void parse_tokens(char buff[],char token[][MAX_TOKEN_SIZE], char no_token){
	char ins_no = 0, dir_no = 0, label_type = 0;
	char label_flag = 0, symbol_flag = 0;
	char *buff_start = buff;
	
	if(no_token==0){																	            // If no tokens, return
		buff[0]='\n'; buff[1]='\0';                                                                 // Empty line in pass 1
		return;
	}
	
	if(token[0][strlen(token[0])-1]==':') label_flag = 1;                                           // Find type of token
	else{
		ins_no = search_instruction(token[0]);
		if(ins_no==0) dir_no = search_directive(token[0]);
		if(ins_no==0&&dir_no==0) symbol_flag = 1;													
	}
	 
	if(label_flag){                                                                                 // If token is a label
		label_type = check_syntax(token[0],LABEL);
		if(label_type){																				// check syntax							 
			token[0][strlen(token[0])-1]='\0';
					
			ins_no = search_instruction(token[1]);												    // Read next token
			if(ins_no==0) dir_no = search_directive(token[1]);
			
			if(ins_no){																				// If next token is instruction, generate op-code with operands
				if(label_type==1) update_symbol_table(token[0],program_counter);
				else {
					program_counter = str2num(token[0]);
					sprintf(buff,"@%X\n",program_counter);
					buff_start = buff + strlen(buff);
				}
				generate_opcode(buff_start,token[2],ins_no);
			}
			else if(dir_no){																		// If next token is directive, handle it with operands
				if(label_type==1) update_symbol_table(token[0],RAM_counter);
				else RAM_counter = str2num(token[0]);	
				handle_directive(token,dir_no,LABEL,buff);
			}	
			else error_code = 20;																	// Expected valid ins/dir after label
		}
		else error_code = 5;																		// Invalid label
	}
	else if(ins_no){																				// If next token is instruction
		generate_opcode(buff_start,token[1],ins_no);											    // Generate op-code with operands
	}
	else if(dir_no){																				// If next token is directive																		
		handle_directive(token,dir_no,0,buff);														// Handle directive with operands
	}
	else if(symbol_flag){																			// If token is a symbol
		if(check_syntax(token[0],SYMBOL)) { 	 					                                // Check syntax & update symbol table with -1	
			dir_no = search_directive(token[1]);													// Read next token
			if(dir_no) handle_directive(token,dir_no,SYMBOL,buff);								    // If next token is directive, handle it with operands											
			else error_code = 25;																    // Expected valid dir after symbol
		}
		else error_code = 10;																		// Invalid symbol
		 
	}
	else error_code = 15;																			// Invalid token
	if(dir_no!=0&&dir_no!=6){
		buff[0]='\n'; buff[1]='\0';                                                                 // Lines containing directives convert as empty line in pass 1 except ORG
	}
}

char search_instruction(char token[]){
	return binary_search(token,&ISA_MNEMONIC[0][0],NO_ISA_MNEMONICS,5);
}

char search_directive(char token[]){
	char i=0;
	for(i=0;i<NO_DIRECTIVES;i++){
		if(strcmp(token,ISA_DIRECTIVE[i])==0) return i+1;
	}
	return 0;
}

void update_symbol_table(char token[],int data){
	struct label *temp,*i;
	if(symbol_table==NULL){ // First entry in symbol table
		temp = (struct label*)malloc(sizeof(struct label));
		temp->data = data;
		strcpy(temp->name,token);
		temp->link = NULL;
		symbol_table = temp;
	}
	else{
		i = symbol_table;
		while(i->link!=NULL){
			if(strcmp(i->name,token)==0){ // If already label present, update data
				i->data = data;
				return;
			}
			i = i->link;
		}
		temp = (struct label*)malloc(sizeof(struct label));
		temp->data = data;
		strcpy(temp->name,token);
		temp->link = NULL;
		i->link = temp;
	}
}


void generate_opcode(char buff[],char token[],char mne_no){
	char op_code[9] = "\0";
	char instruction[9];
	char bytes, nibbles;
	int i,j, num_beg = 0, ins_no = 0;
	
	strcpy(instruction,ISA_MNEMONIC[mne_no-1]);
	i = strlen(instruction);
	if(token[0]!='\0'){                                                                           // Construct instruction using register operands after mnemonic 
		instruction[i++] = ' ';
		if(isregister(token)){
			j=0;
			while(token[j]!='\0'&&token[j]!=',') instruction[i++] = token[j++];
			if(token[j]==',') instruction[i++] = token[j++];
			if(isregister(&token[j])) instruction[i++] = token[j++];
		}
		else if(mne_no==65){                                                                      // RST instruction
			j = 0;
			while(isdigit(token[j])&&token[j]!='\0') instruction[i++] = token[j++];
			if(strcmp(&token[j],"H")!=0&&token[j]!='\0'){
				error_code = 30;                                                                  // Invalid operands for RST
				return;
			}
		}
	}
	instruction[i] = '\0';
	
	ins_no = binary_search(instruction,&INSTRUCTION[0][0], INS_SET_SIZE, 9);
	
	if(ins_no == 0){
		printf("Not found\n");
		error_code = 30;                                                                         // Invalid operands for instruction
		return;
	}
	
	switch(ins_no){
		// 2 or 3 byte instructions
		case 1:
		case 18:
		case 27:
		case 28:
		case 29:
		case 30:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
		case 68:
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
		case 93:
		case 94:
		case 95:
		case 96:
		case 97:
		case 162:
		case 163:
		case 164:
		case 165:
		case 166:
		case 167:
		case 168:
		case 169:
		case 179:
		case 180:
		case 220:
		case 221:
		case 224:
		case 236:
		case 246:
			strcat(op_code,OPCODE[ins_no-1]);
			bytes = INS_SIZE[ins_no-1];
			nibbles = (bytes-1)<<1;
			i = 0;
			while(token[i]!=','&&token[i]!='\0') i++;                                                                          // Analyze numeric operands of 2 and 3 byte instructions       
			if(token[i]==',') num_beg = ++i;
			else num_beg = 0;
			if(check_syntax(&token[num_beg], SYMBOL)){                                  			                           // Check if operand is a symbol/label	
				if(bytes==2) strcat(op_code,"?");                                            		                           // 1-byte operand
				else strcat(op_code,"&");                                                    		                           // 2-byte operand
				strcat(op_code,&token[num_beg]);
				strcat(op_code,"\n");
			}
			else{
				while(token[i]!='\0') i++;
				if(token[i-1]=='H') hexa_arg(op_code,&token[num_beg],i-2-num_beg,nibbles);                                      // Hexadecimal arguement
				else if(isdigit(token[i-1])) dec_arg(op_code,&token[num_beg],i-1-num_beg,nibbles);                              // Decimal arguement
				else if(token[i-1]=='D') dec_arg(op_code,&token[num_beg],i-2-num_beg,nibbles);
				else error_code = 30;                                                                                           // Invalid Operand/Number format after instruction
			}
			break;
		// 1 byte instructions
		default: 
			strcat(op_code,OPCODE[ins_no-1]);
			strcat(op_code,"\n");
			bytes = INS_SIZE[ins_no-1];
			break;		
	}
	strcpy(buff,op_code);
	program_counter+=bytes;                                                                                                     // Increment program counter by number of bytes in instruction
}

void handle_directive(char token[][MAX_TOKEN_SIZE],char dir_no,char start_token, char buff[]){
	char operand[MAX_TOKEN_SIZE], temp[MAX_TOKEN_SIZE];
	int i, j, k, num;
	switch(dir_no){
		case 1:  // DB
		case 3:  // DW
			if(start_token==SYMBOL){
				error_code = 60;
				break;
			}
			if(start_token==LABEL) strcpy(operand,token[2]);
			else strcpy(operand,token[1]);
			i = 0; j = 0; num = 0;
			while(operand[i]!='\0'){
				temp[j++] = operand[i++];
				if(operand[i]==','||operand[i]=='\0'){
					temp[j]='\0';
					if(temp[0]=='\''){                                                                    // Operand is string of characters
						if(temp[j-1]=='\''){
							if(dir_no==1) num+=j-2;                                               		  // DB - 1 byte per character
							else num+=(j-2)<<2;													          // DW - 2 byte per character
						}
						else error_code = 45;
					}
					else if(temp[j-1]=='H'){															  // Operand is hexadecimal number
						j=0; k=0;
						while(temp[j]=='0') j++;
						while(ishexdigit(temp[j])&&ishexdigit(temp[j+1])){
							k++;
							j+=2;
						}
						if(temp[j]!='H'){
							if(ishexdigit(temp[j])) k++;
							else error_code = 35; 
						}
						if(dir_no==1&&k>1) error_code = 50;                                              // DB - greater than 255
						else if(dir_no==3&&k>2) error_code = 50;                                         // DW - greater than 65535
						else num+=k;
					}
					else if(isdigit(temp[j-1])||temp[j-1]=='D'){										 // Operand is decimal number
						temp[j]='\0'; j=0; k = 0;
						while(isdigit(temp[j])) j++;
						if(temp[j]!='\0'&&temp[j]!='D'&&temp[j+1]!='\0') error_code = 35;
						j = atoi(temp);
						if(j==0) k++;
						while(j!=0&&num!=4){
							k++;
							j = j>>8;
						}
						if(dir_no==1&&k>1) error_code = 50;                                              // DB - greater than 255
						else if(dir_no==3&&k>2) error_code = 50;                                         // DW - greater than 65535	
						else num+=k;						
					}
					if(operand[i]==',') i++;
					j=0;
				}
			}
			RAM_counter+=num;
			break;
			
			case 2:    // DS
				if(start_token==SYMBOL){
				error_code = 60;
				break;
				}
				if(start_token==LABEL) strcpy(operand,token[2]);
				else strcpy(operand,token[1]);
				num = str2num(operand);
				RAM_counter+=num;
				break;
			
			case 4: // END
				if(start_token==0&&token[1][0]=='\0'&&token[2][0]=='\0') error_code = -1;   // end assembly
				else if (start_token!=0) error_code = 60;
				else error_code = 55;
				break;
			
			case 5: // EQU
				if(start_token==0||start_token==LABEL){
					error_code = 60;
					break;
				}
				strcpy(operand,token[2]);
				num = str2num(operand);
				if(num>0XFFFF) error_code = 50;
				else if(num>0XFF) RAM_counter+=2;
				else RAM_counter++;
				update_symbol_table(token[0],num);
				break;
				
			case 6: // ORG
				if(start_token==SYMBOL){
					error_code = 60;
					break;
				}
				if(start_token==LABEL){
					update_symbol_table(token[0],program_counter);
					strcpy(operand,token[2]);
				}
				else strcpy(operand,token[1]);
				num = str2num(operand);
				program_counter=num;
				sprintf(buff,"@%X\n",program_counter);        
				break;
			default:
				error_code = 120;                                                                     // Unidentified directive
				break;  
	}
}

int find_label(char label[]){
	struct label *i = symbol_table;
	while(i!=NULL){
		if(strcmp(i->name,label)==0) return i->data;
		i = i->link;
	}
	error_code = 65; // Label/Symbol not declared
	return -1;
}

void search_error(int error_code, int line){
	printf("\nERROR:");
	switch(error_code){
		case 5:
			printf("Line-%d:Invalid Label\n",line);
			break;
		case 10:
			printf("Line-%d:Invalid Symbol\n",line);
			break;
		case 15:
			printf("Line-%d:Invalid instruction/directive\n",line);
			break;
		case 20:
			printf("Line-%d:Expected valid instruction/directive after label\n",line);
			break;
		case 25:
			printf("Line-%d:Expected valid directive after symbol\n",line);
			break;
		case 30:
			printf("Line-%d:Invalid operands after instruction\n",line);
			break;
		case 35:
			printf("Line-%d:Invalid digit in numeric operand\n",line);
			break;
		case 40:
			printf("Line-%d:Numeric operand greater than instruction word size\n",line);
			break;
		case 45:
			printf("Line-%d:String operand not terminated\n",line);
			break;
		case 50:
			printf("Line-%d:Numeric operand greater than directive's limit\n",line);
			break;
		case 55:
			printf("Line-%d:Invalid operands after directive\n",line);
			break;
		case 60:
			printf("Line-%d:Invalid expression before directive\n",line);
			break;
		case 65:
			printf("Line-%d:Label/Symbol not found in symbol table\n",line);
			break;
		case 120:
			printf("Line-%d:Unidentified directive\n",line);
			break;
		default:
			printf("Line-%d:",line);
			break;
	}
}

int binary_search(char str[],const char *master, int len_master, int max_str_len){
	int beg = 0, last = len_master-1;
	int mid;
	char comparison;
	while(beg<=last){
		mid = (beg+last)/2;
		comparison = strcmp_mod(str,master+mid*max_str_len);
		if(comparison==0) return mid+1;
		else if(comparison==-1) last = mid-1;
		else if(comparison==1) beg = mid+1;
		else break;
	}
	return 0;
}

char strcmp_mod(char str[],const char instruction[]){
	int i = 0;
	while(str[i]!='\0'&&instruction[i]!='\0'){
		if(str[i]==instruction[i]) i++;
		else break;
	}
	
	if(str[i]=='\0'&&instruction[i]=='\0') return 0;                        // Both strings equal
	else if(str[i]=='\0') return -1;										// len(str)<len(ins)	
	else if(instruction[i]=='\0') return 1;									// len(str)>len(ins)	
	else if(str[i]<instruction[i]) return -1;								// str is before ins
	else if(str[i]>instruction[i]) return 1;								// str is after ins
	else return 5;															
}

void hexa_arg(char op_code[],char operand[],int end,int nibble){
	int beg,i,j; char number[6];
	i=0;
	while(operand[i]=='0') i++; // Remove leading zeros
	beg = i;
	nibble+=(nibble>>2);
	for(i=0,j=end;i<nibble&&j>=beg;i+=3,j-=2){
		if(ishexdigit(operand[j])){
			if(ishexdigit(operand[j-1])) number[i]=operand[j-1];
			else if(j-1<beg) number[i]='0'; 
			else break;
			number[i+1]=operand[j];
			number[i+2]=' ';
		}
		else break; 
	}
	if(i<nibble&&j>=beg) error_code = 35; // Invalid hexadecimal digit
	else if(i>nibble&&j>=beg) error_code = 40; // Number greater than limit set by nibble
	else{
		if(j<beg&&i<nibble){ // insufficent significant digits entered digits 
			while(i<nibble){
				number[i] = '0';
				number[i+1] = '0';
				number[i+2] = ' ';
				i+=3;
			}
		}
		number[i-1]='\n'; number[i]='\0';
		strcat(op_code,number);
	}
} 

void dec_arg(char op_code[],char operand[],int end,int nibble){
	int num,i,bit,temp;
	i = 0;
	while(i<=end){
		if(isdigit(operand[i])) i++;
		else{
			error_code = 35; // Invalid digit in numeric operand
			return;
		}
	}
	// Conversion of Decimal string to Hexa-Decimal string 
	num = atoi(operand); bit = 0x0F000000; i=0;  
	while((num&bit)==0&&bit!=0){                  // Remove leading zeros
		bit=bit>>4;
	} 
	if(bit==0) operand[i++]='0';
	else{
		while(bit!=0){
			temp = num&bit;
			while(temp>15) temp=temp>>4;
			if(temp<10) operand[i++] = 48+temp;
			else operand[i++] = 65+temp-10;
			bit=bit>>4;
		}
	}
	operand[i] = 'H'; operand[i+1] = '\0';
	end = i-1; 
	if(i>nibble){
		error_code = 40; // Number greater than limit set by nibble
		return;
	}
	hexa_arg(op_code,operand,end,nibble);
}

int str2num(char operand[]){
	int i, j, k, num = 0;
	i = 0;
	while(operand[i]!='\0') i++;
	if(operand[i-1]=='H'){                                                                  // Operand is hexadecimal number 
		i-=2;
		j = i;
		while(i>=0){
			if(ishexdigit(operand[i])){
				if(isdigit(operand[i])) k = operand[i]-'0';
				else k = operand[i]-'A'+10;
				num|=k<<((j-i)*4);
			}
		    else error_code = 35;
		    i--;
		}
	}
	else if(isdigit(operand[i-1])||operand[i-1]=='D'){                                     // Operand is decimal number
		i=0;
		while(isdigit(operand[i])) i++;
		if(operand[i]!='\0'&&operand[i]!='D'&&operand[i+1]!='\0') error_code = 35;
		num = atoi(operand);
	}
	return num;
}

void copy_string_operand(char dest[],char source[], int *i, int *j){
	dest[*j] = source[*i];
	(*i)++; (*j)++;
	while(source[*i]!='\0'){
		dest[*j] = source[*i];                                                                       // Unconditionally copy contents of string operands
		(*i)++; (*j)++;
		if(source[*i-1]=='\''){
			if(source[*i]=='\'') (*i)++;
			else break;
		}
	}
}

char check_syntax(char token[],char operand_type){
	int i;
	switch(operand_type){
		case LABEL: // syntax checking for LABEL 
			i = 0;
			while(isalnum(token[i])) i++;
			if(token[i]==':'){
				if(isalpha(token[0])) return 1; 									// A label
				if(isdigit(token[0])){
					if(token[i-1]=='H') return 2;									// Hex label
					else if(token[i-1]=='D'||isdigit(token[i-1])) return 3;			// Decimal label
					else return 0;
				}
			}
			else return 0;
			break;
		case SYMBOL: // syntax checking for SYMBOL
			if(!isalpha(token[0])) return 0;
			i = 1;
			while(isalnum(token[i])) i++;
			if(token[i]!='\0') return 0;
			return 1;
			break;
	    default:
	   		break;
	}
	return 0;
}

char ishexdigit(char ch){
	if(isdigit(ch)||ch=='A'||ch=='B'||ch=='C'||ch=='D'||ch=='E'||ch=='F') return 1;
	else return 0;
}

char isregister(char ch[]){
	if(((ch[0]=='A'||ch[0]=='B'||ch[0]=='C'||ch[0]=='D'||ch[0]=='E'||ch[0]=='H'||ch[0]=='L'||ch[0]=='M')&&(ch[1]=='\0'||ch[1]==','))||strcmp(ch,"SP")==0||strcmp(ch,"PSW")==0){
		return 1;
	}
	else return 0;
}










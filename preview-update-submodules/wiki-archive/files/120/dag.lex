%{
#include <stdio.h>
#include <stdlib.h>

enum
{
	END_OF_FILE = 0,
	rw_JOB,
	rw_DIR,
	rw_DONE,
	rw_DATA,
	rw_SCRIPT,
	rw_PRE,
	rw_POST,
	rw_PARENT,
	rw_CHILD,
	rw_RETRY,
	rw_ABORT_DAG_ON,
	rw_UNLESS_EXIT,
	rw_RETURN,
	rw_VARS,
	rw_PRIORITY,
	rw_CATEGROY,
	rw_MAXJOBS,
	rw_CONFIG,
	rw_SUBDAG,
	rw_EXTERNAL,
	rw_SPLICE,
	rw_DOT,
	rw_UPDATE,
	rw_DONT_UPDATE,
	rw_OVERWRITE,
	rw_DONT_OVERWRITE,
	rw_INCLUDE,
	NUMBER,
	ASSIGN,
	NAME,
	ERROR,
};

char* convertstring(char *string);

%}

rw_JOB				[Jj][Oo][Bb]
rw_DIR				[Dd][Ii][Rr]
rw_DONE				[Dd][Oo][Nn][Ee]
rw_DATA				[Dd][Aa][Tt][Aa]
rw_SCRIPT			[Ss][Cc][Rr][Ii][Pp][Tt]
rw_PRE				[Pp][Rr][Ee]
rw_POST				[Pp][Oo][Ss][Tt]
rw_PARENT			[Pp][Aa][Rr][Ee][Nn][Tt]
rw_CHILD			[Cc][Hh][Ii][Ll][Dd]
rw_RETRY			[Rr][Ee][Tt][Rr][Yy]
rw_ABORT_DAG_ON		[Aa][Bb][Oo][Rr][Tt][-][Dd][Aa][Gg][-][Oo][Nn]
rw_UNLESS_EXIT		[Uu][Nn][Ll][Ee][Ss][-][Ee][Xx][Ii][Tt]
rw_RETURN			[Rr][Ee][Tt][Uu][Rr][Nn]
rw_VARS				[Vv][Aa][Rr][Ss]
rw_PRIORITY			[Pp][Rr][Ii][Oo][Rr][Ii][Tt][Yy]
rw_CATEGROY			[Cc][Aa][Tt][Ee][Gg][Oo][Rr][Yy]
rw_MAXJOBS			[Mm][Aa][Xx][Jj][Oo][Bb][Ss]
rw_CONFIG			[Cc][Oo][Nn][Ff][Ii][Gg]
rw_SUBDAG			[Ss][Uu][Bb][Dd][Aa][Gg]
rw_EXTERNAL			[Ee][Xx][Tt][Ee][Rr][Nn][Aa][Ll]
rw_SPLICE			[Ss][Pp][Ll][Ii][Cc][Ee]
rw_DOT				[Dd][Oo][Tt]
rw_UPDATE			[Uu][Pp][Dd][Aa][Tt][Ee]
rw_DONT_UPDATE		[Dd][Oo][Nn][Tt][-][Uu][Pp][Dd][Aa][Tt][Ee]
rw_OVERWRITE		[Oo][Vv][Ee][Rr][Ww][Rr][Ii][Tt][Ee]
rw_DONT_OVERWRITE	[Dd][Oo][Nn][Tt][-][Oo][Vv][Ee][Rr][Ww][Rr][Ii][Tt][Ee]
rw_INCLUDE			[Ii][Nn][Cc][Ll][Uu][Dd][Ee]

StringLiteral	["]([^"\\\n]|\\n|\\t|\\\\|\\\")*["]
StringLiteralWithLineBreaks ["]([^"\\]|\\n|\\t|\\\\|\\\")*["]
StringLiteralUnterminated   ["]([^"\\\n]|\\n|\\t|\\\\|\\\")*

Digit				[0-9]
Assign			[=]
Comment			[#].*

NameBare			[A-Za-z_0-9!@#$%^&*()=|<>,?:+/\\.~'`[\]-]+
Name				(({NameBare})|({StringLiteral}))

%%
" "						{ /* do nothing for now */ }
{rw_JOB}				{ return rw_JOB; }
{rw_DIR}				{ return rw_DIR; }
{rw_DONE}				{ return rw_DONE; }
{rw_DATA}				{ return rw_DATA; }
{rw_SCRIPT}				{ return rw_SCRIPT; }
{rw_PRE}				{ return rw_PRE; }
{rw_POST}				{ return rw_POST; }
{rw_PARENT}				{ return rw_PARENT; }
{rw_CHILD}				{ return rw_CHILD; }
{rw_RETRY}				{ return rw_RETRY; }
{rw_ABORT_DAG_ON}		{ return rw_ABORT_DAG_ON; }
{rw_UNLESS_EXIT}		{ return rw_UNLESS_EXIT; }
{rw_RETURN}				{ return rw_RETURN; }
{rw_VARS}				{ return rw_VARS; }
{rw_PRIORITY}			{ return rw_PRIORITY; }
{rw_CATEGROY}			{ return rw_CATEGROY; }
{rw_MAXJOBS}			{ return rw_MAXJOBS; }
{rw_CONFIG}				{ return rw_CONFIG; }
{rw_SUBDAG}				{ return rw_SUBDAG; }
{rw_EXTERNAL}			{ return rw_EXTERNAL; }
{rw_SPLICE}				{ return rw_SPLICE; }
{rw_DOT}				{ return rw_DOT; }
{rw_UPDATE}				{ return rw_UPDATE; }
{rw_DONT_UPDATE}		{ return rw_DONT_UPDATE; }
{rw_OVERWRITE}			{ return rw_OVERWRITE; }
{rw_DONT_OVERWRITE}		{ return rw_DONT_OVERWRITE; }
{rw_INCLUDE}			{ return rw_INCLUDE; }

{Comment}		{ /* do nothing */ }

{Digit}+		{
					printf("Found a number: %s\n", yytext);
					return NUMBER;
				}

{Assign}		{
					return ASSIGN;
				}

{Name}			{
					printf("Name: '%s'\n", convertstring(yytext));
					return NAME;
				}

{StringLiteralWithLineBreaks}	{ printf("ERROR_SLLB: '%s'\n", yytext); return ERROR; }
{StringLiteralUnterminated} 	{ printf("ERROR_SLUN: '%s'\n", yytext); return ERROR; }


"\t"			{ /* do nothing for now */ }
"\n"			{ /* do nothing for now */ }
.				{ printf("Unknown char: %s\n", yytext); exit(1); return ERROR; }

<<EOF>>			{ return END_OF_FILE; }

%%


/* Do a C style in place string conversion, this assumes the string is
	bounded by double quotes. returns a pointer to the string. */
char* convertstring(char *string)
{
	char *read, *write;

	/* Don't convert the string if it isn't a real string literal */
	if (string[0] != '"') {
		return string;
	}

	write = string;
	read = string + 1;

	/* ONE PASS! conversion of the literal string that converts
		each \t to a tab, \n to a newline, \" to a ", and \\
		to a \ character. The write pointer always moves ahead
		by one, while the read skips ahead doing analysis on
		the remaining string. */


	while(*read != '"')
	{
		switch (*read)
		{
			case '\\':
				read++;
				switch(*read)
				{
					case '\\':
						*write++ = '\\';
						break;
					case 't':
						*write++ = '\t';
						break;
					case 'n':
						*write++ = '\n';
						break;
					case '"':
						*write++ = '\"';
						break;
					default:
						printf("DAMN!\n");
						exit(EXIT_FAILURE);
						break;
				}
				read++;
				break;
			default:
				*write++ = *read++;
				break;
		}
	}
	*write = 0;

	return string;
}


void dump(int tok) 
{
	switch(tok)
	{
		case rw_JOB:
			printf("\tJOB\n");
			break;
		case rw_DIR:
			printf("\tDIR\n");
			break;
		case rw_DONE:
			printf("\tDONE\n");
			break;
		case rw_DATA:
			printf("\tDONE\n");
			break;
		case rw_SCRIPT:
			printf("\tSCRIPT\n");
			break;
		case rw_PRE:
			printf("\tPRE\n");
			break;
		case rw_POST:
			printf("\tPOST\n");
			break;
		case rw_PARENT:
			printf("\tPARENT\n");
			break;
		case rw_CHILD:
			printf("\tCHILD\n");
			break;
		case rw_RETRY:
			printf("\tRETRY\n");
			break;
		case rw_ABORT_DAG_ON:
			printf("\tABORT-DAG-ON\n");
			break;
		case rw_UNLESS_EXIT:
			printf("\tUNLESS-EXIT\n");
			break;
		case rw_RETURN:
			printf("\tRETURN\n");
			break;
		case rw_VARS:
			printf("\tVARS\n");
			break;
		case rw_PRIORITY:
			printf("\tPRIORITY\n");
			break;
		case rw_CATEGROY:
			printf("\tCATEGORY\n");
			break;
		case rw_MAXJOBS:
			printf("\tMAXJOBS\n");
			break;
		case rw_CONFIG:
			printf("\tCONFIG\n");
			break;
		case rw_SUBDAG:
			printf("\tSUBDAG\n");
			break;
		case rw_EXTERNAL:
			printf("\tEXTERNAL\n");
			break;
		case rw_SPLICE:
			printf("\tSPLICE\n");
			break;
		case rw_DOT:
			printf("\tDOT\n");
			break;
		case rw_UPDATE:
			printf("\tUPDATE\n");
			break;
		case rw_DONT_UPDATE:
			printf("\tDONT-UPDATE\n");
			break;
		case rw_OVERWRITE:
			printf("\tOVERWRITE\n");
			break;
		case rw_DONT_OVERWRITE:
			printf("\tDONT-OVERWRITE\n");
			break;
		case rw_INCLUDE:
			printf("\tINCLUDE\n");
			break;
		case NUMBER:
			printf("\tNUMBER\n");
			break;
		case NAME:
			printf("\tNAME\n");
			break;
		case ASSIGN:
			printf("\tASSIGN\n");
			break;
		case ERROR:
			printf("\tERROR\n");
			break;
		default:
			printf("WTF?\n");
			break;
		
	};
	return;
}

int main(int argc, char *argv[])
{
	int token;
	FILE *fin = NULL;

	if (argc != 2) {
		printf("Please supply a file to tokenize.\n");
		exit(EXIT_FAILURE);
	}

	fin = fopen(argv[1],"r");
	if (fin == NULL) {
		printf("Can't open file: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	yyin = fin;

	while( (token = yylex()) != END_OF_FILE ) {
		dump(token);
	}
}


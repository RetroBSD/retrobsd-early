#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <setjmp.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

//#define TRACE_TOKEN
//#define TRACE_PASS1

#define FALSE 0
#define TRUE 1

#define TT_EOF 1
#define TT_EOL 2
#define TT_DIRECTIVE 3
#define TT_LABEL 4
#define TT_REG 5
#define TT_WORD 6
#define TT_NUMBER 7
#define TT_STRING 8
#define TT_CHAR 9
#define TT_FUNC 10
#define TT_LP '('
#define TT_RP ')'
#define TT_PLUS '+'
#define TT_MINUS '-'
#define TT_TIMES '*'
#define TT_DIV '/'
#define TT_COMMA ','

// Parser errors - passed as value in longjmp
#define PE_TOKENLEN 1
#define PE_UNKNOWNCHAR 2
#define PE_EXPECTED_EOL 3

#define PE_UNKNOWN_OPCODE 4
#define PE_EXPECTED_COMMA 5
#define PE_EXPECTED_REG 6
#define PE_UNKNOWN_REG 7
#define PE_EXPECTED_CONST_NUMBER 8
#define PE_EXPECTED_RP 9
#define PE_EXPECTED_LP 10
#define PE_INTERNAL_ERROR 11
#define PE_SYCALL_INVALID 12
#define PE_UNKNOWN_DIRECTIVE 13
#define PE_INVALID_EXPRESSION 14
#define PE_UNKNOWN_SYMBOL 15
#define PE_DIVIDE_BY_ZERO 16
#define PE_EXPECTED_STRING 17
#define PE_STRING_NOT_CLOSED 18
#define PE_BRANCH_ALIGNMENT 19
#define PE_JUMP_OUT_OF_RANGE 20

// Fatal errors
#define FE_FILE_OPEN 1

static jmp_buf gFatalJmpBuf;
static jmp_buf gParseJmpBuf;

#define PARSE_ERROR( code ) gParseState.error = code; longjmp( gParseJmpBuf, code );
#define FATAL_ERROR() gParseState.error = 1; longjmp( gFatalJmpBuf, 1 );

#define TEXT_SECTION_ID 0
#define DATA_SECTION_ID 1
#define BSS_SECTION_ID 2

struct ParseState
{
	int error; 
	int errorCount;
	
	int pass;
	
	const char *filename;
	FILE* fp;
	
	// line and pos refer to line and pos of the current char c
	int   line;
	int   pos;
	int   c; 
	
	int tokenType;
	int tokenLen;
	int tokenLine;	// token starts on this line
	int tokenPos;   // token starts at this position
	char token[81];
	
} gParseState;

int NextChar()
{
	if( gParseState.c == EOF )
	{
		return EOF;
	}
	else if( gParseState.c == '\n' )
	{
		gParseState.line++;
		gParseState.pos = 1;
	}
	else
	{
		gParseState.pos++;
	}
	
	gParseState.c = fgetc( gParseState.fp );
	if( gParseState.c == EOF && ferror( gParseState.fp ) )
	{
		fprintf( stderr, "ERROR: Failure reading input file %s. errno: %d\n", gParseState.filename, errno );
		FATAL_ERROR();
	}
	
	return gParseState.c;
}

int NextToken()
{
	// skip white space
	while( isspace( gParseState.c ) && gParseState.c != '\n' )
	{
		NextChar();
	}
	
	if( gParseState.c == '#' )
	{
		while( gParseState.c != '\n' && gParseState.c != -1 )
		{
			NextChar();
		}
	}
	
	if( gParseState.c == EOF )
	{
		gParseState.tokenType = TT_EOF;
		gParseState.tokenLine = gParseState.line;
		gParseState.tokenPos = gParseState.pos;
	}
	else if( gParseState.c == '\n' )
	{
		gParseState.tokenType = TT_EOL;
		gParseState.tokenLine = gParseState.line;
		gParseState.tokenPos = gParseState.pos;
		
		NextChar();
	}
	else if( gParseState.c == '.' )
	{
		// directive token
		gParseState.tokenLine = gParseState.line;
		gParseState.tokenPos = gParseState.pos;

		gParseState.token[0] = gParseState.c;
		gParseState.tokenLen = 1;
		
		NextChar();
		while( isalpha( gParseState.c ) && gParseState.tokenLen < 80 )
		{
			gParseState.token[ gParseState.tokenLen++ ] = gParseState.c;
			NextChar();
		}
		
		if( gParseState.tokenLen == 80 )
		{
			#ifdef TRACE_TOKEN
			fputs("<ERR>",stderr);
			#endif
			PARSE_ERROR( PE_TOKENLEN );
		}
		
		gParseState.tokenType = TT_DIRECTIVE;
		gParseState.token[ gParseState.tokenLen ] = 0;
		#ifdef TRACE_TOKEN
		fputs("<DIR:",stderr);fputs(gParseState.token,stderr);fputs(">",stderr);
		#endif
	}
	else if( isalpha( gParseState.c ) || gParseState.c == '_' || gParseState.c == '$' || gParseState.c == '%' )
	{
		// alpha num token
		gParseState.tokenLine = gParseState.line;
		gParseState.tokenPos = gParseState.pos;
		
		gParseState.token[0] = gParseState.c;
		gParseState.tokenLen = 1;
		NextChar();
		while( (isalpha( gParseState.c ) || isdigit( gParseState.c ) || gParseState.c == '_' ) && gParseState.tokenLen < 80 )
		{
			gParseState.token[ gParseState.tokenLen++ ] = gParseState.c;
			NextChar();
		}
		
		if( gParseState.tokenLen == 80 )
		{
			#ifdef TRACE_TOKEN
			fputs("<ERR TOKENLEN>",stderr);
			#endif
			PARSE_ERROR( PE_TOKENLEN );
		}
		
		gParseState.token[ gParseState.tokenLen ] = 0;
		if( gParseState.token[0] == '$' )
		{
			gParseState.tokenType = TT_REG;
			#ifdef TRACE_TOKEN
			fputs("<REG:",stderr);fputs(gParseState.token,stderr);fputs(">",stderr);
			#endif
		}
		else if( gParseState.token[0] == '%' )
		{
			gParseState.tokenType = TT_FUNC;
			#ifdef TRACE_TOKEN
			fputs("<FUNC:",stderr);fputs(gParseState.token,stderr);fputs(">",stderr);
			#endif
		}
		else
		{
			if( gParseState.c == ':' )
			{
				gParseState.tokenType = TT_LABEL;
				NextChar();
				//#ifdef TRACE_TOKEN
				fputs("LABEL:",stdout);fputs(gParseState.token,stdout);fputs("\n",stdout);
				//#endif
			}
			else
			{
				gParseState.tokenType = TT_WORD;
				#ifdef TRACE_TOKEN
				fputs("<WORD:",stderr);fputs(gParseState.token,stderr);fputs(">",stderr);
				#endif
			}
		}
		return gParseState.tokenType;
	}
	else if( isdigit( gParseState.c ) )
	{
		// alpha num token
		gParseState.tokenLine = gParseState.line;
		gParseState.tokenPos = gParseState.pos;
		
		gParseState.token[0] = gParseState.c;
		gParseState.tokenLen = 1;
		NextChar();
		
		while( (isdigit( gParseState.c ) ) && gParseState.tokenLen < 80 )
		{
			gParseState.token[ gParseState.tokenLen++ ] = gParseState.c;
			NextChar();
		}
		
		if( gParseState.tokenLen == 80 )
		{
			#ifdef TRACE_TOKEN
			fputs("<ERR>",stderr);
			#endif
			PARSE_ERROR( PE_TOKENLEN );
		}
		
		gParseState.token[ gParseState.tokenLen ] = 0;
		gParseState.tokenType = TT_NUMBER;
		#ifdef TRACE_TOKEN
		fputs("<NUM:",stderr);fputs(gParseState.token,stderr);fputs(">",stderr);
		#endif
		return '.';
	}
	else if( gParseState.c == '-'
	         || gParseState.c == '+' 
	         || gParseState.c == '(' 
	         || gParseState.c == ')' 
	         || gParseState.c == ',' 
	         || gParseState.c == '*'
	         || gParseState.c == '/' )
	{
		gParseState.token[0] = gParseState.c;
		gParseState.token[1] = 0;
		gParseState.tokenLen = 1;
		gParseState.tokenType = gParseState.c;
		NextChar();
		#ifdef TRACE_TOKEN
		fputs("<",stderr);fputs(gParseState.token,stderr);fputs(">",stderr);
		#endif
		return '.';
	}
	else if( gParseState.c == '"' )
	{
		gParseState.token[0] = 0;
		gParseState.tokenLen = 0;
		gParseState.tokenType = TT_STRING;
		NextChar();
		#ifdef TRACE_TOKEN
		fputs("<STRSTART>",stderr);
		#endif
		return '.';
	}
	else if( gParseState.c == '\'' )
	{
		gParseState.token[0] = 0;
		gParseState.tokenType = TT_CHAR;
		#ifdef TRACE_TOKEN
		fputs("<CHAR>",stderr);
		#endif
	}
	else
	{
		#ifdef TRACE_TOKEN
		fputs("<ERR>",stderr);
		#endif
		PARSE_ERROR( PE_UNKNOWNCHAR );
	}
	
	return gParseState.tokenType;
}

struct SectionData
{
	int id;
	const char *name;
	FILE* fp;
	int used;
	int baseAddress;
	int usedPass2;
};

//struct SectionData TextSectionData;
//struct SectionData DataSectionData;
//struct SectionData BssSectionData;

struct SectionData gSections[4];
struct SectionData* gCurrentSection = 0;

void InitSectionData( struct SectionData* sd, int id, const char *name )
{
	sd->id = id;
	sd->name = name;
	sd->fp = 0;
	sd->used = 0;
	sd->baseAddress = 0x7f008000;
	sd->fp = fopen( sd->name, "w+" );
	fclose( sd->fp );
	sd->fp = 0;
}

void CleanupSectionData( struct SectionData* sd )
{
	if( sd->fp )
	{
		fclose( sd->fp );
		sd->fp = 0;
	}
}

void ActivateSection( struct SectionData* section )
{
	//fputs( "[[ActivateSection]]", stderr );
	
	if( gCurrentSection && gCurrentSection->fp )
	{
		//fputs( "[[closing]]", stderr );
		fclose( gCurrentSection->fp );
		gCurrentSection->fp = 0;
	}
	gCurrentSection = section;
	gCurrentSection->fp = fopen( gCurrentSection->name, "r+" );
	if( !gCurrentSection->fp )
	{
		fputs( "[[open failed]]", stderr );
		fprintf( stderr, "File of of %s failed\n", gCurrentSection->name );
		FATAL_ERROR();
	}
	//fputs( "[[open]]", stderr );
	fseek( gCurrentSection->fp, 0, L_XTND );
	//fputs( "[[seek]]", stderr );
}

void AddWordToCurrentSection( int word )
{
	if( gParseState.pass == 1 )
	{
		gCurrentSection->used += 4;
	}
	else
	{
		fprintf( stdout, "0x%08x  0x%08x\n", gCurrentSection->usedPass2+gCurrentSection->baseAddress, word );
		fwrite( &word, sizeof( word ), 1, gCurrentSection->fp );
		gCurrentSection->usedPass2 += 4;
	}
}

void AddByteToCurrentSection( int byte )
{
	if( gParseState.pass == 1 )
	{
		gCurrentSection->used += 1;
	}
	else
	{
		fprintf( stdout, "0x%08x  0x%02x\n", gCurrentSection->usedPass2+gCurrentSection->baseAddress, byte&0xff );
		fwrite( &byte, 1, 1, gCurrentSection->fp );
		gCurrentSection->usedPass2 += 1;
	}
}


struct SymbolEntry
{
	int isDefined;
	int sourceLine;
	int sectionId;
	int sectionOffset;
	
	struct SymbolEntry * next;
	 
	char name[ 20 ];
};

struct SymbolEntry* gSymbols = 0;

struct SymbolEntry* SymbolFind( const char *name )
{
	struct SymbolEntry* p = gSymbols;
	while( p )
	{
		if( strncmp( name, p->name, 16 ) == 0 )
		{
			return p;
		}
		p = p->next;
	}
	return 0;
}

struct SymbolEntry* SymbolCreate( const char *name )
{
	//fprintf( stderr, "[[SC1]]" );
	struct SymbolEntry* p = malloc( sizeof( struct SymbolEntry ) );
	if( !p )
	{
		fprintf( stderr, "Out of Memory" );
		FATAL_ERROR();
	}
	//fprintf( stderr, "[[SC2]]" );
	strncpy( p->name, name, 17 );
	p->name[16] = 0;
	p->isDefined = 0;
	//p->sourceLine = sourceLine;
	//p->sectionId = sectionId;
	//p->sectionOffset = sectionOffset;
	//fprintf( stderr, "[[SC3]]" );
	
	p->next = gSymbols;
	gSymbols = p;
	return p;
}

void SymbolDefine( struct SymbolEntry* p )
{
	p->isDefined = 1;
	p->sourceLine = gParseState.line;
	p->sectionId = gCurrentSection->id;
	p->sectionOffset = gCurrentSection->used;
}

void SymbolCleanup()
{
	struct SymbolEntry * p = gSymbols;
	gSymbols = 0;
	while( p )
	{
		struct SymbolEntry * dp = p;
		p = p->next;
		free( dp );
	}
}

void DumpSymbols()
{
	int hasUndefined = 0;
	fprintf( stdout, "\nDefined Symbols\n" );
	struct SymbolEntry* p = gSymbols;
	while( p )
	{
		if( p->isDefined )
		{
			fprintf( stdout, "  Line: %7d %s:0x%08x %s\n", p->sourceLine, gSections[p->sectionId].name, p->sectionOffset, p->name );
		}
		else
		{
			hasUndefined = 1;
		}
		p = p->next;
	}
	if( hasUndefined )
	{
		fprintf( stdout, "\nUndefined Symbols\n" );
		
		p = gSymbols;
		while( p )
		{
			if( !p->isDefined )
			{
				fprintf( stdout, "  %s\n", p->name );
			}
			p = p->next;
		}
	}
}

void RequireCommaToken()
{
	if( gParseState.tokenType != TT_COMMA )
	{
		PARSE_ERROR( PE_EXPECTED_COMMA );
	}
	NextToken();		
}




int EvalConst( int* needsComma )
{
	if( needsComma && *needsComma )
	{
		RequireCommaToken();
		*needsComma = 1;
	}
	
	//fputs( "[[EvalConst]]", stderr );
	int v = 0;
	int m = 0;
	if( gParseState.tokenType == TT_MINUS )
	{
		m = 1;
		NextToken();
	}
	if( gParseState.tokenType == TT_NUMBER )
	{
		v = atoi( gParseState.token );
		NextToken();
		if( m ) v = -v;
		return v;
	}
	
	PARSE_ERROR( PE_EXPECTED_CONST_NUMBER );
}

//#define TRACE_EVAL

int EvalF3()
{
	int minus = 0;
	int v = 0;
	if( gParseState.tokenType == TT_MINUS || gParseState.tokenType == TT_PLUS )
	{
		#ifdef TRACE_EVAL
		fprintf( stderr, "[EF3:+-:%s]\n", gParseState.token );
		#endif
		
		if( gParseState.tokenType == TT_MINUS )
		{
			minus = 1;
		}
		NextToken();
	}
	
	if( gParseState.tokenType == TT_LP )
	{
		#ifdef TRACE_EVAL
		fprintf( stderr, "[EF3:(:%s]\n", gParseState.token );
		#endif
		NextToken();
		v = EvalF1();
		if( gParseState.tokenType != TT_RP )
		{
			PARSE_ERROR( PE_EXPECTED_RP );
		}
		NextToken();
	}
	else if( gParseState.tokenType == TT_NUMBER )
	{
		#ifdef TRACE_EVAL
		fprintf( stderr, "[EF3:NUMBER:%s]\n", gParseState.token );
		#endif
		v = atoi( gParseState.token );
		NextToken();
	}
	else if( gParseState.tokenType == TT_WORD )
	{
		#ifdef TRACE_EVAL
		fprintf( stderr, "[EF3:WORD:%s]\n", gParseState.token );
		#endif
		if( gParseState.pass == 1 )
		{
			v = 1;
		}
		else
		{
			struct SymbolEntry* p = SymbolFind( gParseState.token );
			if( !p )
			{
				fprintf( stderr, "Unknown Symbol %s\n", gParseState.token );
				PARSE_ERROR( PE_UNKNOWN_SYMBOL );
			}
			v = p->sectionOffset + gSections[p->sectionId].baseAddress;
		}
		NextToken();
	}
	else
	{
		PARSE_ERROR( PE_INVALID_EXPRESSION );
	}
	
	if( minus ) v = -v;
	return v;
}

int EvalF2()
{
	int v = EvalF3();
	while( gParseState.tokenType == TT_TIMES || gParseState.tokenType == TT_DIV )
	{
		if( gParseState.tokenType == TT_TIMES )
		{
			NextToken();
			v = v * EvalF3();
		}
		else
		{
			NextToken();
			int v1 = EvalF3();
			if( v == 0 )
			{
				if( gParseState.pass == 1 )
				{
					v = 1;
				}
				else
				{
					PARSE_ERROR( PE_DIVIDE_BY_ZERO );
				}
			}
			v = v / EvalF3();
		} 
	}
	return v;
}

int EvalF1()
{
	int v = EvalF2();
	while( gParseState.tokenType == TT_MINUS || gParseState.tokenType == TT_PLUS )
	{
		if( gParseState.tokenType == TT_MINUS )
		{
			NextToken();
			v = v - EvalF2();
		}
		else
		{
			NextToken();
			v = v + EvalF2();
		} 
	}
	return v;
}

int EvalLabelExpression( int* needsComma )
{
	if( needsComma && *needsComma )
	{
		RequireCommaToken();
		*needsComma = 1;
	}

	int v;

	if( gParseState.tokenType == TT_WORD )
	{
		if( gParseState.pass == 1 )
		{
			v = 4;
		}
		else
		{
			struct SymbolEntry* p = SymbolFind( gParseState.token );
			if( !p )
			{
				fprintf( stderr, "Unknown Symbol %s\n", gParseState.token );
				PARSE_ERROR( PE_UNKNOWN_SYMBOL );
			}
			v = (p->sectionOffset + gSections[p->sectionId].baseAddress) - ( gCurrentSection->usedPass2 + gCurrentSection->baseAddress + 4 ) ;
			fprintf( stderr, "v = 0x%08x\n", v );
		}
		NextToken();
	}
	else
	{
		v = EvalF1();
		if( gParseState.pass == 1 )
		{
			v = 4;
		}
	}
	return v;	
}

int EvalLabelJExpression( int* needsComma )
{
	if( needsComma && *needsComma )
	{
		RequireCommaToken();
		*needsComma = 1;
	}

	int v;

	if( gParseState.tokenType == TT_WORD )
	{
		if( gParseState.pass == 1 )
		{
			v = 4;
		}
		else
		{
			struct SymbolEntry* p = SymbolFind( gParseState.token );
			if( !p )
			{
				fprintf( stderr, "Unknown Symbol %s\n", gParseState.token );
				PARSE_ERROR( PE_UNKNOWN_SYMBOL );
			}
			//v = (p->sectionOffset + gSections[p->sectionId].baseAddress) - ( gCurrentSection->usedPass2 + gCurrentSection->baseAddress + 4 ) ;
			int target = p->sectionOffset + gSections[p->sectionId].baseAddress;
			fprintf( stderr, "target=0x%08x\n", target );
			int nextPc = gCurrentSection->usedPass2 + gCurrentSection->baseAddress + 4;
			fprintf( stderr, "nextPc=0x%08x\n", nextPc );
			if( ( target & 0xf0000000 ) != ( nextPc & 0xf0000000 ) )
			{
				PARSE_ERROR( PE_JUMP_OUT_OF_RANGE );
			}
			if( target & 3 )
			{
				PARSE_ERROR( PE_BRANCH_ALIGNMENT );
			}
			v = ( target & 0x0fffffff );
			fprintf( stderr, "v = 0x%08x\n", v );
		}
		NextToken();
	}
	else
	{
		v = EvalF1();
		if( gParseState.pass == 1 )
		{
			v = 4;
		}
	}
	return v;	
}

int EvalExpression( int* needsComma )
{
	if( needsComma && *needsComma )
	{
		RequireCommaToken();
		*needsComma = 1;
	}
	
	int v = EvalF1();

	return v;
}


//int Dir_String()
//{
	//if( gParseState.tokenType != TT_STRINGSTART
//}

typedef void (*DirectiveFunc)();

typedef struct DirectiveEntryStruct
{
	const char *name;
	DirectiveFunc fp;
} DirectiveEntry;


void Directive_Text()
{
	ActivateSection( &gSections[TEXT_SECTION_ID] );
	NextToken();
}

void Directive_Ascii()
{
	NextToken();
	while(1)
	{
		if( gParseState.tokenType != TT_STRING )
		{
			fprintf( stderr, "Expect start quote" );
			PARSE_ERROR( PE_EXPECTED_STRING );
		}
		//NextChar();
		while( gParseState.c != '\n' && gParseState.c != '"' && gParseState.c != EOF )
		{
			if( gParseState.c == '\\' )
			{
				NextChar();
				if( gParseState.c == '\n' || gParseState.c == EOF )
				{
					PARSE_ERROR( PE_STRING_NOT_CLOSED );
				}
				if( gParseState.c == 'n' )
				{
					gParseState.c = '\n';
				}
				else if( gParseState.c == '0' )
				{
					gParseState.c = '\0';
				} 
			}
			AddByteToCurrentSection( gParseState.c );
			NextChar();
		}
		if( gParseState.c != '"' )
		{
			PARSE_ERROR( PE_STRING_NOT_CLOSED );
		}
		NextChar();
		NextToken();
		if( gParseState.tokenType != TT_COMMA )
		{
			break;
		}
		NextToken();
	}
}

DirectiveEntry gDirectiveTable[] =
{
	{".text", Directive_Text },
	{".ascii", Directive_Ascii },
	{0, 0 },
};

int Directive()
{
	DirectiveEntry* p = gDirectiveTable;
	while( p->name )
	{
		if( strcmp( gParseState.token, p->name ) == 0 )
		{
			(*p->fp)();
			return;
		}
		++p;
	}
	fprintf( stderr, "Unknown directive: %s", gParseState.token );
	PARSE_ERROR( PE_UNKNOWN_DIRECTIVE );
	
#if 0
	if( strcmp( gParseState.token, ".text" ) == 0 )
	{
		ActivateSection( &TextSectionData );
		NextToken();
	}
	else if( strcmp( gParseState.token, ".align" ) == 0 )
	{
		NextToken();
		int c = EvalConst( 0 );
	}
	else if( strcmp( gParseState.token, ".ascii" ) == 0 )
	{
		NextToken();
		Dir_String();
	}
#endif
}

int LabelCurrentAddress()
{
	if( gParseState.pass == 1 )
	{
		struct SymbolEntry* p = SymbolFind( gParseState.token );
		if( !p )
		{
			p = SymbolCreate( gParseState.token );
		}
		if( p->isDefined )
		{
			fprintf( stderr, "\nERROR - LINE: %d POS: %d MSG: Symbol %s already defined.", gParseState.tokenLine, gParseState.tokenPos, gParseState.token );
			gParseState.errorCount++;
		}
		else
		{
			SymbolDefine( p );
		}
	}
	
	NextToken();
}

struct RegTable
{
	const char *name;
	int value;
};

struct RegTable gRegTable[] = 
{
	{ "$0", 0 },
	{ "$1", 1 },
	{ "$2", 2 },
	{ "$3", 3 },
	{ "$4", 4 },
	{ "$5", 5 },
	{ "$6", 6 },
	{ "$7", 7 },
	{ "$8", 8 },
	{ "$9", 9 },
	{ "$10", 10 },
	{ "$11", 11 },
	{ "$12", 12 },
	{ "$13", 13 },
	{ "$14", 14 },
	{ "$15", 15 },
	{ "$16", 16 },
	{ "$17", 17 },
	{ "$18", 18 },
	{ "$19", 19 },
	{ "$20", 20 },
	{ "$21", 21 },
	{ "$22", 22 },
	{ "$23", 23 },
	{ "$24", 24 },
	{ "$25", 25 },
	{ "$26", 26 },
	{ "$27", 27 },
	{ "$28", 28 },
	{ "$29", 29 },
	{ "$30", 30 },
	{ "$31", 31 },

	{ "$zero", 0 },
	{ "$at", 1 },
	{ "$v0", 2 },
	{ "$v1", 3 },
	{ "$a0", 4 },
	{ "$a1", 5 },
	{ "$a2", 6 },
	{ "$a3", 7 },
	{ "$t0", 8 },
	{ "$t1", 9 },
	{ "$t2", 10 },
	{ "$t3", 11 },
	{ "$t4", 12 },
	{ "$t5", 13 },
	{ "$t6", 14 },
	{ "$t7", 15 },
	{ "$s0", 16 },
	{ "$s1", 17 },
	{ "$s2", 18 },
	{ "$s3", 19 },
	{ "$s4", 20 },
	{ "$s5", 21 },
	{ "$s6", 22 },
	{ "$s7", 23 },
	{ "$t8", 24 },
	{ "$t9", 25 },
	{ "$k0", 26 },
	{ "$k1", 27 },
	{ "$gp", 28 },
	{ "$sp", 29 },
	{ "$s8", 30 },
	{ "$fp", 30 },
	{ "$ra", 31 },
	{ 0, 0 }
	
};

int LookupReg()
{
	//fputs( "[[LookupReg]]", stderr );
	if( gParseState.tokenType != TT_REG )
	{
		PARSE_ERROR( PE_EXPECTED_REG );
	}
	
	struct RegTable* p = gRegTable;
	while(p->name)
	{
		if( strcmp( p->name, gParseState.token ) == 0 )
		{
			return p->value;
		}
		++p;
	}
	
	PARSE_ERROR( PE_UNKNOWN_REG );
}

struct	AOutHeader {
	unsigned a_magic;	/* magic number */
#define OMAGIC      0407        /* old impure format */

        unsigned a_text;	/* size of text segment */
        unsigned a_data;	/* size of initialized data */
        unsigned a_bss;		/* size of uninitialized data */
        unsigned a_syms;	/* size of symbol table */
        unsigned a_entry; 	/* entry point */
        unsigned a_unused;	/* not used */
        unsigned a_flag; 	/* relocation info stripped */
};

struct OpcodeTableEntry;

typedef int (*OpcodeFunc)( struct OpcodeTableEntry* te );

struct OpcodeTableEntry
{
	const char *name;
	int code;
	int func;
	int args_asked;
	int args_used_as;
	OpcodeFunc opcodeFunc; 
};

#define ARGS_REG_1 1
#define ARGS_REG_2 2
#define ARGS_REG_3 4
#define ARGS_IMMEDIATE 8
#define ARGS_REG_INDEX 16
#define ARGS_LABEL 32
#define ARGS_LABEL_J 64

#define ARGSR_RT 1
#define ARGSR_RS 2
#define ARGSR_RD 3
#define ARGSR_SA 4
#define ARGSR_IMMEDIATE 5

#define ARGS_REG_1_BIT_POS 0
#define ARGS_REG_2_BIT_POS 3
#define ARGS_REG_3_BIT_POS 6
#define ARGS_IMMEDIATE_BIT_POS 9
#define ARGS_REG_INDEX_BIT_POS 12


struct Args
{
	int reg1;
	int reg2;
	int reg3;
	int aimmediate;
	int aRegIndex;
	
	int rd;
	int rs;
	int rt;
	int sa;
	int immediate;
	
};

struct Args gArgs;


int GetRegArg( int* needsComma )
{
	if( needsComma && *needsComma )
	{
		RequireCommaToken();
	}
	
	int reg = LookupReg();
	NextToken();
	
	if( needsComma ) *needsComma = 1;
	
	return reg;	
}

void AssignArgsValue( int src, int used_as )
{
	switch( used_as )
	{
		case ARGSR_RT:
			gArgs.rt = src;
			break;
		case ARGSR_RS:
			gArgs.rs = src;
			break;
		case ARGSR_RD:
			gArgs.rd = src;
			break;
		case ARGSR_SA:
			gArgs.sa = src;
			break;
		case ARGSR_IMMEDIATE:
			gArgs.immediate = src;
			break;
		default:
			fprintf( stderr, "Internal Error - invalid used_as in AssignArgsValue()\n" );
			PARSE_ERROR( PE_INTERNAL_ERROR );
	}
}

void GetOpArgs( int args_asked, int args_used_as )
{
	#ifdef TRACE_PASS1
	fprintf( stderr, "[GetOpArgs aa: 0x%08x au: 0x%08x]\n", args_asked, args_used_as );
	#endif
	
	int anotherArgNeedsComma = 0;
	if( args_asked & ARGS_REG_1 )
	{
		#ifdef TRACE_PASS1
		fprintf( stderr, "[GetOpArgs-ask Reg1\n" );
		#endif
		gArgs.reg1 = GetRegArg( &anotherArgNeedsComma );
	}
	if( args_asked & ARGS_REG_2 )
	{
		gArgs.reg2 = GetRegArg( &anotherArgNeedsComma );
	}
	if( args_asked & ARGS_REG_3 )
	{
		gArgs.reg3 = GetRegArg( &anotherArgNeedsComma );
	}
	if( args_asked & ARGS_IMMEDIATE )
	{
		#ifdef TRACE_PASS1
		fprintf( stderr, "[GetOpArgs-ask immediate\n" );
		#endif
		gArgs.aimmediate = EvalExpression( &anotherArgNeedsComma);
	}
	if( args_asked & ARGS_REG_INDEX )
	{
		#ifdef TRACE_PASS1
		fprintf( stderr, "[GetOpArgs-ask reg index\n" );
		#endif
		if( gParseState.tokenType != TT_LP )
		{
			PARSE_ERROR( PE_EXPECTED_LP );
		}
		NextToken();
		
		gArgs.aRegIndex = GetRegArg( 0 );
		
		if( gParseState.tokenType != TT_RP )
		{
			PARSE_ERROR( PE_EXPECTED_RP );
		}
		NextToken();
	}	
	else if( args_asked & ARGS_LABEL )
	{
		#ifdef TRACE_PASS1
		fprintf( stderr, "[GetOpArgs-ask immediate\n" );
		#endif
		gArgs.aimmediate = EvalLabelExpression( &anotherArgNeedsComma );
		//fprintf( stderr, "gArgs.aimmediate=0x%08x\n", gArgs.aimmediate );
		if( gArgs.aimmediate & 0x3 )
		{
			PARSE_ERROR( PE_BRANCH_ALIGNMENT );
		}
		gArgs.aimmediate = (gArgs.aimmediate >> 2);
		//fprintf( stderr, "gArgs.aimmediate=0x%08x\n", gArgs.aimmediate );
	}
	
	else if( args_asked & ARGS_LABEL_J )
	{
		#ifdef TRACE_PASS1
		fprintf( stderr, "[GetOpArgs-ask immediate\n" );
		#endif
		gArgs.aimmediate = EvalLabelJExpression( &anotherArgNeedsComma );
		//fprintf( stderr, "gArgs.aimmediate=0x%08x\n", gArgs.aimmediate );
		gArgs.aimmediate = (gArgs.aimmediate >> 2);
		//fprintf( stderr, "gArgs.aimmediate=0x%08x\n", gArgs.aimmediate );
	}
	#ifdef TRACE_PASS1
	fprintf( stderr, "[GetOpArgs-done asking]\n");
	#endif

	gArgs.rd = 0;
	gArgs.rs = 0;
	gArgs.rt = 0;
	gArgs.sa = 0;
	gArgs.immediate = 0;
	
	if( args_asked & ARGS_REG_1 )
	{
		AssignArgsValue( gArgs.reg1, (args_used_as >> ARGS_REG_1_BIT_POS) & 0x7 ); 
	}
	if( args_asked & ARGS_REG_2 )
	{
		AssignArgsValue( gArgs.reg2, ( args_used_as >> ARGS_REG_2_BIT_POS ) & 0x7 );
	}
	if( args_asked & ARGS_REG_3 )
	{
		AssignArgsValue( gArgs.reg3, ( args_used_as >> ARGS_REG_3_BIT_POS ) & 0x7 );
	}
	if( args_asked & ARGS_IMMEDIATE || args_asked & ARGS_LABEL || args_asked & ARGS_LABEL_J)
	{
		AssignArgsValue( gArgs.aimmediate, ( args_used_as >> ARGS_IMMEDIATE_BIT_POS ) & 0x7 );
	}
	if( args_asked & ARGS_REG_INDEX )
	{
		AssignArgsValue( gArgs.aRegIndex, ( args_used_as >> ARGS_REG_INDEX_BIT_POS ) & 0x7 );
	}
	#ifdef TRACE_PASS1
	fprintf( stderr, "[GetOpArgs-done assigning]\n");
	#endif
}
 

int OpI( struct OpcodeTableEntry* te )
{
	#ifdef TRACE_PASS1
	fprintf( stderr, "[OpI code: 0x%08x aa: 0x%08x au: 0x%08x]\n", te->code, te->args_asked, te->args_used_as );
	#endif
	
	GetOpArgs( te->args_asked, te->args_used_as );
	//fprintf( stderr, "gArgs.immediate=0x%08x\n", gArgs.immediate );
	
	int code = ( te->code << 26 ) | ( gArgs.rs << 21 ) | ( gArgs.rt << 16 ) | ( te->func << 16 ) | ( gArgs.immediate & 0xffff );
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %08x\n", code );
}

int OpJ( struct OpcodeTableEntry* te )
{
	#ifdef TRACE_PASS1
	fprintf( stderr, "[OpJ code: 0x%08x aa: 0x%08x au: 0x%08x]\n", te->code, te->args_asked, te->args_used_as );
	#endif
	
	GetOpArgs( te->args_asked, te->args_used_as );
	//fprintf( stderr, "gArgs.immediate=0x%08x\n", gArgs.immediate );
	
	int code = ( te->code << 26 ) | ( gArgs.immediate & 0x0fffffff );
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %08x\n", code );
}

int OpR( struct OpcodeTableEntry* te )
{
	//fputs( "[[OpI]]", stderr );

	GetOpArgs( te->args_asked,  te->args_used_as );
	
	int code = ( te->code << 26 ) | ( gArgs.rs << 21 ) | ( gArgs.rt << 16 ) | ( gArgs.rd << 11 ) | ( gArgs.sa << 6 ) | te->func;
	
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %08x\n", code );
}

int OpMove( struct OpcodeTableEntry* te )
{
	//really addu rd, $zero, rs  100001

	GetOpArgs( te->args_asked,  te->args_used_as );
	
	int code = ( te->code << 26 ) | ( gArgs.rs << 21 ) | ( gArgs.rt << 16 ) | ( gArgs.rd << 11 ) | ( gArgs.sa << 6 ) | te->func;
	
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %08x\n", code );
}

int OpSyscall( struct OpcodeTableEntry* te )
{
	//really addu rd, $zero, rs  100001

	GetOpArgs( te->args_asked,  te->args_used_as );
	
	if( gArgs.immediate > 255 || gArgs.immediate < 0 )
	{
		fprintf( stderr, "SYSCALL value out of range." );
		PARSE_ERROR( PE_SYCALL_INVALID );
	}
	
	int code = ( te->code << 26 ) | ( gArgs.immediate << 6 ) | te->func;
	
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %08x\n", code );
}

int OpNop( struct OpcodeTableEntry* te )
{
	//NextToken();
	int code = 0;
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %08x\n", code );
	
}
 
int OpLi( struct OpcodeTableEntry* te )
{
	//fputs( "[[OpI]]", stderr );

	GetOpArgs( te->args_asked, te->args_used_as );

	int immediate = gArgs.immediate;
	int code;
	
	//if( immediate > 0xffff || immediate < 0xffff8000 )
	{
		// lui 
		code = ( 15 << 26 ) | ( gArgs.rt << 16 ) | ( ( immediate >> 16 ) & 0xffff ) ;
		AddWordToCurrentSection( code );
		//fprintf( stdout, "OP: %x\n", code );
		immediate = immediate & 0xffff;
	}
	
	// ori rt, rs

	code = ( 13 << 26 ) | ( gArgs.rt << 21 ) | ( gArgs.rt << 16 ) | ( immediate & 0xffff );
	
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %x\n", code );
}

int OpLa( struct OpcodeTableEntry* te )
{
	//fputs( "[[OpI]]", stderr );

	GetOpArgs( te->args_asked, te->args_used_as );

	int immediate = gArgs.immediate;
	int code;
	
	//if( immediate > 0xffff || immediate < 0xffff8000 )
	{
		// lui 
		code = ( 15 << 26 ) | ( gArgs.rt << 16 ) | ( ( immediate >> 16 ) & 0xffff ) ;
		AddWordToCurrentSection( code );
		//fprintf( stdout, "OP: %x\n", code );
		immediate = immediate & 0xffff;
	}
	
	// ori rt, rs

	code = ( 13 << 26 ) | ( gArgs.rt << 21 ) | ( gArgs.rt << 16 ) | ( immediate & 0xffff );
	
	AddWordToCurrentSection( code );
	//fprintf( stdout, "OP: %x\n", code );
}


struct OpcodeTableEntry gOpcodeTable[] =
{
	{ "addi",  8, 0, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // addi rt, rs, immediate 001000
		
	{ "addiu", 9, 0,
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // addiu rt, rs, immediate 001001
		
	{ "andi", 0xc, 0,
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // andi rt, rs, immediate 001100

	{ "beq", 4, 0,
		ARGS_REG_1 | ARGS_REG_2 | ARGS_LABEL, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // beq rs, rt, label 000100
		
	{ "bgtz", 7, 0,
		ARGS_REG_1 | ARGS_LABEL, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // bgtz rs, label 000111 rt=00000
	
	{ "blez", 6, 0,
		ARGS_REG_1 | ARGS_LABEL, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // blez rs, label 000110 rt=00000
	
	{ "bgez", 1, 1,
		ARGS_REG_1 | ARGS_LABEL, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // blez rs, label 000110 rt=00000
	
	{ "bltz", 1, 0,
		ARGS_REG_1 | ARGS_LABEL, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // bltz rs, label 000001 rt=00000
	
	{ "bne", 5, 0,
		ARGS_REG_1 | ARGS_REG_2 | ARGS_LABEL, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // bne rs, rt, label 000101

	{ "lb", 0x20, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // lb rt, immediate(rs)
		
	{ "lbu", 0x24, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // lbu rt, immediate(rs)

	{ "lh", 0x21, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // lh rt, immediate(rs)

	{ "lhu", 0x25, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // lhu rt, immediate(rs)
			  // 
	{ "lui",  15,  0, 
		ARGS_REG_1 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // lui rt, immediate 001111

	{ "lw", 0x23, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // lw rt, immediate(rs)

	{ "lwc1", 0x31, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // lwc1 rt, immediate(rs)

	{ "ori",  0xd, 0, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // ori rt, rs, immediate 001000
		
	{ "sb", 0x28, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // sb rt, immediate(rs)

	{ "slti",  0xa, 0, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // slti rt, rs, immediate 001000
		
	{ "sltiu",  0xb, 0, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // sltiu rt, rs, immediate 001000
		
	{ "sh", 0x29, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // sh rt, immediate(rs)

	{ "sw", 0x2b, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // sw rt, immediate(rs)

	{ "swc1", 0x39, 0,
		ARGS_REG_1 | ARGS_IMMEDIATE | ARGS_REG_INDEX, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ) | ( ARGSR_RS << ARGS_REG_INDEX_BIT_POS ) ,
		OpI }, // sw rt, immediate(rs)

	{ "xori",  0xe, 0, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpI }, // xori rt, rs, immediate 001000
		
	//// OpR
	{ "add",  0, 0x20, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // add rd, rs, rt

	{ "addu",  0, 0x21, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // addu rd, rs, rt

	{ "and",  0, 0x24, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // and rd, rs, rt

	{ "break",  0, 0x0d, 
		0, 
		0,
		OpR }, // break

	{ "div",  0, 0x1a, 
		ARGS_REG_1 | ARGS_REG_2, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ),
		OpR }, // div rs, rt

	{ "divu",  0, 0x1b, 
		ARGS_REG_1 | ARGS_REG_2, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ),
		OpR }, // divu rs, rt

	{ "jalr",  0, 0x09, 
		ARGS_REG_1 | ARGS_REG_2, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ),
		OpR }, // jalr rd, rs

	{ "jr",  0, 0x08, 
		ARGS_REG_1, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ),
		OpR }, // jr rs

	{ "mfhi",  0, 0x10, 
		ARGS_REG_1, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ),
		OpR }, // mfhi rd

	{ "mflo",  0, 0x12, 
		ARGS_REG_1, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ),
		OpR }, // mflo rd

	{ "mthi",  0, 0x11, 
		ARGS_REG_1, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ),
		OpR }, // mthi rs

	{ "mtlo",  0, 0x13, 
		ARGS_REG_1, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ),
		OpR }, // mtlo rs

	{ "mult",  0, 0x18, 
		ARGS_REG_1 | ARGS_REG_2, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ),
		OpR }, // mult rs, rt

	{ "multu",  0, 0x19, 
		ARGS_REG_1 | ARGS_REG_2, 
		( ARGSR_RS << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ),
		OpR }, // multu rs, rt

	{ "nor",  0, 0x27, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // nor rd, rs, rt

	{ "or",  0, 0x25, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // or rd, rs, rt

	{ "sll",  0, 0x00, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_SA << ARGS_REG_3_BIT_POS ),
		OpR }, // sll rd, rt, sa

	{ "sllv",  0, 0x04, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_RS << ARGS_REG_3_BIT_POS ),
		OpR }, // sllv rd, rt, rs

	{ "slt",  0, 0x2a, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // slt rd, rs, rt

	{ "sltu",  0, 0x2b, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // sltu rd, rs, rt

	{ "sra",  0, 0x03, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_SA << ARGS_REG_3_BIT_POS ),
		OpR }, // sra rd, rt, sa

	{ "srav",  0, 0x07, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_RS << ARGS_REG_3_BIT_POS ),
		OpR }, // srav rd, rt, rs

	{ "srl",  0, 0x02, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_SA << ARGS_REG_3_BIT_POS ),
		OpR }, // srl rd, rt, sa

	{ "srlv",  0, 0x06, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RT << ARGS_REG_2_BIT_POS ) | ( ARGSR_RS << ARGS_REG_3_BIT_POS ),
		OpR }, // srlv rd, rt, rs

	{ "sub",  0, 0x22, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // sub rd, rs, rt

	{ "subu",  0, 0x23, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // subu rd, rs, rt

	{ "syscall",  0,  12, 
		ARGS_IMMEDIATE, 
		( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpSyscall }, 

	{ "xor",  0, 0x26, 
		ARGS_REG_1 | ARGS_REG_2 | ARGS_REG_3, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ) | ( ARGSR_RT << ARGS_REG_3_BIT_POS ),
		OpR }, // xor rd, rs, rt

	// Psuedo
	{ "move",  0,  33, 
		ARGS_REG_1 | ARGS_REG_2, 
		( ARGSR_RD << ARGS_REG_1_BIT_POS ) | ( ARGSR_RS << ARGS_REG_2_BIT_POS ),
		OpMove }, // really addu rd, $zero, rs  100001

	{ "li",  0, 0, 
		ARGS_REG_1 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpLi }, 
		
	{ "la",  0, 0, 
		ARGS_REG_1 | ARGS_IMMEDIATE, 
		( ARGSR_RT << ARGS_REG_1_BIT_POS ) | ( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpLa }, 
		
	{ "j", 2, 0,
		ARGS_LABEL_J, 
		( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpJ }, // j label
		
	{ "jal", 3, 0,
		ARGS_LABEL_J, 
		( ARGSR_IMMEDIATE << ARGS_IMMEDIATE_BIT_POS ),
		OpJ }, // j label
		
	//{ "syscall",   0, 0xc, 0, OpSyscall },
		
	{ "nop", 	   0,   0, 0, 0, OpNop },
	{ 0, 		   0,   0, 0, 0, 0 }
} ;

void Op()
{
	//fputs( "[[Asm]]", stderr );
	struct OpcodeTableEntry* p = gOpcodeTable;
	while( p->name )
	{
		if( strcmp( p->name, gParseState.token ) == 0 )
		{
			NextToken();
			(*p->opcodeFunc)( p );
			return;
		}
		++p;
	}
	
	fprintf( stderr, "Unknown opcode %s\n", gParseState.token );
	PARSE_ERROR( PE_UNKNOWN_OPCODE );
}

int ParseLine()
{
	if( gParseState.tokenType == TT_LABEL )
	{
		LabelCurrentAddress();
		//NextToken();
	}
	switch( gParseState.tokenType )
	{
		case TT_DIRECTIVE:
			Directive();
			break;
			
		case TT_WORD:
			Op();
			break;
			
	}
	
	if( gParseState.tokenType != TT_EOL && gParseState.tokenType != TT_EOF )
	{
		//fprintf( stderr, "ERROR: Expected EOL" );
		PARSE_ERROR( PE_EXPECTED_EOL );
	}
}





void Initialize( const char * inputFilename )
{
	gParseState.filename = inputFilename;
	
	if( !(gParseState.fp = fopen( inputFilename, "r" ) ) )
	{
		fprintf( stderr, "ERROR: Unable to open %s. Errno: %d\n", gParseState.filename, errno );
		FATAL_ERROR();
	}
	
	gParseState.pass = 1;
	gParseState.line = 1;
	
	NextChar();
	
	InitSectionData( &gSections[TEXT_SECTION_ID], TEXT_SECTION_ID, ".text" );
	InitSectionData( &gSections[DATA_SECTION_ID], DATA_SECTION_ID, ".data" );

	//gCurrentSection = &gSections[TEXT_SECTION_ID];
	ActivateSection( &gSections[TEXT_SECTION_ID] );
	
}

void ReportParseError()
{
	fprintf( stderr, "\nERROR - LINE: %d POS: %d MSG: ", gParseState.tokenLine, gParseState.tokenPos );
	
	switch( gParseState.error )
	{
		case PE_TOKENLEN:
			fprintf( stderr, "Token too long." );
			break;
			
		case PE_UNKNOWNCHAR:
			fprintf( stderr, "Invalid char." );
			break;
			
		case PE_EXPECTED_EOL:
			fprintf( stderr, "Expected EOL." );
			break;
			
		case PE_UNKNOWN_OPCODE:
			fprintf( stderr, "Unknown opcode." );
			break;
			
		case PE_EXPECTED_COMMA:
			fprintf( stderr, "Expected comma." );
			break;
			
		case PE_EXPECTED_REG:
			fprintf( stderr, "Expected register." );
			break;
			
		case PE_UNKNOWN_REG:
			fprintf( stderr, "Unknown register." );
			break;
			
		case PE_EXPECTED_CONST_NUMBER:
			fprintf( stderr, "Expected const number" );
			break;
			
		case PE_EXPECTED_LP:
			fprintf( stderr, "Expected '('." );
			break;
			
		case PE_EXPECTED_RP:
			fprintf( stderr, "Expected ')'." );
			break;
			
		case PE_INTERNAL_ERROR:
			fprintf( stderr, "Internal Error." );
			break;
			
		case PE_SYCALL_INVALID:
			fprintf( stderr, "Sycall argument invalid." );
			break;
			
		case PE_UNKNOWN_DIRECTIVE:
			fprintf( stderr, "Uknown directive." );
			break;
			
		case PE_INVALID_EXPRESSION:
			fprintf( stderr, "Invalid expression." );
			break;
			
		case PE_UNKNOWN_SYMBOL:
			fprintf( stderr, "Unknown symbol." );
			break;
			
		case PE_DIVIDE_BY_ZERO:
			fprintf( stderr, "Divide by zero." );
			break;
			
		case PE_EXPECTED_STRING:
			fprintf( stderr, "Expected a string." );
			break;

		case PE_STRING_NOT_CLOSED:
			fprintf( stderr, "String not closed." );
			break;
		
		case PE_BRANCH_ALIGNMENT:
			fprintf( stderr, "Branch misalignment." );
			break;
			
		case PE_JUMP_OUT_OF_RANGE:
			fprintf( stderr, "Jump out of range." );
			break;
			
		default:
			fprintf( stderr, "Uknown Error." );
			break;
			
			
	}
	
	fprintf( stderr, "\n" );
}

void Pass1()
{
	while( gParseState.tokenType != TT_EOF )
	{
		if( !setjmp(gParseJmpBuf) )
		{
			NextToken();
			ParseLine();
		}
		else
		{
			ReportParseError();
			
			if( ++gParseState.errorCount > 10 )
			{
				fprintf( stderr, "Too many errors. Stopping.\n" );
				FATAL_ERROR();
			}
			else
			{
				// attempt to continue parsing by find the end of the
				// current line and resyncing
				
				while( gParseState.c != EOF && gParseState.c != '\n' )
				{
					NextChar();
				}
				NextChar();
			}
		}
	}
	
	DumpSymbols();
	
	if( gParseState.errorCount )
	{
		fprintf( stderr, "Errors during Pass 1. Stopping.\n" );
		FATAL_ERROR();
	}
	
}

void Pass2()
{
	fprintf( stdout, "Pass 2\n" );
	
	gParseState.pass = 2;
	ActivateSection( &gSections[TEXT_SECTION_ID] );
	gParseState.line = 1;
	gParseState.pos = 1;
	fseek( gParseState.fp, 0, L_SET );
	gParseState.c = ' ';
	gParseState.tokenType = 0;
	NextChar();
	
	while( gParseState.tokenType != TT_EOF )
	{
		if( !setjmp(gParseJmpBuf) )
		{
			NextToken();
			ParseLine();
		}
		else
		{
			ReportParseError();
			
			if( ++gParseState.errorCount > 10 )
			{
				fprintf( stderr, "Too many errors. Stopping.\n" );
				FATAL_ERROR();
			}
			else
			{
				// attempt to continue parsing by find the end of the
				// current line and resyncing
				
				while( gParseState.c != EOF && gParseState.c != '\n' )
				{
					NextChar();
				}
				NextChar();
			}
		}
	}
	
	if( gParseState.errorCount )
	{
		fprintf( stderr, "Errors during Pass 2. Stopping.\n" );
		FATAL_ERROR();
	}
}

void WriteOutput()
{
	fprintf( stderr, "Building executable a.out\n" );
	
	struct AOutHeader aoutHeader;
	
	aoutHeader.a_magic = OMAGIC;
	aoutHeader.a_text = gSections[TEXT_SECTION_ID].used;
	aoutHeader.a_data = gSections[TEXT_SECTION_ID].used;
	aoutHeader.a_bss = 0;
	aoutHeader.a_syms = 0;
	aoutHeader.a_entry = gSections[TEXT_SECTION_ID].baseAddress;
	aoutHeader.a_unused = 0;
	aoutHeader.a_flag = 1;
	
	FILE* aout = fopen( "a.out", "w+" );
	
	//fputs( "\n open a \n", stderr );

	fwrite( &aoutHeader, sizeof( aoutHeader ), 1, aout );
	
	//fputs( "\n write header  \n", stderr );
	
	ActivateSection( &gSections[TEXT_SECTION_ID] );
	fseek( gSections[TEXT_SECTION_ID].fp, 0, L_SET );
	int remaining = gSections[TEXT_SECTION_ID].used;
	while( remaining )
	{
		int read = sizeof( aoutHeader );
		if( read > remaining ) read = remaining;
		fread( &aoutHeader, 1, read, gSections[TEXT_SECTION_ID].fp );
		fwrite( &aoutHeader, 1, read, aout );
		remaining -= read;
	}
	//fputs( "\n copied text section  \n", stderr );
	
	fclose( aout );
	
	//fputs( "\n closed a  \n", stderr ); 
	
	chmod( "a.out", 0777 );
}

void Cleanup()
{
	//fputs( "\n Cleanup \n", stderr ); 
	
	if( gParseState.fp )
	{
		fclose( gParseState.fp );
	}
	
	SymbolCleanup();
	
	CleanupSectionData( &gSections[0] );
	CleanupSectionData( &gSections[1] );
	
}

int main( int argc, char**argv )
{
	if( argc < 2 )
	{
		fprintf( stderr, "Must supply assembly source file as first argument\n" );
		exit(1);
	}
	
	if( !setjmp(gFatalJmpBuf) )
	{
		Initialize( argv[1] );	
		Pass1();
		Pass2();
		WriteOutput();
	}
	
	Cleanup();
};

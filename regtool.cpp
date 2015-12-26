//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//
// Tool to import registry keys via command line
// 
//      Version: 0.97
// Date created: 2002-09-15
//     Modified: 2009-03-03
//       Author: Florian Muecke (dev[AT]mueckeimnetz.de)
//
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

/*
  ===========================
   Minimize executable size
  ===========================
  General:
  - use dynamic linking if possible (if "msvcr*.dll" is available 
    on your target system you are really lucky!)
  - try to avoid CRT functions (this is nearly impossible if you want to use
    real c++)
  - write your own short string cmp functions etc.
  - use Multibyte instead of Unicode 
  Compiler:
  - do not use cpp exceptions
  (- compile as C++ code (/TP))
  - optimize for size (/O1)
  - favor small code (/Os)
  - enable string pooling (/GF)
  - no buffer security check (/GS-)
  - dll runtime lib (/MD)
  - no RTTI (/GR-)
  Linker:
  - specify correct link libraries
  - do not generate debug info
  - do not create manifest
  - eleminate unreferenced data (/OPT:REF)
  - remove redundant COMDATs (/OPT:ICF)
  - define your own program entry point instead of using CRT entry point 
    (/ENTRY:[symbol])
  - merge code and data sections (/MERGE:.src=.dest, USE WITH CAUTION! 
    You may merge .rdata and .text because both are readonly data.)
  - reduce alignment size (/ALIGN:[count]; default is 512; USE WITH CAUTION!
    The linker will tell you if your alignment is too small.)
 References:
 - http://blogs.msdn.com/xiangfan/archive/2008/09/19/minimize-the-size-of-your-program-high-level.aspx
*/

//#include <stdlib.h>
//#include <malloc.h>
#include <windows.h>

// forward declarations
BOOL StrCmp( const char*, const char* );
void StdOut( const char* );
int hex2dw( const char* );
int szCatStr( char*, const char* );
int InitCmdLine();


// defines
#define POS_CMD			1
#define POS_HKEY		2
#define POS_SUBKEY		3
#define POS_VAL_NAME	4
#define POS_TYPE		5
#define POS_DATA		6
// will be used for static argument array size
#define MAX_ARGS		POS_DATA+1 

// GLOBAL VARIABLES (pftuuhhh!)
HANDLE std_out;
int argc = -1;
char delimiters_[3] = { ' ', '\'', '\"' };
const char* argv[MAX_ARGS];
const char* lpLf = "\n";
const char* lpBs = "\\";
const char* HelpText = 
	"\nRegTool v0.97 (c) 2002-2009 by Florian Muecke\n\n" \
	"usage: \"regtool.exe <ADD>    <HKEY> <SubKey> <ValueName> <Type> <Data>\"\n" \
    "       \"regtool.exe <DEL>    <HKEY> <SubKey> <ValueName>\"\n" \
    "       \"regtool.exe <DELKEY> <HKEY> <SubKey>\"  (quiet mode)\n\n" \
    "HKEY - must be one of those (HKCR, HKCU, HKLM, HKU)\n" \
    "SubKey - your desired SubKey\n" \
    "ValueName - the name of the desired value\n" \
    "Type - must be one of those (REG_DWORD, REG_NONE, REG_SZ, REG_BINARY)\n" \
    "Data - the Key Value\n\n" \
    "e.g. regtool.exe add HKLM Software\\RegTool TestString REG_SZ \"This is a test!\"\n" \
    "     regtool.exe add HKLM Software\\RegTool dword REG_DWORD FF0101FF\n";
            // REG_EXPAND_SZ, REG_MULTI_SZ, REG_RESOURCE_LIST

const char* MsgAdd = "adding value ";
const char* MsgDel = "removing value ";
const char* MsgDelKey = "removing key ";
const char* operations[] = { "ADD", "DEL", "DELKEY" };
#define OP_ADD		0
#define OP_DEL		1
#define OP_DELKEY	2
const char* hkeys[] = { "HKLM", "HKCU", "HKCR", "HKU" };
#define HKEYS_HKLM	0
#define HKEYS_HKCU	1
#define HKEYS_HKCR	2
#define HKEYS_HKU	3
const char* reg_types[] = { "REG_SZ", "REG_DWORD", "REG_NONE", "REG_BINARY" };
#define REG_TYPES_SZ		0
#define REG_TYPES_DWORD		1
#define REG_TYPES_NONE		2
#define REG_TYPES_BINARY	3
DWORD DataLen = 0;

void error( const char* message )
{
	StdOut( message );
	ExitProcess( 1 );
}

// =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
BOOL StrCmp( const char* lpSrc, const char* lpDst )
{
	BOOL equal = FALSE;
	if( *lpSrc == *lpDst ) 
		equal = TRUE;
	while( *lpSrc )
	{
		if( *lpSrc != *lpDst )
		{
			equal = FALSE;
			break;
		}
		++lpSrc;
		++lpDst;
	}
	return equal;
}


//---------------------------------------------------------------------
void StdOut( const char* lpszText )
{
	DWORD bytes_written=0;
	WriteFile( std_out, lpszText, lstrlen( lpszText ), &bytes_written, NULL );
}

//---------------------------------------------------------------------
int hex2dw ( const char* String )
{
  // -----------------------------------
  // Convert hex string into dword value
  // Return value
  // -----------------------------------
	int ret_val = 0;
	//__asm
	//{
	//	push ebx
	//	push esi
	//	push edi

	//	mov edi, String
	//	mov esi, String 

	//	ALIGN 4

	//  again:  
	//	mov al,[edi]
	//	inc edi
	//	or  al,al
	//	jnz again
	//	sub esi,edi
	//	xor ebx,ebx
	//	add edi,esi
	//	xor edx,edx
	//	not esi             ;esi = lenth

	//  .while esi != 0
	//	mov al, [edi]
	//	cmp al,'A'
	//	jb figure
	//	sub al,'a'-10
	//	adc dl;
	//	shl dl,5            ;if cf set we get it bl 20h else - 0
	//	add al,dl
	//	jmp next
	//  figure: 
	//	sub al,'0'
	//  next:  
	//	lea ecx,[esi-1]
	//	and eax, 0Fh
	//	shl ecx,2           ;mul ecx by log 16(2)
	//	shl eax,cl          ;eax * 2^ecx
	//	add ebx, eax
	//	inc edi
	//	dec esi
	//  .endw

	//	mov ret_val, ebx
	//}
	return ret_val;
}
//---------------------------------------------------------------------
int szCatStr( char* lpszSource, const char* lpszAdd )
{
	lstrlen( lpszSource );
	__asm 
	{
		mov edx, lpszSource
		mov ecx, lpszAdd
		add edx, eax

	  LOOPY:
		mov al, [ecx]
		mov [edx], al
		inc ecx
		inc edx
		test al, al       // test for zero
		jne LOOPY
	}
    return 0;
}



//---------------------------------------------------------------------
int __cdecl pureStart()
{
	const char* Cmd = "";
	const char* HKey = "";
	const char* lpSubKey = "";
	const char* ValueName = "";
	const char* Data = "";
	const BYTE* lpDataBuffer = NULL;
	const char* RegType = "";
	HKEY hkResult = NULL;
	DWORD dwDisposition = 0;
	DWORD RegTypeVal = 0;
	DWORD DataLength = 0;
	HKEY HKeyVal = NULL;
	char OutBuffer[512];
	int dwordVal;
	BYTE ByteVal;
	LSTATUS retVal;
	OutBuffer[0] = 0;

	std_out = GetStdHandle( STD_OUTPUT_HANDLE );

	if( InitCmdLine() < POS_SUBKEY )
		error( HelpText );

	Cmd = argv[POS_CMD];
	HKey = argv[POS_HKEY];
	lpSubKey = argv[POS_SUBKEY];
	if ( StrCmp( hkeys[HKEYS_HKLM], HKey ) ) HKeyVal = HKEY_LOCAL_MACHINE;
	else if ( StrCmp( hkeys[HKEYS_HKCU], HKey ) ) HKeyVal = HKEY_CURRENT_USER;
	else if ( StrCmp( hkeys[HKEYS_HKCR], HKey ) ) HKeyVal = HKEY_CLASSES_ROOT;
	else if ( StrCmp( hkeys[HKEYS_HKU], HKey ) ) HKeyVal = HKEY_USERS;
	else error( HelpText );
	// ADD ?
	if ( StrCmp( Cmd, operations[OP_ADD] ) )
	{
		if( argc <= POS_TYPE ) 
			error( HelpText );
		ValueName = argv[POS_VAL_NAME];
		RegType = argv[POS_TYPE];
		if( StrCmp( reg_types[REG_TYPES_SZ], RegType ) )
		{
			RegTypeVal = REG_SZ;
			Data = (argc<POS_DATA)? NULL: argv[POS_DATA];	// may be emty !!
			DataLength = (argc<POS_DATA)? 0 : lstrlen( Data ) + 1; // include 0
			lpDataBuffer = (const BYTE*)Data;
		}
		else
		if ( StrCmp( reg_types[REG_TYPES_DWORD], RegType ) )
		{
			RegTypeVal = REG_DWORD;
			DataLength = 4;
			dwordVal = (argc<POS_DATA)? 0 : hex2dw( argv[POS_DATA] );
			lpDataBuffer = (const BYTE*) &dwordVal;
		}
		else
		if ( StrCmp( reg_types[REG_TYPES_BINARY], RegType ) )
		{
			RegTypeVal = REG_BINARY;
			if( argc >= POS_DATA ) 
			{
				Data = argv[POS_DATA];	// may be empty !!
				DataLength = (lstrlen( Data ) + 1) /3; // +1 for simulated ending comma??
			}

			__asm
			{
				lea esi, Data
				lea edi, lpDataBuffer
			binloop:
				lodsb			// get first nibble of byte in al
				cmp al, '9'		// .IF al <= '9'
				jg ELSEIF1		//
				sub al, 0x30	//   sub al, 0x30
				jmp ENDIF1		//
			ELSEIF1:			//
				cmp al, 'F'		// .ELSEIF al <= 'F'
				jg ELSE1		//
				sub al, 0x37	//   sub al, 0x37
				jmp ENDIF1		//
			ELSE1:				// .ELSE
				sub al, 0x57	//   sub al, 0x57
			ENDIF1:				// .ENDIF
				mov bl, 16
				mul bl
				mov ByteVal, al
				lodsb			// get second nibble of byte in al
				cmp al, '9'		// .IF al <= '9'
				jg ELSEIF2		//
				sub al, 0x30	//   sub al, 0x30
				jmp ENDIF2		//
			ELSEIF2:			//
				cmp al, 'F'		// .ELSEIF al <= 'F'
				jg ELSE2		//
				sub al, 0x37	//   sub al, 0x37
				jmp ENDIF2		//
			ELSE2:				// .ELSE
				sub al, 0x57	//   sub al, 0x57
			ENDIF2:				// .ENDIF
				add ByteVal, al
				mov al, ByteVal
				stosb
				inc esi				// skip comma
				inc DataLen
				mov eax, DataLength
				cmp DataLen, eax	// .IF DataLen < eax
				jl binloop			//   jmp binloop
									// .ENDIF
			}//asm
		}//REG_BINARY
		else
		if ( StrCmp( reg_types[REG_TYPES_NONE], RegType ) )
		{
			RegTypeVal = REG_NONE;
			DataLength = 0;
		}
		else
		{
			error( HelpText );
		}
		retVal = RegCreateKeyEx( 
			HKeyVal,		// hKey
			lpSubKey,		// lpSubKey (must not be NULL)
			0,				// reserved (must be 0)
			NULL,			// lpClass (can be NULL)
			0,				// dwOptions (REG_OPTION_NON_VOLATILE=0, REG_OPTION_BACKUP_RESTORE, REG_OPTION_VOLATILE)
			KEY_ALL_ACCESS, // samDesiredAccess
			NULL,			// lpSecurityAttributes (may be NULL)
			&hkResult,		// ptr to variable that will receive the created hkey
			&dwDisposition	// ptr to var that will receive the disposition values
			); 
		if( retVal != ERROR_SUCCESS )
		{
			error( "\nerror creating registry key\n" );
		}
		RegSetValueEx( hkResult, ValueName, 0, RegTypeVal, lpDataBuffer, DataLength );
		RegCloseKey( hkResult );

		// preparing output message
		szCatStr( OutBuffer, MsgAdd );
		szCatStr( OutBuffer, HKey );
		szCatStr( OutBuffer, lpBs );
		szCatStr( OutBuffer, lpSubKey );
		szCatStr( OutBuffer, lpBs );
		szCatStr( OutBuffer, ValueName );
		szCatStr( OutBuffer, "=" );
		szCatStr( OutBuffer, (const char*)lpDataBuffer );
		szCatStr( OutBuffer, lpLf );
		StdOut( OutBuffer );
	}
	else
	// DEL ?
	if ( StrCmp( Cmd, operations[OP_DEL] ) )
	{
		ValueName = argv[POS_VAL_NAME];
		RegCreateKeyEx( HKeyVal, lpSubKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkResult, &dwDisposition );
		RegDeleteValue( hkResult, ValueName );
		RegCloseKey( hkResult );
		// prepare output message
		szCatStr( OutBuffer, MsgDel );
		szCatStr( OutBuffer, HKey );
		szCatStr( OutBuffer, lpBs );
		szCatStr( OutBuffer, lpSubKey );
		szCatStr( OutBuffer, lpBs );
		szCatStr( OutBuffer, ValueName );
		szCatStr( OutBuffer, lpLf );
		StdOut( OutBuffer );
	}
	else
	// DELKEY ?
	if ( StrCmp( Cmd, operations[OP_DELKEY] ) )
	{
		RegDeleteKey( HKeyVal, lpSubKey );
		// prepare output message
		//szCatStr( OutBuffer, MsgDelKey );
		//szCatStr( OutBuffer, HKey );
		//szCatStr( OutBuffer, lpBs );
		//szCatStr( OutBuffer, SubKey );
		//szCatStr( OutBuffer, lpLf );
		//StdOut( OutBuffer );
	}
	else
	{
		error( HelpText );
	}
	return 0;
}

int InitCmdLine()
{
	char* lpCommandLine = GetCommandLine();

	//--------------------------------------------------
	// iterate through command line and count arguments
	// first argument is the program name itself
	//--------------------------------------------------
	char* pos = lpCommandLine;
	while( *pos )
	{
		char delimiter = delimiters_[0]; // ' '
		if( *pos == delimiter ) 
		{
			// skip spaces until "*pos" is the next char
			// or end of string
			while( *pos == delimiter )
			{
				*pos = 0x00; // zero delimiter
				++pos;
			}
			continue;
		}
		if( *pos == delimiters_[1] ) // '\"'
		{
			delimiter = delimiters_[1];
			*pos = 0x00; // zero delimiter
			++pos;
		}
		else
		if( *pos == delimiters_[2] ) // '\''
		{
			delimiter = delimiters_[2];
			*pos = 0x00; // zero delimiter
			++pos;
		}

		// Iterate until next delimiter or end of string.
		// "*pos" must be the first char after the delimiter!
		++argc; // we have a new arg
		while( *pos != delimiter )
		{
			if( *pos == 0x00 )
				break;
			++pos;
		}
		if( *pos == 0x00 )
		{
			if( delimiter != delimiters_[0] ) // check for space
				return -1;
			break;
		}
		*pos = 0x00; // zero delimiter
		++pos;
	}
	//--------------------
	// Parse command line
	//--------------------
	{
		int i = -1;
		argc = (argc > MAX_ARGS)? MAX_ARGS : argc;
		while( i++ < argc )
		{
			while( *lpCommandLine == 0 )
				++lpCommandLine;
			argv[i] = lpCommandLine;
			while( *lpCommandLine != 0 )
				++lpCommandLine;
		}
	}
	return argc;
}
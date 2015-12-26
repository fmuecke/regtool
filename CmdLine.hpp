/********************************************************************
	created:	2009/03/05
	created:	5:3:2009   10:03
	filename: 	CmdLine.hpp
	author:		Florian Muecke
	
	purpose:	
*********************************************************************/
#pragma once

#include <windows.h>
#include <boost/noncopyable.hpp>

class CmdLine : boost::noncopyable
{
public:
	#pragma warning(disable:4351)
	CmdLine()
		: argc_(-1)
		, argv_(NULL)
		, delimiters_()
	{
		delimiters_[0] = ' ';
		delimiters_[1] = '\'';
		delimiters_[2] = '\"';
	}
	#pragma warning(default:4351)   // C4351 re-enabled

	virtual ~CmdLine()
	{
		if( argv_ != NULL )
			delete[] argv_; 
	}
	enum CmdLnParseResult
	{
		CmdLnParseResult_Ok = 0
		, CmdLnParseResult_NoArgs
		, CmdLnParseResult_NonMatchingDelimiter
	};
	CmdLnParseResult Init()
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
			++argc_; // we have a new arg
			while( *pos != delimiter )
			{
				if( *pos == 0x00 )
				{
					if( delimiter == delimiters_[0] ) // check for space
						break;
					return CmdLnParseResult_NonMatchingDelimiter;
				}
				++pos;
			}
			*pos = 0x00; // zero delimiter
			++pos;
		}
		
		//-------------------
		// create argv array
		//-------------------
		argv_ = new const char*[argc_+1];
		
		//--------------------
		// Parse command line
		//--------------------
		int i = -1;
		while( i++ < argc_ )
		{
			while( *lpCommandLine == 0 )
				++lpCommandLine;
			argv_[i] = lpCommandLine;
			while( *lpCommandLine != 0 )
				++lpCommandLine;
		}
		return CmdLnParseResult_Ok;
	}

	/*!
	Will return 0 if command line has no arguments.
	However, GetArg(0) will return the program name.
	*/
	int GetCount() { return argc_; }
	const char* GetArg( unsigned int pos )
	{
		//TODO: assert( argc_ >= 0 );
		if( argc_ < (int) pos )
		{
			return argv_[0];
		}
		return argv_[pos];
	}
private: 
	int argc_;
	char delimiters_[3];
	const char** argv_;
};

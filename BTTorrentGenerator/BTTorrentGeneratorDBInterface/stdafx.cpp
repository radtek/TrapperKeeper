// stdafx.cpp : source file that includes just the standard includes
// BTTorrentGeneratorDBInterface.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"


//
//
//
const char *uitoa(unsigned int nNum, char *pBuf)
{
	if( pBuf == NULL )
		return NULL;
	
	int i = 0;
	char digit = 0;

	digit = ( ( nNum / 1000000000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 100000000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 10000000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 1000000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 100000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 10000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 1000 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 100 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 10 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}
    
	digit = ( ( nNum / 1 ) % 10 ) + 0x30;
	if( digit != '0' || ( digit == '0' && i != 0 ) )
	{
		pBuf[i++] = digit;
	}

	pBuf[i++] = '\0';

	return pBuf;
}

//
// Converts a set of two hex characters into the integer version
//
unsigned char htoi(char left, char right)
{
	// IF left == 0 to 9
	if( left > 0x29 && left < 0x40 )
	{
		// IF right == 0 to 9
		if( right > 0x29 && right < 0x40 )
		{
			return (16 * (left & 0x0f)) + (right & 0x0f);
		}
		else if( ( right >= 'a' && right <= 'f' ) || ( right >= 'A' && right <= 'F' ) )
		{
			return (16 * (left & 0x0f)) + (right & 0x0f) + 9;
		}
	}
	else if( ( left >= 'a' && left <= 'f' ) || ( left >= 'A' && left <= 'F' ) )
	{
		if( right > 0x29 && right < 0x40 )
		{
			return (16 * ( (left & 0x0f) + 9 ) ) + (right & 0x0f);
		}
		else if( ( right >= 'a' && right <= 'f' ) || ( right >= 'A' && right <= 'F' ) )
		{
			return (16 * ( (left & 0x0f) + 9 ) ) + (right & 0x0f) + 9;
		}
	}

	return 0;
}

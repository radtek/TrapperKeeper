#include "StdAfx.h"
#include "uint128.h"

using namespace Kademlia;

CUInt128::CUInt128()
{
	setValue((ULONG)0);
}

CUInt128::CUInt128(bool fill)
{
	if( fill )
	{
		m_data[0] = (ULONG)-1;
		m_data[1] = (ULONG)-1;
		m_data[2] = (ULONG)-1;
		m_data[3] = (ULONG)-1;
	}
	else
		setValue((ULONG)0);
}

CUInt128::CUInt128(ULONG value)
{
	setValue(value);
}

CUInt128::CUInt128(const byte *valueBE)
{
	setValueBE(valueBE);
}

CUInt128::CUInt128(const CUInt128 &value, UINT numBits)
{
	// Copy the whole ULONGs
	UINT numULONGs = numBits / 32;
	for (UINT i=0; i<numULONGs; i++)
		m_data[i] = value.m_data[i];

	// Copy the remaining bits
	for (UINT i=(32*numULONGs); i<numBits; i++)
		setBitNumber(i, value.getBitNumber(i));

	// Pad with random bytes (Not seeding based on time to allow multiple different ones to be created in quick succession)
	for (UINT i=numBits; i<128; i++)
		setBitNumber(i, (rand()%2));
}

CUInt128& CUInt128::setValue(const CUInt128 &value)
{
	m_data[0] = value.m_data[0];
	m_data[1] = value.m_data[1];
	m_data[2] = value.m_data[2];
	m_data[3] = value.m_data[3];
	return *this;
}

CUInt128& CUInt128::setValue(ULONG value)
{
	m_data[0] = 0;
	m_data[1] = 0;
	m_data[2] = 0;
	m_data[3] = value;
	return *this;
}

CUInt128& CUInt128::setValueBE(const byte *valueBE)
{
	setValue((ULONG)0);
	for (int i=0; i<16; i++)
		m_data[i/4] |= ((ULONG)valueBE[i]) << (8*(3-(i%4)));
	return *this;
}

CUInt128& CUInt128::setValueRandom(void)
{
	//AutoSeededRandomPool rng;
	//byte randomBytes[16];
	//rng.GenerateBlock(randomBytes, 16);
	//setValueBE( randomBytes );
	byte random_bytes[16];
	for(int i=0;i<16;i++)
		random_bytes[i]=(byte)rand();
	setValueBE(random_bytes);

	return *this;
}

CUInt128& CUInt128::setValueGUID(void)
{
	setValue((ULONG)0);
	GUID guid;
	if (CoCreateGuid(&guid) != S_OK)
		return *this;
	m_data[0] = guid.Data1;
	m_data[1] = ((ULONG)guid.Data2) << 16 | guid.Data3;
	m_data[2] = ((ULONG)guid.Data4[0]) << 24 | ((ULONG)guid.Data4[1]) << 16 | ((ULONG)guid.Data4[2]) << 8 | ((ULONG)guid.Data4[3]);
	m_data[3] = ((ULONG)guid.Data4[4]) << 24 | ((ULONG)guid.Data4[5]) << 16 | ((ULONG)guid.Data4[6]) << 8 | ((ULONG)guid.Data4[7]);
	return *this;
}

UINT CUInt128::getBitNumber(UINT bit) const
{
	if (bit > 127)
		return 0;
	int ulongNum = bit / 32;
	int shift = 31 - (bit % 32);
	return ((m_data[ulongNum] >> shift) & 1);
}

CUInt128& CUInt128::setBitNumber(UINT bit, UINT value) 
{
	int ulongNum = bit / 32;
	int shift = 31 - (bit % 32);
	m_data[ulongNum] |= (1 << shift);
	if (value == 0)
		m_data[ulongNum] ^= (1 << shift);
	return *this;
}

CUInt128& CUInt128::xor(const CUInt128 &value)
{
	for (int i=0; i<4; i++)
		m_data[i] ^= value.m_data[i];
	return *this;
}

ULONG CUInt128::calcDistance(const CUInt128 &value)
{
	return m_data[0]^value.m_data[0];
}


CUInt128& CUInt128::xorBE(const byte *valueBE)
{
	CUInt128 temp(valueBE);
	return xor(temp);
}

void CUInt128::toHexString(CString *str) const
{
	str->SetString((""));
	CString element;
	for (int i=0; i<4; i++)
	{
		element.Format(("%08X"), m_data[i]);
		str->Append(element);
	}
}

void CUInt128::toBinaryString(CString *str, bool trim) const
{
	str->SetString((""));
	CString element;
	int b;
	for (int i=0; i<128; i++)
	{
		b = getBitNumber(i);
		if ((!trim) || (b != 0))
		{
			element.Format(("%d"), b);
			str->Append(element);
			trim = false;
		}
	}
	if (str->GetLength() == 0)
		str->SetString(("0"));
}

#if defined(_M_IX86) && (_MSC_FULL_VER > 13009037)
#pragma intrinsic(_byteswap_ulong)
#endif
void CUInt128::toByteArray(byte *b) const
{
#if defined(_M_IX86) && (_MSC_FULL_VER > 13009037)
	((uint32*)b)[0] = _byteswap_ulong(m_data[0]);
	((uint32*)b)[1] = _byteswap_ulong(m_data[1]);
	((uint32*)b)[2] = _byteswap_ulong(m_data[2]);
	((uint32*)b)[3] = _byteswap_ulong(m_data[3]);
#else
	for (int i=0; i<16; i++)
		b[i] = (byte)(m_data[i/4] >> (8*(3-(i%4))));
#endif
}

int CUInt128::compareTo(const CUInt128 &other) const
{
	for (int i=0; i<4; i++) 
	{
	    if (m_data[i] < other.m_data[i])
			return -1;
	    if (m_data[i] > other.m_data[i])
			return 1;
	}
	return 0;
}

int CUInt128::compareTo(ULONG value) const
{
	if ((m_data[0] > 0) || (m_data[1] > 0) || (m_data[2] > 0) || (m_data[3] > value))
		return 1;
	if (m_data[3] < value)
		return -1;
	return 0;
}

CUInt128& CUInt128::add(const CUInt128 &value)
{
	if (value == 0)
		return *this;
	__int64 sum = 0;
	for (int i=3; i>=0; i--)
	{
		sum += m_data[i];
		sum += value.m_data[i];
		m_data[i] = (ULONG)sum;
		sum = sum >> 32;
	}
	return *this;
}

CUInt128& CUInt128::add(ULONG value)
{
	if (value == 0)
		return *this;
	CUInt128 temp(value);
	add(temp);
	return *this;
}

CUInt128& CUInt128::subtract(const CUInt128 &value)
{
	if (value == 0)
		return *this;
	__int64 sum = 0;
	for (int i=3; i>=0; i--)
	{
		sum += m_data[i];
		sum -= value.m_data[i];
		m_data[i] = (ULONG)sum;
		sum = sum >> 32;
	}
	return *this;
}

CUInt128& CUInt128::subtract(ULONG value)
{
	if (value == 0)
		return *this;
	CUInt128 temp(value);
	subtract(temp);
	return *this;
}

CUInt128& CUInt128::shiftLeft(UINT bits)
{
    if ((bits == 0) || (compareTo(0) == 0))
        return *this;
	if (bits > 127)
	{
		setValue((ULONG)0);
		return *this;
	}

	ULONG result[] = {0,0,0,0};
	int indexShift = (int)bits / 32;
	__int64 shifted = 0;
	for (int i=3; i>=indexShift; i--)
	{
		shifted += ((__int64)m_data[i]) << (bits % 32);
		result[i-indexShift] = (ULONG)shifted;
		shifted = shifted >> 32;
	}
	for (int i=0; i<4; i++)
		m_data[i] = result[i];
	return *this;
}
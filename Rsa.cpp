// RSA (version 2)
// Author: Tom Weatherhead
// Started: June 20, 2000


#include <stdlib.h>
#include <string.h>		// For memcpy, memset.
#include <stdio.h>


#define USE_IX86_PENTIUM


// Put in "types.h"
typedef void					VOID;
typedef VOID *				PVOID;
typedef bool					BOOL;
typedef int						INT;
typedef long					INT32;
typedef INT32 *				PINT32;
typedef unsigned long UINT32;
typedef UINT32 *			PUINT32;

#undef TRUE
#define TRUE true

#undef FALSE
#define FALSE false


// Put in "bignum.h"
static const UINT32 gunBigNumMaxSize = 128;
static const UINT32 gunBigNumMaxBits = 32 * gunBigNumMaxSize;


class TBigNum;
typedef class TBigNum *			PTBigNum;

class TBigNum		// Make this a template class based on UINT32?
{
private:
	UINT32 m_aunBuf[gunBigNumMaxSize];
	UINT32 m_unLength;	// Number of significant UINT32s.
	//UINT32 m_unBits;		// Length (not buf size) in bits.

public:
	// Constructor.
	TBigNum( VOID );

	// Basic operations.
	VOID SetToZero( VOID );
	VOID Copy( PTBigNum pNum2 );
	VOID Compact( VOID );
	VOID ClearHighDWORDs( VOID );
	BOOL GreaterThanOrEqualTo( PTBigNum pNum2 );
	UINT32 NumBits( VOID );
	BOOL BitTest( UINT32 unBitNum );
	VOID SetToDWORD( UINT32 unNum );
	BOOL Set( PUINT32 punSrc, UINT32 unSrcLength );
	VOID Read( VOID );
	VOID Print( VOID );

	// Shift operations.
	BOOL ShiftLeft( UINT32 unShiftAmount );
	VOID ShiftRight( UINT32 unShiftAmount );

	// Arithmetic operations.
	VOID Add( PTBigNum pNum2 );
	BOOL Subtract( PTBigNum pNum2 );
	VOID Multiply( PTBigNum pNum2, PTBigNum pNumTemp );
	VOID Modulo( PTBigNum pNum2, PTBigNum pNumTemp );
	VOID ExpModulo( PTBigNum pNum2, PTBigNum pNum3,
									PTBigNum pNumTemp, PTBigNum pNumTemp2 );
};


// **** General Functions ****


VOID Assert( BOOL b )
{

	if( !b )
	{
		__asm int 3
	}
}


INT32 MaxInt32( INT32 a, INT32 b )
{
	return( ( a > b ) ? a : b );
}


INT32 MinInt32( INT32 a, INT32 b )
{
	return( ( a < b ) ? a : b );
}


// **** TBigNum Member Functions ****


TBigNum::TBigNum( VOID )
{
	SetToZero();
}


VOID TBigNum::SetToZero( VOID )
{
	m_unLength = 0;
	Assert( sizeof( m_aunBuf ) == 4 * gunBigNumMaxSize );
	memset( (PVOID)m_aunBuf, 0, sizeof( m_aunBuf ) );
}


VOID TBigNum::Copy( PTBigNum pNum2 )
{
	SetToZero();
	memcpy( m_aunBuf, pNum2->m_aunBuf, 4 * pNum2->m_unLength );
	m_unLength = pNum2->m_unLength;
}


VOID TBigNum::Compact( VOID )
{

	while( m_unLength > 0  &&  m_aunBuf[m_unLength - 1] == 0 )
	{
		m_unLength--;
	}
}


VOID TBigNum::ClearHighDWORDs( VOID )
{
	Assert( m_unLength <= gunBigNumMaxSize );
	memset( &m_aunBuf[m_unLength], 0, 4 * ( gunBigNumMaxSize - m_unLength ) );
}


BOOL TBigNum::GreaterThanOrEqualTo( PTBigNum pNum2 )
{
	const UINT32 unLength1 = m_unLength;
	const UINT32 unLength2 = pNum2->m_unLength;
	PUINT32 pun1;
	PUINT32 pun2;
	UINT32 i;

	if( unLength2 > unLength1 )
	{
		return( FALSE );
	}
	else if( unLength1 > unLength2  ||  unLength1 == 0 )
	{
		return( TRUE );
	}

	pun1 = &m_aunBuf[unLength1 - 1];
	pun2 = &pNum2->m_aunBuf[unLength2 - 1];

	for( i = unLength1; i > 0; i-- )
	{
		const UINT32 un1 = *pun1--;
		const UINT32 un2 = *pun2--;
		
		if( un1 > un2 )
		{
			return( TRUE );
		}
		else if( un2 > un1 )
		{
			return( FALSE );
		}
	}

	return( TRUE );
}


BOOL TBigNum::BitTest( UINT32 unBitNum )
{
#ifdef USE_IX86_PENTIUM
	const PUINT32 punSrc = m_aunBuf;
	UINT32 unRtn;

	if( unBitNum >= m_unLength * 32 )	// ie. >= m_unBits.
	{
		return( FALSE );
	}

	__asm
	{
		mov esi, punSrc
		mov ecx, unBitNum
		bt [esi], ecx
		sbb eax, eax
		mov unRtn, eax
	}

	return( ( unRtn != 0 ) ? TRUE : FALSE );
#else
	const UINT32 unMask = 1 << (unBitNum % 32);

	if( unBitNum >= m_unLength * 32 )	// ie. >= m_unBits.
	{
		return( FALSE );
	}

	if( m_aunBuf[unBitNum / 32] & unMask )
	{
		return( TRUE );
	}

	return( FALSE );
#endif
}


VOID TBigNum::SetToDWORD( UINT32 unNum )
{
	SetToZero();

	if( unNum > 0 )
	{
		m_aunBuf[0] = unNum;
		m_unLength = 1;
	}
}


BOOL TBigNum::Set( PUINT32 punSrc, UINT32 unSrcLength )
{
	SetToZero();

	if( unSrcLength > gunBigNumMaxSize )
	{
		return( FALSE );
	}

	memcpy( m_aunBuf, punSrc, 4 * unSrcLength );
	m_unLength = unSrcLength;
	Compact();			// Eat any leading zeros.
	return( TRUE );
}


VOID TBigNum::Read( VOID )
{
	UINT32 unNumDWORDs = 0;
	UINT32 i;

	SetToZero();
	printf( "\nNum DWORDs: " );
	fflush( stdout );
	scanf( "%u", &unNumDWORDs );

	for( i = 0; i < unNumDWORDs; i++ )
	{
		printf( "DWORD %u (in lowercase hex): ", i );
		fflush( stdout );
		scanf( "%x", &m_aunBuf[i] );
	}

	printf( "\n" );
	m_unLength = unNumDWORDs;
	Compact();
}


VOID TBigNum::Print( VOID )
{
	UINT32 i;

	printf( "\nNum DWORDs: %u\n", m_unLength );

	for( i = m_unLength; i > 0; i-- )
	{
		printf( "%08x  ", m_aunBuf[i - 1] );
	}

	printf( "\n\n" );
	//fflush( stdout );
}


VOID TBigNum::Add( PTBigNum pNum2 )
{
	const PUINT32 pBuf1 = m_aunBuf;
	const PUINT32 pBuf2 = pNum2->m_aunBuf;
	const INT nIterations = ( m_unLength > pNum2->m_unLength ) ? m_unLength : pNum2->m_unLength;
	INT nLastCarry;

	// This depends on the unused high DWORDs being zero.
	ClearHighDWORDs();
	pNum2->ClearHighDWORDs();

	__asm
	{
		mov ecx, nIterations
		xor edx, edx
		test ecx, ecx
		jz Label001

		mov edi, pBuf1
		mov esi, pBuf2

Label000:
		mov eax, [esi]						; Doesn't modify flags.
		mov ebx, [edi]
		neg edx										; Set carry bit iff edx != 0 (eg. if edx == -1).
		adc ebx, eax
		sbb edx, edx							; Preserve the carry bit; fill edx with it.
		mov [edi], ebx
		add esi, 4
		add edi, 4
		; dec trashes the carry bit, but we need dec to set the z flag for the jnz.
		dec ecx
		jnz Label000

Label001:
		mov nLastCarry, edx
	}

	m_unLength = nIterations;

	if( nLastCarry )
	{
		Assert( m_unLength < gunBigNumMaxSize );
		m_aunBuf[m_unLength++] = 1;
	}
	else
	{
		// For debugging only.
		Compact();
	}
}


BOOL TBigNum::Subtract( PTBigNum pNum2 )
{
	const UINT32 unIterations = m_unLength;
	const PUINT32 pBuf1 = m_aunBuf;
	const PUINT32 pBuf2 = pNum2->m_aunBuf;

	if( !GreaterThanOrEqualTo( pNum2 ) )
	{
		// Subtraction would lead to a negative result.
		//Assert( FALSE );
		return( FALSE );
	}
	else if( pNum2->m_unLength == 0 )
	{
		return( FALSE );
	}

	Assert( unIterations > 0 );

	__asm
	{
		mov ecx, unIterations
		xor edx, edx

		mov edi, pBuf1
		mov esi, pBuf2

		; Eat any low-order zeros from the source.

Label000:
		cmp [esi], edx			; Hijack edx for a minute.
		jnz Label001
		add esi, 4
		add edi, 4
		dec ecx
#if 1
		jnz Label000
			int 3
#endif
		jmp Label000

		; Do actual subtractions.
		; edx is still zero.
Label001:
			mov eax, [esi]						; Doesn't modify flags.
			mov ebx, [edi]
			neg edx										; Set carry bit iff edx != 0 (eg. if edx == -1).
			sbb ebx, eax
			sbb edx, edx							; Preserve the carry bit; fill edx with it.
			mov [edi], ebx
			add esi, 4
			add edi, 4
			; dec trashes the carry bit, but we need dec to set the z flag for the jnz.
			dec ecx
			jnz Label001

#if 1
		test edx, edx
		jz Label002
			int 3											; The last subtraction yielded a carry!
Label002:
#endif
	}

	Compact();
	return( TRUE );
}


VOID TBigNum::Multiply( PTBigNum pNum2, PTBigNum pNumTemp )
{
	const UINT32 unTotalLength = m_unLength + pNum2->m_unLength;
	const INT32 nMaxi = (INT32)unTotalLength - 2;
	PUINT32 punDst = &pNumTemp->m_aunBuf[0];
	INT32 i;

	if( m_unLength == 0 )
	{
		return;		// Multiplication by zero.
	}
	else if( pNum2->m_unLength == 0 )
	{
		SetToZero();
		return;
	}

	Assert( unTotalLength >= 2 );
	Assert( unTotalLength < gunBigNumMaxSize );

	pNumTemp->SetToZero();

	for( i = 0; i <= nMaxi; i++ )
	{
		const INT32 nMinj = MaxInt32( 0, i - pNum2->m_unLength + 1 );
		const INT32 nMaxj = MinInt32( i, m_unLength - 1 );
		const PUINT32 punSrc1 = &m_aunBuf[nMinj];
		const PUINT32 punSrc2 = &pNum2->m_aunBuf[i - nMinj];
		const INT32 nIterations = nMaxj - nMinj + 1;

		Assert( nIterations > 0 );
		Assert( nMinj >= 0 );
		Assert( nMinj + nIterations - 1 < (INT32)m_unLength );
		Assert( i - nMinj < (INT32)pNum2->m_unLength );
		Assert( i - nMinj - ( nIterations - 1 ) >= 0 );

		__asm
		{
			mov ecx, nIterations
			mov esi, punSrc1
			mov edi, punSrc2
			mov ebx, punDst

Label000:
			mov eax, [esi]
			add esi, 4
			mov edx, [edi]
			sub edi, 4
			mul edx														; Unsigned multiply.
			add [ebx], eax
			adc [ebx + 4], edx
			adc [ebx + 8], 0
			dec ecx
			jnz Label000
		}

		punDst++;
	}

	pNumTemp->m_unLength = unTotalLength;
	pNumTemp->Compact();
	Copy( pNumTemp );
}


UINT32 TBigNum::NumBits( VOID )
{
#if 0
	return( m_unBits );
#else
	UINT32 unHead;
	UINT32 unMask;
	UINT32 unNumBits;

	if( m_unLength == 0 )
	{
		return( 0 );
	}

	unHead = m_aunBuf[m_unLength - 1];
	Assert( unHead > 0 );
	// Use bit scanning asm instruction?
	unMask = 1 << 31;
	unNumBits = 32 * m_unLength;

#ifdef USE_IX86_PENTIUM
	unNumBits -= 32;

	__asm
	{
		bsr eax, unHead
#if 1
		jnz Label000
		int 3
Label000:
#endif
		inc eax
		add unNumBits, eax
	}
#else

	while( ( unHead & unMask ) == 0 )
	{
		unMask >>= 1;
		unNumBits--;
		//Assert( unMask > 0 );
	}
#endif

	return( unNumBits );
#endif
}


BOOL TBigNum::ShiftLeft( UINT32 unShiftAmount )
{
	const UINT32 unOldNumBits = NumBits();
	const UINT32 unNewNumBits = unOldNumBits + unShiftAmount;
	const UINT32 unOldNumDWORDs = m_unLength;
	const PUINT32 punSrcHead = &m_aunBuf[(unOldNumBits - 1) / 32];
	const PUINT32 punDstHead = &m_aunBuf[(unNewNumBits - 1) / 32];
	const PUINT32 punBufStart = m_aunBuf;
	const UINT32 unShiftAmountInDWORDs = unShiftAmount / 32;
	const UINT32 unLocalLeftShiftAmount = unShiftAmount % 32;

	if( unOldNumBits == 0  ||  unShiftAmount == 0 )
	{
		return( TRUE );		// Nothing to do.
	}
	else if( unNewNumBits > gunBigNumMaxSize )
	{
		Assert( FALSE );
		return( FALSE );
	}

	if( unShiftAmount % 32 == 0 )
	{
		const UINT32 unOldNumUINT32s = m_unLength;
		PUINT32 punSrc = &m_aunBuf[unOldNumUINT32s - 1];
		PUINT32 punDst = punSrc + unShiftAmountInDWORDs;
		UINT32 i;

		for( i = unOldNumUINT32s; i > 0; i-- )
		{
			*punDst-- = *punSrc--;
		}

		for( i = unShiftAmountInDWORDs; i > 0; i-- )
		{
			*punDst-- = 0;
		}

		m_unLength += unShiftAmountInDWORDs;
		return( TRUE );
	}

	__asm
	{
		mov esi, punSrcHead
		; mov edi, punDstHead
		mov edx, unOldNumDWORDs		; Number of source DWORDs to read.
		mov ecx, unLocalLeftShiftAmount
		mov eax, unShiftAmountInDWORDs
		lea edi, [esi + 4 * eax + 4]

		xor eax, eax

#if 1
		; Unroll one iteration for conditional write.
		mov ebx, [esi]
		sub esi, 4
		shld eax, ebx, cl
		test eax, eax
		jz Label002
			mov [edi], eax
Label002:
		sub edi, 4
		mov eax, ebx
		dec edx
		jz Label001
#endif

Label000:
			mov ebx, [esi]
			sub esi, 4
			shld eax, ebx, cl
			mov [edi], eax
			sub edi, 4
			mov eax, ebx
			dec edx
			jnz Label000

#if 1
Label001:
#endif
		; Write the last (partial) DWORD.
		shl eax, cl
		mov [edi], eax

		; Zero-fill the vacated low-order DWORDs.
		xor eax, eax
		mov edi, punBufStart
		mov ecx, unShiftAmountInDWORDs
		rep stosd
	}

	m_unLength = (INT32)( ( unNewNumBits + 31 ) / 32 );
	return( TRUE );
}


VOID TBigNum::ShiftRight( UINT32 unShiftAmount )
{
	const UINT32 unOldNumBits = NumBits();
	const UINT32 unNewNumBits = unOldNumBits - unShiftAmount;
	const UINT32 unShiftAmountInDWORDs = unShiftAmount / 32;
	const UINT32 unLocalRightShiftAmount = unShiftAmount % 32;
	const UINT32 unNewNumDWORDs = ( unNewNumBits + 31 ) / 32;
	const UINT32 unBufEraseAmount = m_unLength - unNewNumDWORDs;
	const PUINT32 punSrcTail = m_aunBuf + unShiftAmountInDWORDs;
	const PUINT32 punDstTail = m_aunBuf;

	if( unOldNumBits == 0  ||  unShiftAmount == 0 )
	{
		return;		// Nothing to do.
	}
	else if( unShiftAmount >= unOldNumBits )
	{
		SetToZero();
		return;
	}

	Assert( m_unLength >= unShiftAmountInDWORDs );

	if( unShiftAmount % 32 == 0 )
	{
		const UINT32 unOldNumUINT32s = m_unLength;
		UINT32 i;
		PUINT32 punDst = m_aunBuf;
		PUINT32 punSrc = punDst + unShiftAmountInDWORDs;

		for( i = unShiftAmountInDWORDs; i < m_unLength; i++ )
		{
			*punDst++ = *punSrc++;
		}

		for( i = 0; i < unShiftAmountInDWORDs; i++ )
		{
			*punDst++ = 0;
		}

		m_unLength -= unShiftAmountInDWORDs;
		return;
	}

	__asm
	{
		mov esi, punSrcTail
		mov edi, punDstTail
		mov edx, unNewNumDWORDs
		mov ecx, unLocalRightShiftAmount

		mov eax, [esi]		; The lower half of the shrd quantity.
		add esi, 4

		; dec edx
		; jz Label002

Label000:
			mov ebx, [esi]
			add esi, 4
			shrd eax, ebx, cl
			mov [edi], eax
			add edi, 4
			mov eax, ebx
			dec edx
			jnz Label000

#if 0
; Label002:
		test eax, eax
		jnz Label001
			; Write the last (partial) DWORD.
			shr eax, cl
			mov [edi], eax
			add edi, 4

Label001:
#endif

		; Zero-fill the vacated low-order DWORDs.
		xor eax, eax
		; mov edi, punBufEraseStart
		mov ecx, unBufEraseAmount
		rep stosd
	}

	m_unLength = unNewNumDWORDs;
}


VOID TBigNum::Modulo( PTBigNum pNum2, PTBigNum pNumTemp )
{
	const INT32 nShiftAmount = (INT32)NumBits() - (INT32)pNum2->NumBits();
	const UINT32 unShiftAmount = (UINT32)nShiftAmount;
	UINT32 i;

	if( nShiftAmount < 0 )
	{
		return;		// Already done.
	}

	pNumTemp->Copy( pNum2 );
	pNumTemp->ShiftLeft( unShiftAmount );

	for( i = unShiftAmount + 1; i > 0; i-- )
	{
		// Subtract will only execute if thisNum >= NumTemp.
		Subtract( pNumTemp );
		pNumTemp->ShiftRight( 1 );
	}
}


// Calculate (ThisNum to the power of Num2) mod Num3.

VOID TBigNum::ExpModulo( PTBigNum pNum2, PTBigNum pNum3,
												 PTBigNum pNumTemp, // Hold the intermediate result
												 PTBigNum pNumTemp2 )	// Used for multiplication.
{
	UINT32 unNumBits2 = pNum2->NumBits();

	// We could skip this by copying ThisNum to NumTemp and
	// skipping the MSBit of the exponent.
	pNumTemp->SetToDWORD( 1 );	// Set NumTemp to 1;

	while( unNumBits2-- > 0 )
	{
		pNumTemp->Multiply( pNumTemp, pNumTemp2 );	// Square NumTemp.
		pNumTemp->Modulo( pNum3, pNumTemp2 );				// NumTemp %= Num3.

		if( pNum2->BitTest( unNumBits2 ) )	// unNumBits2 was decremented above.
		{
			pNumTemp->Multiply( this, pNumTemp2 );		// NumTemp *= ThisNum.
			pNumTemp->Modulo( pNum3, pNumTemp2 );			// NumTemp %= Num3.
		}
	}

	Copy( pNumTemp );
}


INT main( VOID )
{
	PTBigNum pNumTemp1 = new TBigNum;
	PTBigNum pNumTemp2 = new TBigNum;
	PTBigNum pNumTemp3 = new TBigNum;

	if( pNumTemp1 == NULL  ||  pNumTemp2 == NULL  ||
			pNumTemp3 == NULL )
	{
		goto cleanup;
	}

	// Unit testing...
	printf( "Testing modulo.\n" );
	pNumTemp1->Read();
	pNumTemp2->Read();
	pNumTemp1->Modulo( pNumTemp2, pNumTemp3 );
	pNumTemp1->Print();

cleanup:

	if( pNumTemp1 != NULL )
	{
		delete pNumTemp1;
		pNumTemp1 = NULL;
	}

	if( pNumTemp2 != NULL )
	{
		delete pNumTemp1;
		pNumTemp2 = NULL;
	}

	if( pNumTemp3 != NULL )
	{
		delete pNumTemp1;
		pNumTemp3 = NULL;
	}

	printf( "Press a key and then Enter to end\n" );

	while( getchar() == 10 )
	{
	}

	return( 0 );
}


// **** End of File ****

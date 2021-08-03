//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef BITBOARDS_H
#define BITBOARDS_H

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "types.h"

extern U64 BB_SINGLE[64];
extern U64 BB_DIR[64][8];
extern U64 BB_BETWEEN[64][64];

extern U64 BB_PAWN_ATTACKS[64][2];
extern U64 BB_KNIGHT_ATTACKS[64];
extern U64 BB_BISHOP_ATTACKS[64];
extern U64 BB_ROOK_ATTACKS[64];
extern U64 BB_QUEEN_ATTACKS[64];
extern U64 BB_KING_ATTACKS[64];

extern U64 BB_HORIZONTAL[8];
extern U64 BB_VERTICAL[8];

extern U64 BB_FIRST_HORIZONTAL[2];
extern U64 BB_SECOND_HORIZONTAL[2];
extern U64 BB_THIRD_HORIZONTAL[2];
extern U64 BB_FOURTH_HORIZONTAL[2];
extern U64 BB_FIFTH_HORIZONTAL[2];
extern U64 BB_SIXTH_HORIZONTAL[2];
extern U64 BB_SEVENTH_HORIZONTAL[2];
extern U64 BB_EIGHTH_HORIZONTAL[2];

// #undef FAST_BITBOARDS

inline int CountBits(U64 b)
{
#ifdef FAST_BITBOARDS
	#ifdef __GNUC__
		return __builtin_popcountll(b);
	#endif
	#ifdef _MSC_VER
		#ifdef _WIN64
			return (int)__popcnt64(b);
		#else
			return __popcnt((U32)b) + __popcnt((U32)(b >> 32));
		#endif
	#endif
#endif

	if (b == 0)
		return 0;

	static const U64 mask_1 = LL(0x5555555555555555);   // 0101 0101 0101 0101 0101 0101 0101 0101 ...
	static const U64 mask_2 = LL(0x3333333333333333);   // 0011 0011 0011 0011 0011 0011 0011 0011 ...
	static const U64 mask_4 = LL(0x0f0f0f0f0f0f0f0f);   // 0000 1111 0000 1111 0000 1111 0000 1111 ...
	static const U64 mask_8 = LL(0x00ff00ff00ff00ff);   // 0000 0000 1111 1111 0000 0000 1111 1111 ...

	U64 x = (b & mask_1) + ((b >> 1) & mask_1);
	x = (x & mask_2) + ((x >> 2) & mask_2);
	x = (x & mask_4) + ((x >> 4) & mask_4);
	x = (x & mask_8) + ((x >> 8) & mask_8);

	U32 y = U32(x) + U32(x >> 32);
	return (y + (y >> 16)) & 0x3f;
}
////////////////////////////////////////////////////////////////////////////////

inline FLD LSB(U64 b)
{
	assert(b != 0);

#ifdef FAST_BITBOARDS
	#ifdef __GNUC__
		return 64 - __builtin_ffsll(b);
	#endif
	#ifdef _MSC_VER
		#ifdef _WIN64
			unsigned long i = 0;
			if (_BitScanForward64(&i, b))
				return (FLD)(63 - i);
		#else
			unsigned long i = 0;
			if (_BitScanForward(&i, (U32)b))
				return (FLD)(63 - i);
			else if (_BitScanForward(&i, (U32)(b >> 32)))
				return (FLD)(31 - i);
		#endif
	#endif
#endif

	static const U32 mult = 0x78291acf;
	static const FLD table[64] =
	{
		 0, 33, 60, 31,  4, 49, 52, 30,
		 3, 39, 13, 54,  8, 44, 42, 29,
		 2, 34, 61, 10, 12, 40, 22, 45,
		 7, 35, 62, 20, 17, 36, 63, 28,
		 1, 32,  5, 59, 58, 14,  9, 57,
		48, 11, 51, 23, 56, 21, 18, 47,
		38,  6, 15, 50, 53, 24, 55, 19,
		43, 16, 25, 41, 46, 26, 27, 37
	};

	U64 x = b ^ (b - 1);
	U32 y = U32(x) ^ U32(x >> 32);
	int index = (y * mult) >> (32 - 6);
	return table[index];
}
////////////////////////////////////////////////////////////////////////////////

inline FLD PopLSB(U64& b)
{
	FLD f = LSB(b);
	b ^= BB_SINGLE[f];
	return f;
}
////////////////////////////////////////////////////////////////////////////////

U64  Attacks(FLD f, U64 occ, PIECE piece);
U64  BishopAttacks(FLD f, U64 occ);
U64  BishopAttacksTrace(FLD f, U64 occ);
int  Delta(int dir);
U64  EnumBits(U64 b, int n);
void FindMagicLSB();
void FindMaskB();
void FindMaskR();
void FindMultB();
void FindMultR();
void FindShiftB();
void FindShiftR();
void InitBitboards();
void Print(U64 b);
void PrintArray(const U64* arr);
void PrintHex(U64 b);
U64  QueenAttacks(FLD f, U64 occ);
U64  QueenAttacksTrace(FLD f, U64 occ);
U64  RookAttacks(FLD f, U64 occ);
U64  RookAttacksTrace(FLD f, U64 occ);
U64  Shift(U64 b, int dir);
void TestMagic();

inline U64 Up(U64 b)    { return b << 8; }
inline U64 Down(U64 b)  { return b >> 8; }
inline U64 Left(U64 b)  { return (b & LL(0x7f7f7f7f7f7f7f7f)) << 1; }
inline U64 Right(U64 b) { return (b & LL(0xfefefefefefefefe)) >> 1; }

inline U64 UpLeft(U64 b)    { return (b & LL(0x007f7f7f7f7f7f7f)) << 9; }
inline U64 UpRight(U64 b)   { return (b & LL(0x00fefefefefefefe)) << 7; }
inline U64 DownLeft(U64 b)  { return (b & LL(0x7f7f7f7f7f7f7f00)) >> 7; }
inline U64 DownRight(U64 b) { return (b & LL(0xfefefefefefefe00)) >> 9; }

inline U64 Backward(U64 b, COLOR side) { return (side == WHITE)? (b >> 8) : (b << 8); }
inline U64 DoubleBackward(U64 b, COLOR side) { return (side == WHITE)? (b >> 16) : (b << 16); }
inline U64 BackwardLeft(U64 b, COLOR side) { return (side == WHITE)? DownLeft(b) : UpRight(b); }
inline U64 BackwardRight(U64 b, COLOR side) { return (side == WHITE)? DownRight(b) : UpLeft(b); }

const U64 BB_WHITE_FIELDS = LL(0xaa55aa55aa55aa55);
const U64 BB_BLACK_FIELDS = LL(0x55aa55aa55aa55aa);

const U64 BB_CENTER[2] = { LL(0x0000181818000000), LL(0x0000001818180000) };

#endif

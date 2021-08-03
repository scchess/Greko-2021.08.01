//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef TYPES_H
#define TYPES_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#ifndef _MSC_VER
#include <stdint.h>
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
#define LL(x) x##LL
#else
typedef unsigned __int8 U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;
typedef unsigned __int64 U64;
typedef __int8 I8;
typedef __int16 I16;
typedef __int32 I32;
typedef __int64 I64;
#define LL(x) x##L
#endif

typedef U8 PIECE;
typedef U8 COLOR;
typedef U8 FLD;

typedef I32 EVAL;
typedef I64 NODES;

enum
{
	NOPIECE = 0,
	PW = 2,
	PB = 3,
	NW = 4,
	NB = 5,
	BW = 6,
	BB = 7,
	RW = 8,
	RB = 9,
	QW = 10,
	QB = 11,
	KW = 12,
	KB = 13
};

enum
{
	PAWN   = 2,
	KNIGHT = 4,
	BISHOP = 6,
	ROOK   = 8,
	QUEEN  = 10,
	KING   = 12
};

enum
{
	WHITE = 0,
	BLACK = 1
};

enum
{
	A8, B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1,
	NF
};

inline int Col(FLD f) { return f % 8; }
inline int Row(FLD f) { return f / 8; }

inline COLOR GetColor(PIECE p) { return p & 1; }
inline PIECE GetPieceType(PIECE p) { return p & 0xfe; }

enum
{
	DIR_R  = 0,
	DIR_UR = 1,
	DIR_U  = 2,
	DIR_UL = 3,
	DIR_L  = 4,
	DIR_DL = 5,
	DIR_D  = 6,
	DIR_DR = 7
};

const EVAL INFINITY_SCORE  = 50000;
const EVAL CHECKMATE_SCORE = 32767;
const EVAL DRAW_SCORE = 0;

inline bool IsNearCheckmate(EVAL score)
{
	return std::abs(std::abs(score) - CHECKMATE_SCORE) < 50;
}
////////////////////////////////////////////////////////////////////////////////

struct Pair
{
	Pair() : mid(0), end(0) {}
	Pair(double mid_, double end_) :
		mid(mid_),
		end(end_)
	{}

	void operator+= (const Pair& other) { mid += other.mid; end += other.end; }
	void operator-= (const Pair& other) { mid -= other.mid; end -= other.end; }

	double mid;
	double end;
};
////////////////////////////////////////////////////////////////////////////////

inline Pair operator+ (const Pair& lhs, const Pair& rhs)
{
	return Pair(lhs.mid + rhs.mid, lhs.end + rhs.end);
}
////////////////////////////////////////////////////////////////////////////////

inline Pair operator- (const Pair& lhs, const Pair& rhs)
{
	return Pair(lhs.mid - rhs.mid, lhs.end - rhs.end);
}
////////////////////////////////////////////////////////////////////////////////

inline Pair operator* (const Pair& lhs, int n)
{
	return Pair(lhs.mid * n, lhs.end * n);
}
////////////////////////////////////////////////////////////////////////////////

inline double DotProduct(const Pair& lhs, const Pair& rhs)
{
	return lhs.mid * rhs.mid + lhs.end * rhs.end;
}
////////////////////////////////////////////////////////////////////////////////

#endif

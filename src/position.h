//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef POSITION_H
#define POSITION_H

#include "bitboards.h"

enum
{
	KINGSIDE = 0,
	QUEENSIDE = 1
};

// [color][flank]
const U8 CASTLINGS[2][2] =
{
	{ 0x01, 0x02 },
	{ 0x10, 0x20 }
};

class Move
{
public:
	Move() : m_data(0) {}
	explicit Move(U32 x) : m_data(x) {}

	Move(U32 from, U32 to, U32 piece, U32 captured = NOPIECE, U32 promotion = NOPIECE) :
		m_data(from | (to << 6) | (piece << 20) | (captured << 16) | (promotion << 12)) {}

	FLD From() const { return m_data & 0x3f; }
	FLD To() const { return (m_data >> 6) & 0x3f; }
	FLD Piece() const { return (m_data >> 20) & 0x0f; }
	FLD Captured() const { return (m_data >> 16) & 0x0f; }
	FLD Promotion() const { return (m_data >> 12) & 0x0f; }

	bool operator==(const Move& mv) const { return m_data == mv.m_data; }
	bool IsNull() const { return m_data == 0; }
	U32 ToInt() const { return m_data; }

private:
	U32 m_data;
};
////////////////////////////////////////////////////////////////////////////////

inline bool IsPawnPushOn7th(Move mv)
{
	if (mv.Piece() == PW && Row(mv.To()) == 1)
		return true;
	else if (mv.Piece() == PB && Row(mv.To()) == 6)
		return true;
	else
		return false;
}
////////////////////////////////////////////////////////////////////////////////

const int DELTA_M[14] = { 0, 0, 0, 0, 3, 3, 3, 3, 5, 5, 10, 10, 0, 0 };

const int PAWN_HASH_BITS = 32;
const int PIECE_HASH_BITS = 64 - PAWN_HASH_BITS;
const U64 PAWN_HASH_MASK = (U64(1) << PAWN_HASH_BITS) - 1;
const U64 PIECE_HASH_MASK = (U64(1) << PIECE_HASH_BITS) - 1;

class Position
{
public:
	U64    Bits(PIECE p) const { return m_bits[p]; }
	U64    BitsAll(COLOR side) const { return m_bitsAll[side]; }
	U64    BitsAll() const { return m_bitsAll[WHITE] | m_bitsAll[BLACK]; }
	U8     Castlings() const { return m_castlings; }
	bool   CanCastle(COLOR side, U8 flank) const;
	int    Count(PIECE p) const { return m_count[p]; }
	FLD    EP() const { return m_ep; }
	string FEN() const;
	int    Fifty() const { return m_fifty; }
	U64    Hash() const;
	bool   InCheck() const { return m_inCheck; }
	bool   IsAttacked(FLD f, COLOR side) const;
	FLD    King(COLOR side) const { return m_Kings[side]; }
	Move   LastMove() const { return m_undos.empty()? Move(0) : m_undos.back().m_mv; }
	bool   MakeMove(Move mv);
	void   MakeNullMove();
	int    MatIndex(COLOR side) const { return m_matIndex[side]; }
	void   Mirror();
	U32    PawnHash() const { return U32((m_hash >> PIECE_HASH_BITS) & PAWN_HASH_MASK); }
	int    Ply() const { return m_ply; }
	void   Print() const;
	int    Repetitions() const;
	Pair   Score(COLOR side) const { return m_score[side]; }
	bool   SetFEN(const string& fen);
	void   SetInitial();
	COLOR  Side() const { return m_side; }

	Pair   Stage() const
	{
		double mid = static_cast<double>((MatIndex(WHITE) + MatIndex(BLACK)) / 64.0);
		double end = static_cast<double>(1.0 - mid);
		return Pair(mid, end);
	}

	void   UnmakeMove();
	void   UnmakeNullMove();
	void   UpdateCheckInfo() { m_inCheck = IsAttacked(King(m_side), m_side ^ 1); }

	const PIECE& operator[] (FLD f) const { return m_board[f]; }

	static void  InitHashNumbers();

private:
	void Clear();
	void Put(FLD f, PIECE p);
	void Remove(FLD f);
	void MovePiece(PIECE p, FLD from, FLD to);

	static U64 s_hash[64][14];
	static U64 s_hashSide[2];
	static U64 s_hashCastlings[256];
	static U64 s_hashEP[256];

	U64   m_bits[14];
	U64   m_bitsAll[2];
	PIECE m_board[64];
	U8    m_castlings;
	int   m_count[14];
	FLD   m_ep;
	int   m_fifty;
	U64   m_hash;
	bool  m_inCheck;
	FLD   m_Kings[2];
	int   m_matIndex[2];
	int   m_ply;
	int   m_population;
	Pair  m_score[2];
	COLOR m_side;

	struct Undo
	{
		U8   m_castlings;
		FLD  m_ep;
		int  m_fifty;
		U64  m_hash;
		bool m_inCheck;
		Move m_mv;
	};
	vector<Undo> m_undos;
};
////////////////////////////////////////////////////////////////////////////////

const FLD AX[2] = { A1, A8 };
const FLD BX[2] = { B1, B8 };
const FLD CX[2] = { C1, C8 };
const FLD DX[2] = { D1, D8 };
const FLD EX[2] = { E1, E8 };
const FLD FX[2] = { F1, F8 };
const FLD GX[2] = { G1, G8 };
const FLD HX[2] = { H1, H8 };

extern const Move MOVE_O_O[2];
extern const Move MOVE_O_O_O[2];

const FLD FLIP[64] =
{
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8
};

const FLD FLIP_SIDE[2][64] =
{
	{
		A8, B8, C8, D8, E8, F8, G8, H8,
		A7, B7, C7, D7, E7, F7, G7, H7,
		A6, B6, C6, D6, E6, F6, G6, H6,
		A5, B5, C5, D5, E5, F5, G5, H5,
		A4, B4, C4, D4, E4, F4, G4, H4,
		A3, B3, C3, D3, E3, F3, G3, H3,
		A2, B2, C2, D2, E2, F2, G2, H2,
		A1, B1, C1, D1, E1, F1, G1, H1
	},
	{
		A1, B1, C1, D1, E1, F1, G1, H1,
		A2, B2, C2, D2, E2, F2, G2, H2,
		A3, B3, C3, D3, E3, F3, G3, H3,
		A4, B4, C4, D4, E4, F4, G4, H4,
		A5, B5, C5, D5, E5, F5, G5, H5,
		A6, B6, C6, D6, E6, F6, G6, H6,
		A7, B7, C7, D7, E7, F7, G7, H7,
		A8, B8, C8, D8, E8, F8, G8, H8
	}
};

const int KNIGHT_MOBILITY_TABLE[64] =
{
	2, 3, 4, 4, 4, 4, 3, 2,
	3, 4, 6, 6, 6, 6, 4, 3,
	4, 6, 8, 8, 8, 8, 6, 4,
	4, 6, 8, 8, 8, 8, 6, 4,
	4, 6, 8, 8, 8, 8, 6, 4,
	4, 6, 8, 8, 8, 8, 6, 4,
	3, 4, 6, 6, 6, 6, 4, 3,
	2, 3, 4, 4, 4, 4, 3, 2
};

#endif

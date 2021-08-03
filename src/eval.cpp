//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "eval_params.h"
#include "utils.h"

extern string g_weightsFile;

const EVAL LAZY_MARGIN = 200;

const int PAWN_HASHTABLE_BITS = 16;
const U64 PAWN_HASHTABLE_MASK = (U64(1) << PAWN_HASHTABLE_BITS) - 1;
PawnStruct g_pawnHash[PAWN_HASHTABLE_MASK + 1];

const size_t MAX_KNIGHT_MOBILITY = 8;
const size_t MAX_KNIGHT_KING_DISTANCE = 9;
const size_t MAX_BISHOP_MOBILITY = 13;
const size_t MAX_BISHOP_KING_DISTANCE = 9;
const size_t MAX_ROOK_MOBILITY = 14;
const size_t MAX_ROOK_KING_DISTANCE = 9;
const size_t MAX_QUEEN_MOBILITY = 27;
const size_t MAX_QUEEN_KING_DISTANCE = 9;
const size_t MAX_KING_PAWN_SHIELD = 9;
const size_t MAX_KING_PAWN_STORM = 9;
const size_t MAX_KING_EXPOSED = 27;
const size_t MAX_PIECE_PAIRS = 15;

Pair PSQ[14][64];
Pair PSQ_PAWN_PASSED[2][64];
Pair PSQ_PAWN_DOUBLED[2][64];
Pair PSQ_PAWN_ISOLATED[2][64];
Pair PSQ_PAWN_BACKWARDS[2][64];
Pair PSQ_KNIGHT_STRONG[2][64];
Pair PSQ_BISHOP_STRONG[2][64];

Pair KNIGHT_MOBILITY[MAX_KNIGHT_MOBILITY + 1];
Pair KNIGHT_KING_DISTANCE[MAX_KNIGHT_KING_DISTANCE + 1];
Pair BISHOP_MOBILITY[MAX_BISHOP_MOBILITY + 1];
Pair BISHOP_KING_DISTANCE[MAX_BISHOP_KING_DISTANCE + 1];
Pair ROOK_MOBILITY[MAX_ROOK_MOBILITY + 1];
Pair ROOK_KING_DISTANCE[MAX_ROOK_KING_DISTANCE + 1];
Pair ROOK_OPEN;
Pair ROOK_SEMIOPEN;
Pair ROOK_7TH;
Pair QUEEN_MOBILITY[MAX_QUEEN_MOBILITY + 1];
Pair QUEEN_KING_DISTANCE[MAX_QUEEN_KING_DISTANCE + 1];
Pair KING_PAWN_SHIELD[MAX_KING_PAWN_SHIELD + 1];
Pair KING_PAWN_STORM[MAX_KING_PAWN_STORM + 1];
Pair KING_EXPOSED[MAX_KING_EXPOSED + 1];
Pair PIECE_PAIRS[MAX_PIECE_PAIRS + 1];
Pair ATTACK_KING;
Pair ATTACK_STRONGER;
Pair TEMPO;

void InitPSQ(const vector<double>& x);
int KingPawnShield(const Position& pos, COLOR side, const PawnStruct& ps);
int KingPawnStorm(const Position& pos, COLOR side, const PawnStruct& ps);

void AddPsq(
	vector<double>& features,
	size_t index,
	COLOR side,
	FLD f,
	double stage)
{
	double UNIT = (1 - 2 * side) * stage;
	FLD f1 = FLIP_SIDE[side][f];

#ifdef PSQ_5
	double X = (Col(f1) - 3.5) / 3.5;
	double Y = (3.5 - Row(f1)) / 3.5;
	double X2 = X * X;
	double Y2 = Y * Y;
	double XY = X * Y;

	features[index] += UNIT;
	features[index + 1] += UNIT * X2;
	features[index + 2] += UNIT * X;
	features[index + 3] += UNIT * Y2;
	features[index + 4] += UNIT * Y;
	features[index + 5] += UNIT * XY;
#endif

#ifdef PSQ_12
	int col = (Col(f1) < 4)? Col(f1) : 7 - Col(f1);
	int row = Row(f1);
	features[index] += UNIT;
	features[index + 1 + col] += UNIT;
	features[index + 5 + row] += UNIT;
#endif

#ifdef PSQ_16
	int col = Col(f1);
	int row = Row(f1);
	features[index] += UNIT;
	features[index + 1 + col] += UNIT;
	features[index + 9 + row] += UNIT;
#endif

#ifdef PSQ_64
	features[index] += UNIT;
	features[index + 1 + f1] += UNIT;
#endif
}
////////////////////////////////////////////////////////////////////////////////

void AddQuadratic(
	vector<double>& features,
	size_t index,
	COLOR side,
	double z,
	double stage)
{
	double UNIT = (1 - 2 * side) * stage;

	features[index] += UNIT * z * z;
	features[index + 1] += UNIT * z;
}
////////////////////////////////////////////////////////////////////////////////

void AddLinear(
	vector<double>& features,
	size_t index,
	COLOR side,
	double z,
	double stage)
{
	double UNIT = (1 - 2 * side) * stage;
	features[index] += UNIT * z;
}
////////////////////////////////////////////////////////////////////////////////

int Distance(FLD f1, FLD f2)
{
	static const int dist[100] =
	{
		0, 1, 1, 1, 2, 2, 2, 2, 2, 3,
		3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9
	};

	int drow = Row(f1) - Row(f2);
	int dcol = Col(f1) - Col(f2);
	return dist[drow * drow + dcol * dcol];
}
////////////////////////////////////////////////////////////////////////////////

Pair EvalSide(const Position& pos, COLOR side, const PawnStruct& ps)
{
	COLOR opp = side ^ 1;
	U64 x, y, occ = pos.BitsAll();
	U64 zoneK = BB_KING_ATTACKS[pos.King(opp)];
	U64 stronger = pos.BitsAll(opp) & ~pos.Bits(PAWN | opp);
	FLD f = NF;
	Pair score(0, 0);

	//
	//   PAWNS
	//

	U64 pawns = pos.Bits(PAWN | side);

	x = pawns & ps.passed;
	while (x)
	{
		f = PopLSB(x);
		score += PSQ_PAWN_PASSED[side][f];
	}

	x = pawns & ps.doubled;
	while (x)
	{
		f = PopLSB(x);
		score += PSQ_PAWN_DOUBLED[side][f];
	}

	x = pawns & ps.isolated;
	while (x)
	{
		f = PopLSB(x);
		score += PSQ_PAWN_ISOLATED[side][f];
	}

	x = pawns & ps.backwards;
	while (x)
	{
		f = PopLSB(x);
		score += PSQ_PAWN_BACKWARDS[side][f];
	}

	if (ps.attBy[side] & stronger)
		score += ATTACK_STRONGER;

	stronger &= ~pos.Bits(KNIGHT | opp);
	stronger &= ~pos.Bits(BISHOP | opp);

	//
	//   KNIGHTS
	//

	x = pos.Bits(KNIGHT | side);
	while (x)
	{
		f = PopLSB(x);
		score += KNIGHT_MOBILITY[KNIGHT_MOBILITY_TABLE[f]];

		if (BB_KNIGHT_ATTACKS[f] & zoneK)
			score += ATTACK_KING;

		if (BB_KNIGHT_ATTACKS[f] & stronger)
			score += ATTACK_STRONGER;

		int dist = Distance(f, pos.King(opp));
		score += KNIGHT_KING_DISTANCE[dist];
	}

	x = pos.Bits(KNIGHT | side) & ps.attBy[side] & ps.safe[side];
	while (x)
	{
		f = PopLSB(x);
		score += PSQ_KNIGHT_STRONG[side][f];
	}

	//
	//   BISHOPS
	//

	x = pos.Bits(BISHOP | side);
	while (x)
	{
		f = PopLSB(x);
		y = BishopAttacks(f, occ);
		score += BISHOP_MOBILITY[CountBits(y)];

		if (y & zoneK)
			score += ATTACK_KING;

		if (y & stronger)
			score += ATTACK_STRONGER;

		int dist = Distance(f, pos.King(opp));
		score += BISHOP_KING_DISTANCE[dist];
	}

	x = pos.Bits(BISHOP | side) & ps.attBy[side] & ps.safe[side];
	while (x)
	{
		f = PopLSB(x);
		score += PSQ_BISHOP_STRONG[side][f];
	}

	stronger &= ~pos.Bits(ROOK | opp);

	//
	//   ROOKS
	//

	x = pos.Bits(ROOK | side);
	while (x)
	{
		f = PopLSB(x);
		y = RookAttacks(f, occ);
		score += ROOK_MOBILITY[CountBits(y)];

		if (y & zoneK)
			score += ATTACK_KING;

		if (y & stronger)
			score += ATTACK_STRONGER;

		int dist = Distance(f, pos.King(opp));
		score += ROOK_KING_DISTANCE[dist];

		const int file = Col(f) + 1;
		if (ps.ranks[side][file] == 0 + 7 * side)
		{
			if (ps.ranks[opp][file] == 0 + 7 * opp)
				score += ROOK_OPEN;
			else
				score += ROOK_SEMIOPEN;
		}

		if (Row(f) == 1 + 5 * side)
			score += ROOK_7TH;
	}

	//
	//   QUEENS
	//

	x = pos.Bits(QUEEN | side);
	while (x)
	{
		f = PopLSB(x);
		y = QueenAttacks(f, occ);
		score += QUEEN_MOBILITY[CountBits(y)];

		if (y & zoneK)
			score += ATTACK_KING;

		int dist = Distance(f, pos.King(opp));
		score += QUEEN_KING_DISTANCE[dist];
	}

	//
	//   KINGS
	//

	{
		f = pos.King(side);

		int shield = KingPawnShield(pos, side, ps);
		score += KING_PAWN_SHIELD[shield];

		int storm = KingPawnStorm(pos, side, ps);
		score += KING_PAWN_STORM[storm];

		y = QueenAttacks(f, occ);
		score += KING_EXPOSED[CountBits(y)];
	}

	//
	//   PIECE PAIRS
	//

	for (int i = 0; i < 4; ++i)
	{
		PIECE p1 = (KNIGHT | side) + 2 * i;
		for (int j = 0; j < 4; ++j)
		{
			PIECE p2 = (KNIGHT | side) + 2 * j;
			if (p1 == p2)
			{
				if (pos.Count(p1) == 2)
					score += PIECE_PAIRS[4 * i + j];
			}
			else
			{
				if (pos.Count(p1) == 1 && pos.Count(p2) == 1)
					score += PIECE_PAIRS[4 * i + j];
			}
		}
	}

	if (pos.Side() == side)
		score += TEMPO;

	return score;
}
////////////////////////////////////////////////////////////////////////////////

EVAL Evaluate(const Position& pos, EVAL alpha, EVAL beta)
{
	EVAL lazy = FastEval(pos);
	if (lazy < alpha - LAZY_MARGIN)
		return alpha;
	if (lazy > beta + LAZY_MARGIN)
		return beta;

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;

	size_t index = pos.PawnHash() & PAWN_HASHTABLE_MASK;
	PawnStruct& ps = g_pawnHash[index];
	if (ps.pawnHash != pos.PawnHash())
		ps.Read(pos);

	Pair score = pos.Score(side) - pos.Score(opp);
	score += EvalSide(pos, side, ps);
	score -= EvalSide(pos, opp, ps);

	EVAL e = (EVAL)DotProduct(pos.Stage(), score);

	if (e > 0 && pos.Bits(PAWN | side) == 0 && pos.MatIndex(side) < 5)
		e = 0; 
	if (e < 0 && pos.Bits(PAWN | opp) == 0 && pos.MatIndex(opp) < 5)
		e = 0;

	if (pos.Fifty() > 100)
		e = 0;
	else
		e = e * (100 - pos.Fifty()) / 100;

	return e;
}
////////////////////////////////////////////////////////////////////////////////

EVAL FastEval(const Position& pos)
{
	return (EVAL)DotProduct(
		pos.Stage(),
		pos.Score(pos.Side()) - pos.Score(pos.Side() ^ 1));
}
////////////////////////////////////////////////////////////////////////////////

void GetFeaturesSide(const Position& pos, vector<double>& features, COLOR side, const PawnStruct& ps)
{
	COLOR opp = side ^ 1;
	U64 x, y, occ = pos.BitsAll();
	U64 zoneK = BB_KING_ATTACKS[pos.King(opp)];
	U64 stronger = pos.BitsAll(opp) & ~pos.Bits(PAWN | opp);
	FLD f;

	Pair stage = pos.Stage();
	double mid = stage.mid;
	double end = stage.end;

#define ADD_PSQ(tag)                               \
    {                                              \
        AddPsq(features, Mid_##tag, side, f, mid); \
        AddPsq(features, End_##tag, side, f, end); \
    }

#define ADD_FEATURE(tag)                           \
{                                                  \
    AddLinear(features, Mid_##tag, side, 1, mid);  \
    AddLinear(features, End_##tag, side, 1, end);  \
}

#define ADD_FEATURE_VECTOR(tag, index)                     \
{                                                          \
    AddLinear(features, Mid_##tag + index, side, 1, mid);  \
    AddLinear(features, End_##tag + index, side, 1, end);  \
}

#ifdef FEATURES_LINEAR
    #define ADD_FEATURE_SCALED(tag, value, maxValue)          \
    {                                                         \
        double z = double(value) / maxValue;                  \
        AddLinear(features, Mid_##tag, side, z, mid);         \
        AddLinear(features, End_##tag, side, z, end);         \
    }
#endif

#ifdef FEATURES_QUADRATIC
    #define ADD_FEATURE_SCALED(tag, value, maxValue)          \
    {                                                         \
        double z = double(value) / maxValue;                  \
        AddQuadratic(features, Mid_##tag, side, z, mid);      \
        AddQuadratic(features, End_##tag, side, z, end);      \
    }
#endif

#ifdef FEATURES_TABULAR
    #define ADD_FEATURE_SCALED(tag, value, maxValue)          \
    {                                                         \
        AddLinear(features, Mid_##tag + value, side, 1, mid); \
        AddLinear(features, End_##tag + value, side, 1, end); \
    }
#endif

	//
	//   PAWNS
	//

	U64 pawns = pos.Bits(PAWN | side);

	x = pawns;
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(Pawn)
	}

	x = pawns & ps.passed;
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(PawnPassed)
	}

	x = pawns & ps.doubled;
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(PawnDoubled)
	}

	x = pawns & ps.isolated;
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(PawnIsolated)
	}

	x = pawns & ps.backwards;
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(PawnBackwards)
	}

	if (ps.attBy[side] & stronger)
		ADD_FEATURE(AttackStronger)

	stronger &= ~pos.Bits(KNIGHT | opp);
	stronger &= ~pos.Bits(BISHOP | opp);

	//
	//   KNIGHTS
	//

	x = pos.Bits(KNIGHT | side);
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(Knight)

		ADD_FEATURE_SCALED(KnightMobility, KNIGHT_MOBILITY_TABLE[f], MAX_KNIGHT_MOBILITY)

		if (BB_KNIGHT_ATTACKS[f] & zoneK)
			ADD_FEATURE(AttackKing)

		if (BB_KNIGHT_ATTACKS[f] & stronger)
			ADD_FEATURE(AttackStronger)

		int dist = Distance(f, pos.King(opp));
		ADD_FEATURE_SCALED(KnightKingDistance, dist, MAX_KNIGHT_KING_DISTANCE)
	}

	x = pos.Bits(KNIGHT | side) & ps.attBy[side] & ps.safe[side];
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(KnightStrong)
	}

	//
	//   BISHOPS
	//

	x = pos.Bits(BISHOP | side);
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(Bishop)

		y = BishopAttacks(f, occ);
		ADD_FEATURE_SCALED(BishopMobility, CountBits(y), MAX_BISHOP_MOBILITY)

		if (y & zoneK)
			ADD_FEATURE(AttackKing)

		if (y & stronger)
			ADD_FEATURE(AttackStronger)

		int dist = Distance(f, pos.King(opp));
		ADD_FEATURE_SCALED(BishopKingDistance, dist, MAX_BISHOP_KING_DISTANCE)
	}

	x = pos.Bits(BISHOP | side) & ps.attBy[side] & ps.safe[side];
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(BishopStrong)
	}

	stronger &= ~pos.Bits(ROOK | opp);

	//
	//   ROOKS
	//

	x = pos.Bits(ROOK | side);
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(Rook)

		y = RookAttacks(f, occ);
		ADD_FEATURE_SCALED(RookMobility, CountBits(y), MAX_ROOK_MOBILITY)

		if (y & zoneK)
			ADD_FEATURE(AttackKing)

		if (y & stronger)
			ADD_FEATURE(AttackStronger)

		int dist = Distance(f, pos.King(opp));
		ADD_FEATURE_SCALED(RookKingDistance, dist, MAX_ROOK_KING_DISTANCE)

		const int file = Col(f) + 1;
		if (ps.ranks[side][file] == 0 + 7 * side)
		{
			if (ps.ranks[opp][file] == 0 + 7 * opp)
				ADD_FEATURE(RookOpen)
			else
				ADD_FEATURE(RookSemiOpen)
		}

		if (Row(f) == 1 + 5 * side)
			ADD_FEATURE(Rook7th)
	}

	//
	//   QUEENS
	//

	x = pos.Bits(QUEEN | side);
	while (x)
	{
		f = PopLSB(x);
		ADD_PSQ(Queen)

		y = QueenAttacks(f, occ);
		ADD_FEATURE_SCALED(QueenMobility, CountBits(y), MAX_QUEEN_MOBILITY)

		if (y & zoneK)
			ADD_FEATURE(AttackKing)

		int dist = Distance(f, pos.King(opp));
		ADD_FEATURE_SCALED(QueenKingDistance, dist, MAX_QUEEN_KING_DISTANCE)
	}

	//
	//   KINGS
	//

	{
		f = pos.King(side);
		ADD_PSQ(King)

		int shield = KingPawnShield(pos, side, ps);
		ADD_FEATURE_SCALED(KingPawnShield, shield, MAX_KING_PAWN_SHIELD)

		int storm = KingPawnStorm(pos, side, ps);
		ADD_FEATURE_SCALED(KingPawnStorm, storm, MAX_KING_PAWN_STORM)

		y = QueenAttacks(f, occ);
		ADD_FEATURE_SCALED(KingExposed, CountBits(y), MAX_KING_EXPOSED)
	}

	//
	//   PIECE PAIRS
	//

	for (int i = 0; i < 4; ++i)
	{
		PIECE p1 = (KNIGHT | side) + 2 * i;
		for (int j = 0; j < 4; ++j)
		{
			PIECE p2 = (KNIGHT | side) + 2 * j;
			if (p1 == p2)
			{
				if (pos.Count(p1) == 2)
					ADD_FEATURE_VECTOR(PiecePairs, 4 * i + j)
			}
			else
			{
				if (pos.Count(p1) == 1 && pos.Count(p2) == 1)
					ADD_FEATURE_VECTOR(PiecePairs, 4 * i + j)
			}
		}
	}

	if (pos.Side() == side)
		ADD_FEATURE(Tempo)

#undef ADD_PSQ
#undef ADD_FEATURE
#undef ADD_FEATURE_VECTOR
#undef ADD_FEATURE_SCALED
}
////////////////////////////////////////////////////////////////////////////////

void GetFeatures(const Position& pos, vector<double>& features)
{
	features.clear();
	features.resize(NUMBER_OF_FEATURES);

	size_t index = pos.PawnHash() & PAWN_HASHTABLE_MASK;
	PawnStruct& ps = g_pawnHash[index];
	if (ps.pawnHash != pos.PawnHash())
		ps.Read(pos);

	GetFeaturesSide(pos, features, WHITE, ps);
	GetFeaturesSide(pos, features, BLACK, ps);
}
////////////////////////////////////////////////////////////////////////////////

void InitEval(const vector<double>& x)
{
	g_w = x;
	InitPSQ(x);

#define INIT_WEIGHT(var, tag) \
    var.mid = x[Mid_##tag];   \
    var.end = x[End_##tag];

#define INIT_VECTOR(table, tag)             \
for (size_t i = 0; i <= MAX_##table; ++i)   \
{                                           \
    table[i].mid = x[Mid_##tag + i];        \
    table[i].end = x[End_##tag + i];        \
}

#ifdef FEATURES_LINEAR
    #define INIT_WEIGHTS(table, tag)            \
    for (size_t i = 0; i <= MAX_##table; ++i)   \
    {                                           \
        double z = double(i) / MAX_##table;     \
        table[i].mid =                          \
            x[Mid_##tag] * z;                   \
        table[i].end =                          \
            x[End_##tag] * z;                   \
    }
#endif

#ifdef FEATURES_QUADRATIC
    #define INIT_WEIGHTS(table, tag)            \
    for (size_t i = 0; i <= MAX_##table; ++i)   \
    {                                           \
        double z = double(i) / MAX_##table;     \
        table[i].mid =                          \
            x[Mid_##tag] * z * z +              \
            x[Mid_##tag + 1] * z;               \
        table[i].end =                          \
            x[End_##tag] * z * z +              \
            x[End_##tag + 1] * z;               \
    }
#endif

#ifdef FEATURES_TABULAR
    #define INIT_WEIGHTS(table, tag)            \
        INIT_VECTOR(table, tag)
#endif

	INIT_WEIGHTS(KNIGHT_MOBILITY, KnightMobility)
	INIT_WEIGHTS(KNIGHT_KING_DISTANCE, KnightKingDistance)
	INIT_WEIGHTS(BISHOP_MOBILITY, BishopMobility)
	INIT_WEIGHTS(BISHOP_KING_DISTANCE, BishopKingDistance)
	INIT_WEIGHTS(ROOK_MOBILITY, RookMobility)
	INIT_WEIGHTS(ROOK_KING_DISTANCE, RookKingDistance)
	INIT_WEIGHT(ROOK_OPEN, RookOpen)
	INIT_WEIGHT(ROOK_SEMIOPEN, RookSemiOpen)
	INIT_WEIGHT(ROOK_7TH, Rook7th)
	INIT_WEIGHTS(QUEEN_MOBILITY, QueenMobility)
	INIT_WEIGHTS(QUEEN_KING_DISTANCE, QueenKingDistance)
	INIT_WEIGHTS(KING_PAWN_SHIELD, KingPawnShield)
	INIT_WEIGHTS(KING_PAWN_STORM, KingPawnStorm)
	INIT_WEIGHTS(KING_EXPOSED, KingExposed)
	INIT_VECTOR(PIECE_PAIRS, PiecePairs)
	INIT_WEIGHT(ATTACK_KING, AttackKing)
	INIT_WEIGHT(ATTACK_STRONGER, AttackStronger)
	INIT_WEIGHT(TEMPO, Tempo)

#undef INIT_WEIGHTS
}
////////////////////////////////////////////////////////////////////////////////

void InitEval()
{
	InitFeatures();

	vector<double> x;
	if (!ReadWeights(x, g_weightsFile))
	{
		SetDefaultWeights(x);
		WriteWeights(x, g_weightsFile);
	}
	InitEval(x);
}
////////////////////////////////////////////////////////////////////////////////

void InitPSQ(const vector<double>& x)
{
	for (FLD f = 0; f < 64; ++f)
	{
#ifdef PSQ_5
		double X = (Col(f) - 3.5) / 3.5;
		double Y = (3.5 - Row(f)) / 3.5;
		double X2 = X * X;
		double Y2 = Y * Y;
		double XY = X * Y;

#define INIT_PSQ(table, index, tag)                           \
        table[index][f].mid =                                 \
            x[Mid_##tag] +                                    \
            x[Mid_##tag + 1] * X2 +                           \
            x[Mid_##tag + 2] * X +                            \
            x[Mid_##tag + 3] * Y2 +                           \
            x[Mid_##tag + 4] * Y +                            \
            x[Mid_##tag + 5] * XY;                            \
        table[index][f].end =                                 \
            x[End_##tag] +                                    \
            x[End_##tag + 1] * X2 +                           \
            x[End_##tag + 2] * X +                            \
            x[End_##tag + 3] * Y2 +                           \
            x[End_##tag + 4] * Y +                            \
            x[End_##tag + 5] * XY;                            \
        table[index ^ 1][FLIP[f]].mid = table[index][f].mid;  \
        table[index ^ 1][FLIP[f]].end = table[index][f].end;
#endif

#ifdef PSQ_12
#define INIT_PSQ(table, index, tag)                           \
        {                                                     \
        int col = (Col(f) < 4)? Col(f) : 7 - Col(f);          \
        int row = Row(f);                                     \
        table[index][f].mid =                                 \
            x[Mid_##tag] +                                    \
            x[Mid_##tag + 1 + col] +                          \
            x[Mid_##tag + 5 + row];                           \
        table[index][f].end =                                 \
            x[End_##tag] +                                    \
            x[End_##tag + 1 + col] +                          \
            x[End_##tag + 5 + row];                           \
        table[index ^ 1][FLIP[f]].mid = table[index][f].mid;  \
        table[index ^ 1][FLIP[f]].end = table[index][f].end;  \
        }
#endif

#ifdef PSQ_16
#define INIT_PSQ(table, index, tag)                           \
        {                                                     \
        int col = Col(f);                                     \
        int row = Row(f);                                     \
        table[index][f].mid =                                 \
            x[Mid_##tag] +                                    \
            x[Mid_##tag + 1 + col] +                          \
            x[Mid_##tag + 9 + row];                           \
        table[index][f].end =                                 \
            x[End_##tag] +                                    \
            x[End_##tag + 1 + col] +                          \
            x[End_##tag + 9 + row];                           \
        table[index ^ 1][FLIP[f]].mid = table[index][f].mid;  \
        table[index ^ 1][FLIP[f]].end = table[index][f].end;  \
        }
#endif

#ifdef PSQ_64
#define INIT_PSQ(table, index, tag)                           \
        {                                                     \
        table[index][f].mid =                                 \
            x[Mid_##tag] +                                    \
            x[Mid_##tag + 1 + f];                             \
        table[index][f].end =                                 \
            x[End_##tag] +                                    \
            x[End_##tag + 1 + f];                             \
        table[index ^ 1][FLIP[f]].mid = table[index][f].mid;  \
        table[index ^ 1][FLIP[f]].end = table[index][f].end;  \
        }
#endif

		INIT_PSQ(PSQ, PW, Pawn)
		INIT_PSQ(PSQ, NW, Knight)
		INIT_PSQ(PSQ, BW, Bishop)
		INIT_PSQ(PSQ, RW, Rook)
		INIT_PSQ(PSQ, QW, Queen)
		INIT_PSQ(PSQ, KW, King)

		INIT_PSQ(PSQ_PAWN_PASSED, WHITE, PawnPassed)
		INIT_PSQ(PSQ_PAWN_DOUBLED, WHITE, PawnDoubled)
		INIT_PSQ(PSQ_PAWN_ISOLATED, WHITE, PawnIsolated)
		INIT_PSQ(PSQ_PAWN_BACKWARDS, WHITE, PawnBackwards)
		INIT_PSQ(PSQ_KNIGHT_STRONG, WHITE, KnightStrong)
		INIT_PSQ(PSQ_BISHOP_STRONG, WHITE, BishopStrong)

#undef INIT_PSQ
	}
}
////////////////////////////////////////////////////////////////////////////////

int KingPawnShield(const Position& pos, COLOR side, const PawnStruct& ps)
{
	FLD K = pos.King(side);
	int file = Col(K) + 1;
	
	const int bonus[2][8] =
	{
		{ 0, 0, 0, 0, 1, 2, 3, 0 },
		{ 0, 3, 2, 1, 0, 0, 0, 0 }
	};

	int shield = 0;
	for (int i = file - 1; i < file + 2; ++i)
	{
		int rank = ps.ranks[side][i];
		shield += bonus[side][rank];
	}
	return shield;
}
////////////////////////////////////////////////////////////////////////////////

int KingPawnStorm(const Position& pos, COLOR side, const PawnStruct& ps)
{
	FLD K = pos.King(side);
	int file = Col(K) + 1;
	
	const int penalty[2][8] =
	{
		{ 0, 0, 0, 0, 1, 2, 3, 3 },
		{ 3, 3, 2, 1, 0, 0, 0, 0 }
	};

	int storm = 0;
	for (int i = file - 1; i < file + 2; ++i)
	{
		int rank = ps.ranks[side ^ 1][i];
		storm += penalty[side][rank];
	}
	return storm;
}
////////////////////////////////////////////////////////////////////////////////

void PawnStruct::Clear()
{
	pawnHash = 0;
	passed = 0;
	doubled = 0;
	isolated = 0;
	backwards = 0;

	attBy[WHITE] = attBy[BLACK] = 0;
	safe[WHITE] = safe[BLACK] = LL(0xffffffffffffffff);

	memset(ranks[WHITE], 0, 10);
	memset(ranks[BLACK], 7, 10);
}
////////////////////////////////////////////////////////////////////////////////

void PawnStruct::Read(const Position& pos)
{
	// Algorithm from TSCP 1.81 by Tom Kerrigan

	Clear();
	pawnHash = pos.PawnHash();

	U64 x, y;
	FLD f;

	// first pass

	x = pos.Bits(PW);
	while (x)
	{
		f = PopLSB(x);
		int file = Col(f) + 1;
		int rank = Row(f);
		if (rank > ranks[WHITE][file])
			ranks[WHITE][file] = rank;

		attBy[WHITE] |= BB_PAWN_ATTACKS[f][WHITE];
		y = Left(BB_DIR[f][DIR_U]) | Right(BB_DIR[f][DIR_U]);
		safe[BLACK] &= ~y;
	}

	x = pos.Bits(PB);
	while (x)
	{
		f = PopLSB(x);
		int file = Col(f) + 1;
		int rank = Row(f);
		if (rank < ranks[BLACK][file])
			ranks[BLACK][file] = rank;

		attBy[BLACK] |= BB_PAWN_ATTACKS[f][BLACK];
		y = Left(BB_DIR[f][DIR_D]) | Right(BB_DIR[f][DIR_D]);
		safe[WHITE] &= ~y;
	}

	// second pass

	x = pos.Bits(PW);
	while (x)
	{
		f = PopLSB(x);
		int file = Col(f) + 1;
		int rank = Row(f);

		if (BB_DIR[f][DIR_U] & pos.Bits(PW))
			doubled |= BB_SINGLE[f];

		if (ranks[WHITE][file - 1] == 0 && ranks[WHITE][file + 1] == 0)
			isolated |= BB_SINGLE[f];
		else if (rank > ranks[WHITE][file - 1] && rank > ranks[WHITE][file + 1])
			backwards |= BB_SINGLE[f];

		if (rank < ranks[BLACK][file] && rank <= ranks[BLACK][file - 1] && rank <= ranks[BLACK][file + 1])
			passed |= BB_SINGLE[f];
	}

	x = pos.Bits(PB);
	while (x)
	{
		f = PopLSB(x);
		int file = Col(f) + 1;
		int rank = Row(f);

		if (BB_DIR[f][DIR_D] & pos.Bits(PB))
			doubled |= BB_SINGLE[f];

		if (ranks[BLACK][file - 1] == 7 && ranks[BLACK][file + 1] == 7)
			isolated |= BB_SINGLE[f];
		else if (rank < ranks[BLACK][file - 1] && rank < ranks[BLACK][file + 1])
			backwards |= BB_SINGLE[f];

		if (rank > ranks[WHITE][file] && rank >= ranks[WHITE][file - 1] && rank >= ranks[WHITE][file + 1])
			passed |= BB_SINGLE[f];
	}
}
////////////////////////////////////////////////////////////////////////////////

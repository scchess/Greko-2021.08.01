//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef EVAL_PARAMS_H
#define EVAL_PARAMS_H

#include "types.h"

// #define PSQ_5
// #define PSQ_12
// #define PSQ_16
#define PSQ_64

// #define FEATURES_LINEAR
// #define FEATURES_QUADRATIC
#define FEATURES_TABULAR

#ifdef PSQ_5
#define PSQ_SIZE 5
#endif

#ifdef PSQ_12
#define PSQ_SIZE 12
#endif

#ifdef PSQ_16
#define PSQ_SIZE 16
#endif

#ifdef PSQ_64
#define PSQ_SIZE 64
#endif

#ifdef FEATURES_LINEAR
#define MOB_KNIGHT_SIZE 1
#define MOB_BISHOP_SIZE 1
#define MOB_ROOK_SIZE 1
#define MOB_QUEEN_SIZE 1
#define DISTANCE_SIZE 1
#define KING_PAWN_SHIELD_SIZE 1
#define KING_PAWN_STORM_SIZE 1
#define KING_EXPOSED_SIZE 1
#endif

#ifdef FEATURES_QUADRATIC
#define MOB_KNIGHT_SIZE 2
#define MOB_BISHOP_SIZE 2
#define MOB_ROOK_SIZE 2
#define MOB_QUEEN_SIZE 2
#define DISTANCE_SIZE 2
#define KING_PAWN_SHIELD_SIZE 2
#define KING_PAWN_STORM_SIZE 2
#define KING_EXPOSED_SIZE 2
#endif

#ifdef FEATURES_TABULAR
#define MOB_KNIGHT_SIZE 9
#define MOB_BISHOP_SIZE 14
#define MOB_ROOK_SIZE 15
#define MOB_QUEEN_SIZE 28
#define DISTANCE_SIZE 10
#define KING_PAWN_SHIELD_SIZE 10
#define KING_PAWN_STORM_SIZE 10
#define KING_EXPOSED_SIZE 28
#endif

extern size_t Mid_Pawn;
extern size_t Mid_PawnPassed;
extern size_t Mid_PawnDoubled;
extern size_t Mid_PawnIsolated;
extern size_t Mid_PawnBackwards;
extern size_t Mid_Knight;
extern size_t Mid_KnightMobility;
extern size_t Mid_KnightKingDistance;
extern size_t Mid_KnightStrong;
extern size_t Mid_Bishop;
extern size_t Mid_BishopMobility;
extern size_t Mid_BishopKingDistance;
extern size_t Mid_BishopStrong;
extern size_t Mid_Rook;
extern size_t Mid_RookMobility;
extern size_t Mid_RookKingDistance;
extern size_t Mid_RookOpen;
extern size_t Mid_RookSemiOpen;
extern size_t Mid_Rook7th;
extern size_t Mid_Queen;
extern size_t Mid_QueenMobility;
extern size_t Mid_QueenKingDistance;
extern size_t Mid_King;
extern size_t Mid_KingPawnShield;
extern size_t Mid_KingPawnStorm;
extern size_t Mid_KingExposed;
extern size_t Mid_PiecePairs;
extern size_t Mid_AttackKing;
extern size_t Mid_AttackStronger;
extern size_t Mid_Tempo;

extern size_t End_Pawn;
extern size_t End_PawnPassed;
extern size_t End_PawnDoubled;
extern size_t End_PawnIsolated;
extern size_t End_PawnBackwards;
extern size_t End_Knight;
extern size_t End_KnightMobility;
extern size_t End_KnightKingDistance;
extern size_t End_KnightStrong;
extern size_t End_Bishop;
extern size_t End_BishopMobility;
extern size_t End_BishopKingDistance;
extern size_t End_BishopStrong;
extern size_t End_Rook;
extern size_t End_RookMobility;
extern size_t End_RookKingDistance;
extern size_t End_RookOpen;
extern size_t End_RookSemiOpen;
extern size_t End_Rook7th;
extern size_t End_Queen;
extern size_t End_QueenMobility;
extern size_t End_QueenKingDistance;
extern size_t End_King;
extern size_t End_KingPawnShield;
extern size_t End_KingPawnStorm;
extern size_t End_KingExposed;
extern size_t End_PiecePairs;
extern size_t End_AttackKing;
extern size_t End_AttackStronger;
extern size_t End_Tempo;

extern size_t NUMBER_OF_FEATURES;
extern vector<double> g_w;

template<typename W, typename F>
double DotProduct(const W weights, const F features)
{
	assert(weights.size() == features.size());

	double e = 0;
	for (size_t i = 0; i < weights.size(); ++i)
		e += weights[i] * features[i];
	return e;
}
////////////////////////////////////////////////////////////////////////////////

void InitFeatures();
string FeatureName(size_t index);
bool ReadWeights(vector<double>& x, const string& file);
void SetDefaultWeights(vector<double>& x);
void WriteWeights(const vector<double>& x, const string& file);

#endif

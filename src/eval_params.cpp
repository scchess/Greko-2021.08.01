//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval_params.h"
#include "notation.h"
#include "utils.h"

struct FeatureInfo
{
	FeatureInfo(const string& name_, size_t index_, size_t len_) :
		name(name_),
		index(index_),
		len(len_)
	{}

	string name;
	size_t index;
	size_t len;
};
vector<FeatureInfo> g_features;

vector<double> g_w;

size_t Mid_Pawn;
size_t Mid_PawnPassed;
size_t Mid_PawnDoubled;
size_t Mid_PawnIsolated;
size_t Mid_PawnBackwards;
size_t Mid_Knight;
size_t Mid_KnightMobility;
size_t Mid_KnightKingDistance;
size_t Mid_KnightStrong;
size_t Mid_Bishop;
size_t Mid_BishopMobility;
size_t Mid_BishopKingDistance;
size_t Mid_BishopStrong;
size_t Mid_Rook;
size_t Mid_RookMobility;
size_t Mid_RookKingDistance;
size_t Mid_RookOpen;
size_t Mid_RookSemiOpen;
size_t Mid_Rook7th;
size_t Mid_Queen;
size_t Mid_QueenMobility;
size_t Mid_QueenKingDistance;
size_t Mid_King;
size_t Mid_KingPawnShield;
size_t Mid_KingPawnStorm;
size_t Mid_KingExposed;
size_t Mid_PiecePairs;
size_t Mid_AttackKing;
size_t Mid_AttackStronger;
size_t Mid_Tempo;

size_t End_Pawn;
size_t End_PawnPassed;
size_t End_PawnDoubled;
size_t End_PawnIsolated;
size_t End_PawnBackwards;
size_t End_Knight;
size_t End_KnightMobility;
size_t End_KnightKingDistance;
size_t End_KnightStrong;
size_t End_Bishop;
size_t End_BishopMobility;
size_t End_BishopKingDistance;
size_t End_BishopStrong;
size_t End_Rook;
size_t End_RookMobility;
size_t End_RookKingDistance;
size_t End_RookOpen;
size_t End_RookSemiOpen;
size_t End_Rook7th;
size_t End_Queen;
size_t End_QueenMobility;
size_t End_QueenKingDistance;
size_t End_King;
size_t End_KingPawnShield;
size_t End_KingPawnStorm;
size_t End_KingExposed;
size_t End_PiecePairs;
size_t End_AttackKing;
size_t End_AttackStronger;
size_t End_Tempo;

size_t NUMBER_OF_FEATURES;
size_t maxTagLength = 0;

void InitFeatures()
{
	size_t index = 0;
	g_features.clear();

#define REGISTER_FEATURE(var, len)                            \
	var = index;                                              \
	g_features.push_back(FeatureInfo(#var, index, len));      \
	index += len;                                             \
	if (strlen(#var) > maxTagLength)                          \
	    maxTagLength = strlen(#var);

	REGISTER_FEATURE(Mid_Pawn, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_PawnPassed, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_PawnDoubled, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_PawnIsolated, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_PawnBackwards, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_Knight, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_KnightMobility, MOB_KNIGHT_SIZE)
	REGISTER_FEATURE(Mid_KnightKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(Mid_KnightStrong, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_Bishop, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_BishopMobility, MOB_BISHOP_SIZE)
	REGISTER_FEATURE(Mid_BishopKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(Mid_BishopStrong, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_Rook, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_RookMobility, MOB_ROOK_SIZE)
	REGISTER_FEATURE(Mid_RookKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(Mid_RookOpen, 1)
	REGISTER_FEATURE(Mid_RookSemiOpen, 1)
	REGISTER_FEATURE(Mid_Rook7th, 1)
	REGISTER_FEATURE(Mid_Queen, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_QueenMobility, MOB_QUEEN_SIZE)
	REGISTER_FEATURE(Mid_QueenKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(Mid_King, PSQ_SIZE + 1)
	REGISTER_FEATURE(Mid_KingPawnShield, KING_PAWN_SHIELD_SIZE)
	REGISTER_FEATURE(Mid_KingPawnStorm, KING_PAWN_STORM_SIZE)
	REGISTER_FEATURE(Mid_KingExposed, KING_EXPOSED_SIZE)
	REGISTER_FEATURE(Mid_PiecePairs, 16)
	REGISTER_FEATURE(Mid_AttackKing, 1)
	REGISTER_FEATURE(Mid_AttackStronger, 1)
	REGISTER_FEATURE(Mid_Tempo, 1)

	REGISTER_FEATURE(End_Pawn, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_PawnPassed, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_PawnDoubled, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_PawnIsolated, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_PawnBackwards, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_Knight, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_KnightMobility, MOB_KNIGHT_SIZE)
	REGISTER_FEATURE(End_KnightKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(End_KnightStrong, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_Bishop, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_BishopMobility, MOB_BISHOP_SIZE)
	REGISTER_FEATURE(End_BishopKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(End_BishopStrong, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_Rook, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_RookMobility, MOB_ROOK_SIZE)
	REGISTER_FEATURE(End_RookKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(End_RookOpen, 1)
	REGISTER_FEATURE(End_RookSemiOpen, 1)
	REGISTER_FEATURE(End_Rook7th, 1)
	REGISTER_FEATURE(End_Queen, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_QueenMobility, MOB_QUEEN_SIZE)
	REGISTER_FEATURE(End_QueenKingDistance, DISTANCE_SIZE)
	REGISTER_FEATURE(End_King, PSQ_SIZE + 1)
	REGISTER_FEATURE(End_KingPawnShield, KING_PAWN_SHIELD_SIZE)
	REGISTER_FEATURE(End_KingPawnStorm, KING_PAWN_STORM_SIZE)
	REGISTER_FEATURE(End_KingExposed, KING_EXPOSED_SIZE)
	REGISTER_FEATURE(End_PiecePairs, 16)
	REGISTER_FEATURE(End_AttackKing, 1)
	REGISTER_FEATURE(End_AttackStronger, 1)
	REGISTER_FEATURE(End_Tempo, 1)

	NUMBER_OF_FEATURES = index;

#undef REGISTER_FEATURE
}
////////////////////////////////////////////////////////////////////////////////

string FeatureName(size_t index)
{
	for (size_t i = 0; i < g_features.size(); ++i)
	{
		if (g_features[i].index <= index &&
			index < g_features[i].index + g_features[i].len)
		{
			stringstream ss;
			ss << g_features[i].name <<
				"[" << index - g_features[i].index << "]";
			return ss.str();
		}
	}
	return "<unknown>";
}
////////////////////////////////////////////////////////////////////////////////

void SetDefaultWeights(vector<double>& x)
{
	x.clear();
	x.resize(NUMBER_OF_FEATURES);

#ifdef PSQ_64
#ifdef FEATURES_TABULAR
	static const double src[] =
	{
		109, 0, 0, 0, 0, 0, 0, 0, 0, 83, 72, 67, 51, 38, 26, 0, 27, 2, 10,
		8, 35, 51, 36, 0, 15, -17, -5, 2, 18, 24, 4, 3, -28, -36, -24, -8,
		17, 19, 10, -15, -32, -26, -18, -13, -9, -7, -17, 8, -23, -43, -14,
		-40, -39, -36, -23, 3, -40, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0,
		0, 0, 0, 0, 83, 72, 67, 51, 38, 26, 0, 27, 30, 58, 29, 10, 0, 28, 7,
		-26, -6, 1, 0, 0, 0, 3, 18, -17, -2, -23, -35, -24, -60, -30, -8, -10,
		-26, -20, -38, -8, -15, -12, -42, -21, -29, -7, -10, -10, -6, 9, -12,
		-1, 0, 0, 0, 0, 0, 0, 0, 0, -15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0, 0, -3, 10,
		-6, -12, -5, -24, -9, 7, 7, 6, 11, 0, -7, -3, 8, 4, -12, 13, 3, -22,
		4, 9, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, -7, 0, 0, 0, 0, 0, 0, 0, 0, 50,
		17, 32, 18, 7, 5, 0, 38, 12, -14, 11, 16, 0, -5, 3, 23, 11, 0, 0, -15,
		-36, 0, 9, 11, 11, 0, -22, -23, -21, -19, -12, 11, 0, -5, -19, -17,
		-28, -2, -30, -4, 7, -12, 7, 0, -12, 0, -14, 3, 0, 0, 0, 0, 0, 0, 0,
		0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 5, 3, 4,
		0, 0, 0, 14, 30, 34, 26, 31, 25, 31, 24, 7, -7, 3, -16, -13, -28, 17,
		11, 3, -14, -17, -21, -25, -5, -23, 0, -6, -12, -8, -14, -13, -8, -14,
		-3, 0, 0, 0, 0, 0, 0, 0, 0, 275, -54, 0, -1, 0, 6, -21, 0, 0, -23,
		-6, -29, 25, -21, 6, 0, -4, -14, 14, 8, 23, 19, 48, 13, 0, 33, 4, 0,
		3, -7, 43, 7, 58, 0, -4, -9, -4, 2, 5, 11, 10, -8, 3, -16, -9, 4, -4,
		23, -4, 9, -14, -13, 1, 4, 0, 20, 34, 0, 5, -4, 0, 10, -1, 11, 12,
		0, 0, -44, 43, 63, 0, 87, 0, 105, 0, 61, 68, 32, 30, 15, 11, 12, 8,
		-11, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, -15, -9, 0, 0, 21,
		5, -9, 0, -8, -4, 0, 0, 6, 4, 14, 13, -1, 42, 0, 9, -2, 17, 26, 17,
		0, 0, 14, -25, 9, -26, -6, -14, -33, -19, -13, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 331, 17, -7, 0, -3, 0, -24, 3, 0, -46, -33,
		-32, -13, -29, 0, -27, -18, -53, -4, -1, -1, 44, 29, -6, 1, -10, 11,
		23, 32, 4, 36, 7, 11, 14, -1, 16, 19, 31, 15, 12, 6, 13, 33, 23, 11,
		0, 21, 21, 15, 32, 20, 0, -2, 13, 7, 27, 30, -30, 14, -3, -12, 9, -11,
		-5, -12, 0, 0, -16, 0, 15, 21, 23, 28, 33, 27, 33, 41, 50, 19, 0, 12,
		85, 32, 25, 25, 24, 26, 34, 16, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0,
		0, 0, 0, 0, 0, -10, -7, -17, -5, 11, 9, 0, 0, -1, 16, -23, 13, 18,
		18, 15, -23, -41, 0, 20, 17, 11, -3, -8, 7, -9, -2, 17, 2, 16, -15,
		18, -15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 434, 37, 36,
		60, 49, 37, 19, 12, 26, -11, -20, 0, 16, 16, 28, 3, 0, 2, 1, 18, 41,
		24, 40, 30, -6, 2, -15, 0, 1, -7, 17, -14, -20, -28, -23, -12, -20,
		-12, 0, -5, -21, -3, -11, -1, -11, -2, 6, 6, -7, -8, -8, 1, 3, -5,
		-2, 21, -19, 0, 4, 7, 20, 34, 27, 4, 14, 0, 0, -7, 0, 5, 12, 17, 16,
		22, 25, 29, 31, 38, 53, 131, 0, 109, 103, 49, 39, 23, 13, 10, 6, 23,
		65, 28, 43, 990, -2, 39, 26, 15, 28, 22, 10, 29, 0, -12, -25, -2, -22,
		-2, 33, 27, 10, -4, -11, -2, 0, 33, 20, 0, 13, -6, -3, 0, -3, 0, 0,
		18, 6, 3, -11, 7, 0, 0, 5, 3, 4, 12, 9, 12, 3, 8, 17, 9, 33, 32, 27,
		26, 31, 22, 28, 0, 50, 32, 35, 34, 51, 9, 17, 23, 0, 0, 0, 0, -8, 25,
		24, 28, 25, 28, 31, 32, 31, 39, 39, 36, 43, 46, 43, 44, 48, 47, 50,
		74, 61, 43, 9, 0, 0, 0, 210, 135, 123, 119, 117, 99, 93, 39, 0, 0,
		0, 0, 0, 0, 0, 0, 0, -1, 6, 0, 0, 0, 0, 0, 0, 0, 7, 1, -1, 0, 1, 12,
		6, 0, 0, 2, -5, -2, 4, 1, -4, -10, -8, 0, 0, -12, 7, -16, -17, -13,
		-9, -2, -17, 0, 8, -2, -16, 23, 37, 4, 0, 2, 10, 33, 43, -70, 9, 16,
		-31, -1, -45, 22, 0, -54, -26, -25, -11, -1, 8, 15, 21, 35, 35, 34,
		21, 0, 24, 0, -27, 3, -14, -48, -1, 0, 0, 0, 0, 81, 50, 35, 23, 17,
		14, 15, 10, 6, 2, -5, -12, -23, -29, -42, -29, -43, -28, -17, 0, -17,
		0, -4, -4, 42, 0, -13, 3, 0, 83, -14, 9, -13, -14, 81, 11, 3, 9, 11,
		0, 9, 65, 24, 117, 0, 0, 0, 0, 0, 0, 0, 0, 53, 64, 56, 44, 42, 46,
		66, 57, 39, 4, 29, 7, 20, 31, 38, 32, 5, 3, -11, -15, -17, -10, 4,
		-2, -5, -1, -23, -31, -33, -30, -11, -19, -14, -9, -9, -18, -9, -13,
		-17, -29, -9, -17, -5, 0, -10, -8, -5, -38, 0, 0, 0, 0, 0, 0, 0, 0,
		52, 0, 0, 0, 0, 0, 0, 0, 0, 53, 64, 56, 44, 42, 46, 66, 57, 51, 59,
		44, 26, 0, 4, 17, 23, 14, 26, 6, 0, -12, 0, 0, 19, -15, -7, -4, -23,
		-2, -14, -2, -4, -38, -28, -31, -38, -38, -28, -12, -34, -32, -26,
		-35, -40, -48, -50, -34, -45, 0, 0, 0, 0, 0, 0, 0, 0, -29, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -8, 0, 0, -7, 0, 0, 0, -8, 0,
		0, -7, 0, -8, 0, -8, -10, -16, 0, 0, 6, 11, 10, 0, -5, 4, -4, 12, 3,
		28, 3, -12, 1, -18, 10, 0, 25, 9, 11, -5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		-13, 0, 0, 0, 0, 0, 0, 0, 0, -19, -47, -15, -2, 0, 2, -18, -11, -15,
		-1, -12, -4, 2, 1, 0, 10, 9, -16, -1, -8, 10, 3, -5, 3, 8, -9, 1, 12,
		-1, 9, -5, 3, 7, -7, -6, 2, -6, -3, 7, 20, 16, 9, -3, -5, 0, 11, 8,
		34, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 11, 12, 16, 0, -4, 4, 4, -8, 8, 14, 32, 19, -8, 14,
		-12, -15, -11, 0, -12, 11, -7, -6, -8, -8, -9, -11, -9, -7, -3, -7,
		2, 0, -4, -14, 5, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 162, -25, 1, 10,
		12, 16, 0, 9, 0, 7, 10, 0, 11, 17, 12, 11, 0, 14, 0, 1, 7, 2, -1, 8,
		15, 2, 4, 2, 21, 9, 0, 29, 5, -7, 0, 10, 10, 14, 0, 7, 0, 0, 0, -6,
		-3, 0, -13, -25, -1, 0, 0, -5, -4, -8, -20, -19, 15, 10, -3, 0, -6,
		-20, -19, -13, 11, 0, 0, 6, 10, 21, 0, 47, 0, 61, 0, -3, 4, 26, 30,
		30, 27, 18, 3, -1, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, -8,
		0, 0, 2, 0, 6, 17, 12, 9, 0, 8, -15, 15, 0, 22, 23, -27, 10, -17, 7,
		-4, 5, 1, 7, -13, -19, -14, 0, -2, 4, -9, -1, -8, -12, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 215, 14, 9, 3, 11, 16, 0, 13, 18,
		12, 7, 15, 8, 8, 5, 10, 4, 20, 14, 14, 1, -12, 12, 23, 17, 4, 0, 0,
		4, 28, 0, 10, 0, -12, 0, 13, 18, 6, -5, 0, 0, -14, -6, 0, 2, 0, -2,
		-25, 3, 0, -25, 0, -8, -21, -23, -13, -26, 25, 0, -19, 1, -21, -20,
		-8, -5, 0, 0, -37, -8, -13, 0, 24, 32, 36, 43, 45, 29, 27, 16, 0, 2,
		3, 21, 28, 33, 24, 22, 19, 5, 6, 0, -7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 8, 0, 8, -18, 3, 7, 6, 7, 7, 0, 0, 0, -25, 23, -10, -11, -2,
		-12, -7, 0, 0, -4, 4, 0, 7, 2, -1, 5, 8, 12, 4, -20, 13, -15, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 361, 24, 21, 19, 20, 17,
		16, 20, 19, 13, 11, 6, 0, -2, -12, -1, 0, 29, 36, 11, 10, 12, 8, 10,
		15, 19, 21, 20, 21, 24, 6, 20, 13, 29, 22, 18, 8, 10, 4, 8, 4, 0, 0,
		3, 4, -6, -15, -8, -1, -15, -2, -10, -7, -10, -15, -41, -16, -12, -16,
		-9, -11, -33, -35, -10, -30, 0, 0, -29, -13, 7, 22, 33, 40, 40, 49,
		52, 50, 46, 40, -24, 0, 6, 11, 28, 32, 38, 39, 46, 57, 43, -14, 16,
		17, 723, 23, 29, 22, 7, 19, 0, 32, 17, 28, 17, 44, 18, 12, 37, 30,
		12, 2, 15, 29, 23, 18, 9, 22, -2, -5, 18, 23, 36, 51, 25, 15, -16,
		3, 11, 15, 25, 26, 13, 0, 0, 1, 8, 31, -19, 0, 9, -14, 2, 0, 0, -9,
		-9, -18, -12, -30, 0, 7, -2, -12, -45, -25, -17, -2, 0, 0, 0, 0, 0,
		0, -14, -21, -9, -9, -29, -2, 29, 30, 36, 42, 60, 62, 53, 66, 70, 71,
		57, 72, 26, 43, 7, 2, -2, 0, 0, 151, 165, 131, 98, 38, 41, 23, 21,
		0, -9, -14, 0, -10, -18, -15, -23, 0, -23, 20, 12, 16, -6, -15, -14,
		0, 12, 0, 26, 27, 21, 26, 20, -14, -4, 4, 28, 45, 50, 31, 11, -17,
		-4, 8, 28, 46, 48, 28, 12, -15, -17, 8, 15, 29, 24, 17, 0, -19, -20,
		-8, 0, 5, 0, 1, -11, -37, -43, -39, -28, -20, -45, -22, -38, -68, 16,
		19, 10, 6, -5, -1, -1, -9, -6, -20, -26, -2, 25, -25, 0, 32, -21, 0,
		62, -35, 0, 0, 0, 0, -27, 0, 1, 0, 10, 4, 6, 5, 7, 8, 15, 15, 14, 15,
		20, 11, 9, 3, -6, -11, -25, -38, -36, -53, 20, 1, 20, 27, 1, 52, 22,
		15, 20, 22, 94, 44, 27, 15, 44, 0, 3, 52, 9
	};
	memcpy(&(x[0]), src, NUMBER_OF_FEATURES * sizeof(double));
#endif
#endif
}
////////////////////////////////////////////////////////////////////////////////

bool ReadWeights(vector<double>& x, const string& file)
{
	x.clear();
	x.resize(NUMBER_OF_FEATURES);

	ifstream ifs(file.c_str());
	if (ifs.good())
	{
		string s;
		while (getline(ifs, s))
		{
			istringstream iss(s);
			string name;
			double value;

			if (!(iss >> name))
				continue;

			for (size_t i = 0; i < g_features.size(); ++i)
			{
				if (g_features[i].name == name)
				{
					size_t index = g_features[i].index;
					size_t offset = 0;
					while (iss >> value)
					{
						x[index + offset] = value;
						++offset;
						if (offset >= g_features[i].len)
							break;
					}
					break;
				}
			}
		}
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////

void WriteWeights(const vector<double>& x, const string& file)
{
	ofstream ofs(file.c_str());
	if (ofs.good())
	{
		for (size_t i = 0; i < g_features.size(); ++i)
		{
			string name = g_features[i].name;
			size_t index = g_features[i].index;
			size_t len = g_features[i].len;

			ofs << setw(maxTagLength + 2) << left << name;
			for (size_t j = 0; j < len; ++j)
				ofs << " " << (int)x[index + j];
			ofs << endl;

			if (i == g_features.size() / 2 - 1)
				ofs << endl;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

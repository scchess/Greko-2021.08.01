//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef EVAL_H
#define EVAL_H

#include "eval_params.h"
#include "position.h"

void GetFeatures(const Position& pos, vector<double>& features);
EVAL Evaluate(const Position& pos, EVAL alpha = -INFINITY_SCORE, EVAL beta = INFINITY_SCORE);
EVAL FastEval(const Position& pos);
void InitEval();
void InitEval(const vector<double>& x);

struct PawnStruct
{
	PawnStruct()
	{
		Clear();
	}

	void Clear();
	void Read(const Position& pos);

	U32 pawnHash;
	U64 passed;
	U64 doubled;
	U64 isolated;
	U64 backwards;
	U8 ranks[2][10];

	U64 attBy[2];
	U64 safe[2];
};

#endif

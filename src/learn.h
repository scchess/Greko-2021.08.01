//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef LEARN_H
#define LEARN_H

#include "types.h"

enum
{
	STOCHASTIC_GRADIENT_DESCENT = 0x01,
	COORDINATE_DESCENT = 0x02,
};

int    CountGames(const string& file);
bool   PgnToFen(const string& pgnFile, const string& fenFile, int minPly, int maxPly, int fensPerGame);
double Predict(const string& fenFile, const vector<double>& weights);
void   CoordinateDescent(const string& fenFile, vector<double>& x0, int numIters, const vector<double>& learnParams, U64 startTime);
void   Sgd(const string& fenFile, vector<double>& x0, const vector<double>& learnParams, U64 startTime);
double ScoreToProbability(double score);

#endif

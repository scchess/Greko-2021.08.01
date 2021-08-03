//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "eval_params.h"
#include "learn.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

extern string g_weightsFile;

void CoordinateDescentInfo(size_t param, double value, double y0Start, double y0, int iter, int numIters, U64 startTime);
void SgdInfo(double y0Start, double y0, double LR, int iter, U64 startTime);
void SgdIteration(const string& fenFile, vector<double>& x, double LR, const vector<double>& learnParams);

void CoordinateDescent(
	const string& fenFile,
	vector<double>& x0,
	int numIters,
	const vector<double>& learnParams,
	U64 startTime)
{
	double y0 = Predict(fenFile, x0);
	double y0Start = y0;

	cout << endl;
	cout << "Algorithm:     coordinate descent" << endl;
	cout << "Parameters:    " << x0.size() << endl;
	cout << "Initial value: " << left << y0Start << endl;
	cout << endl;

	for (int iter = 0; iter < numIters; ++iter)
	{
		for (size_t param = 0; param < learnParams.size(); ++param)
		{
			if (learnParams[param] == 0)
				continue;

			CoordinateDescentInfo(param, x0[param], y0Start, y0, iter, numIters, startTime);

			double step = static_cast<double>((iter == 0 && numIters > 1)? 4 : 1);
			while (step >= 1)
			{
				vector<double> x1 = x0;
				x1[param] += step;

				if (x1[param] != x0[param])
				{
					double y1 = Predict(fenFile, x1);
					if (y1 < y0)
					{
						x0 = x1;
						y0 = y1;
						CoordinateDescentInfo(param, x0[param], y0Start, y0, iter, numIters, startTime);
						WriteWeights(x0, g_weightsFile);
						continue;
					}
				}

				vector<double> x2 = x0;
				x2[param] -= step;

				if (x2[param] != x0[param])
				{
					double y2 = Predict(fenFile, x2);
					if (y2 < y0)
					{
						x0 = x2;
						y0 = y2;
						CoordinateDescentInfo(param, x0[param], y0Start, y0, iter, numIters, startTime);
						WriteWeights(x0, g_weightsFile);
						continue;
					}
				}

				step /= 2;
			}
		}
	}

	cout << endl;
	InitEval(x0);
}
////////////////////////////////////////////////////////////////////////////////

void CoordinateDescentInfo(
	size_t param,
	double value,
	double y0Start,
	double y0,
	int iter,
	int numIters,
	U64 startTime)
{
	int t = static_cast<int>((GetProcTime() - startTime) / 1000);
	int hh = t / 3600;
	int mm = (t - 3600 * hh) / 60;
	int ss = t - 3600 * hh - 60 * mm;

	stringstream buf;
	static int prev_len = 0;

	buf << right;
	buf << setw(2) << setfill('0') << hh << ":";
	buf << setw(2) << setfill('0') << mm << ":";
	buf << setw(2) << setfill('0') << ss << " ";
	buf << setfill(' ');

	buf << left << "      " << setw(8) << y0 <<
		" " << setprecision(6) << 100 * (y0 - y0Start) / y0Start << " % " <<
		" Iter " << iter + 1 << "/" << numIters << " " <<
		FeatureName(param) << " = " << (int)value;

	int len = (int)buf.str().length();
	for (int i = 0; i < prev_len - len; ++i)
		buf << " ";
	prev_len = len;

	buf << "\r";
	cout << buf.str();
}
////////////////////////////////////////////////////////////////////////////////

int CountGames(const string& file)
{
	int games = 0;
	ifstream ifs(file.c_str());
	if (ifs.good())
	{
		string s;
		while (getline(ifs, s))
		{
			if (s.find("[Event") == 0)
				++games;
		}
	}
	return games;
}
////////////////////////////////////////////////////////////////////////////////

double ErrSq(const string& s, const vector<double>& weights)
{
	char chRes = s[0];
	double result = 0;
	if (chRes == '1')
		result = 1;
	else if (chRes == '0')
		result = 0;
	else if (chRes == '=')
		result = 0.5;
	else
	{
		cout << "Illegal string: " << s << endl;
		return -1;
	}

	string fen = string(s.c_str() + 2);
	Position pos;
	if (!pos.SetFEN(fen))
	{
		cout << "ERR FEN: " << fen << endl;
		return -1;
	}

	vector<double> features;
	GetFeatures(pos, features);
	double e = DotProduct(weights, features);

	double prediction = ScoreToProbability(e);
	return (prediction - result) * (prediction - result);
}
////////////////////////////////////////////////////////////////////////////////

bool PgnToFen(const string& pgnFile,
	const string& fenFile,
	int minPly,
	int maxPly,
	int fensPerGame)
{
	RandSeed(time(0));

	ifstream ifs(pgnFile.c_str());
	if (!ifs.good())
	{
		cout << "Can't open file: " << pgnFile << endl << endl;
		return false;
	}

	ofstream ofs(fenFile.c_str());
	if (!ofs.good())
	{
		cout << "Can't open file: " << fenFile << endl << endl;
		return false;
	}

	string s;
	Position pos;
	vector<string> fens;
	int games = 0;
	int total = CountGames(pgnFile);

	while (getline(ifs, s))
	{
		if (s.empty())
			continue;
		if (s.find("[Event") == 0)
		{
			++games;
			cout << "Processing: game " << games << " of " << total << "\r";
			pos.SetInitial();
			fens.clear();
			continue;
		}
		if (s.find("[") == 0)
			continue;

		vector<string> tokens;
		Split(s, tokens, ". ");

		for (size_t i = 0; i < tokens.size(); ++i)
		{
			string tk = tokens[i];
			if (tk == "1-0" || tk == "0-1" || tk == "1/2-1/2" || tk == "*")
			{
				char r = '?';
				if (tk == "1-0")
					r = '1';
				else if (tk == "0-1")
					r = '0';
				else if (tk == "1/2-1/2")
					r = '=';
				else
				{
					fens.clear();
					break;
				}

				if (!fens.empty())
				{
					for (int j = 0; j < fensPerGame; ++j)
					{
						int index = Rand32() % fens.size();
						ofs << r << " " << fens[index] << endl;
					}
				}

				fens.clear();
				break;
			}

			if (!CanBeMove(tk))
				continue;

			Move mv = StrToMove(tk, pos);
			if (!mv.IsNull() && pos.MakeMove(mv))
			{
				if (pos.Ply() >= minPly && pos.Ply() <= maxPly)
				{
					if (!pos.InCheck() && !mv.Captured() && !mv.Promotion())
						fens.push_back(pos.FEN());
				}
			}
		}
	}
	cout << endl;
	return true;
}
////////////////////////////////////////////////////////////////////////////////

double Predict(const string& fenFile, const vector<double>& weights)
{
	ifstream ifs(fenFile.c_str());
	int n = 0;
	double errSqSum = 0;
	string s;

	while (getline(ifs, s))
	{
		if (s.length() < 5)
			continue;

		double errSq = ErrSq(s, weights);
		if (errSq < 0)
			continue;

		++n;
		errSqSum += errSq;
	}

	return sqrt(errSqSum / n);
}
////////////////////////////////////////////////////////////////////////////////

double ScoreToProbability(double score)
{
	return static_cast<double>(1. / (1. + exp(-score / 180)));
}
////////////////////////////////////////////////////////////////////////////////

void Sgd(const string& fenFile,
	vector<double>& x0,
	const vector<double>& learnParams,
	U64 startTime)
{
	double y0 = Predict(fenFile, x0);
	double y0Start = y0;

	cout << endl;
	cout << "Algorithm:     stochastic gradient descent" << endl;
	cout << "Parameters:    " << x0.size() << endl;
	cout << "Initial value: " << left << y0Start << endl;
	cout << endl;

	vector<double> x = x0;
	y0 = 1e10;

	double LR = 1.0;
	int iter = 0;

	while (LR > 1e-10)
	{
		SgdIteration(fenFile, x, LR, learnParams);
		double y = Predict(fenFile, x);
		if (y < y0)
		{
			SgdInfo(y0Start, y, LR, ++iter, startTime);
			y0 = y;
			x0 = x;
			g_w = x;
			WriteWeights(g_w, g_weightsFile);
		}
		else
		{
			SgdInfo(y0Start, y0, LR, ++iter, startTime);
			cout << "\n";

			LR /= 10.0;
			iter = 0;
		}
	}

	InitEval(x0);
}
////////////////////////////////////////////////////////////////////////////////

void SgdInfo(double y0Start, double y0, double LR, int iter, U64 startTime)
{
	int t = static_cast<int>((GetProcTime() - startTime) / 1000);
	int hh = t / 3600;
	int mm = (t - 3600 * hh) / 60;
	int ss = t - 3600 * hh - 60 * mm;

	cout << right;
	cout << setw(2) << setfill('0') << hh << ":";
	cout << setw(2) << setfill('0') << mm << ":";
	cout << setw(2) << setfill('0') << ss << " ";
	cout << setfill(' ');

	printf("      %8lf %8.6lf %c   (%d) LR = %12.10lf\r",
		y0, 100 * (y0 - y0Start) / y0Start, '%', iter, LR);
}
////////////////////////////////////////////////////////////////////////////////

void SgdIteration(const string& fenFile,
	vector<double>& x,
	double LR,
	const vector<double>& learnParams)
{
	ifstream ifs(fenFile.c_str());
	string s;

	while (getline(ifs, s))
	{
		if (s.length() < 5)
			continue;

		char chRes = s[0];
		double result = 0;
		if (chRes == '1')
			result = 1;
		else if (chRes == '0')
			result = 0;
		else if (chRes == '=')
			result = 0.5;
		else
			continue;

		string fen = string(s.c_str() + 2);
		Position pos;
		if (!pos.SetFEN(fen))
			continue;

		vector<double> features;
		GetFeatures(pos, features);

		double e = DotProduct(x, features);
		double prob = ScoreToProbability(e);

		for (size_t i = 0; i < x.size(); ++i)
			x[i] -= LR * features[i] * (prob - result) * prob * (1 - prob) * learnParams[i];
	}

	for (size_t i = 0; i < x.size(); ++i)
		x[i] = static_cast<double>((int)x[i]);
}
////////////////////////////////////////////////////////////////////////////////


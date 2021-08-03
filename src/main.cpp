//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "learn.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

const string PROGRAM_NAME = "GreKo 2021.08";
const string AUTHOR_NAME = "Vladimir Medvedev";
const string RELEASE_DATE = "01-Aug-2021";

string g_weightsFile = "weights.txt";

const int MIN_HASH_SIZE = 1;
const int MAX_HASH_SIZE = 4096;
const int DEFAULT_HASH_SIZE = 128;

Position g_pos;
deque<string> g_queue;
FILE* g_log = NULL;

bool g_console = false;
bool g_xboard = false;
bool g_uci = true;

static string g_s;
static vector<string> g_tokens;
static bool g_force = false;

static int g_increment = 0;
static int g_restTime = 0;
static int g_restMoves = 0;
static int g_movesPerSession = 0;

void OnZero();
void ParseSelfplayLimits(const char* s, int& gamesLimit, int& timeLimitInSeconds);

void CalculateTimeLimits()
{
	if (g_restTime == 0)
		return;

	Search::s_params.maxTimeSoft = g_restTime / 40;
	Search::s_params.maxTimeHard = g_restTime / 2;

	Log("TIME: rest = %d, moves = %d, inc = %d ==> stHard = %d, stSoft = %d",
		g_restTime, g_restMoves, g_increment, Search::s_params.maxTimeHard, Search::s_params.maxTimeSoft);
}
////////////////////////////////////////////////////////////////////////////////

void OnAnalyze()
{
	Search::ClearHash();
	Position pos = g_pos;

	Search::s_params.analysis = true;
	Search::s_params.limitTime = false;
	Search::s_params.maxDepth = 0;
	Search::s_params.maxNodes = 0;
	Search::s_params.maxTimeHard = 0;
	Search::s_params.maxTimeSoft = 0;
	Search::s_params.silent = false;

	Search::StartSearch(pos);
}
////////////////////////////////////////////////////////////////////////////////

void OnDump()
{
	ofstream ofs("default_params.cpp");
	if (!ofs.good())
		return;
	ofs << "\tstatic const double src[] =\n";
	ofs << "\t{\n";
	ofs << "\t\t";

	stringstream ss;
	size_t cur_len = 0;
	for (size_t i = 0; i < g_w.size(); ++i)
	{
		ss << g_w[i];
		if (i != g_w.size() - 1)
		{
			ss << ",";
			if (ss.str().length() - cur_len > 65)
			{
				ss << "\n\t\t";
				cur_len = ss.str().length();
			}
			else
				ss << " ";
		}
	}
	ofs << ss.str() << "\n\t};\n";
}
////////////////////////////////////////////////////////////////////////////////

void OnEval()
{
	Out("%d\n", Evaluate(g_pos));
}
////////////////////////////////////////////////////////////////////////////////

void OnFEN()
{
	Out("%s\n", g_pos.FEN().c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnFlip()
{
	g_pos.Mirror();
	g_pos.Print();
}
////////////////////////////////////////////////////////////////////////////////

void OnGoUci()
{
	Search::s_params.analysis = false;
	Search::s_params.maxDepth = 0;
	Search::s_params.maxNodes = 0;
	Search::s_params.maxTimeHard = 0;
	Search::s_params.maxTimeSoft = 0;
	Search::s_params.silent = false;
	g_restTime = 0;

	for (size_t i = 1; i < g_tokens.size(); ++i)
	{
		string token = g_tokens[i];
		if (token == "infinite")
		{
			Search::s_params.analysis = true;
		}
		else if (i < g_tokens.size() - 1)
		{
			if (token == "movetime")
			{
				int t = atoi(g_tokens[i + 1].c_str());
				Search::s_params.limitTime = true;
				Search::s_params.maxTimeHard = t;
				Search::s_params.maxTimeSoft = t;
				++i;
			}
			else if ((token == "wtime" && g_pos.Side() == WHITE) || (token == "btime" && g_pos.Side() == BLACK))
			{
				Search::s_params.limitTime = true;
				g_restTime = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if ((token == "winc" && g_pos.Side() == WHITE) || (token == "binc" && g_pos.Side() == BLACK))
			{
				Search::s_params.limitTime = true;
				g_increment = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if (token == "movestogo")
			{
				Search::s_params.limitTime = true;
				g_restMoves = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if (token == "depth")
			{
				Search::s_params.limitDepth = true;
				Search::s_params.maxDepth = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if (token == "nodes")
			{
				Search::s_params.limitNodes = true;
				Search::s_params.maxNodes = atoi(g_tokens[i + 1].c_str());
				++i;
			}
		}
	}

	CalculateTimeLimits();
	Search::StartSearch(g_pos);

	Move bestMove = Search::s_results.bestMove;
	if (!bestMove.IsNull())
		Out("bestmove %s\n", MoveToStrLong(bestMove).c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnGo()
{
	if (g_uci)
	{
		OnGoUci();
		return;
	}

	Search::s_params.analysis = false;
	Search::s_params.silent = false;

	g_force = false;

	string result, comment;
	if (Search::IsGameOver(g_pos, result, comment))
	{
		Out("%s %s\n", result.c_str(), comment.c_str());
		return;
	}

	CalculateTimeLimits();
	Search::StartSearch(g_pos);

	Move bestMove = Search::s_results.bestMove;
	if (!bestMove.IsNull())
	{
		Highlight(true);
		Out("move %s\n\n", MoveToStrLong(bestMove).c_str());
		Highlight(false);

		g_pos.MakeMove(bestMove);

		if (Search::IsGameOver(g_pos, result, comment))
			Out("%s %s\n", result.c_str(), comment.c_str());
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnIsready()
{
	Out("readyok\n");
}
////////////////////////////////////////////////////////////////////////////////

void OnLearn(string fileName, int numIters, U8 alg, U64 startTime)
{
	string pgnFile, fenFile;

	if (fileName.find(".pgn") != string::npos)
	{
		pgnFile = fileName;
		size_t extPos = fileName.find(".pgn");
		if (extPos != string::npos)
			fenFile = fileName.substr(0, extPos) + ".fen";
	}
	else if (fileName.find(".fen") != string::npos)
	{
		fenFile = fileName;
		size_t extPos = fileName.find(".fen");
		if (extPos != string::npos)
			pgnFile = fileName.substr(0, extPos) + ".pgn";
	}
	else
	{
		pgnFile = fileName + ".pgn";
		fenFile = fileName + ".fen";
	}

	{
		ifstream test(fenFile.c_str());
		if (!test.good())
		{
			if (!PgnToFen(pgnFile, fenFile, 6, 999, 1))
				return;
		}
	}

	vector<double> x0 = g_w;

	vector<double> learnParams;
	learnParams.resize(g_w.size());

	if (!ReadWeights(learnParams, "learn_params.txt"))
	{
		for (size_t i = 0; i < learnParams.size(); ++i)
			learnParams[i] = 1;
	}

	if (alg & (STOCHASTIC_GRADIENT_DESCENT | COORDINATE_DESCENT))
	{
		if (alg & STOCHASTIC_GRADIENT_DESCENT)
			Sgd(fenFile, x0, learnParams, startTime);
		if (alg & COORDINATE_DESCENT)
			CoordinateDescent(fenFile, x0, numIters, learnParams, startTime);
	}
	else
		Out("Unknown algorithm: %d\n", alg);
}
////////////////////////////////////////////////////////////////////////////////

void OnLearn()
{
	if (g_tokens.size() < 2)
		return;

	string fileName = g_tokens[1];

	int alg = (g_tokens.size() > 2)?
		atoi(g_tokens[2].c_str()) :
		STOCHASTIC_GRADIENT_DESCENT;

	int numIters = (g_tokens.size() > 3)? atoi(g_tokens[3].c_str()) : 2;

	OnLearn(fileName, numIters, alg, GetProcTime());
	Out("\n");
}
////////////////////////////////////////////////////////////////////////////////

void OnLevel()
{
	if (g_tokens.size() > 1)
		g_movesPerSession = atoi(g_tokens[1].c_str());
	if (g_tokens.size() > 3)
		g_increment = 1000 * atoi(g_tokens[3].c_str());

	Search::s_params.limitDepth = false;
	Search::s_params.limitNodes = false;
	Search::s_params.limitTime = true;
}
////////////////////////////////////////////////////////////////////////////////

void OnList()
{
	MoveList mvlist;
	GenAllMoves(g_pos, mvlist);

	stringstream ss;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		ss << MoveToStrLong(mv) << " ";
	}
	ss << " -- total: " << mvlist.Size() << endl;
	Out("%s\n", ss.str().c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnLoad()
{
	if (g_tokens.size() < 2)
		return;

	ifstream ifs(g_tokens[1].c_str());
	if (!ifs.good())
	{
		Out("Can't open file: %s\n", g_tokens[1].c_str());
		return;
	}

	int line = 1;
	if (g_tokens.size() > 2)
	{
		line = atoi(g_tokens[2].c_str());
		if (line <= 0)
			line = 1;
	}

	string fen;
	for (int i = 0; i < line; ++i)
	{
		if(!getline(ifs, fen)) break;
	}

	if (g_pos.SetFEN(fen))
	{
		g_pos.Print();
		Out("%s\n\n", fen.c_str());
	}
	else
		Out("Illegal FEN\n");
}
////////////////////////////////////////////////////////////////////////////////

void OnMT()
{
	if (g_tokens.size() < 2)
		return;

	ifstream ifs(g_tokens[1].c_str());
	if (!ifs.good())
	{
		Out("Can't open file: %s\n", g_tokens[1].c_str());
		return;
	}

	string s;
	while (getline(ifs, s))
	{
		Position pos;
		if (pos.SetFEN(s))
		{
			Out("%s\n", s.c_str());
			EVAL e1 = Evaluate(pos);
			pos.Mirror();
			EVAL e2 = Evaluate(pos);
			if (e1 != e2)
			{
				pos.Mirror();
				pos.Print();
				Out("e1 = %d, e2 = %d\n\n", e1, e2);
				return;
			}
		}
	}
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnNew()
{
	g_force = false;
	g_pos.SetInitial();

	Search::ClearHash();
}
////////////////////////////////////////////////////////////////////////////////

void OnPerft()
{
	if (g_tokens.size() < 2)
		return;
	int depth = atoi(g_tokens[1].c_str());
	Search::StartPerft(g_pos, depth);
}
////////////////////////////////////////////////////////////////////////////////

void OnPgnToFen()
{
	if (g_tokens.size() < 3)
		return;

	string pgnFile = g_tokens[1];
	string fenFile = g_tokens[2];
	int fensPerGame = 1;
	if (g_tokens.size() >= 4)
		fensPerGame = atoi(g_tokens[3].c_str());
	PgnToFen(pgnFile, fenFile, 6, 999, fensPerGame);
}
////////////////////////////////////////////////////////////////////////////////

void OnPing()
{
	if (g_tokens.size() > 1)
		Out("pong %s\n", g_tokens[1].c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnPosition()
{
	if (g_tokens.size() < 2)
		return;

	size_t movesTag = 0;
	if (g_tokens[1] == "fen")
	{
		string fen = "";
		for (size_t i = 2; i < g_tokens.size(); ++i)
		{
			if (g_tokens[i] == "moves")
			{
				movesTag = i;
				break;
			}
			if (!fen.empty())
				fen += " ";
			fen += g_tokens[i];
		}
		g_pos.SetFEN(fen);
	}
	else if (g_tokens[1] == "startpos")
	{
		g_pos.SetInitial();
		for (size_t i = 2; i < g_tokens.size(); ++i)
		{
			if (g_tokens[i] == "moves")
			{
				movesTag = i;
				break;
			}
		}
	}

	if (movesTag)
	{
		for (size_t i = movesTag + 1; i < g_tokens.size(); ++i)
		{
			Move mv = StrToMove(g_tokens[i], g_pos);
			g_pos.MakeMove(mv);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnPredict()
{
	if (g_tokens.size() < 2)
		return;

	string file = g_tokens[1];
	if (file.find(".fen") == string::npos)
		file += ".fen";

	Out("%.10lf\n", Predict(file, g_w));
}
////////////////////////////////////////////////////////////////////////////////

void OnProtover()
{
	Out("feature myname=\"%s\" setboard=1 analyze=1 colors=0 san=0 ping=1 name=1 done=1\n", PROGRAM_NAME.c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnSample()
{
	if (g_tokens.size() < 4)
	{
		printf("Usage: sample <src> <dest> <num_lines>\n");
		return;
	}

	FILE* src = fopen(g_tokens[1].c_str(), "rt");
	if (src == NULL)
	{
		printf("Can't open %s\n", g_tokens[1].c_str());
		return;
	}

	FILE* dst = fopen(g_tokens[2].c_str(), "wt");
	if (dst == NULL)
	{
		printf("Can't open %s\n", g_tokens[2].c_str());
		fclose(src);
		return;
	}

	int N = 0;
	char buf[256];
	vector<int> v;

	while (fgets(buf, sizeof(buf), src))
	{
		++N;
		v.push_back(N);
	}
	fclose(src);

	int Nout = atoi(g_tokens[3].c_str());
	if (Nout > N)
	{
		printf("Argument %d is too large, file has only %d lines\n", Nout, N);
		fclose(dst);
		return;
	}

	srand((unsigned int)time(0));
	random_shuffle(v.begin(), v.end());
	sort(v.begin(), v.begin() + Nout);

	src = fopen(g_tokens[1].c_str(), "rt");
	if (src == NULL)
	{
		printf("Can't open %s\n", g_tokens[1].c_str());
		return;
	}

	N = 0;
	int i = 0;
	while (fgets(buf, sizeof(buf), src))
	{
		++N;
		if (N == v[i])
		{
			int len = (int)strlen(buf);
			if (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n'))
				buf[len - 1] = 0;
			fprintf(dst, "%s\n", buf);
			++i;
			if (i >= Nout)
				break;
		}
	}

	fclose(src);
	fclose(dst);
}
////////////////////////////////////////////////////////////////////////////////

void OnSD()
{
	if (g_tokens.size() < 2)
		return;

	Search::s_params.limitDepth = true;
	Search::s_params.limitNodes = false;
	Search::s_params.limitTime = false;

	Search::s_params.maxDepth = atoi(g_tokens[1].c_str());
	Search::s_params.maxNodes = 0;
	Search::s_params.maxTimeHard = 0;
	Search::s_params.maxTimeSoft = 0;
}
////////////////////////////////////////////////////////////////////////////////

void OnSEE()
{
	if (g_tokens.size() < 2)
		return;
	Move mv = StrToMove(g_tokens[1], g_pos);
	if (!mv.IsNull())
		Out("%d\n", Search::SEE(g_pos, mv));
}
////////////////////////////////////////////////////////////////////////////////

void OnSelfPlay(
	const string& pgn_file,
	int gamesLimit,
	int timeLimitInSeconds,
	U64 startTime)
{
	RandSeed(time(0));
	int games = 0;
	int lastReportTime = 0;

	Search::s_params.silent = true;
	U64 t0 = GetProcTime();

	while (1)
	{
		Position pos;
		pos.SetInitial();

		string result, comment;
		string pgn, line;

		while (!Search::IsGameOver(pos, result, comment))
		{
			Move mv;
			if (pos.Ply() < 6)
				mv = Search::GetRandomMove(pos);
			else
			{
				Search::StartSearch(pos);
				mv = Search::s_results.bestMove;
			}

			if (mv.IsNull())
			{
				result = "1/2-1/2";
				comment = "{Adjudication: zero move generated}";
				break;
			}

			MoveList mvlist;
			GenAllMoves(pos, mvlist);
			if (pos.Side() == WHITE)
			{
				stringstream ss;
				ss << pos.Ply() / 2 + 1 << ". ";
				line = line + ss.str();
			}
			line = line + MoveToStrShort(mv, pos, mvlist) + string(" ");

			pos.MakeMove(mv);

			if (line.length() > 60 && pos.Side() == WHITE)
			{
				pgn = pgn + "\n" + line;
				line.clear();
			}

			if (pos.Ply() > 600)
			{
				result = "1/2-1/2";
				comment = "{Adjudication: too long}";
				break;
			}
		}

		stringstream header;
		header << "[Event \"Self-play\"]" << endl <<
			"[Date \"" << CurrentDateStr() << "\"]" << endl <<
			"[White \"" << PROGRAM_NAME << "\"]" << endl <<
			"[Black \"" << PROGRAM_NAME << "\"]" << endl <<
			"[Result \"" << result << "\"]" << endl;

		if (!line.empty())
		{
			pgn = pgn + "\n" + line;
			line.clear();
		}

		++games;

		if (pos.Ply() > 20)
		{
			std::ofstream ofs;
			ofs.open(pgn_file.c_str(), ios::app);
			if (ofs.good())
				ofs << header.str() << pgn << endl << result << " " << comment << endl << endl;
		}
		else
			--games;

		U64 t1 = GetProcTime();

		bool breakCondition = false;
		if (gamesLimit > 0 && games >= gamesLimit)
			breakCondition = true;
		if (timeLimitInSeconds > 0 && static_cast<int>((t1 - t0) / 1000) >= timeLimitInSeconds)
			breakCondition = true;

		int t = static_cast<int>((GetProcTime() - startTime) / 1000);
		if (t - lastReportTime > 0 || breakCondition)
		{
			int hh = t / 3600;
			int mm = (t - 3600 * hh) / 60;
			int ss = t - 3600 * hh - 60 * mm;

			cout << right;
			cout << setw(2) << setfill('0') << hh << ":";
			cout << setw(2) << setfill('0') << mm << ":";
			cout << setw(2) << setfill('0') << ss << " ";
			cout << setfill(' ');

			cout << "Playing: game " << games;
			if (gamesLimit > 0)
				cout << " of " << gamesLimit;
			cout << "\r";

			lastReportTime = t;
		}

		if (breakCondition)
			break;
	}
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnSelfPlay()
{
	if (g_tokens.size() < 2)
		return;

	int gamesLimit = 0;
	int timeLimitInSeconds = 0;
	const char* s = g_tokens[1].c_str();

	ParseSelfplayLimits(s, gamesLimit, timeLimitInSeconds);
	OnSelfPlay("games.pgn", gamesLimit, timeLimitInSeconds, GetProcTime());
}
////////////////////////////////////////////////////////////////////////////////

void OnSetboard()
{
	if (g_pos.SetFEN(g_s.c_str() + 9))
		g_pos.Print();
	else
		Out("Illegal FEN\n");
}
////////////////////////////////////////////////////////////////////////////////

void OnSetoption()
{
	if (g_tokens.size() < 5)
		return;
	if (g_tokens[1] != "name" && g_tokens[3] != "value")
		return;

	string name = g_tokens[2];
	string value = g_tokens[4];

	if (name == "Hash")
		Search::SetHashSize(atoi(value.c_str()));
#ifndef SINGLE_THREAD
	else if (name == "Threads")
		Search::InitThreads(atoi(value.c_str()));
#endif
	else if (name == "MultiPV")
		Search::s_params.multipv = atoi(value.c_str());
	else if (name == "Strength")
		Search::SetStrength(atoi(value.c_str()));
	else if (name == "Log" && value == "true")
	{
		if (g_log == NULL)
			g_log = fopen("GreKo.log", "at");
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnSN()
{
	if (g_tokens.size() < 2)
		return;

	Search::s_params.limitDepth = false;
	Search::s_params.limitNodes = true;
	Search::s_params.limitTime = false;

	Search::s_params.maxDepth = 0;
	Search::s_params.maxNodes = atoi(g_tokens[1].c_str());
	Search::s_params.maxTimeHard = 0;
	Search::s_params.maxTimeSoft = 0;
}
////////////////////////////////////////////////////////////////////////////////

void OnST()
{
	if (g_tokens.size() < 2)
		return;

	Search::s_params.limitDepth = false;
	Search::s_params.limitNodes = false;
	Search::s_params.limitTime = true;

	Search::s_params.maxDepth = 0;
	Search::s_params.maxNodes = 0;
	Search::s_params.maxTimeHard = U32(1000 * atof(g_tokens[1].c_str()));
	Search::s_params.maxTimeSoft = U32(1000 * atof(g_tokens[1].c_str()));
}
////////////////////////////////////////////////////////////////////////////////

void OnTest()
{
}
////////////////////////////////////////////////////////////////////////////////

void OnTime()
{
	if (g_tokens.size() < 2)
		return;

	g_restTime = 10 * atoi(g_tokens[1].c_str());

	Search::s_params.limitDepth = false;
	Search::s_params.limitNodes = false;
	Search::s_params.limitTime = true;
}
////////////////////////////////////////////////////////////////////////////////

void OnTraining(int gamesLimit, int timeLimitInSeconds, int firstIter, int lastIter)
{
	RandSeed(time(0));

	char weights_file[256];
	sprintf(weights_file, "weights_%d.txt", firstIter - 1);
	WriteWeights(g_w, weights_file);
	cout << "Weights saved in " << weights_file << endl;
	U64 startTime = GetProcTime();

	for (int iter = firstIter; iter <= lastIter; ++iter)
	{
		cout << endl << "*** MATCH " << iter - firstIter + 1
			<< " of " << lastIter - firstIter + 1<< " ***" << endl;

		char pgn_file[256];
		char fen_file[256];

		sprintf(pgn_file, "games.pgn");
		sprintf(fen_file, "games_%d.fen", iter);
		sprintf(weights_file, "weights_%d.txt", iter);

		OnSelfPlay(pgn_file, gamesLimit, timeLimitInSeconds, startTime);
		PgnToFen(pgn_file, fen_file, 6, 999, 10);

		OnZero();
		OnLearn(fen_file, 2, STOCHASTIC_GRADIENT_DESCENT, startTime);

		WriteWeights(g_w, weights_file);
		cout << "Weights saved in " << weights_file << endl << endl;
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnTraining()
{
	if (g_tokens.size() < 2)
		return;

	// training 10000      -> 10000 games, 1 match
	// training 10000 6    -> 10000 games, 6 matches (numbering: 1-6)
	// training 10000 7 12 -> 10000 games, 6 matches (numbering: 7-12)
	// training 30m 10     -> 30 min per match, 10 matches
	// training 2h 10      -> 1 hour per match, 10 matches

	int gamesLimit = 0;
	int timeLimitInSeconds = 0;
	const char* s = g_tokens[1].c_str();
	ParseSelfplayLimits(s, gamesLimit, timeLimitInSeconds);

	// default values
	int firstIter = 1;
	int lastIter = 1;

	if (g_tokens.size() == 3)
		lastIter = atoi(g_tokens[2].c_str());
	else if (g_tokens.size() >= 4)
	{
		firstIter = atoi(g_tokens[2].c_str());
		lastIter = atoi(g_tokens[3].c_str());
	}

	OnTraining(gamesLimit, timeLimitInSeconds, firstIter, lastIter);
}
////////////////////////////////////////////////////////////////////////////////

void OnUCI()
{
	g_console = false;
	g_xboard = false;
	g_uci = true;

	Out("id name %s\n", PROGRAM_NAME.c_str());
	Out("id author %s\n", AUTHOR_NAME.c_str());

	Out("option name Hash type spin default %d min %d max %d\n",
		DEFAULT_HASH_SIZE,
		MIN_HASH_SIZE,
		MAX_HASH_SIZE);

#ifndef SINGLE_THREAD
	Out("option name Threads type spin default 1 min 1 max %d\n",
		MAX_NUM_THREADS);
#endif

	Out("option name MultiPV type spin default 1 min 1 max 16\n");
	Out("option name Strength type spin default 100 min 0 max 100\n");
	Out("option name Log type check default false\n");
	Out("uciok\n");
}
////////////////////////////////////////////////////////////////////////////////

void OnXboard()
{
	g_console = false;
	g_xboard = true;
	g_uci = false;

	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnZero()
{
	g_w.clear();
	g_w.resize(NUMBER_OF_FEATURES);
	WriteWeights(g_w, "weights.txt");
}
////////////////////////////////////////////////////////////////////////////////

void ParseSelfplayLimits(const char* s, int& gamesLimit, int& timeLimitInSeconds)
{
	if (strstr(s, "h"))
	{
		int n = 0;
		int scanned = sscanf(s, "%dh", &n);
		if (scanned == 1)
			timeLimitInSeconds = 3600 * n;
	}
	else if (strstr(s, "m"))
	{
		int n = 0;
		int scanned = sscanf(s, "%dm", &n);
		if (scanned == 1)
			timeLimitInSeconds = 60 * n;
	}
	else if (strstr(s, "s"))
	{
		int n = 0;
		int scanned = sscanf(s, "%ds", &n);
		if (scanned == 1)
			timeLimitInSeconds = n;
	}
	else
		gamesLimit = atoi(s);
}
////////////////////////////////////////////////////////////////////////////////

void OnQuit()
{
	Search::QuitThreads();
	exit(0);
}
////////////////////////////////////////////////////////////////////////////////

void RunCommandLine()
{
	while (1)
	{
		if (!g_queue.empty())
		{
			g_s = g_queue.front();
			g_queue.pop_front();
		}
		else
		{
			if (g_console)
			{
				if (g_pos.Side() == WHITE)
					cout << "White(" << g_pos.Ply() / 2 + 1 << "): ";
				else
					cout << "Black(" << g_pos.Ply() / 2 + 1 << "): ";
			}
			getline(cin, g_s);
		}

		if (g_s.empty())
			continue;

		Log("> %s\n", g_s.c_str());

		Split(g_s, g_tokens);
		if (g_tokens.empty())
			continue;

		string cmd = g_tokens[0];

#define ON_CMD(pattern, minLen, action) \
    if (Is(cmd, #pattern, minLen))      \
    {                                   \
      action;                           \
      continue;                         \
    }

		ON_CMD(analyze,    1, OnAnalyze())
		ON_CMD(board,      1, g_pos.Print())
		ON_CMD(dump,       2, OnDump())
		ON_CMD(eval,       2, OnEval())
		ON_CMD(fen,        2, OnFEN())
		ON_CMD(flip,       2, OnFlip())
		ON_CMD(force,      2, g_force = true)
		ON_CMD(go,         1, OnGo())
		ON_CMD(isready,    1, OnIsready())
		ON_CMD(learn,      3, OnLearn())
		ON_CMD(level,      3, OnLevel())
		ON_CMD(list,       2, OnList())
		ON_CMD(load,       2, OnLoad())
		ON_CMD(mt,         2, OnMT())
		ON_CMD(new,        1, OnNew())
		ON_CMD(perft,      2, OnPerft())
		ON_CMD(pgntofen,   2, OnPgnToFen())
		ON_CMD(ping,       2, OnPing())
		ON_CMD(position,   2, OnPosition())
		ON_CMD(predict,    3, OnPredict())
		ON_CMD(protover,   3, OnProtover())
		ON_CMD(quit,       1, OnQuit())
		ON_CMD(sample,     2, OnSample())
		ON_CMD(sd,         2, OnSD())
		ON_CMD(see,        2, OnSEE())
		ON_CMD(selfplay,   4, OnSelfPlay())
		ON_CMD(sp,         2, OnSelfPlay())
		ON_CMD(setboard,   3, OnSetboard())
		ON_CMD(setoption,  3, OnSetoption())
		ON_CMD(sn,         2, OnSN())
		ON_CMD(st,         2, OnST())
		ON_CMD(test,       2, OnTest())
		ON_CMD(time,       2, OnTime())
		ON_CMD(training,   2, OnTraining())
		ON_CMD(uci,        1, OnUCI())
		ON_CMD(ucinewgame, 4, OnNew())
		ON_CMD(undo,       1, g_pos.UnmakeMove())
		ON_CMD(xboard,     1, OnXboard())
		ON_CMD(zero,       1, OnZero())
#undef ON_CMD

		if (CanBeMove(cmd))
		{
			Move mv = StrToMove(cmd, g_pos);
			if (!mv.IsNull())
			{
				g_pos.MakeMove(mv);
				string result, comment;
				if (Search::IsGameOver(g_pos, result, comment))
					Out("%s %s\n\n", result.c_str(), comment.c_str());
				else if (!g_force)
					OnGo();
				continue;
			}
		}

		if (!g_uci)
			Out("Unknown command: %s\n", cmd.c_str());
	}
}
////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[])
{
	InitIO();

	InitBitboards();
	Position::InitHashNumbers();

	double hashMb = DEFAULT_HASH_SIZE;
	int threads = 1;
	int strength = 100;

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-log"))
			g_log = fopen("GreKo.log", "at");
		else if (i < argc - 1)
		{
			if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-hash"))
			{
				hashMb = atof(argv[i + 1]);
				if (hashMb < MIN_HASH_SIZE || hashMb > MAX_HASH_SIZE)
					hashMb = DEFAULT_HASH_SIZE;
			}
			else if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "-weights"))
			{
				g_weightsFile = argv[i + 1];
			}
			else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "-threads"))
			{
				threads = atoi(argv[i + 1]);
			}
			else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "-strength"))
			{
				strength = atoi(argv[i + 1]);
			}
		}
	}

	InitEval();
	g_pos.SetInitial();

	Search::SetHashSize(hashMb);
	Search::InitThreads(threads);
	Search::SetStrength(strength);

	if (IsPipe())
	{
		Out("%s by %s\n", PROGRAM_NAME.c_str(), AUTHOR_NAME.c_str());
		g_uci = true;
		g_xboard = false;
		g_console = false;
	}
	else
	{
		Highlight(true);
		cout << endl;
		Out("%s (%s)\n", PROGRAM_NAME.c_str(), RELEASE_DATE.c_str());
		cout << endl;
		Highlight(false);
		g_uci = false;
		g_xboard = false;
		g_console = true;
	}

	RunCommandLine();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////

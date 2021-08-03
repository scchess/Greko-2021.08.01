//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "learn.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

extern Position g_pos;
extern deque<string> g_queue;
extern bool g_console;
extern bool g_xboard;
extern bool g_uci;

SearchParams Search::s_params;
SearchResults Search::s_results;

EVAL         Search::s_alpha;
EVAL         Search::s_beta;
HashEntry*   Search::s_hash = NULL;
U8           Search::s_hashAge = 0;
size_t       Search::s_hashFull = 0;
size_t       Search::s_hashSize = 0;
U64          Search::s_hashMask = 0;
int          Search::s_iter = 0;
U64          Search::s_startTime = 0;
int          Search::s_numThreads = 1;
Position     Search::s_pos;
EVAL         Search::s_score;
SearchThread Search::s_threads[MAX_NUM_THREADS];

enum SortType
{
	SORT_HASH        = 6000000,
	SORT_CAPTURE     = 5000000,
	SORT_MATE_KILLER = 4000000,
	SORT_KILLER      = 3000000,
	SORT_REFUTATION  = 2000000,
	SORT_MAX_HISTORY = 1000000,
	SORT_OTHER       = 0
};

enum NodeType
{
	NODE_PV = 0,
	NODE_NON_PV = 1
};

const EVAL WINDOW_ROOT = 100;

const int USE_FUTILITY[]                = { 0, 1 };
const int USE_NULLMOVE[]                = { 0, 1 };
const int USE_LMR[]                     = { 0, 1 };
const int USE_PAWN_7TH_EXTENSIONS[]     = { 1, 1 };
const int USE_RECAPTURE_EXTENSIONS[]    = { 1, 1 };
const int USE_SINGLE_REPLY_EXTENSIONS[] = { 1, 1 };
const int USE_IID[]                     = { 1, 1 };
const int USE_SEE_PRUNING[]             = { 1, 1 };
const int USE_MATE_PRUNING[]            = { 1, 1 };
const int USE_HASH_EXACT_EVAL[]         = { 1, 1 };
const int USE_HASH_PRUNING[]            = { 1, 1 };
const int USE_QCHECKS[]                 = { 1, 1 };

const EVAL FUTILITY_MARGIN_ALPHA[] = { 0, 50, 350, 550 };
const EVAL FUTILITY_MARGIN_BETA[]  = { 0, 50, 350, 550 };

const int NULLMOVE_MIN_DEPTH = 2;
const int NULLMOVE_BASE_R = 3;
const double NULLMOVE_DEPTH_DIVISOR = 6.0;
const double NULLMOVE_EVAL_DIVISOR = 120.0;

const int LMR_MIN_DEPTH = 4;
const int LMR_MIN_MOVE = 3;
const double LMR_DEPTH_DIVISOR = 10.0;
const double LMR_MOVE_DIVISOR = 10.0;
const int LMR_MAX_SUCCESS_RATE = 50;

const int SEE_PRUNING_MIN_QPLY = 0;

static const EVAL SEE_VALUE[14] =
{
	0, 0, 100, 100, 300, 300, 300, 300, 500, 500, 900, 900, 20000, 20000
};

void Search::ClearHash()
{
	assert(s_hash != NULL);
	assert(s_hashSize > 0);
	memset((char*)s_hash, 0, s_hashSize * sizeof(HashEntry));
	s_hashAge = 0;
	s_hashFull = 0;
}
////////////////////////////////////////////////////////////////////////////////

int Search::CountLegalMoves(Position& pos, const MoveList& mvlist, int upperLimit)
{
	int legalMoves = 0;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			pos.UnmakeMove();
			++legalMoves;
			if (legalMoves >= upperLimit)
				break;
		}
	}
	return legalMoves;
}
////////////////////////////////////////////////////////////////////////////////

U64 Search::CurrentSearchTime()
{
	return GetProcTime() - s_startTime;
}
////////////////////////////////////////////////////////////////////////////////

Move Search::GetRandomMove(Position& pos)
{
	SearchThread& thread = s_threads[0];
	thread.NewSearch(pos);
	EVAL e0 = thread.AlphaBeta(-INFINITY_SCORE, INFINITY_SCORE, 1, 0);

	MoveList mvlist;
	GenAllMoves(pos, mvlist);
	vector<Move> cand_moves;

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			EVAL e = -thread.AlphaBetaQ(-INFINITY_SCORE, INFINITY_SCORE, 0, 0);
			pos.UnmakeMove();

			if (e >= e0 - 100)
				cand_moves.push_back(mv);
		}
	}

	if (cand_moves.empty())
		return Move(0);
	else
	{
		size_t ind = Rand32() % cand_moves.size();
		return cand_moves[ind];
	}
}
////////////////////////////////////////////////////////////////////////////////

void Search::InitThreads(int numThreads)
{
	for (int i = 1; i < s_numThreads; ++i)
		s_threads[i].Quit();

	if (numThreads > MAX_NUM_THREADS)
		numThreads = MAX_NUM_THREADS;
	if (numThreads < 1)
		numThreads = 1;

	s_numThreads = numThreads;

	for (int i = 1; i < s_numThreads; ++i)
		s_threads[i].Init(i);
}
////////////////////////////////////////////////////////////////////////////////

bool Search::IsGameOver(Position& pos, string& result, string& comment)
{
	if (pos.Bits(PW) == 0 &&
		pos.Bits(PB) == 0 &&
		pos.MatIndex(WHITE) < 5 &&
		pos.MatIndex(BLACK) < 5)
	{
		result = "1/2-1/2";
		comment = "{Insufficient material}";
		return true;
	}

	MoveList mvlist;
	GenAllMoves(pos, mvlist);

	if (CountLegalMoves(pos, mvlist, 1) == 0)
	{
		if (pos.InCheck())
		{
			if (pos.Side() == WHITE)
			{
				result = "0-1";
				comment = "{Black mates}";
			}
			else
			{
				result = "1-0";
				comment = "{White mates}";
			}
		}
		else
		{
			result = "1/2-1/2";
			comment = "{Stalemate}";
		}
		return true;
	}

	if (pos.Fifty() >= 100)
	{
		result = "1/2-1/2";
		comment = "{Fifty moves rule}";
		return true;
	}

	if (pos.Repetitions() >= 3)
	{
		result = "1/2-1/2";
		comment = "{Threefold repetition}";
		return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////

NODES Search::Perft(Position& pos, int depth, int ply)
{
	if (depth <= 0)
		return 1;

	NODES total = 0;

	MoveList mvlist;
	GenAllMoves(pos, mvlist);

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			total += Perft(pos, depth - 1, ply + 1);
			pos.UnmakeMove();
		}
	}

	return total;
}
////////////////////////////////////////////////////////////////////////////////

void Search::PrintPV(int multipv)
{
	const SearchThread& thread = s_threads[0];
	const vector<Move>& pv = thread.m_pvs[0];
	U64 time = CurrentSearchTime();

	if (pv.empty())
		return;

	stringstream ss;
	if (!g_uci)
	{
		ss <<
			setw(2) << s_iter <<
			setw(8) << s_score <<
			setw(10) << time / 10 <<
			setw(12) << thread.m_nodes;
		ss << "   ";
		Position tmp = s_pos;
		int plyCount = tmp.Ply();

		if (tmp.Side() == BLACK)
		{
			if (plyCount == 0)
				++plyCount;
			ss << plyCount / 2 + 1 << ". ... ";
		}

		for (size_t m = 0; m < pv.size(); ++m)
		{
			Move mv = pv[m];
			MoveList mvlist;
			GenAllMoves(tmp, mvlist);
			if (tmp.Side() == WHITE)
				ss << plyCount / 2 + 1 << ". ";
			++plyCount;
			ss << MoveToStrShort(mv, tmp, mvlist);
			if (!tmp.MakeMove(mv))
				break;
			if (tmp.InCheck())
			{
				if (s_score + m + 1 == CHECKMATE_SCORE)
					ss << "#";
				else if (s_score - (int)m + 1 == -CHECKMATE_SCORE)
					ss << "#";
				else
					ss << "+";
			}
			if (m == 0)
			{
				if (s_score >= s_beta)
				{
					ss << "!";
					break;
				}
				else if (s_score <= s_alpha)
					ss << "?";
			}
			if (m < pv.size() - 1)
				ss << " ";
		}
	}
	else
	{
		ss << "info depth " << s_iter;
		ss << " seldepth " << thread.m_selDepth;

		if (s_score > CHECKMATE_SCORE - 50 && s_score <= CHECKMATE_SCORE)
			ss << " score mate " << (CHECKMATE_SCORE - s_score) / 2 + 1;
		else if (s_score < -CHECKMATE_SCORE + 50 && s_score >= -CHECKMATE_SCORE)
			ss << " score mate " << (-CHECKMATE_SCORE - s_score) / 2;
		else
			ss << " score cp " << s_score;

		ss << " time " << time;
		ss << " nodes " << thread.m_nodes;

		if (!pv.empty())
		{
			if (s_params.multipv > 1)
				ss << " multipv " << multipv + 1;

			ss << " pv";
			for (size_t i = 0; i < pv.size(); ++i)
				ss << " " << MoveToStrLong(pv[i]);
		}
	}

	Out("%s\n", ss.str().c_str());
}
////////////////////////////////////////////////////////////////////////////////

HashEntry* Search::ProbeHash(const Position& pos)
{
	assert(s_hash != NULL);
	assert(s_hashSize > 0);

	U64 hash = pos.Hash();

	int index = static_cast<int>(hash & s_hashMask);
	HashEntry* pEntry = s_hash + index;

	if (pEntry->Fits(hash))
		return pEntry;
	else
		return NULL;
}
////////////////////////////////////////////////////////////////////////////////

void Search::QuitThreads()
{
	for (int i = 1; i < s_numThreads; ++i)
		s_threads[i].Quit();
}
////////////////////////////////////////////////////////////////////////////////

void Search::RecordHash(const Position& pos, Move mv, EVAL score, int depth, int ply, U8 hashType)
{
	assert(s_hash != NULL);
	assert(s_hashSize > 0);

	U64 hash = pos.Hash();
	int index = static_cast<int>(hash & s_hashMask);
	HashEntry& entry = s_hash[index];

	if (entry.IsEmpty())
		++s_hashFull;

	entry.SetMove(mv);
	entry.SetScore(score, ply);
	entry.SetDepth(depth);
	entry.SetType(hashType);
	entry.SetAge(s_hashAge);

	entry.LockHash(hash);
}
////////////////////////////////////////////////////////////////////////////////

EVAL Search::SEE(const Position& pos, Move mv)
{
	COLOR side = GetColor(mv.Piece());
	EVAL score = SEE_VALUE[mv.Captured()];
	EVAL target = SEE_VALUE[mv.Piece()];
	U64 occ = pos.BitsAll() & ~BB_SINGLE[mv.From()];

	return -SEE_Exchange(pos, mv.To(), side ^ 1, -score, target, occ);
}
////////////////////////////////////////////////////////////////////////////////

EVAL Search::SEE_Exchange(const Position& pos, FLD f, COLOR side, EVAL score, EVAL target, U64 occ)
{
	U64 x;
	FLD from = NF;
	EVAL newTarget = 0;

	// find least attacker
	do
	{
		// pawns
		x = BB_PAWN_ATTACKS[f][side ^ 1] & pos.Bits(PAWN | side) & occ;
		if (x)
		{
			from = LSB(x);
			newTarget = SEE_VALUE[PAWN | side];
			break;
		}

		// knights
		x = BB_KNIGHT_ATTACKS[f] & pos.Bits(KNIGHT | side) & occ;
		if (x)
		{
			from = LSB(x);
			newTarget = SEE_VALUE[KNIGHT | side];
			break;
		}

		// bishops
		x = BishopAttacks(f, occ) & pos.Bits(BISHOP | side) & occ;
		if (x)
		{
			from = LSB(x);
			newTarget = SEE_VALUE[BISHOP | side];
			break;
		}

		// rooks
		x = RookAttacks(f, occ) & pos.Bits(ROOK | side) & occ;
		if (x)
		{
			from = LSB(x);
			newTarget = SEE_VALUE[ROOK | side];
			break;
		}

		// queens
		x = QueenAttacks(f, occ) & pos.Bits(QUEEN | side) & occ;
		if (x)
		{
			from = LSB(x);
			newTarget = SEE_VALUE[QUEEN | side];
			break;
		}

		// kings
		x = BB_KING_ATTACKS[f] & pos.Bits(KING | side) & occ;
		if (x)
		{
			from = LSB(x);
			newTarget = SEE_VALUE[KING | side];
			break;
		}

		return score;
	} while (0);

	EVAL exchangeScore = -SEE_Exchange(pos, f, side ^ 1,
		-(score + target), newTarget,
		occ & ~BB_SINGLE[from]);

	return std::max(score, exchangeScore);
}
////////////////////////////////////////////////////////////////////////////////

void Search::SetHashSize(double mb)
{
	if (s_hash != NULL)
		delete[] s_hash;

	size_t Nmax = (size_t)(1024 * 1024 * mb / sizeof(HashEntry));

	s_hashSize = 1;
	while (2 * s_hashSize <= Nmax)
		s_hashSize *= 2;

	s_hash = new HashEntry[s_hashSize];
	s_hashMask = s_hashSize - 1;
}
////////////////////////////////////////////////////////////////////////////////

void Search::SetStrength(int level)
{
	level = std::max(0, level);
	level = std::min(100, level);

	if (level == 100)
	{
		s_params.limitKnps = false;
		s_params.maxKnps = 0.0;
		return;
	}

	double minKnps = 0.1;
	double maxKnps = 1000;

	s_params.limitKnps = true;
	double k = pow(maxKnps / minKnps, 0.01);
	s_params.maxKnps = (minKnps * pow(k, level));
}
////////////////////////////////////////////////////////////////////////////////

void Search::StartPerft(Position& pos, int depth)
{
	NODES total = 0;
	U64 t0 = GetProcTime();

	MoveList mvlist;
	GenAllMoves(pos, mvlist);

	cout << endl;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			NODES delta = Perft(pos, depth - 1, 1);
			total += delta;
			cout << " " << MoveToStrLong(mv) << " - " << delta << endl;
			pos.UnmakeMove();
		}
	}
	U64 t1 = GetProcTime();
	double dt = static_cast<double>((t1 - t0) / 1000.);

	cout << endl;
	cout << " Nodes: " << total << endl;
	cout << " Time:  " << dt << endl;
	if (dt > 0) cout << " Knps:  " << total / dt / 1000. << endl;
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void Search::StartSearch(const Position& pos)
{
	s_startTime = GetProcTime();
	s_pos = pos;
	++s_hashAge;

	s_results.bestMove = Move(0);
	s_results.depth = 0;

	for (int i = 0; i < s_numThreads; ++i)
		s_threads[i].NewSearch(pos);

	Log("SEARCH: %s", pos.FEN().c_str());

	if (g_console && !s_params.silent)
	{
		string result, comment;
		if (IsGameOver(s_pos, result, comment))
			Out("%s %s\n", result.c_str(), comment.c_str());
		Out("\n");
	}

	MoveList mvlist;
	GenAllMoves(pos, mvlist);

	const int legalMoves = CountLegalMoves(s_pos, mvlist, 2);
	if (legalMoves == 0) return;
	bool singleReply = (legalMoves == 1);

	for (int i = 0; i < s_numThreads; ++i)
		s_threads[i].Work();

	SearchThread& thread = s_threads[0];
	s_alpha = -INFINITY_SCORE;
	s_beta = INFINITY_SCORE;

	for (s_iter = 1; s_iter < MAX_PLY; ++s_iter)
	{
		thread.ClearExcludedMoves();
		for (int multipv = 0; multipv < s_params.multipv; ++multipv)
		{
			s_score = thread.AlphaBeta(s_alpha, s_beta, s_iter, 0);
			if (thread.Stopped()) break;

			U64 time = CurrentSearchTime();
			NODES nodes = thread.m_nodes;

			if (s_params.limitKnps && s_iter > 1)
			{
				U64 expectedTime = static_cast<U64>(nodes / s_params.maxKnps);
				while (time < expectedTime)
				{
					SleepMillisec(10);
					time = CurrentSearchTime();

					thread.CheckLimits();
					thread.CheckInput(true);
					if (thread.Stopped()) break;
				}
			}

			if (!s_params.silent)
				PrintPV(multipv);

			if (s_score > s_alpha && s_score < s_beta)
			{
				const vector<Move>& pv = thread.m_pvs[0];
				if (!pv.empty())
				{
					thread.ExcludeMove(pv[0]);   // for multipv mode
					s_results.bestMove = pv[0];
					s_results.depth = s_iter;
				}

				if (s_params.analysis == false)
				{
					if (s_params.limitTime &&
						time >= s_params.maxTimeSoft)
					{
						thread.Stop();
						Log("Search stopped by stSoft, dt = %d", time);
						break;
					}

					if (singleReply)
					{
						thread.Stop();
						break;
					}

					if (s_score + s_iter >= CHECKMATE_SCORE)
					{
						thread.Stop();
						break;
					}

					if (s_params.limitDepth &&
						s_iter >= s_params.maxDepth)
					{
						thread.Stop();
						break;
					}
				}

				if (s_params.multipv == 1)
				{
					s_alpha = std::max(s_score - WINDOW_ROOT / 2, -INFINITY_SCORE);
					s_beta = std::min(s_score + WINDOW_ROOT / 2, INFINITY_SCORE);
				}
			}
			else
			{
				s_alpha = -INFINITY_SCORE;
				s_beta = INFINITY_SCORE;
				--s_iter;
			}

			if (g_uci && time > 1000)
				Out("info time %d nodes %d nps %d hashfull %d\n",
					(int)time,
					(int)nodes,
					(int)(1000 * nodes / time),
					(int)(1000 * s_hashFull / s_hashSize));

		} // for (int i = 0; i < s_params.multipv; ++i)
	} // for (int depth = 1; depth < MAX_PLY; ++depth)

	if (g_console && !s_params.silent)
		Out("\n");

	for (int i = 1; i < s_numThreads; ++i)
		s_threads[i].Stop();

	if (s_params.analysis)
	{
		while (!thread.Stopped())
		{
			string s;
			getline(cin, s);
			thread.ProcessInput(s);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

EVAL SearchThread::AlphaBeta(const EVAL alpha, const EVAL beta, const int depth, const int ply)
{
	if (ply > MAX_PLY)
		return alpha;

	m_selDepth = std::max(m_selDepth, depth);
	m_selDepth = std::max(m_selDepth, ply);

	m_pvs[ply].clear();

	CheckLimits();
	CheckInput();

	if (Stopped())
		return alpha;

	Position& pos = m_pos;

	if (ply > 0 && pos.Repetitions() >= 2)
		return DRAW_SCORE;

	int nodeType = (beta - alpha > 1) ? NODE_PV : NODE_NON_PV;
	if (USE_MATE_PRUNING[nodeType])
	{
		if (alpha >= CHECKMATE_SCORE - ply)
			return alpha;
	}

	//
	//   PROBING HASH
	//

	Move hashMove;
	HashEntry* pEntry = Search::ProbeHash(pos);

	if (pEntry != NULL)
	{
		hashMove = pEntry->GetMove();
		if (pEntry->GetDepth() >= depth && ply > 0)
		{
			EVAL hashScore = pEntry->GetScore(ply);
			if (USE_HASH_EXACT_EVAL[nodeType] && pEntry->GetType() == HASH_EXACT)
				return hashScore;
			if (USE_HASH_PRUNING[nodeType])
			{
				if (pEntry->GetType() == HASH_ALPHA && hashScore <= alpha)
					return alpha;
				if (pEntry->GetType() == HASH_BETA && hashScore >= beta)
					return beta;
			}
		}
	}

	COLOR side = pos.Side();
	Move lastMove = pos.LastMove();
	bool inCheck = pos.InCheck();
	bool isNull = lastMove.IsNull();
	Move bestMove;
	EVAL score = alpha;
	U8 hashType = HASH_ALPHA;

	//
	//   QSEARCH
	//

	if (!inCheck && depth <= 0)
		return AlphaBetaQ(alpha, beta, ply, 0);

	//
	//   FUTILITY
	//

	EVAL staticScore = Evaluate(pos);

	if (USE_FUTILITY[nodeType] &&
		!inCheck &&
		!isNull &&
		depth >= 1 &&
		depth <= 3)
	{
		if (staticScore <= alpha - FUTILITY_MARGIN_ALPHA[depth])
			return AlphaBetaQ(alpha, beta, ply, 0);
		if (staticScore >= beta + FUTILITY_MARGIN_BETA[depth])
			return beta;
	}

	//
	//   NULLMOVE
	//

	if (USE_NULLMOVE[nodeType] &&
		!inCheck &&
		!isNull &&
		pos.MatIndex(side) > 0 &&
		depth >= NULLMOVE_MIN_DEPTH)
	{
		int R = static_cast<int>(
			NULLMOVE_BASE_R +
			(depth - NULLMOVE_MIN_DEPTH) / NULLMOVE_DEPTH_DIVISOR +
			std::max(0, staticScore - beta) / NULLMOVE_EVAL_DIVISOR);

		pos.MakeNullMove();
		EVAL nullScore = -AlphaBeta(-beta, -score, depth - 1 - R, ply + 1);
		pos.UnmakeNullMove();

		if (Stopped())
			return alpha;

		if (nullScore >= beta)
			return beta;
	}

	//
	//   IID
	//

	if (USE_IID[nodeType] && hashMove.IsNull() && depth > 4)
	{
		AlphaBeta(alpha, beta, depth - 4, ply);
		if (!m_pvs[ply].empty())
			hashMove = m_pvs[ply][0];
	}

	MoveList& mvlist = m_mvlists[ply];
	if (inCheck)
		GenMovesInCheck(pos, mvlist);
	else
		GenAllMoves(pos, mvlist);
	UpdateSortScores(mvlist, hashMove, ply, lastMove);

	bool singleReply = false;
	if (USE_SINGLE_REPLY_EXTENSIONS[nodeType])
	{
		int numReplies = 0;
		for (size_t i = 0; i < mvlist.Size(); ++i)
		{
			Move mv = mvlist[i].m_mv;
			if (pos.MakeMove(mv))
			{
				pos.UnmakeMove();
				if (++numReplies > 1)
					break;
			}
		}
		singleReply = (numReplies == 1);
	}

	int legalMoves = 0;
	int quietMoves = 0;
	int deltaHist = 1;

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = GetNextBest(mvlist, i);

		bool exclude = false;
		if (ply == 0)
		{
			for (size_t j = 0; j < m_exclude.size(); ++j)
			{
				if (mv == m_exclude[j])
				{
					exclude = true;
					break;
				}
			}
		}
		if (exclude)
			continue;

		if (pos.MakeMove(mv))
		{
			++m_nodes;
			++legalMoves;

			m_histTry[mv.To()][mv.Piece()] += deltaHist;

			if (m_id == 0 && ply == 0 && g_uci && Search::CurrentSearchTime() > 1000)
				Out("info currmove %s currmovenumber %d\n", MoveToStrLong(mv).c_str(), legalMoves);

			int newDepth = depth - 1;

			//
			//   EXTENSIONS
			//

			if (inCheck)
				++newDepth;

			if (ply + depth <= 2 * Search::CurrentIteration())
			{
				if (USE_PAWN_7TH_EXTENSIONS[nodeType])
				{
					if (mv.Piece() == PW && Row(mv.To()) == 1)
						++newDepth;
					else if (mv.Piece() == PB && Row(mv.To()) == 6)
						++newDepth;
				}
				else if (USE_RECAPTURE_EXTENSIONS[nodeType])
				{
					if (mv.To() == lastMove.To() &&
						mv.Captured() != NOPIECE &&
						lastMove.Captured() != NOPIECE)
					{
						++newDepth;
					}
				}
				else if (USE_SINGLE_REPLY_EXTENSIONS[nodeType])
				{
					if (singleReply)
						++newDepth;
				}
			}

			//
			//   LMR
			//

			int reduction = 0;
			if (USE_LMR[nodeType] &&
				depth >= LMR_MIN_DEPTH &&
				!isNull &&
				!inCheck &&
				!pos.InCheck() &&
				!mv.Captured() &&
				!mv.Promotion() &&
				SuccessRate(mv) <= LMR_MAX_SUCCESS_RATE)
			{
				++quietMoves;
				if (quietMoves >= LMR_MIN_MOVE)
				{
					reduction = static_cast<int>(
						1 +
						(depth - LMR_MIN_DEPTH) / LMR_DEPTH_DIVISOR +
						(quietMoves - LMR_MIN_MOVE) / LMR_MOVE_DIVISOR);
				}
			}

			EVAL e;
			if (legalMoves == 1)
				e = -AlphaBeta(-beta, -score, newDepth, ply + 1);
			else
			{
				e = -AlphaBeta(-score - 1, -score, newDepth - reduction, ply + 1);
				if (e > score && reduction > 0)
					e = -AlphaBeta(-score - 1, -score, newDepth, ply + 1);
				if (e > score && e < beta)
					e = -AlphaBeta(-beta, -score, newDepth, ply + 1);
			}

			pos.UnmakeMove();

			if (Stopped())
				return alpha;

			if (ply == 0 && legalMoves == 1)
				UpdatePV(mv, ply);

			if (e > score)
			{
				score = e;
				bestMove = mv;
				UpdatePV(mv, ply);
				hashType = HASH_EXACT;
			}

			if (score >= beta)
			{
				if (!mv.Captured() && !mv.Promotion())
				{
					if (score > CHECKMATE_SCORE - 50 && score < CHECKMATE_SCORE)
						m_mateKillers[ply] = mv;
					else
						m_killers[ply] = mv;

					m_refutations[ply][lastMove.To()][lastMove.Piece()] = mv;
					m_histSuccess[mv.To()][mv.Piece()] += deltaHist;
				}
				hashType = HASH_BETA;
				break;
			}
		}
	}

	if (legalMoves == 0)
	{
		if (inCheck)
			score = -CHECKMATE_SCORE + ply;
		else
			score = DRAW_SCORE;
	}
	else
	{
		if (pos.Fifty() >= 100)
			score = DRAW_SCORE;
	}

	if (!Stopped())
		Search::RecordHash(pos, bestMove, score, depth, ply, hashType);

	return score;
}
////////////////////////////////////////////////////////////////////////////////

EVAL SearchThread::AlphaBetaQ(const EVAL alpha, const EVAL beta, const int ply, const int qply)
{
	if (ply > MAX_PLY)
		return alpha;

	m_pvs[ply].clear();

	m_selDepth = std::max(m_selDepth, ply);

	CheckLimits();
	CheckInput();

	if (Stopped())
		return alpha;

	Position& pos = m_pos;

	int nodeType = (beta - alpha > 1) ? NODE_PV : NODE_NON_PV;
	bool inCheck = pos.InCheck();
	Move lastMove = pos.LastMove();
	EVAL score = alpha;
	EVAL staticScore = Evaluate(pos, alpha, beta);

	if (!inCheck)
	{
		if (staticScore > alpha)
			score = staticScore;
		if (score >= beta)
			return beta;
	}

	MoveList& mvlist = m_mvlists[ply];
	if (inCheck)
		GenMovesInCheck(pos, mvlist);
	else
	{
		GenCapturesAndPromotions(pos, mvlist, score - staticScore);
		if (qply < USE_QCHECKS[nodeType])
			AddSimpleChecks(pos, mvlist);
	}
	UpdateSortScores(mvlist, Move(0), ply, lastMove);

	int legalMoves = 0;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = GetNextBest(mvlist, i);

		if (USE_SEE_PRUNING[nodeType] &&
			!inCheck &&
			qply >= SEE_PRUNING_MIN_QPLY)
		{
			if (Search::SEE(pos, mv) < 0)
				continue;
		}

		if (pos.MakeMove(mv))
		{
			++m_nodes;
			++legalMoves;

			EVAL e = -AlphaBetaQ(-beta, -score, ply + 1, qply + 1);
			pos.UnmakeMove();

			if (Stopped())
				return alpha;

			if (e > score)
			{
				score = e;
				UpdatePV(mv, ply);
			}
			if (score >= beta)
				break;
		}
	}

	if (legalMoves == 0)
	{
		if (inCheck)
			score = -CHECKMATE_SCORE + ply;
	}

	return score;
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::CheckInput(bool force)
{
	if (m_id != 0 || Stopped() || Search::s_results.depth < 1)
		return;

	if (force || (m_nodes % 8192 == 0))
	{
		if (InputAvailable())
		{
			string s;
			getline(cin, s);
			Log("> %s\n", s.c_str());
			ProcessInput(s);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::CheckLimits()
{
	if (m_id != 0 || Stopped() || Search::s_results.depth < 1)
		return;

	if (Search::s_params.analysis == false)
	{
		if (Search::s_params.limitTime)
		{
			if (Search::CurrentSearchTime() >= Search::s_params.maxTimeHard)
			{
				Stop();
				Log("Search stopped by stHard, dt = %d", Search::CurrentSearchTime());
			}
		}

		if (Search::s_params.limitNodes &&
			m_nodes >= Search::s_params.maxNodes)
		{
			Stop();
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::ClearHistory()
{
	memset(m_histTry, 0, 64 * 14 * sizeof(int));
	memset(m_histSuccess, 0, 64 * 14 * sizeof(int));
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::ClearKillersAndRefutations()
{
	memset((void*)m_killers, 0, (MAX_PLY + 1) * sizeof(Move));
	memset((void*)m_mateKillers, 0, (MAX_PLY + 1) * sizeof(Move));
	memset((void*)m_refutations, 0, (MAX_PLY + 1) * 64 * 14 * sizeof(Move));
}
////////////////////////////////////////////////////////////////////////////////

Move SearchThread::GetNextBest(MoveList& mvlist, size_t i)
{
	for (size_t j = i + 1; j < mvlist.Size(); ++j)
	{
		if (mvlist[j].m_score > mvlist[i].m_score)
			swap(mvlist[i], mvlist[j]);
	}
	return mvlist[i].m_mv;
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::HelperProc()
{
#ifndef SINGLE_THREAD
	Log("Thread %d started\n", m_id);

	while (m_state != THREAD_QUIT)
	{
		{
			std::unique_lock<std::mutex> lock(m_stateMutex);
			m_state = THREAD_SLEEP;
			Log("Thread %d is sleeping\n", m_id);
			m_cv.wait(lock, [this] { return m_state != THREAD_SLEEP; });
			Log("Thread %d is awaken, m_state = %d\n", m_id, m_state);
		}

		if (m_state == THREAD_WORK)
		{
			int startDepth = 1 + m_id;
			Log("Thread %d is working from depth = %d\n", m_id, startDepth);

			EVAL alpha = -INFINITY_SCORE;
			EVAL beta = INFINITY_SCORE;
			int depth = 0;

			for (depth = startDepth; depth < MAX_PLY; ++depth)
			{
				std::lock_guard<std::mutex> lock(m_posMutex);

				EVAL e = AlphaBeta(alpha, beta, depth, 0);

				if (m_state != THREAD_WORK)
					break;

				if (e > alpha && e < beta)
				{
					alpha = std::max(e - WINDOW_ROOT / 2, -INFINITY_SCORE);
					beta = std::min(e + WINDOW_ROOT / 2, INFINITY_SCORE);
				}
				else
				{
					alpha = -INFINITY_SCORE;
					beta = INFINITY_SCORE;
					--depth;
				}
			}
			Log("Thread %d finished loop; depth = %d, m_state = %d\n",
				m_id,
				depth,
				m_state);
		}
	}

	Log("Thread %d is quitting\n", m_id);
#endif
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::Init(int id)
{
	Log("Init(%d) called\n", id);
	if (m_state != THREAD_NEW)
		return;
	m_id = id;
#ifndef SINGLE_THREAD
	m_thread = std::thread(std::bind(&SearchThread::HelperProc, this));
#endif
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::NewSearch(const Position& pos)
{
#ifndef SINGLE_THREAD
	std::lock_guard<std::mutex> lock(m_posMutex);
#endif

	ClearKillersAndRefutations();
	ClearHistory();
	m_nodes = 0;
	m_pos = pos;
	m_selDepth = 0;
	m_state = THREAD_WORK;
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::ProcessInput(const string& s)
{
	if (m_id != 0)
		return;

	vector<string> tokens;
	Split(s, tokens);
	if (tokens.empty())
		return;

	string cmd = tokens[0];

	if (Search::s_params.analysis)
	{
		if (CanBeMove(cmd))
		{
			Move mv = StrToMove(cmd, g_pos);
			if (!mv.IsNull())
			{
				g_pos.MakeMove(mv);
				g_queue.push_back("analyze");
				Stop();
			}
		}
		else if (Is(cmd, "undo", 1) && Search::s_params.analysis)
		{
			g_pos.UnmakeMove();
			g_queue.push_back("analyze");
			Stop();
		}
	}

	if (Search::s_params.analysis == false)
	{
		if (cmd == "?")
			Stop();
		else if (Is(cmd, "result", 1))
		{
			m_pvs[0].clear();
			Stop();
		}
	}

	if (Is(cmd, "board", 1))
		g_pos.Print();
	else if (Is(cmd, "exit", 1))
		Stop();
	else if (Is(cmd, "isready", 1))
		Out("readyok\n");
	else if (Is(cmd, "quit", 1))
	{
		Search::QuitThreads();
		exit(0);
	}
	else if (Is(cmd, "stop", 1))
		Stop();
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::Quit()
{
#ifndef SINGLE_THREAD
	Log("Quit(%d) called\n", m_id);

	{
		std::lock_guard<std::mutex> lock(m_stateMutex);
		m_state = THREAD_QUIT;
		m_cv.notify_one();
	}

	m_thread.join();
	m_state = THREAD_NEW;   // for future restart
#endif
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::Stop()
{
	Log("Stop(%d) called\n", m_id);
#ifndef SINGLE_THREAD
	std::lock_guard<std::mutex> lock(m_stateMutex);
	m_state = THREAD_SLEEP;
	m_cv.notify_one();
#else
	m_state = THREAD_SLEEP;
#endif
}
////////////////////////////////////////////////////////////////////////////////

int SearchThread::SuccessRate(Move mv)
{
	if (m_histTry[mv.To()][mv.Piece()] == 0)
		return 0;

	double histTry = static_cast<double>(m_histTry[mv.To()][mv.Piece()]);
	double histSuccess = static_cast<double>(m_histSuccess[mv.To()][mv.Piece()]);
	return int(100 * histSuccess / histTry);
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::UpdatePV(Move mv, int ply)
{
	if (ply >= MAX_PLY)
		return;

	vector<Move>& pv = m_pvs[ply];
	vector<Move>& child_pv = m_pvs[ply + 1];

	pv.clear();
	pv.push_back(mv);
	pv.insert(pv.end(), child_pv.begin(), child_pv.end());
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::UpdateSortScores(MoveList& mvlist, Move hashMove, int ply, Move lastMove)
{
	Move killerMove = m_killers[ply];
	Move mateKillerMove = m_mateKillers[ply];
	Move refutationMove = m_refutations[ply][lastMove.To()][lastMove.Piece()];

	for (size_t j = 0; j < mvlist.Size(); ++j)
	{
		Move mv = mvlist[j].m_mv;
		if (mv == hashMove)
			mvlist[j].m_score = SORT_HASH;
		else if (mv.Captured() || mv.Promotion())
		{
			int s_piece = mv.Piece() / 2;
			int s_captured = mv.Captured() / 2;
			int s_promotion = mv.Promotion() / 2;
			mvlist[j].m_score = SORT_CAPTURE + 6 * (s_captured + s_promotion) - s_piece;
		}
		else if (mv == mateKillerMove)
			mvlist[j].m_score = SORT_MATE_KILLER;
		else if (mv == killerMove)
			mvlist[j].m_score = SORT_KILLER;
		else if (mv == refutationMove)
			mvlist[j].m_score = SORT_REFUTATION;
		else
			mvlist[j].m_score = SORT_OTHER + SuccessRate(mv);
	}
}
////////////////////////////////////////////////////////////////////////////////

void SearchThread::Work()
{
	Log("Work(%d) called\n", m_id);

#ifndef SINGLE_THREAD
	std::lock_guard<std::mutex> lock(m_stateMutex);
	m_state = THREAD_WORK;
	m_cv.notify_one();
#else
	m_state = THREAD_WORK;
#endif
}
////////////////////////////////////////////////////////////////////////////////

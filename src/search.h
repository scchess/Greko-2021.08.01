//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef SEARCH_H
#define SEARCH_H

#ifndef SINGLE_THREAD
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

#include "position.h"
#include "utils.h"

const int MAX_PLY = 256;

struct SearchParams
{
	SearchParams() :
		analysis(false),
		silent(false),
		limitDepth(false),
		limitNodes(false),
		limitTime(true),
		limitKnps(false),
		maxDepth(0),
		maxNodes(0),
		maxTimeHard(2000),
		maxTimeSoft(2000),
		maxKnps(0.0),
		multipv(1)
	{}

	bool  analysis;
	bool  silent;

	bool  limitDepth;
	bool  limitNodes;
	bool  limitTime;
	bool  limitKnps;

	int    maxDepth;
	NODES  maxNodes;
	U32    maxTimeHard;
	U32    maxTimeSoft;
	double maxKnps;
	int    multipv;
};
////////////////////////////////////////////////////////////////////////////////

struct SearchResults
{
	Move         bestMove;
	int          depth;
};
////////////////////////////////////////////////////////////////////////////////

enum HashType
{
	HASH_ALPHA = 0,
	HASH_BETA  = 1,
	HASH_EXACT = 2
};

class HashEntry
{
public:
	HashEntry()
	{
		memset(this, 0, sizeof(HashEntry));
	}

	Move GetMove() const { return Move(m_mv); }
	int GetDepth() const { return m_depth; }
	EVAL GetScore(int ply) const
	{
		EVAL score = m_score;
		if (score > CHECKMATE_SCORE - 50 && score <= CHECKMATE_SCORE)
			score -= ply;
		if (score < -CHECKMATE_SCORE + 50 && score >= -CHECKMATE_SCORE)
			score += ply;
		return score;
	}
	U8 GetType() const { return m_type; }
	U8 GetAge() const { return m_age; }

	bool IsEmpty() const { return m_hashLock == 0; }
	bool Fits(U64 hash) const { return m_hashLock == (U32)(hash >> 32); }
	void LockHash(U64 hash) { m_hashLock = (U32)(hash >> 32); }

	void SetMove(Move mv) { m_mv = mv.ToInt(); }
	void SetDepth(int depth) { m_depth = (I16)depth; }
	void SetScore(EVAL score, int ply)
	{
		if (score > CHECKMATE_SCORE - 50 && score <= CHECKMATE_SCORE)
			score += ply;
		if (score < -CHECKMATE_SCORE + 50 && score >= CHECKMATE_SCORE)
			score -= ply;
		m_score = score;
	}
	void SetType(U8 type) { m_type = type; }
	void SetAge(U8 age) { m_age = age; }

private:
	U32 m_hashLock;   // 4
	U32 m_mv;         // 4
	I32 m_score;      // 4
	I16 m_depth;      // 2
	U8  m_type;       // 1
	U8  m_age;        // 1
};
////////////////////////////////////////////////////////////////////////////////

enum ThreadState
{
	THREAD_NEW   = 0,
	THREAD_SLEEP = 1,
	THREAD_WORK  = 2,
	THREAD_QUIT  = 3
};

class SearchThread
{
public:
	SearchThread() :
		m_id(0),
		m_state(THREAD_NEW) {}
	~SearchThread()
	{
#ifndef SINGLE_THREAD
		if (m_thread.joinable())
			m_thread.join();
#endif
	}

	EVAL AlphaBeta(const EVAL alpha, const EVAL beta, const int depth, const int ply);
	EVAL AlphaBetaQ(const EVAL alpha, const EVAL beta, const int ply, const int qply);
	void CheckInput(bool force = false);
	void CheckLimits();
	void ClearExcludedMoves() { m_exclude.clear(); }
	void ExcludeMove(Move mv) { m_exclude.push_back(mv); }
	void Init(int id);
	void NewSearch(const Position& pos);
	void ProcessInput(const string& s);
	void Stop();
	bool Stopped() const { return m_state != THREAD_WORK; }
	void Work();
	void Quit();

	NODES        m_nodes;
	vector<Move> m_pvs[MAX_PLY + 1];
	int          m_selDepth;

private:
	void ClearHistory();
	void ClearKillersAndRefutations();
	Move GetNextBest(MoveList& mvlist, size_t i);
	int  SuccessRate(Move mv);
	void UpdatePV(Move mv, int ply);
	void UpdateSortScores(MoveList& mvlist, Move hashMove, int ply, Move lastMove);

	vector<Move> m_exclude;
	int          m_histTry[64][14];
	int          m_histSuccess[64][14];
	int          m_id;
	Move         m_killers[MAX_PLY + 1];
	Move         m_mateKillers[MAX_PLY + 1];
	MoveList     m_mvlists[MAX_PLY + 1];
	Position     m_pos;
	Move         m_refutations[MAX_PLY + 1][64][14];
	ThreadState  m_state;

	void HelperProc();

#ifndef SINGLE_THREAD
	std::condition_variable m_cv;
	std::mutex              m_posMutex;
	std::mutex              m_stateMutex;
	std::thread             m_thread;
#endif
};
////////////////////////////////////////////////////////////////////////////////

const int MAX_NUM_THREADS = 16;

class Search
{
public:
	static void       ClearHash();
	static int        CurrentIteration() { return s_iter; }
	static U64        CurrentSearchTime();
	static Move       GetRandomMove(Position& pos);
	static void       InitThreads(int numThreads);
	static bool       IsGameOver(Position& pos, string& result, string& comment);
	static HashEntry* ProbeHash(const Position& pos);
	static void       QuitThreads();
	static void       RecordHash(const Position& pos, Move mv, EVAL score, int depth, int ply, U8 hashType);
	static EVAL       SEE(const Position& pos, Move mv);
	static void       SetHashSize(double mb);
	static void       SetStrength(int level);
	static void       StartPerft(Position& pos, int depth);
	static void       StartSearch(const Position& pos);

	static SearchParams  s_params;
	static SearchResults s_results;

private:
	static int        CountLegalMoves(Position& pos, const MoveList& mvlist, int upperLimit);
	static NODES      Perft(Position& pos, int depth, int ply);
	static void       PrintPV(int multipv);
	static EVAL       SEE_Exchange(const Position& pos, FLD f, COLOR side, EVAL score, EVAL target, U64 occ);

	static EVAL          s_alpha;
	static EVAL          s_beta;
	static HashEntry*    s_hash;
	static U8            s_hashAge;
	static size_t        s_hashFull;
	static size_t        s_hashSize;
	static U64           s_hashMask;
	static int           s_iter;
	static int           s_numThreads;
	static Position      s_pos;
	static EVAL          s_score;
	static U64           s_startTime;
	static SearchThread  s_threads[MAX_NUM_THREADS];
};
////////////////////////////////////////////////////////////////////////////////

#endif

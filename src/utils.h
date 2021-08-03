//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

string CurrentDateStr();
U64    GetProcTime();
void   Highlight(bool on);
void   InitIO();
bool   InputAvailable();
bool   IsPipe();
bool   Is(const string& cmd, const string& pattern, size_t minLen);
U32    Rand32();
U64    Rand64();
U64    Rand64(int bits);
double RandDouble();
void   RandSeed(U64 seed);
void   SleepMillisec(int msec);
void   Split(const string& s, vector<string>& tokens, const string& sep = " ");
string Timestamp();

extern FILE* g_log;

inline void Log(const char* x1)
{
	if (g_log == NULL) return;
	fprintf(g_log, "%s %s", Timestamp().c_str(), x1);
	fflush(g_log);
}

template <class T1, class T2>
void Log(T1 x1, T2 x2)
{
	if (g_log == NULL) return;
	fprintf(g_log, "%s ", Timestamp().c_str());
	fprintf(g_log, x1, x2);
	fflush(g_log);
}

template <class T1, class T2, class T3>
void Log(T1 x1, T2 x2, T3 x3)
{
	if (g_log == NULL) return;
	fprintf(g_log, "%s ", Timestamp().c_str());
	fprintf(g_log, x1, x2, x3);
	fflush(g_log);
}

template <class T1, class T2, class T3, class T4>
void Log(T1 x1, T2 x2, T3 x3, T4 x4)
{
	if (g_log == NULL) return;
	fprintf(g_log, "%s ", Timestamp().c_str());
	fprintf(g_log, x1, x2, x3, x4);
	fflush(g_log);
}

template <class T1, class T2, class T3, class T4, class T5>
void Log(T1 x1, T2 x2, T3 x3, T4 x4, T5 x5)
{
	if (g_log == NULL) return;
	fprintf(g_log, "%s ", Timestamp().c_str());
	fprintf(g_log, x1, x2, x3, x4, x5);
	fflush(g_log);
}

template <class T1, class T2, class T3, class T4, class T5, class T6>
void Log(T1 x1, T2 x2, T3 x3, T4 x4, T5 x5, T6 x6)
{
	if (g_log == NULL) return;
	fprintf(g_log, "%s ", Timestamp().c_str());
	fprintf(g_log, x1, x2, x3, x4, x5, x6);
	fflush(g_log);
}

inline void Out(const char* x1)
{
	printf("%s", x1);
	fflush(stdout);
	Log(x1);
}

template <class T1, class T2>
void Out(T1 x1, T2 x2)
{
	printf(x1, x2);
	fflush(stdout);
	Log(x1, x2);
}

template <class T1, class T2, class T3>
void Out(T1 x1, T2 x2, T3 x3)
{
	printf(x1, x2, x3);
	fflush(stdout);
	Log(x1, x2, x3);
}

template <class T1, class T2, class T3, class T4>
void Out(T1 x1, T2 x2, T3 x3, T4 x4)
{
	printf(x1, x2, x3, x4);
	fflush(stdout);
	Log(x1, x2, x3, x4);
}

template <class T1, class T2, class T3, class T4, class T5>
void Out(T1 x1, T2 x2, T3 x3, T4 x4, T5 x5)
{
	printf(x1, x2, x3, x4, x5);
	fflush(stdout);
	Log(x1, x2, x3, x4, x5);
}

#endif

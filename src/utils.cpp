//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include <stdarg.h>
#include "bitboards.h"
#include "utils.h"

extern FILE* g_log;

U64 g_rand64 = 42;

string CurrentDateStr()
{
	time_t t = time(0);   // get time now
	struct tm* now = localtime(&t);
	stringstream ss;
	ss << (now->tm_year + 1900) << "." << (now->tm_mon + 1) << "." <<  now->tm_mday;
	return ss.str();
}
////////////////////////////////////////////////////////////////////////////////

bool Is(const string& cmd, const string& pattern, size_t minLen)
{
	return (pattern.find(cmd) == 0 && cmd.length() >= minLen);
}
////////////////////////////////////////////////////////////////////////////////

U32 Rand32()
{
	return U32(Rand64() >> 32);
}
////////////////////////////////////////////////////////////////////////////////

U64 Rand64()
{
	static const U64 A = LL(2862933555777941757);
	static const U64 B = LL(3037000493);
	g_rand64 = A * g_rand64 + B;
	return g_rand64;
}
////////////////////////////////////////////////////////////////////////////////

U64 Rand64(int bits)
{
	U64 r = 0;
	for (int i = 0; i < bits; ++i)
	{
		int index = Rand32() % 64;
		if (r & BB_SINGLE[index])
		{
			--i;
			continue;
		}
		r |= BB_SINGLE[index];
	}
	return r;
}
////////////////////////////////////////////////////////////////////////////////

double RandDouble()
{
	double x = static_cast<double>(Rand32());
	x /= LL(0xffffffff);
	return x;
}
////////////////////////////////////////////////////////////////////////////////

void RandSeed(U64 seed)
{
	g_rand64 = seed;
}
////////////////////////////////////////////////////////////////////////////////

void Split(const string& s, vector<string>& tokens, const string& sep)
{
	size_t i = 0, begin = 0;
	bool inWord = false;
	tokens.clear();

	for (i = 0; i < s.length(); ++i)
	{
		if (sep.find_first_of(s[i]) != string::npos)
		{
			// separator character
			if (inWord)
				tokens.push_back(s.substr(begin, i - begin));
			inWord = false;
		}
		else
		{
			// non-separator character
			if (!inWord)
				begin = i;
			inWord = true;
		}
	}
	if (inWord) tokens.push_back(s.substr(begin, i - begin));
}
////////////////////////////////////////////////////////////////////////////////

string Timestamp()
{
	time_t t = time(0);   // get time now
	struct tm* now = localtime(&t);
	char buf[16];
	sprintf(buf, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);
	return string(buf);
}
////////////////////////////////////////////////////////////////////////////////

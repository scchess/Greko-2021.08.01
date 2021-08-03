//   GreKo chess engine
//   (c) 2002-2021 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "moves.h"
#include "notation.h"

bool CanBeMove(const string& s)
{
	static const string goodChars = "12345678abcdefghNBRQKOxnbrq=-+#!?";
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (goodChars.find_first_of(s[i]) == string::npos)
			return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////

string FldToStr(FLD f)
{
	char buf[3];
	buf[0] = 'a' + Col(f);
	buf[1] = '8' - Row(f);
	buf[2] = 0;
	return string(buf);
}
////////////////////////////////////////////////////////////////////////////////

FLD StrToFld(const string& s)
{
	if (s.length() < 2)
		return NF;

	int col = s[0] - 'a';
	int row = '8' - s[1];
	if (col < 0 || col > 7 || row < 0 || row > 7)
		return NF;

	return 8 * row + col;
}
////////////////////////////////////////////////////////////////////////////////

Move StrToMove(const string& s, Position& pos)
{
	string s1 = s;
	while (s1.length() > 2)
	{
		const string special = "+#?!";
		char ch = s1[s1.length() - 1];
		if (special.find_first_of(ch) == string::npos) break;
		s1 = s1.substr(0, s1.length() - 1);
	}

	MoveList mvlist;
	GenAllMoves(pos, mvlist);

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (MoveToStrLong(mv) == s1)
		{
			if (pos.MakeMove(mv))
			{
				pos.UnmakeMove();
				return mv;
			}
		}
	}

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (MoveToStrShort(mv, pos, mvlist) == s1)
		{
			if (pos.MakeMove(mv))
			{
				pos.UnmakeMove();
				return mv;
			}
		}
	}

	return Move(0);
}
////////////////////////////////////////////////////////////////////////////////

string MoveToStrLong(Move mv)
{
	string s = FldToStr(mv.From()) + FldToStr(mv.To());
	switch (mv.Promotion())
	{
		case QW: case QB: s += "q"; break;
		case RW: case RB: s += "r"; break;
		case BW: case BB: s += "b"; break;
		case NW: case NB: s += "n"; break;
		default: break;
	}
	return s;
}
////////////////////////////////////////////////////////////////////////////////

string MoveToStrShort(Move mv, Position& pos, const MoveList& mvlist)
{
	if (mv == MOVE_O_O[WHITE] || mv == MOVE_O_O[BLACK])
		return "O-O";
	if (mv == MOVE_O_O_O[WHITE] || mv == MOVE_O_O_O[BLACK])
		return "O-O-O";

	string strPiece, strCapture, strFrom, strTo, strPromotion;
	FLD from = mv.From();
	FLD to = mv.To();
	PIECE piece = mv.Piece();
	PIECE captured = mv.Captured();
	PIECE promotion = mv.Promotion();

	switch (piece)
	{
		case PW: case PB: break;
		case NW: case NB: strPiece = "N"; break;
		case BW: case BB: strPiece = "B"; break;
		case RW: case RB: strPiece = "R"; break;
		case QW: case QB: strPiece = "Q"; break;
		case KW: case KB: strPiece = "K"; break;
		default: break;
	}

	strTo = FldToStr(to);

	if (captured)
	{
		strCapture = "x";
		if (piece == PW || piece == PB)
			strFrom = FldToStr(from).substr(0, 1);
	}

	switch (promotion)
	{
		case QW: case QB: strPromotion = "=Q"; break;
		case RW: case RB: strPromotion = "=R"; break;
		case BW: case BB: strPromotion = "=B"; break;
		case NW: case NB: strPromotion = "=N"; break;
		default: break;
	}

	bool ambiguity = false;
	bool uniqCol = true;
	bool uniqRow = true;

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv1 = mvlist[i].m_mv;
		if (mv1.From() == from)
			continue;
		if (mv1.To() != to)
			continue;
		if (mv1.Piece() != piece)
			continue;

		if (!pos.MakeMove(mv1))
			continue;
		pos.UnmakeMove();

		ambiguity = true;
		if (Col(mv1.From()) == Col(from))
			uniqCol = false;
		if (Row(mv1.From()) == Row(from))
			uniqRow = false;
	}

	if (ambiguity)
	{
		if (uniqCol)
			strFrom = FldToStr(from).substr(0, 1);
		else if (uniqRow)
			strFrom = FldToStr(from).substr(1, 1);
		else
			strFrom = FldToStr(from);
	}

	return strPiece + strFrom + strCapture + strTo + strPromotion;
}
////////////////////////////////////////////////////////////////////////////////

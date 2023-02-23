#pragma once

typedef tuple<string, int, string> lextabline;
typedef tuple<string, int, int> sentstructline;

typedef vector<lextabline> lextab;
typedef vector<sentstructline> sentstruct;

lextab create_lt(string line);

sentstruct create_sst(lextab line);

void print_lt_sst(lextab lt, sentstruct sst, int sentnum, string sent);
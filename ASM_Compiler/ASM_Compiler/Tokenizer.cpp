#include "Dependencies.h"
#include "Tokenizer.h"
#include "Dictionary.h"

string aquire_lex_info(string lex)
{
	string info = "";
	if (Analyze(Identifier, lex.c_str()) >= 0)
	{
		info = info + 'i';
		if (Analyze(Reg, lex.c_str()) >= 0)
		{
			info = info + "_r";
			if (Analyze(EightBitReg, lex.c_str()) >= 0)
			{
				info = info + '8';
			}
			else if (Analyze(SixteenBitReg, lex.c_str()) >= 0)
			{
				info = info + "16";
			}
			if (Analyze(SegReg, lex.c_str()) >= 0)
			{
				info = info + 's';
			}
		}
		else if (Analyze(Instr, lex.c_str()) >= 0)
		{
			info = info + "_i";
		}
		else if (Analyze(Dir, lex.c_str()) >= 0)
		{
			info = info + "_d";
			if (Analyze(END, lex.c_str()) >= 0)
			{
				info = info + "ep";
			}
			else if (Analyze(SEGMENT, lex.c_str()) >= 0)
			{
				info = info + "s";
			}
			else if (Analyze(ENDS, lex.c_str()) >= 0)
			{
				info = info + "es";
			}
			else if (Analyze(MACRO, lex.c_str()) >= 0)
			{
				info = info + "m";
			}
			else if (Analyze(ENDM, lex.c_str()) >= 0)
			{
				info = info + "em";
			}
			else if (Analyze(DB, lex.c_str()) >= 0)
			{
				info = info + "t1";
			}
			else if (Analyze(DW, lex.c_str()) >= 0)
			{
				info = info + "t2";
			}
			else if (Analyze(DD, lex.c_str()) >= 0)
			{
				info = info + "t4";
			}
		}
		else if (Analyze(Type, lex.c_str()) >= 0)
		{
			info = info + "_t";
			if (Analyze(BYTE, lex.c_str()) >= 0)
			{
				info = info + "1";
			}
			else if (Analyze(WORD, lex.c_str()) >= 0)
			{
				info = info + "2";
			}
		}
		else if (Analyze(Op, lex.c_str()) >= 0)
		{
			info = info + "_o";
			if (Analyze(PTR, lex.c_str()) >= 0)
			{
				info = info + "p";
			}
		}
		else
		{
			info = info + "_u";
		}
	}
	else if (Analyze(NumConst, lex.c_str()) >= 0)
	{
		info = info + "c_n";
		if (Analyze(BinConst, lex.c_str()) >= 0)
		{
			info = info + "b";
		}
		else if (Analyze(DecConst, lex.c_str()) >= 0)
		{
			info = info + "d";
		}
		else if (Analyze(HexConst, lex.c_str()) >= 0)
		{
			info = info + "h";
		}
	}
	else if ((Analyze(Single, lex.c_str())) >= 0)
	{
		info = info + "s";
	}
	return info;
}

lextab create_lt(string line)
{
	lextab s;
	regex lexeme("[A-Z0-9\\x27\\x22]+|[\\:\\[\\]\\+,]");
	smatch found;
	while (regex_search(line, found, lexeme)) 
	{
		for (auto x : found)
		{
			if (regex_match(static_cast<string>(x), regex("(\\x22[^\\x22]+\\x22)|(\\x27[^\\x27]+\\x27)"))) s.emplace_back(x, x.length() - 2, "c_t");
			else s.emplace_back(x, x.length(), aquire_lex_info(static_cast<string>(x)));
		}
		line = found.suffix().str();
	}
	return s;
}

sentstruct create_sst(lextab line)
{
	sentstruct s;
	auto start = line.begin();
	auto end = line.end();

	int cur_lex_num = 1;

	int label = 0;
	int name = 0;
	int mnem = 0;
	int op1 = 0;
	int op2 = 0;
	while (start != end)
	{
		int field_size = 1;
		tuple<string, int, string> cur_lt_line;
		string cur_lex;
		string cur_lex_desc;

		cur_lt_line = *start;
		cur_lex = get<0>(cur_lt_line);
		cur_lex_desc = get<2>(cur_lt_line);

		if (mnem == 0)
		{
			if (regex_match(cur_lex_desc, regex("(i_u)")))
			{

				start++;
				if (start != end)
				{
					cur_lt_line = *start;
					cur_lex = get<0>(cur_lt_line);
					cur_lex_desc = get<2>(cur_lt_line);

					if (cur_lex == ":")
					{
						s.emplace_back("label", cur_lex_num, field_size);

						label = 1;
					}
					else if (regex_match(cur_lex_desc, regex("(i_d)[a-z1-4]+")))
					{
						s.emplace_back("name", cur_lex_num, field_size);

						name = 1;
					}
					else
					{
						s.emplace_back("mnem", cur_lex_num, field_size);

						mnem = 1;
					}
				}
				else
				{
					s.emplace_back("mnem", cur_lex_num, field_size);

					mnem = 1;
				}
				start--;
			}
			else if (regex_match(cur_lex_desc, regex("(i_d)[a-z1-4]+|(i_i)")))
			{
				s.emplace_back("mnem", cur_lex_num, field_size);

				mnem = 1;
			}
		}
		else
		{
			if (op1 == 0)
			{
				int first_lex_num = cur_lex_num;
				start++;
				if (start == end)
				{
					s.emplace_back("op1", first_lex_num, field_size);

					op1 = 1;

					break;
				}
				cur_lt_line = *start;
				cur_lex = get<0>(cur_lt_line);

				while ((cur_lex != ",") && (start != end))
				{
					field_size++;
					cur_lex_num++;
					start++;
					if (start == end) continue;
					cur_lt_line = *start;
					cur_lex = get<0>(cur_lt_line);
				}
				s.emplace_back("op1", first_lex_num, field_size);

				op1 = 1;

				if (cur_lex == ",")
				{
					start++;
					cur_lex_num++;
				}
			}
			else
			{
				int first_lex_num = cur_lex_num;
				start++;
				while (start != end)
				{
					field_size++;
					cur_lex_num++;
					start++;
					if (start == end) continue;
					cur_lt_line = *start;
					cur_lex = get<0>(cur_lt_line);
				}
				s.emplace_back("op2", first_lex_num, field_size);

				op2 = 1;

				break;
			}
		}
		if (start == end) break;
		start++;
		cur_lex_num++;
	}
	return s;
}
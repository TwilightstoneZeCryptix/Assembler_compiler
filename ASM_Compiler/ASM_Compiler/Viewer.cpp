#include "Dependencies.h"
#include "Tokenizer.h"
#include "Reserved_identifiers_table.h"
#include "Viewer.h"

int total_errors = 0;

vector<string> macrolib;

vector<string> macropartab;
vector<string> macroargtab;

int active_seg = 0;

typedef tuple<int, string, int, int> segtabline;
vector<segtabline> segtab;

typedef tuple<string, string, int, int, bool> useridstabline;
vector<useridstabline> useridstab;

typedef tuple<string, int, string> allocated_mem;
vector <allocated_mem> mem_distribution;

typedef vector<lextabline> oplexs; //operand lexems

/*
* represents location of first lexem of each part of sentence.If element equals - 1, lexeme is missing.
* #1 - label, #2 - name, #3 - mnemonic, #4 - lexems of first operand, #5 - lexems of second operand.
*/
typedef tuple<int, int, int, int, int> sentcode;
typedef tuple<string, string, string, oplexs, oplexs> sentmask;

typedef vector<string> sentbytesvals;
typedef tuple<int, int, sentbytesvals, string> listing_line;

struct
{
	bool in_segment;
	bool macro;
	bool macro_def;
	bool cur_is_ENDM;
	bool init_macro;
	bool end_of_view;
} signs;

template <int bd>
string hex_val(int val, int format) //make adapts depending on value
{
	string value;
	bitset<bd> bin_val(val);
	ostringstream transformed;
	transformed << setw(format) << setfill('0') << hex << uppercase << bin_val.to_ulong();
	value = transformed.str();
	return value;
}

int num_const_to_integer(string nc, char type)
{
	regex hex_const("[0-9A-F]+");
	regex bin_const("[01]+");
	regex dec_const("[0-9]+");
	smatch match;

	int res;

	if (type == 'd')
	{
		regex_search(nc, match, dec_const);
		nc = match.str();
		res = stoi(nc, nullptr, 10);
	}
	else if (type == 'b')
	{
		regex_search(nc, match, bin_const);
		nc = match.str();
		res = stoi(nc, nullptr, 2);
	}
	else if (type == 'h')
	{
		regex_search(nc, match, hex_const);
		nc = match.str();
		if (nc[0] == '0') nc.insert(1, 1, 'X');
		res = stoi(nc, nullptr, 16);
	}

	return res;
}

template string hex_val<8>(int val, int format);
template string hex_val<16>(int val, int format);
template string hex_val<32>(int val, int format);

int locate_lexem(sentstruct sst, string type)
{
	int location = -1;
	for (sentstructline x : sst)
	{
		if (get<0>(x) == type)
		{
			location = get<1>(x);
		}
	}
	return location;
}

int get_current_offset(segtabline& stl)
{
	return get<3>(stl);
}

void increase_offset(segtabline& stl, int value)
{
	get<3>(stl) = get<3>(stl) + value;
}

void set_offset(segtabline& stl, int value)
{
	get<3>(stl) = value;
}

int get_lexems_num(sentstruct sst, int loc)
{
	int num = 0;
	for (sentstructline x : sst)
	{
		if (get<1>(x) == loc)
		{
			num = get<2>(x);
		}
	}
	return num;
}

string get_monolexem_sent_part(lextab lt, int loc)
{
	string sent_part;
	lextabline cur;
	int position = loc - 1;
	cur = lt.at(position);
	sent_part = get<0>(cur);
	return sent_part;
}

template <int op>
oplexs extract_operand_lexems(lextab lt, sentstruct ssc, sentcode sc)
{
	int position = get<op + 2>(sc) - 1;
	int num_of_lexems = get_lexems_num(ssc, position + 1);
	oplexs op_lexems;
	lextabline cur;
	for (int i = position; i <= (position + num_of_lexems - 1); i++)
	{
		cur = lt.at(i);
		op_lexems.emplace_back(get<0>(cur), get<1>(cur), get<2>(cur));
	}
	return op_lexems;
}

template oplexs extract_operand_lexems<1>(lextab lt, sentstruct ssc, sentcode sc);
template oplexs extract_operand_lexems<2>(lextab lt, sentstruct ssc, sentcode sc);

sentmask generate_sent_mask(lextab lt, sentstruct sst, sentcode sc)
{
	string no_label = "none";
	string no_name = "none";
	string no_mnem = "none";
	oplexs no_op;

	sentmask sm;

	if (get<0>(sc) == -1) get<0>(sm) = no_label;
	else
	{
		get<0>(sm) = get_monolexem_sent_part(lt, get<0>(sc));
	}

	if (get<1>(sc) == -1) get<1>(sm) = no_name;
	else
	{
		get<1>(sm) = get_monolexem_sent_part(lt, get<1>(sc));
	}

	if (get<2>(sc) == -1) get<2>(sm) = no_mnem;
	else
	{
		get<2>(sm) = get_monolexem_sent_part(lt, get<2>(sc));
	}

	if (get<3>(sc) == -1) get<3>(sm) = no_op;
	else
	{
		get<3>(sm) = extract_operand_lexems<1>(lt, sst, sc);
	}

	if (get<4>(sc) == -1) get<4>(sm) = no_op;
	else
	{
		get<4>(sm) = extract_operand_lexems<2>(lt, sst, sc);
	}

	return sm;
}

char determine_mnem(string mnem)
{
	char type = '?';
	for (DVline x : DV)
	{
		if (get<0>(x) == mnem) type = 'd';
	}
	for (ICVline x : ICV)
	{
		if (get<0>(x) == mnem) type = 'i';
	}
	for (MVline x : MV)
	{
		if (get<0>(x) == mnem) type = 'm';
	}
	return type;
}

void print_listing_line(listing_line ll, string listing_file_name)
{
	fstream result;
	result.open(listing_file_name + ".lst", ios::out | ios::app);

	if (get<2>(ll).empty() == true)
	{
		if (get<1>(ll) == -1) result << "\t" << get<0>(ll) << "      " << get<3>(ll) << endl;
		else result << "\t" << get<0>(ll) << " " << hex_val<16>(get<1>(ll), 4) << " " << get<3>(ll) << endl;
	}
	else
	{
		result << "\t" << get<0>(ll) << " " << hex_val<16>(get<1>(ll), 4) << " ";
		for (string x : get<2>(ll))
		{
			result << x << " ";
		}
		result << "\t" << get<3>(ll) << endl;
	}
}

void print_error(string listing_file_name)
{
	fstream result;
	result.open(listing_file_name + ".lst", ios::out | ios::app);
	result << "\t" << "Error" << endl;
	cout << "\t" << "Error" << endl;
}

void print_tabs(string listing_file_name)
{
	fstream result;
	result.open(listing_file_name + ".lst", ios::out | ios::app);

	result << endl;

	result << "\t" << setw(8) << left << "NAME" << "\t" << "TYPE" << "\t" << "VALUE" << "\t" << "ATTR" << endl;

	for (useridstabline x : useridstab)
	{
		if (get<3>(x) == -1) result << "\t" << setw(8) << left << get<0>(x) << "\t" << "NEAR" << "\t" << hex_val<16>(get<2>(x), 4) << "\t" << get<1>(x) << endl;
		else if (get<3>(x) == 1) result << "\t" << setw(8) << left << get<0>(x) << "\t" << "DB" << "\t" << hex_val<16>(get<2>(x), 4) << "\t" << get<1>(x) << endl;
		else if (get<3>(x) == 2) result << "\t" << setw(8) << left << get<0>(x) << "\t" << "DW" << "\t" << hex_val<16>(get<2>(x), 4) << "\t" << get<1>(x) << endl;
		else if (get<3>(x) == 4) result << "\t" << setw(8) << left << get<0>(x) << "\t" << "DD" << "\t" << hex_val<16>(get<2>(x), 4) << "\t" << get<1>(x) << endl;
	}

	result << endl;

	result << "\t" << setw(8) << left << "NAME" << "\t" << "SIZE" << "\t" << "LENGTH" << "\t" << "COMBINE" << "\t" << "CLASS" << endl;
	for (segtabline x : segtab)
	{
		result << "\t" << setw(8) << left << get<1>(x) << "\t" << get<2>(x) << "\t" << hex_val<16>(get<3>(x), 4) << "\t" << "PARA" << "\t" << "NONE" << endl;
	}

	result << endl;

	result << "\t" << "NAME" << endl;
	for (MVline x : MV)
	{
		result << "\t" << get<0>(x) << endl;
	}
}

int locate_macro(string macro_name)
{
	int location = -1;
	for (MVline x : MV)
	{
		if (get<0>(x) == macro_name) location = get<1>(x);
	}
	return location;
}

bool check_userid(string uid)
{
	bool existence = false;
	if (useridstab.empty() == false)
	{
		for (useridstabline x : useridstab)
		{
			if (get<0>(x) == uid) existence = true;
		}
	}
	return existence;
}

int get_label_offset(string label)
{
	int offset;
	for (useridstabline x : useridstab)
	{
		if (get<0>(x) == label) offset = get<2>(x);
	}
	return offset;
}

int get_userid_type(string id)
{
	int type = -2;
	for (useridstabline x : useridstab)
	{
		if (get<0>(x) == id) type = get<3>(x);
	}
	return type;
}

string find_op_type(oplexs operand)
{
	string optype = "uknown";
	if (operand.size() == 1)
	{
		lextabline opltl = operand.at(0);
		string op = get<0>(opltl);
		for (EBRVline x : EBRV)
		{
			if (get<0>(x) == op) optype = "reg8";
		}
		for (SBRVline x : SBRV)
		{
			if (get<0>(x) == op) optype = "reg16";
		}
		if ((optype != "reg8") && (optype != "reg16")) optype = "imm";
	}
	else
	{
		int mem_8 = 0;
		int mem_16 = 0;
		int tid = 0;
		for (lextabline x : operand)
		{
			if (get<2>(x) == "i_u")
			{
				if (get_userid_type(get<0>(x)) == 1) mem_8 = 1;
				else if (get_userid_type(get<0>(x)) == 2) mem_16 = 1;
			}
			else
			{
				for (string y : TV)
				{
					if (get<0>(x) == y)
					{
						if (get<0>(x) == "BYTE") tid = 1;
						else if (get<0>(x) == "WORD") tid = 2;
					}
				}
				for (string y : OV)
				{
					if (get<0>(x) == y)
					{
						if (get<0>(x) == "PTR")
						{
							if (tid == 1) mem_8 = 1;
							else if (tid == 2) mem_16 = 1;
						}
					}
				}
			}
		}
		if ((mem_8 == 1) && (mem_16 == 0)) optype = "mem8";
		else if ((mem_8 == 0) && (mem_16 == 1)) optype = "mem16";
	}
	return optype;
}

string get_mem_op_description(oplexs memop)
{
	string descr = "";

	int uterm = 0;
	int segshift = 0;
	int modrm = 0;
	int offset = 0;

	int adress_expr = 0;

	for (lextabline x : memop)
	{
		if (get<2>(x) == "i_u")
		{
			descr = descr + "-sr- ";
			descr = descr + "-at- ";
			segshift = 1;
		}
		if ((get<2>(x) == "i_rs") && (segshift == 0)) descr = descr + "-sr- ";
		if (get<0>(x) == "[") adress_expr = 1;
		if ((get<2>(x) == "i_r16") && (adress_expr == 1)) descr = descr + "-modrm- ";
		if ((get<2>(x) == "c_nd") && (get<0>(x) != "0") && (adress_expr == 1)) descr = descr + "-ofs- ";
	}
	return descr;
}

void set_opcode_lower_three_bytes(bitset<8>& opcode, bitset<3> val)
{
	for (int i = 2; i >= 0; i--)
	{
		opcode[i] = val[i];
	}
}

void set_modrm(bitset<8>& modrm, bitset<2> mod, bitset<3> reg, bitset<3> rm)
{
	for (int i = 7; i >= 6; i--)
	{
		modrm[i] = mod[i - 6];
	}
	for (int i = 5; i >= 3; i--)
	{
		modrm[i] = reg[i - 3];
	}
	for (int i = 2; i >= 0; i--)
	{
		modrm[i] = rm[i];
	}
}

void instruction_handler_for_mem_allocation(string& cur_s, segtabline& current_segment, allocated_mem& current_allocated_mem, int& current_offset, string instr, oplexs op1, oplexs op2)
{
	current_offset = get_current_offset(current_segment);

	get<0>(current_allocated_mem) = cur_s;
	get<1>(current_allocated_mem) = get_current_offset(current_segment);

	string tagline = "";
	string memtagline = "";

	string op1s = "empty";
	string op2s = "empty";
	op1s = find_op_type(op1);

	int modrm = 0;

	if (op2.empty() == false)
	{
		op2s = find_op_type(op2);
		if (op2s == "imm")
		{
			if ((op1s == "mem8") || (op1s == "reg8")) op2s = op2s + "8";
			else if ((op1s == "mem16") || (op1s == "reg16")) op2s = op2s + "16";
		}
	}

	for (ICVline x : ICV)
	{
		if ((get<0>(x) == instr) && (get<1>(x) == op1s) && (get<2>(x) == op2s))
		{
			if (get<6>(x) == 1)
			{
				tagline = tagline + "-ic1- ";
				increase_offset(current_segment, 1);
			}

			if (regex_search(get<7>(x), regex("/r|/[0-9]")) == true)
			{
				tagline = tagline + "-modrm- ";
				increase_offset(current_segment, 1);
				modrm = 1;
			}

			if ((regex_search(get<7>(x), regex("ib")) == true) || (op2s == "imm8"))
			{
				tagline = tagline + "-imm8- ";
				increase_offset(current_segment, 1);
			}
			else if ((regex_search(get<7>(x), regex("iw")) == true) || (op2s == "imm16"))
			{
				tagline = tagline + "-imm16- ";
				increase_offset(current_segment, 2);
			}
		}
	}

	if ((op1s == "mem8") || (op1s == "mem16")) memtagline = get_mem_op_description(op1);
	else if ((op2s == "mem8") || (op2s == "mem16")) memtagline = get_mem_op_description(op2);

	if (memtagline != "")
	{
		if (regex_search(memtagline, regex("-sr-")) == true)
		{
			tagline = "-sr- " + tagline;
			increase_offset(current_segment, 1);
		}
		if ((regex_search(memtagline, regex("-modrm-")) == true) && (modrm == 0))
		{
			tagline = tagline + "-modrm- ";
			increase_offset(current_segment, 1);
			modrm = 1;
		}
		if (regex_search(memtagline, regex("-at-")) == true)
		{
			tagline = tagline + "-at- ";
			increase_offset(current_segment, 2);
		}
		if (regex_search(memtagline, regex("-ofs-")) == true)
		{
			tagline = tagline + "-ofs- ";
			string val_line;
			int adress_expr = 0;
			int neg_marker = 0;
			for (lextabline x : op1)
			{
				if (get<0>(x) == "[") adress_expr = 1;
				
				if ((get<2>(x) == "c_nd") && (get<2>(x) != "0") && (adress_expr == 1))
				{
					if (neg_marker == 1) val_line = "-" + get<0>(x);
					else val_line = get<0>(x);
				}

				if ((adress_expr == 1) && (get<2>(x) == "-")) neg_marker = 1;
				else neg_marker = 0;
			}
			for (lextabline x : op2)
			{
				if (get<0>(x) == "[") adress_expr = 1;
				
				if ((get<2>(x) == "c_nd") && (get<2>(x) != "0") && (adress_expr == 1))
				{
					if (neg_marker == 1) val_line = "-" + get<0>(x);
					else val_line = get<0>(x);
				}

				if ((adress_expr == 1) && (get<2>(x) == "-")) neg_marker = 1;
				else neg_marker = 0;
			}
			int val = num_const_to_integer(val_line, 'd');
			if ((val >= -128) || (val < 128)) increase_offset(current_segment, 1);
			else increase_offset(current_segment, 2);
		}
	}

	get<2>(current_allocated_mem) = tagline;
}

sentbytesvals mem_dir_byte_determination(sentmask current_sm, allocated_mem current_am)
{
	sentbytesvals bytevals;

	string tagline = get<2>(current_am);
	regex bytenum1("-cnb1-");
	regex bytenum2("-cnb2-");
	regex bytestring1("-csb1-");
	regex wordnum1("-cnw1-");
	regex dwordnum1("-cnd1-");

	smatch bs;
	regex string_const("[A-Z0-9]+");

	oplexs op1 = get<3>(current_sm);
	oplexs op2 = get<4>(current_sm);

	if (regex_search(get<2>(current_am), bytestring1) == true)
	{
		lextabline op1l = op1.at(0);
		string s_const = get<0>(op1l);
		regex_search(s_const, bs, string_const);
		s_const = bs.str();
		string new_byte;
		for (char x : s_const)
		{
			new_byte = hex_val<8>(static_cast<int>(x), 2);
			bytevals.emplace_back(new_byte);
		}
	}
	else if ((regex_search(get<2>(current_am), bytenum1) == true) && (regex_search(get<2>(current_am), bytenum2) == true))
	{
		lextabline op1l = op1.at(0);
		lextabline op2l = op2.at(0);
		string op1s = get<0>(op1l);
		string op2s = get<0>(op2l);
		bytevals.emplace_back(hex_val<8>(num_const_to_integer(op1s, 'h'), 2));
		bytevals.emplace_back(hex_val<8>(num_const_to_integer(op2s, 'h'), 2));
	}
	else if ((regex_search(get<2>(current_am), bytenum1) == true))
	{
		lextabline op1l = op1.at(0);
		string op1s = get<0>(op1l);
		if (get<2>(op1l) == "c_nd") bytevals.emplace_back(hex_val<8>(num_const_to_integer(op1s, 'd'), 2));
		else if (get<2>(op1l) == "c_nb") bytevals.emplace_back(hex_val<8>(num_const_to_integer(op1s, 'b'), 2));
		else if (get<2>(op1l) == "c_nh") bytevals.emplace_back(hex_val<8>(num_const_to_integer(op1s, 'h'), 2));
	}
	else if ((regex_search(get<2>(current_am), wordnum1) == true))
	{
		lextabline op1l = op1.at(0);
		string op1s = get<0>(op1l);
		if (get<2>(op1l) == "c_nd") bytevals.emplace_back(hex_val<16>(num_const_to_integer(op1s, 'd'), 4));
		else if (get<2>(op1l) == "c_nb") bytevals.emplace_back(hex_val<16>(num_const_to_integer(op1s, 'b'), 4));
		else if (get<2>(op1l) == "c_nh") bytevals.emplace_back(hex_val<16>(num_const_to_integer(op1s, 'h'), 4));
	}
	else if ((regex_search(get<2>(current_am), dwordnum1) == true))
	{
		lextabline op1l = op1.at(0);
		string op1s = get<0>(op1l);
		if (get<2>(op1l) == "c_nd") bytevals.emplace_back(hex_val<32>(num_const_to_integer(op1s, 'd'), 8));
		else if (get<2>(op1l) == "c_nb") bytevals.emplace_back(hex_val<32>(num_const_to_integer(op1s, 'b'), 8));
		else if (get<2>(op1l) == "c_nh") bytevals.emplace_back(hex_val<32>(num_const_to_integer(op1s, 'h'), 8));
	}

	return bytevals;
}

sentbytesvals instruction_handler_for_byte_determination(string& cur_s, segtabline& current_segment, allocated_mem& current_allocated_mem, int& current_offset, string instr, oplexs op1, oplexs op2)
{
	sentbytesvals res;

	string seg_rep_prefix = "3E:";
	bitset<8> opcode1;
	bitset<8> modrm;
	string addres_or_offset = "none";
	string immediate = "none";

	bitset<2> field_mod;
	bitset<3> field_reg;
	bitset<3> field_rm;

	regex ic1("-ic1-");
	regex mrm("-modrm-");
	regex i8("-imm8-");
	regex i16("-imm16-");
	regex segrep("-sr-");
	regex adterm("-at-");
	regex ofst("-ofs-");

	regex reg8_in_opcode("\\+rb");
	regex reg16_in_opcode("\\+rw");
	regex val_in_reg("/[0-9]");

	string tagline = get<2>(current_allocated_mem);

	int offset_bit_depth = 1;

	current_offset = get_current_offset(current_segment);

	string op1s = "empty";
	string op2s = "empty";
	op1s = find_op_type(op1);

	if (op2.empty() == false)
	{
		op2s = find_op_type(op2);
		if (op2s == "imm")
		{
			if ((op1s == "mem8") || (op1s == "reg8")) op2s = op2s + "8";
			else if ((op1s == "mem16") || (op1s == "reg16")) op2s = op2s + "16";
		}
	}

	if (regex_search(tagline, regex("-ic1-")))
	{
		increase_offset(current_segment, 1);

		for (ICVline x : ICV)
		{
			if ((get<0>(x) == instr) && (get<1>(x) == op1s) && (get<2>(x) == op2s))
			{
				opcode1 = get<3>(x);

				if (regex_search(get<7>(x), val_in_reg) == true) field_reg = get<5>(x);

				if (regex_search(get<7>(x), reg8_in_opcode) == true)
				{
					lextabline reg8 = op1.at(0);
					for (EBRVline x : EBRV)
					{
						if (get<0>(x) == get<0>(reg8)) set_opcode_lower_three_bytes(opcode1, get<1>(x));
					}
				}
				else if (regex_search(get<7>(x), reg16_in_opcode) == true)
				{
					lextabline reg16 = op1.at(0);
					for (SBRVline x : SBRV)
					{
						if (get<0>(x) == get<0>(reg16)) set_opcode_lower_three_bytes(opcode1, get<1>(x));
					}
				}
			}
		}
	}

	if (regex_search(tagline, regex("-imm8-")))
	{
		increase_offset(current_segment, 1);

		lextabline op2l = op2.at(0);
		string op2s = get<0>(op2l);
		if (get<2>(op2l) == "c_nd") immediate = (hex_val<8>(num_const_to_integer(op2s, 'd'), 2));
		else if (get<2>(op2l) == "c_nb") immediate = (hex_val<8>(num_const_to_integer(op2s, 'b'), 2));
		else if (get<2>(op2l) == "c_nh") immediate = (hex_val<8>(num_const_to_integer(op2s, 'h'), 2));
	}
	else if (regex_search(tagline, regex("-imm16-")))
	{
		increase_offset(current_segment, 2);

		lextabline op2l = op2.at(0);
		string op2s = get<0>(op2l);
		if (get<2>(op2l) == "c_nd") immediate = (hex_val<16>(num_const_to_integer(op2s, 'd'), 4));
		else if (get<2>(op2l) == "c_nb") immediate = (hex_val<16>(num_const_to_integer(op2s, 'b'), 4));
		else if (get<2>(op2l) == "c_nh") immediate = (hex_val<16>(num_const_to_integer(op2s, 'h'), 4));
	}
	else
	{
		immediate = "none";
	}

	if (regex_search(tagline, regex("-sr-")) == true)
	{
		increase_offset(current_segment, 1);
		if (regex_search(tagline, regex("-at-")) == true)
		{
			if ((op1s == "mem8") || (op1s == "mem16"))
			{
				string reg1 = "empty";
				string reg2 = "empty";
				int add_reg = 0;
				for (lextabline x : op1)
				{
					if (get<0>(x) == "[") add_reg = 1;
					else if (get<0>(x) == "]") add_reg = 0;
					else if ((get<2>(x) == "i_r16") && (reg1 == "empty")) reg1 = get<0>(x);
					else if ((get<2>(x) == "i_r16") && (reg1 != "empty")) reg2 = get<0>(x);
				}
				for (ARPVline x : ARPV)
				{
					if ((get<0>(x) == reg1) && (get<1>(x) == reg2)) seg_rep_prefix = get<3>(x) + ":";
				}
			}
			else if ((op2s == "mem8") || (op2s == "mem16"))
			{
				string reg1 = "empty";
				string reg2 = "empty";
				int add_reg = 0;
				for (lextabline x : op2)
				{
					if (get<0>(x) == "[") add_reg = 1;
					else if (get<0>(x) == "]") add_reg = 0;
					else if ((get<2>(x) == "i_r16") && (reg1 == "empty")) reg1 = get<0>(x);
					else if ((get<2>(x) == "i_r16") && (reg1 != "empty")) reg2 = get<0>(x);
				}
				for (ARPVline x : ARPV)
				{
					if ((get<0>(x) == reg1) && (get<1>(x) == reg2)) seg_rep_prefix = get<3>(x) + ":";
				}
			}
		}
		else
		{
			if ((op1s == "mem8") || (op1s == "mem16"))
			{
				string seg_reg;
				for (lextabline x : op1)
				{
					if (get<2>(x) == "i_rs") seg_reg = get<0>(x);
				}
				for (SRPVline x : SRPV)
				{
					if (get<0>(x) == seg_reg) seg_rep_prefix = get<1>(x) + ":";
				}
			}
			if ((op2s == "mem8") || (op2s == "mem16"))
			{
				string seg_reg;
				for (lextabline x : op2)
				{
					if (get<2>(x) == "i_rs") seg_reg = get<0>(x);
				}
				for (SRPVline x : SRPV)
				{
					if (get<0>(x) == seg_reg) seg_rep_prefix = get<1>(x) + ":";
				}
			}
		}
		res.emplace_back(seg_rep_prefix);
	}

	if (regex_search(tagline, regex("-at-")) == true)
	{
		increase_offset(current_segment, 2);
		offset_bit_depth = 2;
		lextabline adrt;
		if ((op1s == "mem8") || (op1s == "mem16"))
		{
			for (lextabline x : op1)
			{
				if (get<2>(x) == "i_u") adrt = x;
			}
		}
		else if ((op2s == "mem8") || (op2s == "mem16"))
		{
			for (lextabline x : op2)
			{
				if (get<2>(x) == "i_u") adrt = x;
			}
		}
		for (useridstabline x : useridstab)
		{
			if (get<0>(x) == get<0>(adrt)) addres_or_offset = hex_val<16>(get<2>(x), 4) + "r";
		}
	}

	if (regex_search(tagline, regex("-ofs-")) == true)
	{
		string val_line;
		int adress_expr = 0;
		int neg_marker = 0;
		for (lextabline x : op1)
		{
			if (get<0>(x) == "[") adress_expr = 1;

			if ((get<2>(x) == "c_nd") && (get<2>(x) != "0") && (adress_expr == 1))
			{
				if (neg_marker == 1) val_line = "-" + get<0>(x);
				else val_line = get<0>(x);
			}

			if ((adress_expr == 1) && (get<2>(x) == "-")) neg_marker = 1;
			else neg_marker = 0;
		}
		for (lextabline x : op2)
		{
			if (get<0>(x) == "[") adress_expr = 1;

			if ((get<2>(x) == "c_nd") && (get<2>(x) != "0") && (adress_expr == 1))
			{
				if (neg_marker == 1) val_line = "-" + get<0>(x);
				else val_line = get<0>(x);
			}

			if ((adress_expr == 1) && (get<2>(x) == "-")) neg_marker = 1;
			else neg_marker = 0;
		}
		int val = num_const_to_integer(val_line, 'd');
		if ((val >= -128) || (val < 128))
		{
			increase_offset(current_segment, 1);
			offset_bit_depth = 1;
			addres_or_offset = hex_val<8>(val, 2);
		}
		else
		{
			increase_offset(current_segment, 2);
			offset_bit_depth = 2;
			addres_or_offset = hex_val<16>(val, 4);
		}
	}
	
	if ((regex_search(tagline, regex("-at-")) == false) && (regex_search(tagline, regex("-ofs-")) == false))
	{
		addres_or_offset = "none";
	}

	if (regex_search(tagline, regex("-modrm-")) == true)
	{
		increase_offset(current_segment, 1);

		for (ICVline x : ICV)
		{
			if ((get<0>(x) == instr) && (get<1>(x) == op1s) && (get<2>(x) == op2s))
			{
				if ((regex_search(tagline, regex("-at-")) == true) || (regex_search(tagline, regex("-ofs-")) == true))
				{
					if ((op1s == "mem8") || (op1s == "mem16"))
					{
						if (offset_bit_depth == 1) field_mod = bitset<2>(string("01"));
						else if (offset_bit_depth == 2) field_mod = bitset<2>(string("10"));

						string reg1 = "empty";
						string reg2 = "empty";
						int add_reg = 0;
						for (lextabline x : op1)
						{
							if (get<0>(x) == "[") add_reg = 1;
							else if (get<0>(x) == "]") add_reg = 0;
							else if ((get<2>(x) == "i_r16") && (reg1 == "empty")) reg1 = get<0>(x);
							else if ((get<2>(x) == "i_r16") && (reg1 != "empty")) reg2 = get<0>(x);
						}
						for (ARPVline x : ARPV)
						{
							if ((get<0>(x) == reg1) && (get<1>(x) == reg2)) field_rm = get<2>(x);
						}
						if ((op2s != "imm8") && (op2s != "imm16") && (op2s != "empty"))
						{
							lextabline reg = op2.at(0);
							for (EBRVline x : EBRV)
							{
								if (get<0>(x) == get<0>(reg)) field_reg = get<1>(x);
							}
							for (SBRVline x : SBRV)
							{
								if (get<0>(x) == get<0>(reg)) field_reg = get<1>(x);
							}
						}
					}
					else if ((op2s == "mem8") || (op2s == "mem16"))
					{
						if (offset_bit_depth == 1) field_mod = bitset<2>(string("01"));
						else if (offset_bit_depth == 2) field_mod = bitset<2>(string("10"));

						string reg1 = "empty";
						string reg2 = "empty";
						int add_reg = 0;
						for (lextabline x : op2)
						{
							if (get<0>(x) == "[") add_reg = 1;
							else if (get<0>(x) == "]") add_reg = 0;
							else if ((get<2>(x) == "i_r16") && (reg1 == "empty")) reg1 = get<0>(x);
							else if ((get<2>(x) == "i_r16") && (reg1 != "empty")) reg2 = get<0>(x);
						}
						for (ARPVline x : ARPV)
						{
							if ((get<0>(x) == reg1) && (get<1>(x) == reg2)) field_rm = get<2>(x);
						}
						if ((op1s != "imm8") && (op1s != "imm16"))
						{
							lextabline reg = op1.at(0);
							for (EBRVline x : EBRV)
							{
								if (get<0>(x) == get<0>(reg)) field_reg = get<1>(x);
							}
							for (SBRVline x : SBRV)
							{
								if (get<0>(x) == get<0>(reg)) field_reg = get<1>(x);
							}
						}
					}
				}
				else
				{
					field_mod = bitset<2>(string("11"));

					lextabline reg1 = op1.at(0);
					for (EBRVline x : EBRV)
					{
						if (get<0>(x) == get<0>(reg1)) field_reg = get<1>(x);
					}
					for (SBRVline x : SBRV)
					{
						if (get<0>(x) == get<0>(reg1)) field_reg = get<1>(x);
					}

					lextabline reg2 = op2.at(0);
					for (EBRVline x : EBRV)
					{
						if (get<0>(x) == get<0>(reg2)) field_rm = get<1>(x);
					}
					for (SBRVline x : SBRV)
					{
						if (get<0>(x) == get<0>(reg2)) field_rm = get<1>(x);
					}
				}
			}
		}
		set_modrm(modrm, field_mod, field_reg, field_rm);
	}
	else
	{
		modrm.reset();
	}

	res.emplace_back(hex_val<8>(opcode1.to_ulong(), 2));
	if ((modrm.any() == false) && (addres_or_offset == "none") && (immediate != "none"))
	{
		res.emplace_back(immediate);
	}
	else if ((modrm.any() == true) && (addres_or_offset == "none") && (immediate == "none"))
	{
		res.emplace_back(hex_val<8>(modrm.to_ulong(), 2));
	}
	else if ((modrm.any() == true) && (addres_or_offset != "none") && (immediate == "none"))
	{
		res.emplace_back(hex_val<8>(modrm.to_ulong(), 2));
		res.emplace_back(addres_or_offset);
	}
	else if ((modrm.any() == true) && (addres_or_offset != "none") && (immediate != "none"))
	{
		res.emplace_back(hex_val<8>(modrm.to_ulong(), 2));
		res.emplace_back(addres_or_offset);
		res.emplace_back(immediate);
	}

	return res;
}

void do_inits()
{
	initresidstab();
}

void first_view(string name_of_file)
{
	signs.in_segment = false;
	signs.macro = false;
	signs.macro_def = false;
	signs.cur_is_ENDM = false;
	signs.init_macro = false;
	signs.end_of_view = false;

	regex sentence("[A-Z0-9',\\+\\[\\]: ]+");
	smatch code_lines;
	fstream src(name_of_file);

	int listing_line_number = 0;
	int curent_segment_num = 1;
	int current_offset = 0;
	int current_macrolib_free_place = 1;
	int current_pos_in_lib = 0;

	segtabline curent_segment;
	useridstabline curent_user_identifier;
	allocated_mem current_allocated_mem;

	do
	{
		string cur_s;
		if (signs.macro == false)
		{
			getline(src, cur_s);
		}
		else if (signs.init_macro == true)
		{
			cur_s = macrolib.at(current_pos_in_lib-1);
		}
		else if (signs.init_macro == false)
		{
			cur_s = macrolib.at(current_pos_in_lib++);
			if ((macroargtab.empty() == false) && (macropartab.empty() == false))
			{
				regex par(macropartab.at(0));
				string arg = macroargtab.at(0);
				cur_s = regex_replace(cur_s, par, arg);
			}
		}

		regex_search(cur_s, code_lines, sentence);

		string match;
		match = code_lines.str(0);
		if (match != "")
		{
			lextab cur_s_lt = create_lt(match);
			sentstruct cur_s_ss = create_sst(cur_s_lt);

			sentcode cur_s_sc(locate_lexem(cur_s_ss, "label"), locate_lexem(cur_s_ss, "name"), locate_lexem(cur_s_ss, "mnem"), locate_lexem(cur_s_ss, "op1"), locate_lexem(cur_s_ss, "op2"));
			sentmask cur_s_sm = generate_sent_mask(cur_s_lt, cur_s_ss, cur_s_sc);

			if (signs.init_macro == true)
			{
				if (get<3>(cur_s_sm).empty() == false)
				{
					lextabline macropar = get<3>(cur_s_sm).at(0);
					macropartab.emplace_back(get<0>(macropar));
				}
				signs.init_macro = false;
				continue;
			}

			if (signs.macro_def == true)
			{
				macrolib.emplace_back(match);
				current_macrolib_free_place ++;
			}

			if (get<2>(cur_s_sc) != -1)
			{
				char type = determine_mnem(get<2>(cur_s_sm));
				if (type == 'd')
				{
					string dir = get<2>(cur_s_sm);
					if ((dir == "SEGMENT") && (signs.macro_def == false))
					{
						get<0>(curent_segment) = curent_segment_num;
						get<1>(curent_segment) = get<1>(cur_s_sm);
						get<2>(curent_segment) = 16;
						get<3>(curent_segment) = 0;
						current_offset = 0;
						signs.in_segment = true;
						active_seg = curent_segment_num;
						curent_segment_num++;

						get<0>(current_allocated_mem) = cur_s;
						get<1>(current_allocated_mem) = get_current_offset(curent_segment);
						get<2>(current_allocated_mem) = "-ss-";
					}
					else if ((dir == "ENDS") && (signs.macro_def == false))
					{
						current_offset = get_current_offset(curent_segment);

						get<0>(current_allocated_mem) = cur_s;
						get<1>(current_allocated_mem) = get_current_offset(curent_segment);
						get<2>(current_allocated_mem) = "-se-";

						signs.in_segment = false;

						active_seg = 0;
					}
					else if (((dir == "DB") || (dir == "DW") || (dir == "DD")) && (signs.macro_def == false))
					{
						if (dir == "DB")
						{
							current_offset = get_current_offset(curent_segment);

							lextab op1 = get<3>(cur_s_sm);
							lextab op2 = get<4>(cur_s_sm);
							if (op2.empty() == false)
							{
								increase_offset(curent_segment, 2);
								get<0>(current_allocated_mem) = cur_s;
								get<1>(current_allocated_mem) = current_offset;
								get<2>(current_allocated_mem) = "-cnb1- -cnb2-";
							}
							else
							{
								lextabline analyzed = op1.at(0);
								if (get<2>(analyzed) == "c_t")
								{
									increase_offset(curent_segment, get<1>(analyzed));
									get<0>(current_allocated_mem) = cur_s;
									get<1>(current_allocated_mem) = current_offset;
									get<2>(current_allocated_mem) = "-csb1-";
								}
								else
								{
									increase_offset(curent_segment, 1);
									get<0>(current_allocated_mem) = cur_s;
									get<1>(current_allocated_mem) = current_offset;
									get<2>(current_allocated_mem) = "-cnb1-";
								}
							}
							if (get<1>(cur_s_sc) != -1)
							{
								get<0>(curent_user_identifier) = get<1>(cur_s_sm);
								get<1>(curent_user_identifier) = get<1>(curent_segment);
								get<2>(curent_user_identifier) = current_offset;
								get<3>(curent_user_identifier) = 1;
								get<4>(curent_user_identifier) = check_userid(get<1>(cur_s_sm));
								useridstab.emplace_back(curent_user_identifier);
							}
						}
						if (dir == "DW")
						{
							current_offset = get_current_offset(curent_segment);

							increase_offset(curent_segment, 2);
							get<0>(current_allocated_mem) = cur_s;
							get<1>(current_allocated_mem) = current_offset;
							get<2>(current_allocated_mem) = "-cnw1-";
							if (get<1>(cur_s_sc) != -1)
							{
								get<0>(curent_user_identifier) = get<1>(cur_s_sm);
								get<1>(curent_user_identifier) = get<1>(curent_segment);
								get<2>(curent_user_identifier) = current_offset;
								get<3>(curent_user_identifier) = 2;
								get<4>(curent_user_identifier) = check_userid(get<1>(cur_s_sm));
								useridstab.emplace_back(curent_user_identifier);
							}
						}
						if (dir == "DD")
						{
							current_offset = get_current_offset(curent_segment);

							increase_offset(curent_segment, 4);
							get<0>(current_allocated_mem) = cur_s;
							get<1>(current_allocated_mem) = current_offset;
							get<2>(current_allocated_mem) = "-cnd1-";
							if (get<1>(cur_s_sc) != -1)
							{
								get<0>(curent_user_identifier) = get<1>(cur_s_sm);
								get<1>(curent_user_identifier) = get<1>(curent_segment);
								get<2>(curent_user_identifier) = current_offset;
								get<3>(curent_user_identifier) = 4;
								get<4>(curent_user_identifier) = check_userid(get<1>(cur_s_sm));
								useridstab.emplace_back(curent_user_identifier);
							}
						}
					}
					else if ((dir == "MACRO") && (signs.macro_def == false))
					{
					    signs.macro_def = true;
						MV.emplace_back(get<1>(cur_s_sm), current_macrolib_free_place);
						macrolib.emplace_back(match);
					}
					else if (dir == "ENDM")
					{
					    if (signs.macro == true)
					    {
							signs.macro = false;
							current_pos_in_lib = 0;
							macroargtab.clear();
							macropartab.clear();
							continue;
						}
						signs.cur_is_ENDM = true;
					}
					else if (dir == "END")
					{
					    signs.end_of_view = true;
                    }
				}
				if (type == 'm')
				{
					signs.macro = true;
					signs.init_macro = true;
					current_pos_in_lib = locate_macro(get<2>(cur_s_sm));
					if (get<3>(cur_s_sm).empty() == false)
					{
						lextabline macroarg = get<3>(cur_s_sm).at(0);
						macroargtab.emplace_back(get<0>(macroarg));
					}
				}
				if ((type == 'i') && (signs.macro_def == false))
				{
					string instr = get<2>(cur_s_sm);
					if (instr == "CPUID")
					{
						signs.macro_def = true;
						MV.emplace_back(get<2>(cur_s_sm), current_macrolib_free_place);
						macrolib.emplace_back(match);
					}
					else if (instr == "JG")
					{
						oplexs analyzed_op = get<3>(cur_s_sm);
						lextabline current = analyzed_op.at(0);
						if (check_userid(get<0>(current)) == false)
						{
							current_offset = get_current_offset(curent_segment);

							increase_offset(curent_segment, 4);
							get<0>(current_allocated_mem) = cur_s;
							get<1>(current_allocated_mem) = current_offset;
							get<2>(current_allocated_mem) = "-ic1- -ic2- -lo1- -lo2-";
						}
						else
						{
							current_offset = get_current_offset(curent_segment);

							int label_offset = get_label_offset(get<0>(current));
							int distance = label_offset - current_offset + 2;

							if ((distance < -128) || (distance > 127))
							{
								increase_offset(curent_segment, 4);
								get<0>(current_allocated_mem) = cur_s;
								get<1>(current_allocated_mem) = current_offset;
								get<2>(current_allocated_mem) = "-ic1- -ic2- -lo1- -lo2-";
							}
							else
							{
								increase_offset(curent_segment, 2);
								get<0>(current_allocated_mem) = cur_s;
								get<1>(current_allocated_mem) = current_offset;
								get<2>(current_allocated_mem) = "-ic1- -lo1-";
							}
						}
					}
					else
					{
						instruction_handler_for_mem_allocation(cur_s, curent_segment, current_allocated_mem, current_offset, instr, get<3>(cur_s_sm), get<4>(cur_s_sm));
					}
				}
				if (type == '?')
				{
					continue;
				}
			}
			
			if (get<0>(cur_s_sc) != -1)
			{
				current_offset = get_current_offset(curent_segment);

				get<0>(current_allocated_mem) = cur_s;
				get<1>(current_allocated_mem) = get_current_offset(curent_segment);
				get<2>(current_allocated_mem) = "-label-";

				get<0>(curent_user_identifier) = get<0>(cur_s_sm);
				get<1>(curent_user_identifier) = get<1>(curent_segment);
				get<2>(curent_user_identifier) = get_current_offset(curent_segment);
				get<3>(curent_user_identifier) = -1;
				get<4>(curent_user_identifier) = false;
				useridstab.emplace_back(curent_user_identifier);
			}

			listing_line_number++;
			if ((signs.macro_def == false) && (signs.init_macro == false) && (signs.end_of_view == false))
			{
				mem_distribution.emplace_back(current_allocated_mem);
			}

			if (signs.cur_is_ENDM == true)
			{
				signs.cur_is_ENDM = false;
				signs.macro_def = false;
				current_macrolib_free_place++;
			}

			if ((signs.in_segment == false) && (signs.end_of_view == false))
			{
				segtab.emplace_back(curent_segment);
				get<3>(curent_segment) = 0;
			}
		}
	} while ((!(src.eof())) && (!(signs.end_of_view == true)));
	src.close();
}

void second_view(string name_of_file, string name_of_listing_file)
{
	signs.in_segment = false;
	signs.macro = false;
	signs.macro_def = false;
	signs.cur_is_ENDM = false;
	signs.init_macro = false;
	signs.end_of_view = false;

	regex sentence("[A-Z0-9',\\+\\[\\]: ]+");
	smatch code_lines;
	fstream src(name_of_file);

	int listing_line_number = 0;
	int curent_segment_num = 1;
	int current_offset = 0;
	int current_macrolib_free_place = 1;
	int current_pos_in_lib = 0;

	segtabline curent_segment;
	useridstabline curent_user_identifier;
	allocated_mem current_allocated_mem;

	sentbytesvals cur_s_sb;

	do
	{
		get<0>(curent_user_identifier) = "none";
		get<1>(curent_user_identifier) = "none";
		get<2>(curent_user_identifier) = 0;
		get<3>(curent_user_identifier) = 0;
		get<4>(curent_user_identifier) = 0;

		string cur_s;
		if (signs.macro == false)
		{
			getline(src, cur_s);
		}
		else if (signs.init_macro == true)
		{
			cur_s = macrolib.at(current_pos_in_lib - 1);
		}
		else if (signs.init_macro == false)
		{
			cur_s = macrolib.at(current_pos_in_lib++);
			if ((macroargtab.empty() == false) && (macropartab.empty() == false))
			{
				regex par(macropartab.at(0));
				string arg = macroargtab.at(0);
				cur_s = regex_replace(cur_s, par, arg);
			}
		}

		regex_search(cur_s, code_lines, sentence);

		string match;
		match = code_lines.str(0);
		if (match != "")
		{
			lextab cur_s_lt = create_lt(match);
			sentstruct cur_s_ss = create_sst(cur_s_lt);

			sentcode cur_s_sc(locate_lexem(cur_s_ss, "label"), locate_lexem(cur_s_ss, "name"), locate_lexem(cur_s_ss, "mnem"), locate_lexem(cur_s_ss, "op1"), locate_lexem(cur_s_ss, "op2"));
			sentmask cur_s_sm = generate_sent_mask(cur_s_lt, cur_s_ss, cur_s_sc);

			if ((active_seg == 0) && ((signs.in_segment == false) && (signs.macro_def == false)) && (get<2>(cur_s_sm) != "END") && (get<2>(cur_s_sm) != "ENDS") && (get<2>(cur_s_sm) != "MACRO") && (get<2>(cur_s_sm) != "SEGMENT"))
			{
				print_error(name_of_listing_file);
				total_errors++;
				continue;
			}

			for (useridstabline x : useridstab)
			{
				if (get<1>(cur_s_sm) == get<0>(x)) curent_user_identifier = x;
			}

			if (get<0>(curent_user_identifier) != "none")
			{
				if (get<4>(curent_user_identifier) == true)
				{
					print_error(name_of_listing_file);
					total_errors++;
					continue;
				}
				else if (get<2>(curent_user_identifier) != get_current_offset(curent_segment))
				{
					print_error(name_of_listing_file);
					total_errors++;
					continue;
				}
				else if ((get<1>(curent_user_identifier) != get<1>(curent_segment)) && (get<3>(curent_user_identifier) == -1))
				{
					print_error(name_of_listing_file);
					total_errors++;
					continue;
				}
			}

			if ((signs.in_segment == true) && (signs.macro_def == false))
			{
				for (allocated_mem x : mem_distribution)
				{
					if ((get<0>(x) == cur_s) && (get<1>(x) == get_current_offset(curent_segment)))
					{
						current_allocated_mem = x;
					}
				}
			}

			if (signs.init_macro == true)
			{
				if (get<3>(cur_s_sm).empty() == false)
				{
					lextabline macropar = get<3>(cur_s_sm).at(0);
					macropartab.emplace_back(get<0>(macropar));
				}
				signs.init_macro = false;
				continue;
			}

			if (get<2>(cur_s_sc) != -1)
			{
				char type = determine_mnem(get<2>(cur_s_sm));

				if (get<3>(cur_s_sm).empty() == false)
				{
					lextabline cpuid_test = get<3>(cur_s_sm).at(0);
					if ((get<2>(cur_s_sm) == "CPUID") && (get<0>(cpuid_test) == "MACRO")) type = 'i';
				}

				if (type == 'd')
				{
					string dir = get<2>(cur_s_sm);
					if ((dir == "SEGMENT") && (signs.macro_def == false))
					{
						current_offset = 0;
						signs.in_segment = true;
						active_seg = curent_segment_num;

						for (segtabline x : segtab)
						{
							if (get<0>(x) == curent_segment_num) curent_segment = x;
						}

						set_offset(curent_segment, 0);

						curent_segment_num++;
					}
					else if ((dir == "ENDS") && (signs.macro_def == false))
					{
						current_offset = get_current_offset(curent_segment);

						signs.in_segment = false;

						active_seg = 0;
					}
					else if (((dir == "DB") || (dir == "DW") || (dir == "DD")) && (signs.macro_def == false))
					{
						if (dir == "DB")
						{
							cur_s_sb = mem_dir_byte_determination(cur_s_sm, current_allocated_mem);

							current_offset = get_current_offset(curent_segment);

							lextab op1 = get<3>(cur_s_sm);
							lextab op2 = get<4>(cur_s_sm);
							if (op2.empty() == false)
							{
								increase_offset(curent_segment, 2);
							}
							else
							{
								lextabline analyzed = op1.at(0);
								if (get<2>(analyzed) == "c_t")
								{
									increase_offset(curent_segment, get<1>(analyzed));
								}
								else
								{
									increase_offset(curent_segment, 1);
								}
							}
						}
						if (dir == "DW")
						{
							cur_s_sb = mem_dir_byte_determination(cur_s_sm, current_allocated_mem);

							current_offset = get_current_offset(curent_segment);

							increase_offset(curent_segment, 2);
						}
						if (dir == "DD")
						{
							cur_s_sb = mem_dir_byte_determination(cur_s_sm, current_allocated_mem);

							current_offset = get_current_offset(curent_segment);

							increase_offset(curent_segment, 4);
						}
					}
					else if ((dir == "MACRO") && (signs.macro_def == false))
					{
						signs.macro_def = true;
					}
					else if (dir == "ENDM")
					{
						if (signs.macro == true)
						{
							signs.macro = false;
							current_pos_in_lib = 0;
							macroargtab.clear();
							macropartab.clear();
							continue;
						}
						signs.cur_is_ENDM = true;
					}
					else if (dir == "END")
					{
						signs.end_of_view = true;
					}
				}
				if (type == 'm')
				{
					signs.macro = true;
					signs.init_macro = true;
					current_pos_in_lib = locate_macro(get<2>(cur_s_sm));
					if (get<3>(cur_s_sm).empty() == false)
					{
						lextabline macroarg = get<3>(cur_s_sm).at(0);
						macroargtab.emplace_back(get<0>(macroarg));
					}
				}
				if ((type == 'i') && (signs.macro_def == false))
				{
					string instr = get<2>(cur_s_sm);
					if (instr == "CPUID")
					{
						signs.macro_def = true;
					}
					else if (instr == "JG")
					{
						oplexs analyzed_op = get<3>(cur_s_sm);
						lextabline current = analyzed_op.at(0);
						for (useridstabline x : useridstab)
						{
							if (get<0>(current) == get<0>(x))
							{
								current_offset = get_current_offset(curent_segment);

								if (regex_search(get<2>(current_allocated_mem), regex("-ic2-")) == true)
								{
									int label_offset = get<2>(x);

									int off_minus_current = label_offset - current_offset - 4;
									if (off_minus_current >= 0) off_minus_current += 2;

									if (off_minus_current > 2)
									{
										string ic1;
										string ic2;

										for (ICVline y : ICV)
										{
											if ((get<0>(y) == instr) && (get<1>(y) == "rel16"))
											{
												ic1 = hex_val<8>(get<3>(y).to_ulong(), 2);
												ic2 = hex_val<8>(get<4>(y).to_ulong(), 2);
											}
										}

										string offs = hex_val<16>(off_minus_current, 4);
										string first = offs;
										string second = offs;
										first.erase(2, 2);
										second.erase(0, 2);
										cur_s_sb.emplace_back(ic1);
										cur_s_sb.emplace_back(ic2);
										cur_s_sb.emplace_back(first);
										cur_s_sb.emplace_back(second);
									}
									else
									{
										string ic1;

										for (ICVline y : ICV)
										{
											if ((get<0>(y) == instr) && (get<1>(y) == "rel8"))
											{
												ic1 = hex_val<8>(get<3>(y).to_ulong(), 2);
											}
										}

										string offs = hex_val<8>(off_minus_current, 2);
										cur_s_sb.emplace_back(ic1);
										cur_s_sb.emplace_back(offs);
										cur_s_sb.emplace_back("90");
										cur_s_sb.emplace_back("90");
									}

									increase_offset(curent_segment, 4);
								}
								else
								{
									int label_offset = get<2>(x);

									int off_minus_current = label_offset - current_offset - 2;

									string ic1;

									for (ICVline y : ICV)
									{
										if ((get<0>(y) == instr) && (get<1>(y) == "rel8"))
										{
											ic1 = hex_val<8>(get<3>(y).to_ulong(), 2);
										}
									}

									string offs = hex_val<8>(off_minus_current, 2);
									cur_s_sb.emplace_back(ic1);
									cur_s_sb.emplace_back(offs);

									increase_offset(curent_segment, 2);
								}
							}
						}
					}
					else
					{
						cur_s_sb = instruction_handler_for_byte_determination(cur_s, curent_segment, current_allocated_mem, current_offset, instr, get<3>(cur_s_sm), get<4>(cur_s_sm));
					}
				}
				if (type == '?')
				{
					print_error(name_of_listing_file);
					total_errors++;
					continue;
				}
			}

			if (get<0>(cur_s_sc) != -1)
			{
				current_offset = get_current_offset(curent_segment);
			}

			listing_line_number++;
			if ((signs.macro_def == false) && (signs.init_macro == false) && (signs.end_of_view == false))
			{
				listing_line current_s_ll(listing_line_number, current_offset, cur_s_sb, cur_s);
				print_listing_line(current_s_ll, name_of_listing_file);
				cur_s_sb.clear();
			}
			else
			{
				sentbytesvals empty;
				listing_line current_s_ll(listing_line_number, -1, empty, cur_s);
				print_listing_line(current_s_ll, name_of_listing_file);
			}

			if (signs.cur_is_ENDM == true)
			{
				signs.cur_is_ENDM = false;
				signs.macro_def = false;
				current_macrolib_free_place++;
			}

			if ((signs.in_segment == false) && (signs.end_of_view == false))
			{
				get<3>(curent_segment) = 0;
			}
		}
	} while ((!(src.eof())) && (!(signs.end_of_view == true)));
	src.close();

	print_tabs(name_of_listing_file);

	cout << "Total errors: " << total_errors << endl;
}
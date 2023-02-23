#pragma once

//EightBitRegsVector. Tuple members: identifier, code
typedef tuple<string, bitset<3>> EBRVline;
vector<EBRVline> EBRV;
//SixteenBitRegsVector. Tuple members: identifier, code
typedef tuple<string, bitset<3>> SBRVline;
vector<SBRVline> SBRV;
//AddressRegPairsVector. Tuple members: identifier of first, identifier of second, code
typedef tuple<string, string, bitset<3>, string> ARPVline;
vector<ARPVline> ARPV;
//SegRegPrefixesVector.Tuple members : identifier, prefix
typedef tuple<string, string> SRPVline;
vector<SRPVline> SRPV;
//InstructionCodesVector. Tuple members: identifier, op1, op2, opcode byte 1, opcode byte 2, "reg" field value, number of opcode bytes, feature
typedef tuple<string, string, string, bitset<8>, bitset<8>, bitset<3>, int, string> ICVline;
vector<ICVline> ICV;
//DirectivesVector. Tuple member: directive, number of bytes for memory directives
typedef tuple<string, int> DVline;
vector<DVline> DV;
//OperatorsVector. Member: operator
vector<string> OV;
//TypesVector. Member: type
vector<string> TV;
//MacrosVector. Member: macrocomand definiton, place in macrolib after macrodefinition
typedef tuple<string, int> MVline;
vector<MVline> MV;

void initresidstab()
{
	EBRV.emplace_back("AL", string("000"));
	EBRV.emplace_back("CL", string("001"));
	EBRV.emplace_back("DL", string("010"));
	EBRV.emplace_back("BL", string("011"));
	EBRV.emplace_back("AH", string("100"));
	EBRV.emplace_back("CH", string("101"));
	EBRV.emplace_back("DH", string("110"));
	EBRV.emplace_back("BH", string("111"));

	SBRV.emplace_back("AX", string("000"));
	SBRV.emplace_back("CX", string("001"));
	SBRV.emplace_back("DX", string("010"));
	SBRV.emplace_back("BX", string("011"));
	SBRV.emplace_back("SP", string("100"));
	SBRV.emplace_back("BP", string("101"));
	SBRV.emplace_back("SI", string("110"));
	SBRV.emplace_back("DI", string("111"));

	ARPV.emplace_back("BX", "SI", string("000"), "DS");
	ARPV.emplace_back("BX", "DI", string("001"), "DS");
	ARPV.emplace_back("BP", "SI", string("010"), "SS");
	ARPV.emplace_back("BP", "DI", string("011"), "SS");

	SRPV.emplace_back("ES", "26");
	SRPV.emplace_back("CS", "2E");
	SRPV.emplace_back("SS", "36");
	SRPV.emplace_back("DS", "3E");
	SRPV.emplace_back("FS", "64");
	SRPV.emplace_back("GS", "65");

	ICV.emplace_back("INC", "reg8", "empty", 0XFE, 0X0, 0X0, 1, "/0");
	ICV.emplace_back("INC", "reg16", "empty", 0X40, 0X0, 0X0, 1, "+rw");
	ICV.emplace_back("DEC", "mem8", "empty", 0XFE, 0X0, 0X1, 1, "/1");
	ICV.emplace_back("DEC", "mem16", "empty", 0XFF, 0X0, 0X1, 1, "/1");
	ICV.emplace_back("ADD", "mem8", "reg8", 0X0, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("ADD", "mem16", "reg16", 0X1, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("ADD", "reg8", "reg8", 0X2, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("ADD", "reg16", "reg16", 0X3, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("CMP", "reg8", "mem8", 0X3A, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("CMP", "reg16", "mem16", 0X3B, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("AND", "mem8", "reg8", 0X20, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("AND", "mem16", "reg16", 0X21, 0X0, 0X0, 1, "/r");
	ICV.emplace_back("MOV", "reg8", "imm8", 0XB0, 0X0, 0X0, 1, "+rb");
	ICV.emplace_back("MOV", "reg16", "imm16", 0XB8, 0X0, 0X0, 1, "+rw");
	ICV.emplace_back("OR", "mem8", "imm8", 0X80, 0X0, 0X1, 1, "/1 ib");
	ICV.emplace_back("OR", "mem16", "imm16", 0X81, 0X0, 0X1, 1, "/1 iw");
	ICV.emplace_back("JG", "rel8", "empty", 0X7F, 0X0, 0X0, 1, "cb");
	ICV.emplace_back("JG", "rel16", "empty", 0XF, 0X8F, 0X0, 2, "cw");
	ICV.emplace_back("CPUID", "mdef", "empty", 0XF, 0XA2, 0X0, 2, "m");

	DV.emplace_back("END", 0);
	DV.emplace_back("SEGMENT", 0);
	DV.emplace_back("ENDS", 0);
	DV.emplace_back("MACRO", 0);
	DV.emplace_back("ENDM", 0);
	DV.emplace_back("DB", 1);
	DV.emplace_back("DW", 2);
	DV.emplace_back("DD", 4);

	OV.emplace_back("PTR");

	TV.emplace_back("BYTE");
	TV.emplace_back("WORD");

	cout << "Table of reserved identifiers initialized" << endl;
}
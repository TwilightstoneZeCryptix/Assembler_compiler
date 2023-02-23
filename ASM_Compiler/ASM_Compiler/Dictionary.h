#pragma once
Token Digit('0','9');
Token Letter('A','Z');
Token Other(",:[+]");

Token DigitExceptZero('1', '9');
Token HexConstLetter('A', 'F');
Token BinConstDigit("01");
Lexem HexConstChar = Digit | HexConstLetter;

Token HexConstEnd('H');
Token DecConstEnd('D');
Token BinConstEnd('B');

Lexem BinConst = Iterate(1, BinConstDigit) + BinConstEnd;
Lexem DecConst = Iterate(1, Digit) | Iterate(1, Digit) + DecConstEnd;
Lexem HexConst = DigitExceptZero + Iterate(0, HexConstChar) + HexConstEnd | "0" + HexConstLetter + Iterate(0, HexConstChar) + HexConstEnd | "0" + HexConstEnd;

Lexem AH = "AH";
Lexem AL = "AL";
Lexem BH = "BH";
Lexem BL = "BL";
Lexem CH = "CH";
Lexem CL = "CL";
Lexem DH = "DH";
Lexem DL = "DL";
Lexem AX = "AX";
Lexem BX = "BX";
Lexem CX = "CX";
Lexem DX = "DX";
Lexem SP = "SP";
Lexem BP = "BP";
Lexem SI = "SI";
Lexem DI = "DI";
Lexem DS = "DS";
Lexem CS = "CS";
Lexem SS = "SS";
Lexem ES = "ES";
Lexem FS = "FS";
Lexem GS = "GS";

Lexem CPUID = "CPUID";
Lexem INC = "INC";
Lexem DEC = "DEC";
Lexem ADD = "ADD";
Lexem CMP = "CMP";
Lexem AND = "AND";
Lexem MOV = "MOV";
Lexem OR = "OR";
Lexem JG = "JG";

Lexem END = "END";
Lexem SEGMENT = "SEGMENT";
Lexem ENDS = "ENDS";
Lexem MACRO = "MACRO";
Lexem ENDM = "ENDM";
Lexem DB = "DB";
Lexem DW = "DW";
Lexem DD = "DD";

Lexem PTR = "PTR";

Lexem BYTE = "BYTE";
Lexem WORD = "WORD";

Lexem LetterOrDigit = Letter | Digit;

Lexem Identifier = Letter + Iterate(1, LetterOrDigit);
Lexem NumConst = BinConst | DecConst | HexConst;
Lexem Single = Other;

Lexem EightBitReg = AH | AL | BH | BL | CH | CL | DH | DL;
Lexem SixteenBitReg = AX | BX | CX | DX | SP | BP | SI | DI;
Lexem SegReg = DS | CS | SS | ES | FS | GS;

Lexem Reg = EightBitReg | SixteenBitReg | SegReg;
Lexem Instr = CPUID | INC | DEC | ADD | CMP | AND | MOV | OR | JG;
Lexem Dir = END | SEGMENT | ENDS | MACRO | ENDM | DB | DW | DD;
Lexem Type = BYTE | WORD;
Lexem Op = PTR;

#include "parser.h"
#include "exp.h"

std::vector<XPython::OpInfo> XPython::Parser::OPList = {
	OpInfo{{
			"False", "None", "True", "and", /*0-3*/
			"as", "assert", "async", "await", /*4-7*/
			"break", "class", "continue", /*8-10*/
			"def", "del", "elif", "else", /*11-14*/
			"except", "finally", "for", /*15-17*/
			"from", "global", "if", "import", /*18-21*/
			"in", "is", "lambda", "nonlocal", /*22-25*/
			"not", "or", "pass", "raise", "return", /*26-30*/
			"try", "while", "with", "yield",/*31-34*/
	}},
	OpInfo{{"range"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		auto op = new AST::Range(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},Alias::Range},
	OpInfo{{"return"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		auto op = new AST::UnaryOp(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},Alias::Return},
	OpInfo{{"for"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		auto op = new AST::For(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},Alias::For},
	OpInfo{{"in"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		auto op = new AST::InOp(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},Alias::In},
	OpInfo{{"def"},[](Parser* p,short opIndex,OpAction* opAct) 
	{
		auto func = new AST::Func();
		return (AST::Operator*)func;
	},Alias::Func},
	OpInfo{{
			//Python Assignment Operators --index range[35,47]
			"=","+=","-=","*=","/=","%=","//=","**=","&=","|=","^=",">>=","<<=",
	},[](Parser* p,short opIndex,OpAction* opAct) 
	{
		auto op = new AST::Assign(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},Alias::None,Precedence_Min},
	OpInfo{{
			//Python Arithmetic Operators --index range[48,54]
			"+","-","*","/","%","**","//",
	},[](Parser* p,short opIndex,OpAction* opAct) 
	{
		auto op = new AST::BinaryOp(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},AList(Alias::Add,Alias::Minus,Alias::Multiply,Alias::Div,
		Alias::Modulus,Alias::Exponentiation,Alias::FloorDivision)},
	OpInfo{{
			//Python Comparison Operators --index range[55,60]
			"==","!=",">","<",">=","<=",
	},[](Parser* p,short opIndex,OpAction* opAct)
	{
		auto op = new AST::BinaryOp(opIndex,opAct->alias);
		return (AST::Operator*)op;
	}},
	//set precedence just higher 1 with reqular
	OpInfo{{"*","/","%","**","//"},nil,Alias::None,Precedence_Reqular+1},
	//Override for +-* which may be an unary Operator
	OpInfo{{"+","-","*"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		AST::Operator* op = nil;
		if (p->PreTokenIsOp())
		{
			op = new AST::UnaryOp(opIndex, opAct->alias);
		}
		else
		{
			op = new AST::BinaryOp(opIndex, opAct->alias);
		}
		return op;
	}},

	//Python Bitwise Operators --index range[61,66]
	OpInfo{{"&","|","^","~","<<",">>",}},
	OpInfo{{"~"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		AST::Operator* op = nil;
		if (p->PreTokenIsOp())
		{
			op = new AST::UnaryOp(opIndex, opAct->alias);
		}
		else
		{
			//error
		}
		return op;
	},Alias::Invert},
	OpInfo{{"(","[","{"},[](Parser* p,short opIndex,OpAction* opAct) 
	{
		return p->PairLeft(opIndex,opAct);
	},AList(Alias::Parenthesis_L,
		Alias::Brackets_L,
		Alias::Curlybracket_L)},
	OpInfo{{")"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		p->PairRight(Alias::Parenthesis_L);
		return (AST::Operator*)nil;
	}},
	OpInfo{{"]"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		AST::Operator* op = nil;
		p->PairRight(Alias::Brackets_L);
		return op;
	}},
	OpInfo{{"}"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		p->PairRight(Alias::Curlybracket_L);
		return (AST::Operator*)nil;
	}},
	OpInfo{{":",".",","},[](Parser* p,short opIndex,OpAction* opAct) 
	{
		auto op = new AST::BinaryOp(opIndex,opAct->alias);
		return (AST::Operator*)op;
	},AList(Alias::Colon,Alias::Dot,Alias::Comma)},
	//change precedence for ':'
	//OpInfo{{"in"},nil,Alias::In,Precedence_Reqular + 1},
	OpInfo{{":"},nil,Alias::None,Precedence_Reqular + 2},
	OpInfo{{"\n"},[](Parser* p,short opIndex,OpAction* opAct) 
	{
		p->NewLine();
		return (AST::Operator*)nil;
	}},
	OpInfo{{"\t","\r","\\"},[](Parser* p,short opIndex,OpAction* opAct)
	{
		auto op = new AST::Operator(opIndex,opAct->alias);
		return op;
	},AList(Alias::Tab,Alias::CR,Alias::Slash)},
};
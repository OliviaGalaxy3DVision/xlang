#pragma once
#include "exp.h"
#include "op.h"
#include "glob.h"
#include <stack>
//for compile not for runtime
namespace X {
	namespace AST { class Block; }

struct PairInfo
{
	int opid;
	bool IsLambda = false;
};
class BlockState
{
	AST::Block* m_pBlock = nullptr;
	std::stack<AST::Expression*> m_operands;
	std::stack<AST::Operator*> m_ops;
	int m_pair_cnt = 0;//count for {} () and [],if
	std::stack<PairInfo> m_stackPair;

	inline OpAction OpAct(short idx)
	{
		return G::I().OpAct(idx);
	}
public:
	//below,before meet first non-tab char,it is true 
	bool m_NewLine_WillStart = true;
	int m_TabCountAtLineBegin = 0;
	int m_LeadingSpaceCountAtLineBegin = 0;

	BlockState(AST::Block* pBlock)
	{
		m_pBlock = pBlock;
	}
	std::stack<PairInfo>& StackPair() { return m_stackPair; }
	AST::Block* Block() { return m_pBlock; }
	inline void PushExp(AST::Expression* exp)
	{
		m_operands.push(exp);
	}
	inline void PushOp(AST::Operator* op)
	{
		m_ops.push(op);
	}
	inline bool IsOpStackEmpty()
	{
		return m_ops.empty();
	}
	inline bool IsOperandStackEmpty()
	{
		return m_operands.empty();
	}
	inline void OperandPop()
	{
		m_operands.pop();
	}
	inline AST::Expression* OperandTop()
	{
		return m_operands.top();
	}
	inline AST::Operator* OpTop()
	{
		return m_ops.top();
	}
	inline void OpPop()
	{
		m_ops.pop();
	}
	inline int PairCount() { return m_pair_cnt; }
	inline void IncPairCnt() { m_pair_cnt++; }
	inline void DecPairCnt() { m_pair_cnt--; }
	inline void ProcessPrecedenceOp(short lastToken,
		AST::Operator* curOp)
	{
		while (!m_ops.empty())
		{
			auto top = m_ops.top();
			OpAction topAct = OpAct(top->getOp());
			OpAction cur_opAct = OpAct(curOp->getOp());
			//check this case .[test1,test2](....)
			//after . it is a ops,not var
			if (lastToken != top->getOp()
				&& top->m_type != AST::ObType::Pair
				&& topAct.precedence > cur_opAct.precedence)
			{
				DoOpTop();
			}
			else
			{
				break;
			}
		}
	}
	inline bool DoOpTop()
	{
		auto top = m_ops.top();
		m_ops.pop();
		return top->OpWithOperands(m_operands);
	}
};
}
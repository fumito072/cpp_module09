#include "RPN.hpp"
#include <sstream>
#include <stdexcept>
#include <cctype>

// =============================================================================
// OCF
// =============================================================================
RPN::RPN() {}

RPN::RPN(const RPN& other) : _stack(other._stack) {}

RPN& RPN::operator=(const RPN& other)
{
	if (this != &other)
		_stack = other._stack;
	return (*this);
}

RPN::~RPN() {}

// =============================================================================
// isOperator
// =============================================================================
bool RPN::isOperator(char c)
{
	return (c == '+' || c == '-' || c == '*' || c == '/');
}

// =============================================================================
// applyOperator
// -----------------------------------------------------------------------------
// a op b を計算する(a が先に積まれた値、b が後に積まれた値)。
// 例: スタックに [.., a, b] と積まれている状態で op を適用すると a op b。
// =============================================================================
int RPN::applyOperator(int a, int b, char op)
{
	switch (op)
	{
		case '+': return (a + b);
		case '-': return (a - b);
		case '*': return (a * b);
		case '/':
			if (b == 0)
				throw std::runtime_error("division by zero");
			return (a / b);
	}
	// ここには到達しない(呼ぶ前に isOperator で確認済み)
	throw std::runtime_error("unknown operator");
}

// =============================================================================
// evaluate
// -----------------------------------------------------------------------------
// 式をスペース区切りでトークンに分け、スタックを使って評価する。
//
// アルゴリズム:
//   各トークン t について:
//     - t が1桁の数字(0〜9)なら push
//     - t が演算子なら:
//         スタックに2要素以上ある事を確認 → b=pop, a=pop → push(a op b)
//     - それ以外(2桁の数・括弧・小数点・未知文字)はエラー
//   ループ後、スタックにちょうど1要素あればそれが答え。違えばエラー。
// =============================================================================
int RPN::evaluate(const std::string& expression)
{
	// 状態をリセット(同じインスタンスを使い回しても安全にするため)
	while (!_stack.empty())
		_stack.pop();

	std::istringstream iss(expression);
	std::string token;

	while (iss >> token)
	{
		// トークンは「1文字」でなければならない(2桁の数や "++" 等は不正)
		if (token.size() != 1)
			throw std::runtime_error("invalid token");

		char c = token[0];

		if (std::isdigit(static_cast<unsigned char>(c)))
		{
			// 1桁の数字 → 整数に変換して push
			_stack.push(c - '0');
		}
		else if (isOperator(c))
		{
			// 演算子 → 2項取り出して計算
			if (_stack.size() < 2)
				throw std::runtime_error("not enough operands");
			int b = _stack.top(); _stack.pop();
			int a = _stack.top(); _stack.pop();
			_stack.push(applyOperator(a, b, c));
		}
		else
		{
			// 括弧・小数点・アルファベット等はすべて不正
			throw std::runtime_error("invalid token");
		}
	}

	// 正常な後置式なら最終的にちょうど1個残る
	if (_stack.size() != 1)
		throw std::runtime_error("invalid expression");

	return (_stack.top());
}

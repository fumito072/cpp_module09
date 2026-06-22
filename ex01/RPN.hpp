#ifndef RPN_HPP
# define RPN_HPP

# include <string>
# include <stack>

// =============================================================================
// RPN (Reverse Polish Notation = 逆ポーランド記法) 評価クラス
// -----------------------------------------------------------------------------
// "8 9 * 9 - 9 - 9 - 4 - 1 +" のような後置記法の式を受け取り、std::stack を
// 使って左から順に評価して結果を返す。
//
//   - 数字 (0〜9, 1桁) を見たらスタックに push
//   - 演算子 (+ - * /) を見たらスタックから2つ pop して計算し、結果を push
//   - 最終的にスタックに1つだけ残ればそれが答え
//
// 不正な入力(トークン不正・スタック不足・ゼロ除算・最後に複数残る等)は
// std::runtime_error を投げる。
// =============================================================================
class RPN
{
	public:
		// --- OCF ---
		RPN();
		RPN(const RPN& other);
		RPN& operator=(const RPN& other);
		~RPN();

		// 式を評価して結果を返す。不正なら例外を投げる。
		int evaluate(const std::string& expression);

	private:
		// 計算途中の値を積むスタック。
		// std::stack はデフォルトで std::deque をコンテナアダプタとして使う
		// LIFO(後入れ先出し)構造。push / top / pop / size だけ使える。
		std::stack<int> _stack;

		// 演算子1文字 (+ - * /) かどうか
		static bool isOperator(char c);
		// 2項演算を行う。ゼロ除算は例外。
		static int  applyOperator(int a, int b, char op);
};

#endif

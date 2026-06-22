#include "RPN.hpp"
#include <iostream>

// =============================================================================
// main
// -----------------------------------------------------------------------------
// 使い方: ./RPN "<逆ポーランド記法の式>"
//   - 引数はちょうど1つ(式全体をダブルクォートで囲む)。
//   - 成功時: 計算結果を標準出力へ。
//   - 失敗時: "Error" を標準エラーへ。
// =============================================================================
int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Error" << std::endl;
		return (1);
	}

	RPN rpn;
	try
	{
		int result = rpn.evaluate(argv[1]);
		std::cout << result << std::endl;
	}
	catch (const std::exception& e)
	{
		// すべてのエラーは仕様どおり "Error" の一語で表す。
		std::cerr << "Error" << std::endl;
		return (1);
	}
	return (0);
}

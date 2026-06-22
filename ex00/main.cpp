#include "BitcoinExchange.hpp"
#include <iostream>

// =============================================================================
// main
// -----------------------------------------------------------------------------
// 使い方: ./btc <input_file>
//   - 引数の数が正しくなければ "Error: could not open file."
//   - data.csv を読み込み、入力ファイルを1行ずつ処理する。
// =============================================================================
int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "Error: could not open file." << std::endl;
		return (1);
	}

	BitcoinExchange btc;
	try
	{
		// 価格DBを読み込む。固定で "data.csv" を使う。
		btc.loadDatabase("data.csv");
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
		return (1);
	}

	// 入力ファイルを処理(各行のエラーはその行ごとに出力される)
	btc.processInput(argv[1]);
	return (0);
}

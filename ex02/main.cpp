#include "PmergeMe.hpp"
#include <iostream>

// =============================================================================
// main
// -----------------------------------------------------------------------------
// 使い方: ./PmergeMe <正の整数列...>
//   例: ./PmergeMe 3 5 9 7 4
//   - 引数が無い、または不正(非数字・負数・0・オーバーフロー)なら "Error"。
// =============================================================================
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "Error" << std::endl;
		return (1);
	}

	PmergeMe pm;
	try
	{
		pm.parse(argc, argv);
		pm.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error" << std::endl;
		return (1);
	}
	return (0);
}

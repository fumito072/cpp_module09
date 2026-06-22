#include "PmergeMe.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <climits>
#include <stdexcept>
#include <algorithm>

// =============================================================================
// OCF
// =============================================================================
PmergeMe::PmergeMe() {}

PmergeMe::PmergeMe(const PmergeMe& other)
	: _vec(other._vec), _deq(other._deq) {}

PmergeMe& PmergeMe::operator=(const PmergeMe& other)
{
	if (this != &other)
	{
		_vec = other._vec;
		_deq = other._deq;
	}
	return (*this);
}

PmergeMe::~PmergeMe() {}

// =============================================================================
// parse
// -----------------------------------------------------------------------------
// 引数を1つずつ検証して両コンテナに格納する。
//   - 空文字列・数字以外を含む → エラー
//   - 範囲は正の整数 (1 以上 INT_MAX 以下)。負数・0・オーバーフローはエラー。
//     ※「正の整数列」なので 0 も弾く方針(課題例に合わせやすい)。
// =============================================================================
void PmergeMe::parse(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		std::string s(argv[i]);
		if (s.empty())
			throw std::runtime_error("empty argument");

		// すべて数字か確認(符号や小数点、空白も不正)
		for (std::string::size_type j = 0; j < s.size(); ++j)
		{
			if (!std::isdigit(static_cast<unsigned char>(s[j])))
				throw std::runtime_error("non-digit character");
		}

		// long で受けて INT_MAX を超えないか確認(オーバーフロー検査)
		errno = 0;
		char* endp = NULL;
		long val = std::strtol(s.c_str(), &endp, 10);
		if (*endp != '\0' || val <= 0 || val > INT_MAX)
			throw std::runtime_error("out of range");

		_vec.push_back(static_cast<int>(val));
		_deq.push_back(static_cast<int>(val));
	}

	if (_vec.empty())
		throw std::runtime_error("no input");
}

// =============================================================================
// printVector
// -----------------------------------------------------------------------------
// "Before: 3 5 9 ..." の形で表示する。要素数が多いと長いが、課題例どおり全表示。
// =============================================================================
void PmergeMe::printVector(const std::string& prefix,
						   const std::vector<int>& v) const
{
	std::cout << prefix;
	for (std::vector<int>::size_type i = 0; i < v.size(); ++i)
	{
		std::cout << v[i];
		if (i + 1 < v.size())
			std::cout << " ";
	}
	std::cout << std::endl;
}

// =============================================================================
// Jacobsthal 数列
// -----------------------------------------------------------------------------
// J(0)=0, J(1)=1, J(n) = J(n-1) + 2*J(n-2)
//   0, 1, 1, 3, 5, 11, 21, 43, 85, 171, ...
// Ford-Johnson の「ペンド要素を挿入する順番」を決めるのに使う。
// この順序で挿入すると、各挿入の二分探索の探索範囲が常に「2の冪 - 1」以下に収まり、
// 必要な比較回数の最悪値を最小化できる。
// =============================================================================
static unsigned long jacobsthal(unsigned long n)
{
	if (n == 0)
		return (0);
	if (n == 1)
		return (1);
	unsigned long a = 0; // J(0)
	unsigned long b = 1; // J(1)
	for (unsigned long i = 2; i <= n; ++i)
	{
		unsigned long c = b + 2 * a;
		a = b;
		b = c;
	}
	return (b);
}

// =============================================================================
//  ============================ VECTOR 版 ============================
// =============================================================================
//
// ここからが Ford-Johnson の本体(vector 版)。比較回数を理論的最小に近づけるため、
// 「ペア(大きい方 big と小さい方 small)」を1つの構造体としてまとめて扱う。
// こうすると、small を main へ挿入するとき「その相棒 big の位置まで」に二分探索の
// 範囲を限定でき、Ford-Johnson 本来の比較削減が効く。
// -----------------------------------------------------------------------------

// 1ペアを表す構造体。big >= small。
struct VPair
{
	int big;
	int small;
};

// VPair を big の昇順に並べるための比較関数(std::sort ではなく自前再帰で使う)。
// ※ ソートそのものは下の mergeSortPairsVector が Ford-Johnson の発想で行う。

// -----------------------------------------------------------------------------
// メインチェーンへの「範囲限定つき」二分挿入(vector 版)。
//   main  : 昇順に並んだメインチェーン
//   value : 挿入したい small の値
//   limit : 探索してよい右端(この位置の手前までに必ず入る = 相棒 big の位置)
// std::upper_bound を [begin, begin+limit) に限定して呼ぶことで、二分探索の幅を
// 相棒 big の位置までに抑える。これが比較回数削減の肝。
// -----------------------------------------------------------------------------
static void binaryInsertVector(std::vector<int>& main, int value, std::size_t limit)
{
	std::vector<int>::iterator hi = main.begin() + limit;
	std::vector<int>::iterator pos = std::upper_bound(main.begin(), hi, value);
	main.insert(pos, value);
}

// -----------------------------------------------------------------------------
// VPair の列を「big の値」で昇順ソートする(Ford-Johnson の再帰部分)。
// これ自体を再帰的な merge-insertion で行うことで、大きい方の列が整列する。
// ここでは可読性のため、ペアをさらにペアにして…という完全再帰ではなく、
// 「big を取り出して再帰ソート → small を対応付け」する実装にしている。
// (big の列のソートに同じ sortVector を使う = 真の再帰)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// sortVector : Ford-Johnson 本体(vector 版)
//
// ステップ:
//   1) 隣り合う2要素をペアにし、各ペアで (big, small) を決める。
//      奇数個なら最後の1個を straggler(余り)として退避。
//   2) big だけを取り出して再帰的にソートする(これがメインチェーンの骨格)。
//   3) ソート済みの big の順に、相棒の small を並べ替える(ペアの対応を保つ)。
//   4) メインチェーン main = [b1, a1, a2, ..., aN] を作る。
//      b1 は最小ペアの small。a1..aN はソート済み big。
//      (b1 <= a1 が必ず成り立つので、b1 は安全に先頭へ置ける)
//   5) 残りの small(b2..bN) を Jacobsthal 数列が示す順番で、
//      かつ「相棒 big の位置まで」に範囲限定して二分挿入する。
//   6) straggler があれば最後に main 全体へ二分挿入する。
// -----------------------------------------------------------------------------
void PmergeMe::sortVector(std::vector<int>& v)
{
	if (v.size() <= 1)
		return;

	// --- 1) ペアリング ---
	bool hasStraggler = (v.size() % 2 == 1);
	int  straggler = 0;
	if (hasStraggler)
		straggler = v.back();

	std::size_t pairCount = v.size() / 2;
	std::vector<int> bigs;     // 各ペアの大きい方
	std::vector<int> smalls;   // 各ペアの小さい方(bigs と同じ index で対応)
	bigs.reserve(pairCount);
	smalls.reserve(pairCount);
	for (std::size_t i = 0; i < pairCount; ++i)
	{
		int x = v[2 * i];
		int y = v[2 * i + 1];
		if (x < y)
			std::swap(x, y); // x=big, y=small
		bigs.push_back(x);
		smalls.push_back(y);
	}

	// --- 2) big を再帰的にソート ---
	// bigs を昇順にしたものを得る。ここで sortVector を再帰呼び出しするのが
	// Ford-Johnson の「大きい方を先に整列させる」部分。
	std::vector<int> sortedBigs = bigs;
	sortVector(sortedBigs);

	// --- 3) ソート済み big に相棒 small を対応付ける ---
	// (元の bigs/smalls のペア対応を、sortedBigs の順に並べ替える)
	std::vector<bool> taken(pairCount, false);
	std::vector<int> sortedSmalls;
	sortedSmalls.reserve(pairCount);
	for (std::size_t i = 0; i < sortedBigs.size(); ++i)
	{
		int target = sortedBigs[i];
		for (std::size_t j = 0; j < pairCount; ++j)
		{
			if (!taken[j] && bigs[j] == target)
			{
				taken[j] = true;
				sortedSmalls.push_back(smalls[j]);
				break;
			}
		}
	}

	// --- 4) メインチェーンを作る ---
	// main = [b1, a1, a2, ..., aN]
	std::vector<int> main;
	main.reserve(v.size());
	main.push_back(sortedSmalls[0]);          // b1
	for (std::size_t i = 0; i < sortedBigs.size(); ++i)
		main.push_back(sortedBigs[i]);        // a1..aN

	// 各 small(pend) の「相棒 big が main 上のどこか」を表す印を、値そのものではなく
	// 「big の番号」で管理する。挿入で main が伸びると big の位置がずれるので、
	// big 値を upper_bound で引き直して現在位置を得る。big は互いに異なるとは限らない
	// (重複入力あり)ため、位置の特定には「これまで何個 small を入れたか」を加味する。
	//
	// シンプルかつ確実にするため、ここでは「相棒 big の現在位置」を、main 上で
	// sortedBigs[k] の値を lower_bound で探し、そこから挿入済みオフセットを足して
	// 求めるのではなく、明示的にインデックスを追跡する方式を採る。

	// boundIndex[k] = pend 要素 k (= sortedSmalls[k+1] = b_{k+2}) の相棒 big の、
	//                 現在の main 上での添字。
	// 初期状態: main = [b1, a1, a2, ...] なので、
	//   pend index 0 -> b2 の相棒 a2 は main[2]
	//   pend index 1 -> b3 の相棒 a3 は main[3] ...
	//   一般に pend index k の相棒は main[k + 2]。
	std::vector<std::size_t> boundIndex(pairCount > 0 ? pairCount - 1 : 0);
	for (std::size_t k = 0; k + 1 < pairCount; ++k)
		boundIndex[k] = k + 2; // a_{k+2} の初期位置

	// --- 5) Jacobsthal 順で範囲限定二分挿入 ---
	std::size_t pendCount = (pairCount > 0) ? pairCount - 1 : 0; // b2..bN
	if (pendCount > 0)
	{
		// 挿入順(pend のインデックス)を Jacobsthal 数列から生成。
		std::vector<std::size_t> order;
		std::vector<bool> used(pendCount, false);
		unsigned long t = 2;
		unsigned long prevJ = jacobsthal(1); // 1
		while (used.size() && true)
		{
			unsigned long curJ = jacobsthal(t);
			// グループは pend の 1-based 番号で (prevJ, curJ]。降順に拾う。
			unsigned long hi = curJ;
			if (hi > pendCount)
				hi = pendCount;
			unsigned long lo = prevJ + 1;
			for (unsigned long idx = hi; idx >= lo && idx >= 1; --idx)
			{
				std::size_t z = static_cast<std::size_t>(idx - 1); // 0-based
				if (z < pendCount && !used[z])
				{
					order.push_back(z);
					used[z] = true;
				}
				if (idx == 1)
					break;
			}
			prevJ = curJ;
			++t;
			if (curJ >= pendCount)
				break;
		}
		for (std::size_t z = 0; z < pendCount; ++z)
			if (!used[z])
				order.push_back(z);

		// 実際の挿入。順番に処理し、挿入のたびに「自分より後ろにある boundIndex」を
		// +1 ずらす(main が1つ伸びるため)。
		for (std::size_t oi = 0; oi < order.size(); ++oi)
		{
			std::size_t k = order[oi];            // pend index
			int value = sortedSmalls[k + 1];      // b_{k+2}
			std::size_t limit = boundIndex[k];    // 相棒 a_{k+1} の現在位置(この手前までに入る)

			// [begin, begin+limit) に範囲を限定して二分挿入。
			std::vector<int>::iterator hi = main.begin() + limit;
			std::vector<int>::iterator pos = std::upper_bound(main.begin(), hi, value);
			std::size_t insertedAt = static_cast<std::size_t>(pos - main.begin());
			main.insert(pos, value);

			// 挿入位置 insertedAt 以降にある相棒インデックスを1つ後ろへずらす。
			for (std::size_t m = 0; m < boundIndex.size(); ++m)
				if (boundIndex[m] >= insertedAt)
					boundIndex[m] += 1;
		}
	}

	// --- 6) straggler を最後に挿入(範囲は main 全体) ---
	if (hasStraggler)
		binaryInsertVector(main, straggler, main.size());

	v = main;
}

// =============================================================================
//  ============================ DEQUE 版 ============================
//  (vector 版と同じロジック。コンテナ型だけ deque に置き換えてある)
// =============================================================================

static void binaryInsertDeque(std::deque<int>& main, int value, std::size_t limit)
{
	std::deque<int>::iterator hi = main.begin() + limit;
	std::deque<int>::iterator pos = std::upper_bound(main.begin(), hi, value);
	main.insert(pos, value);
}

void PmergeMe::sortDeque(std::deque<int>& d)
{
	if (d.size() <= 1)
		return;

	bool hasStraggler = (d.size() % 2 == 1);
	int  straggler = 0;
	if (hasStraggler)
		straggler = d.back();

	std::size_t pairCount = d.size() / 2;
	std::deque<int> bigs;
	std::deque<int> smalls;
	for (std::size_t i = 0; i < pairCount; ++i)
	{
		int x = d[2 * i];
		int y = d[2 * i + 1];
		if (x < y)
			std::swap(x, y);
		bigs.push_back(x);
		smalls.push_back(y);
	}

	std::deque<int> sortedBigs = bigs;
	sortDeque(sortedBigs);

	std::vector<bool> taken(pairCount, false);
	std::deque<int> sortedSmalls;
	for (std::size_t i = 0; i < sortedBigs.size(); ++i)
	{
		int target = sortedBigs[i];
		for (std::size_t j = 0; j < pairCount; ++j)
		{
			if (!taken[j] && bigs[j] == target)
			{
				taken[j] = true;
				sortedSmalls.push_back(smalls[j]);
				break;
			}
		}
	}

	std::deque<int> main;
	main.push_back(sortedSmalls[0]);
	for (std::size_t i = 0; i < sortedBigs.size(); ++i)
		main.push_back(sortedBigs[i]);

	std::vector<std::size_t> boundIndex(pairCount > 0 ? pairCount - 1 : 0);
	for (std::size_t k = 0; k + 1 < pairCount; ++k)
		boundIndex[k] = k + 2;

	std::size_t pendCount = (pairCount > 0) ? pairCount - 1 : 0;
	if (pendCount > 0)
	{
		std::vector<std::size_t> order;
		std::vector<bool> used(pendCount, false);
		unsigned long t = 2;
		unsigned long prevJ = jacobsthal(1);
		while (true)
		{
			unsigned long curJ = jacobsthal(t);
			unsigned long hi = curJ;
			if (hi > pendCount)
				hi = pendCount;
			unsigned long lo = prevJ + 1;
			for (unsigned long idx = hi; idx >= lo && idx >= 1; --idx)
			{
				std::size_t z = static_cast<std::size_t>(idx - 1);
				if (z < pendCount && !used[z])
				{
					order.push_back(z);
					used[z] = true;
				}
				if (idx == 1)
					break;
			}
			prevJ = curJ;
			++t;
			if (curJ >= pendCount)
				break;
		}
		for (std::size_t z = 0; z < pendCount; ++z)
			if (!used[z])
				order.push_back(z);

		for (std::size_t oi = 0; oi < order.size(); ++oi)
		{
			std::size_t k = order[oi];
			int value = sortedSmalls[k + 1];
			std::size_t limit = boundIndex[k];

			std::deque<int>::iterator hi = main.begin() + limit;
			std::deque<int>::iterator pos = std::upper_bound(main.begin(), hi, value);
			std::size_t insertedAt = static_cast<std::size_t>(pos - main.begin());
			main.insert(pos, value);

			for (std::size_t m = 0; m < boundIndex.size(); ++m)
				if (boundIndex[m] >= insertedAt)
					boundIndex[m] += 1;
		}
	}

	if (hasStraggler)
		binaryInsertDeque(main, straggler, main.size());

	d = main;
}

// =============================================================================
// run
// -----------------------------------------------------------------------------
// Before を表示 → vector で計測ソート → deque で計測ソート →
// After を表示 → 両方の処理時間を表示する。
//
// 時間計測は <ctime> の std::clock() を使う。CLOCKS_PER_SEC で割って秒に直し、
// 1e6 を掛けてマイクロ秒(us)で表示する。
// =============================================================================
#include <ctime>

void PmergeMe::run()
{
	// Before(ソート前)を表示。vector の中身を使う(deque と同一内容)。
	printVector("Before: ", _vec);

	// --- vector 版を計測 ---
	std::clock_t startV = std::clock();
	sortVector(_vec);
	std::clock_t endV = std::clock();
	double usV = static_cast<double>(endV - startV) / CLOCKS_PER_SEC * 1e6;

	// --- deque 版を計測 ---
	std::clock_t startD = std::clock();
	sortDeque(_deq);
	std::clock_t endD = std::clock();
	double usD = static_cast<double>(endD - startD) / CLOCKS_PER_SEC * 1e6;

	// After(ソート後)を表示。
	printVector("After:  ", _vec);

	// 処理時間を表示。小数を出すため stream のフォーマットを使う。
	std::cout.setf(std::ios::fixed);
	std::cout.precision(5);
	std::cout << "Time to process a range of " << _vec.size()
			  << " elements with std::vector : " << usV << " us" << std::endl;
	std::cout << "Time to process a range of " << _deq.size()
			  << " elements with std::deque  : " << usD << " us" << std::endl;
}

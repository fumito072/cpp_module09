#ifndef PMERGEME_HPP
# define PMERGEME_HPP

# include <vector>
# include <deque>
# include <string>

// =============================================================================
// PmergeMe
// -----------------------------------------------------------------------------
// 正の整数列を Ford-Johnson 法 (merge-insertion sort) でソートするクラス。
//
// このモジュールでは「演習ごとに違うコンテナ」を使う制約があり、
//   ex00 = std::map, ex01 = std::stack
// を既に使ったので、ここでは std::vector と std::deque の2種類を使う。
// 同じ Ford-Johnson アルゴリズムを2つのコンテナで別々に実装し、
// それぞれの処理時間を計測して比較する。
//
// 「汎用テンプレート関数を避けよ」という課題PDFの助言に従い、vector版と deque版を
// あえて別関数として書き下している(読みやすさ・評価での説明しやすさ重視)。
// =============================================================================
class PmergeMe
{
	public:
		// --- OCF ---
		PmergeMe();
		PmergeMe(const PmergeMe& other);
		PmergeMe& operator=(const PmergeMe& other);
		~PmergeMe();

		// コマンドライン引数(argv の 1 番目以降)をパースして内部に取り込む。
		// 非数字・負数・オーバーフローがあれば例外を投げる。
		void parse(int argc, char** argv);

		// 両コンテナでソートを実行し、Before/After と処理時間を表示する。
		void run();

	private:
		std::vector<int> _vec;   // vector 版の作業用データ
		std::deque<int>  _deq;   // deque 版の作業用データ

		// ---- vector 版 Ford-Johnson ----
		// 入力列を破壊的にソートする入口。
		void  sortVector(std::vector<int>& v);

		// ---- deque 版 Ford-Johnson ----
		void  sortDeque(std::deque<int>& d);

		// 表示ヘルパ
		void  printVector(const std::string& prefix,
						  const std::vector<int>& v) const;
};

#endif

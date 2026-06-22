#ifndef BITCOINEXCHANGE_HPP
# define BITCOINEXCHANGE_HPP

# include <map>
# include <string>

// =============================================================================
// BitcoinExchange
// -----------------------------------------------------------------------------
// ビットコインの価格データベース(date -> exchange_rate)を std::map で保持し、
// 入力ファイルに書かれた「日付 | 量」に対して、その日付の為替レートを掛けた値を
// 計算して表示するクラス。
//
// データベースに該当する日付が無い場合は、「その日付以下で最も近い日付」(lower
// bound) のレートを使う。これを std::map の lower_bound / upper_bound を使って
// 効率よく実現するのがこの演習の肝。
// =============================================================================
class BitcoinExchange
{
	public:
		// --- OCF (Orthodox Canonical Form) ---
		BitcoinExchange();                                       // デフォルト
		BitcoinExchange(const BitcoinExchange& other);           // コピー
		BitcoinExchange& operator=(const BitcoinExchange& other);// 代入
		~BitcoinExchange();                                      // デストラクタ

		// data.csv (価格DB) を読み込んで内部の map に格納する。
		// 読み込みに失敗したら例外を投げる。
		void loadDatabase(const std::string& dbFile);

		// 入力ファイルを1行ずつ処理して結果(または各行のエラー)を出力する。
		void processInput(const std::string& inputFile) const;

	private:
		// 日付(YYYY-MM-DD) をキー、為替レートを値として持つ。
		// std::map はキーで自動的に昇順ソートされる(内部は赤黒木 = 平衡二分探索木)。
		std::map<std::string, double> _database;

		// --- 入力1行を解釈して結果を出力するヘルパ ---
		void processLine(const std::string& line) const;

		// --- バリデーション系ヘルパ ---
		// "YYYY-MM-DD" として正しいか(桁・区切り・実在する月日か)を判定する。
		static bool isValidDate(const std::string& date);
		// 文字列を double に変換する。数値として不正なら false を返す。
		static bool parseValue(const std::string& str, double& out);

		// 指定された日付に対応するレートを返す。
		// 完全一致が無ければ「その日付以下で最も近い日付」のレートを返す。
		double getRate(const std::string& date) const;
};

#endif

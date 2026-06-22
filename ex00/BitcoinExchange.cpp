#include "BitcoinExchange.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// =============================================================================
// OCF
// =============================================================================
BitcoinExchange::BitcoinExchange() {}

BitcoinExchange::BitcoinExchange(const BitcoinExchange& other)
	: _database(other._database) {}

BitcoinExchange& BitcoinExchange::operator=(const BitcoinExchange& other)
{
	if (this != &other)
		_database = other._database;
	return (*this);
}

BitcoinExchange::~BitcoinExchange() {}

// =============================================================================
// 文字列の前後の空白を取り除く小さなユーティリティ(無名 = ファイルローカル)
// =============================================================================
static std::string trim(const std::string& s)
{
	const std::string ws = " \t\r\n";
	std::string::size_type start = s.find_first_not_of(ws);
	if (start == std::string::npos)
		return ("");
	std::string::size_type end = s.find_last_not_of(ws);
	return (s.substr(start, end - start + 1));
}

// =============================================================================
// loadDatabase
// -----------------------------------------------------------------------------
// data.csv の各行 "date,exchange_rate" を読み、map に格納する。
// 1行目はヘッダ "date,exchange_rate" なので読み飛ばす。
// =============================================================================
void BitcoinExchange::loadDatabase(const std::string& dbFile)
{
	std::ifstream file(dbFile.c_str());
	if (!file.is_open())
		throw std::runtime_error("could not open database file.");

	std::string line;
	bool firstLine = true;
	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty())
			continue;
		// ヘッダ行 (date,exchange_rate) はスキップ
		if (firstLine)
		{
			firstLine = false;
			if (line == "date,exchange_rate")
				continue;
		}
		std::string::size_type comma = line.find(',');
		if (comma == std::string::npos)
			continue; // 区切りが無い行はDBとして無効なので無視
		std::string date = trim(line.substr(0, comma));
		std::string rateStr = trim(line.substr(comma + 1));
		// strtod でレートを数値化(DB側は信頼してシンプルに扱う)
		char* endp = NULL;
		double rate = std::strtod(rateStr.c_str(), &endp);
		if (endp == rateStr.c_str())
			continue; // 数値に変換できない行は無視
		_database[date] = rate;
	}
	file.close();

	if (_database.empty())
		throw std::runtime_error("database is empty or invalid.");
}

// =============================================================================
// isValidDate
// -----------------------------------------------------------------------------
// "YYYY-MM-DD" 形式かつ実在する日付かを判定する。
//   - 長さ10、位置4と7が '-'
//   - 年月日が全て数字
//   - 月は 1〜12、日は その月の日数に収まるか(うるう年も考慮)
// =============================================================================
bool BitcoinExchange::isValidDate(const std::string& date)
{
	if (date.size() != 10)
		return (false);
	if (date[4] != '-' || date[7] != '-')
		return (false);
	for (std::string::size_type i = 0; i < date.size(); ++i)
	{
		if (i == 4 || i == 7)
			continue;
		if (!std::isdigit(static_cast<unsigned char>(date[i])))
			return (false);
	}

	int year  = std::atoi(date.substr(0, 4).c_str());
	int month = std::atoi(date.substr(5, 2).c_str());
	int day   = std::atoi(date.substr(8, 2).c_str());

	if (month < 1 || month > 12)
		return (false);
	if (day < 1)
		return (false);

	int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	// うるう年判定: 4で割り切れ、かつ(100で割り切れない or 400で割り切れる)
	bool isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
	if (month == 2 && isLeap)
	{
		if (day > 29)
			return (false);
	}
	else if (day > daysInMonth[month - 1])
		return (false);

	return (true);
}

// =============================================================================
// parseValue
// -----------------------------------------------------------------------------
// 文字列を double に変換する。余分な文字が残っていたら不正とみなす。
// 戻り値: 変換に成功したか(数値として妥当か)。範囲チェックは呼び出し側で行う。
// =============================================================================
bool BitcoinExchange::parseValue(const std::string& str, double& out)
{
	if (str.empty())
		return (false);

	char* endp = NULL;
	double v = std::strtod(str.c_str(), &endp);

	// 文字列全体が数値として消費されたか(末尾にゴミが無いか)を確認
	if (endp == str.c_str() || *endp != '\0')
		return (false);

	out = v;
	return (true);
}

// =============================================================================
// getRate
// -----------------------------------------------------------------------------
// その日付のレートを返す。完全一致が無い場合は「その日付以下で最も近い日付」の
// レートを返す。
//
// lower_bound(date) は「date 以上(>=)で最初の要素」を指すイテレータを返す。
//   - *it のキーが date と完全一致  → その要素を使う
//   - *it のキーが date より大きい   → 一つ前(--it)が「date より小さい最大の日付」
//   - it == end()                    → 全要素が date より小さい → 最後の要素を使う
//
// 文字列の "YYYY-MM-DD" は辞書順比較が日付の大小と一致するため、文字列キーで
// そのまま正しく比較できる(ゼロ埋め固定長フォーマットだから成立する)。
// =============================================================================
double BitcoinExchange::getRate(const std::string& date) const
{
	std::map<std::string, double>::const_iterator it = _database.lower_bound(date);

	// 完全一致
	if (it != _database.end() && it->first == date)
		return (it->second);

	// date より小さい日付が一つも無い = 最古のDB日付より前 → エラー扱い
	if (it == _database.begin())
		throw std::runtime_error("no earlier date in database.");

	// それ以外は一つ前(= date 未満で最大の日付)を使う
	--it;
	return (it->second);
}

// =============================================================================
// processLine
// -----------------------------------------------------------------------------
// 入力1行 "date | value" を解釈して結果またはエラーを出力する。
// =============================================================================
void BitcoinExchange::processLine(const std::string& rawLine) const
{
	std::string line = trim(rawLine);
	if (line.empty())
		return;

	// 区切りは " | "。まずは '|' の位置を探す。
	std::string::size_type pipe = line.find('|');
	if (pipe == std::string::npos)
	{
		std::cout << "Error: bad input => " << line << std::endl;
		return;
	}

	std::string date    = trim(line.substr(0, pipe));
	std::string valueSt = trim(line.substr(pipe + 1));

	// 日付バリデーション
	if (!isValidDate(date))
	{
		std::cout << "Error: bad input => " << line << std::endl;
		return;
	}

	// 値のパース
	double value;
	if (valueSt.empty() || !parseValue(valueSt, value))
	{
		std::cout << "Error: bad input => " << line << std::endl;
		return;
	}

	// 範囲チェック: 0未満は負、1000超は大きすぎ
	if (value < 0)
	{
		std::cout << "Error: not a positive number." << std::endl;
		return;
	}
	if (value > 1000)
	{
		std::cout << "Error: too large a number." << std::endl;
		return;
	}

	// レート取得(DBに最古日付より前しか無い等で投げられる場合がある)
	try
	{
		double rate = getRate(date);
		double result = value * rate;
		std::cout << date << " => " << valueSt << " = " << result << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: bad input => " << line << std::endl;
	}
}

// =============================================================================
// processInput
// -----------------------------------------------------------------------------
// 入力ファイルを開き、1行目のヘッダ "date | value" を飛ばして各行を処理する。
// =============================================================================
void BitcoinExchange::processInput(const std::string& inputFile) const
{
	std::ifstream file(inputFile.c_str());
	if (!file.is_open())
	{
		std::cout << "Error: could not open file." << std::endl;
		return;
	}

	std::string line;
	bool firstLine = true;
	while (std::getline(file, line))
	{
		std::string trimmed = trim(line);
		// ヘッダ "date | value" は読み飛ばす
		if (firstLine)
		{
			firstLine = false;
			if (trimmed == "date | value")
				continue;
		}
		if (trimmed.empty())
			continue;
		processLine(line);
	}
	file.close();
}

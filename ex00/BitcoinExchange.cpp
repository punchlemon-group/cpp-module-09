#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "BitcoinExchange.hpp"

BitcoinExchange::BitcoinExchange() : _filename(""), _delimiter(',') {}

BitcoinExchange::BitcoinExchange(const std::string& filename, char delimiter)
    : _filename(filename), _delimiter(delimiter) {
    processFile();
}

BitcoinExchange::BitcoinExchange(const BitcoinExchange& other)
    : _db(other._db), _filename(other._filename), _delimiter(other._delimiter) {}

BitcoinExchange& BitcoinExchange::operator=(const BitcoinExchange& other) {
    if (this != &other) {
        _db = other._db;
        _filename = other._filename;
        _delimiter = other._delimiter;
    }
    return *this;
}

BitcoinExchange::~BitcoinExchange() {}

bool BitcoinExchange::printError(const std::string* p_filename, const int lineNumber, const std::string& msg) const {
    if (!p_filename) {
        std::cerr << _filename << ":" << lineNumber << ": ";
    }
    std::cerr << "Error: " << msg << std::endl;
    return false;
}

std::string BitcoinExchange::makeErrorString(const bool isInputFile, const int lineNumber, const std::string& msg) const {
    std::ostringstream oss;
    if (!isInputFile) {
        oss << _filename << ":" << lineNumber << ": ";
    }
    oss << "Error: " << msg;
    return oss.str();
}

std::pair<std::string, double> BitcoinExchange::processLine(const bool isInputFile, const int lineNumber, const std::string& line, const char delim) const {
    std::istringstream iss(line);
    std::string date, valueStr;
    if (!(getline(iss, date, delim) && getline(iss, valueStr)))
        throw std::runtime_error(makeErrorString(isInputFile, lineNumber, "bad input => " + line));
    // Trim whitespace
    date.erase(0, date.find_first_not_of(" \t"));
    date.erase(date.find_last_not_of(" \t") + 1);
    valueStr.erase(0, valueStr.find_first_not_of(" \t"));
    valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
    // Date format check
    if (!isValidDate(date))
        throw std::runtime_error(makeErrorString(isInputFile, lineNumber, "bad input => " + line));
    // Value format and range check
    if (!isValidValue(valueStr))
        throw std::runtime_error(makeErrorString(isInputFile, lineNumber, "bad input => " + line));
    double value = std::strtod(valueStr.c_str(), NULL);
    return std::make_pair(date, value);
}

void BitcoinExchange::processFile(const std::string* inputFile, const char delimiter) {
    bool isInputFile = (inputFile != NULL);
    std::string filename = isInputFile ? *inputFile : _filename;
    const char delim = isInputFile ? delimiter : _delimiter;
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error(makeErrorString(isInputFile, 0, "could not open file."));
    std::string line;
    int lineNumber = 1;
    if (!getline(file, line)) {
        file.close();
        throw std::runtime_error(makeErrorString(isInputFile, lineNumber, "failed to read header line."));
    }
    lineNumber++;
    std::string dateStr;
    while (getline(file, line)) {
        try {
            std::pair<std::string, double> data = processLine(isInputFile, lineNumber, line, delim);
            const std::string& date = data.first;
            double value = data.second;
            if (value < 0) {
                throw std::runtime_error(makeErrorString(isInputFile, lineNumber, "not a positive number."));
            }
            if (!isInputFile) {
                _db[date] = value;
            }
            else {
                if (value > 1000) {
                    throw std::runtime_error(makeErrorString(isInputFile, lineNumber, "too large a number."));
                }
                else {
                    double rate = getRate(isInputFile, date);
                    std::cout << date << " => " << value << " = " << value * rate << std::endl;
                }
            }
        } catch (const std::exception& e) {
            if (!isInputFile) {
                file.close();
                throw;
            }
            else {
                std::cerr << e.what() << std::endl;
            }
        }
        lineNumber++;
    }
    file.close();
}

double BitcoinExchange::getRate(const bool isInputFile, const std::string& date) const {
    std::map<std::string, double>::const_iterator it = _db.find(date);
    if (it != _db.end())
        return it->second;
    it = _db.lower_bound(date);
    if (it == _db.begin())
        throw std::runtime_error(makeErrorString(isInputFile, 0, "No lower date found in DB"));
    --it;
    return it->second;
}

bool isLeap(int year) {
    // 4で割り切れ、かつ100で割り切れない年、または400で割り切れる年
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

bool BitcoinExchange::isValidDate(const std::string& date) const {
    if (date.size() != 10) return false;
    for (int i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) {
            if (date[i] != '-') return false;
        } else {
            if (!isdigit(date[i])) return false;
        }
    }
    std::istringstream iss(date);
    int year, month, day;
    char d1, d2; // ハイフンを読み飛ばすためのダミー

    // iss >> year >> d1 >> month >> d2 >> day のように一度に読み込む
    // iss.fail()で、数値として読めなかった場合（例: "202a-10-10"）をチェック
    if (!(iss >> year >> d1 >> month >> d2 >> day) || iss.fail()) {
        return false;
    }

    // --- ③ 日付のルールと照合 ---
    // 年と月の基本的な範囲チェック
    if (year < 2009 || year > 2100) return false;
    if (month < 1 || month > 12) return false;

    // 日の範囲チェック
    int daysInMonth;
    switch (month) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            daysInMonth = 31;
            break;
        case 4: case 6: case 9: case 11:
            daysInMonth = 30;
            break;
        case 2: // 2月はうるう年を考慮
            if (isLeap(year)) {
                daysInMonth = 29;
            } else {
                daysInMonth = 28;
            }
            break;
        default:
            return false;
    }

    if (day < 1 || day > daysInMonth) {
        return false;
    }

    // 全てのチェックを通過
    return true;
}

bool BitcoinExchange::isValidValue(const std::string& value) const {
    // 正規表現 [-+]?\d{1,17}(\.\d{1,17})?([eE][-+]?\d{1,3})? に沿った手動チェック
    size_t i = 0;
    if (value.empty()) return false;
    if (value[0] == '-' || value[0] == '+') i = 1;
    // 整数部（1～17桁）
    size_t intStart = i;
    while (i < value.size() && isdigit(value[i]) && (i - intStart) < 17) ++i;
    if ((i - intStart) < 1 || (i - intStart) > 17) return false;
    // 小数部（任意、1～17桁）
    if (i < value.size() && value[i] == '.') {
        ++i;
        size_t fracStart = i;
        while (i < value.size() && isdigit(value[i]) && (i - fracStart) < 17) ++i;
        if ((i - fracStart) < 1 || (i - fracStart) > 17) return false;
    }
    // 指数部（任意、e/E[-+]?1～3桁）
    if (i < value.size() && (value[i] == 'e' || value[i] == 'E')) {
        ++i;
        if (i < value.size() && (value[i] == '-' || value[i] == '+')) ++i;
        size_t expStart = i;
        while (i < value.size() && isdigit(value[i]) && (i - expStart) < 3) ++i;
        if ((i - expStart) < 1 || (i - expStart) > 3) return false;
    }
    if (i != value.size()) return false;
    // strtodで変換し、endが文字列終端であることを確認
    char* end;
    errno = 0;
    std::strtod(value.c_str(), &end);
    if (end != value.c_str() + value.size()) return false;
    if (errno == ERANGE) return false; // オーバーフロー・アンダーフロー検出
    return true;
}

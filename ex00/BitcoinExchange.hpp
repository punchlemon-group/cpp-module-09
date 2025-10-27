#ifndef BITCOINEXCHANGE_HPP
#define BITCOINEXCHANGE_HPP

#include <string>
#include <map>
#include <cmath>
#include <limits>

class BitcoinExchange {
public:
    BitcoinExchange(const std::string& filename, char delimiter);
    BitcoinExchange(const BitcoinExchange& other);
    BitcoinExchange& operator=(const BitcoinExchange& other);
    ~BitcoinExchange();
    void processFile(const std::string* p_filename = NULL, const char delimiter = ',');
private:
    std::string makeErrorString(const bool isInputFile, const int lineNumber, const std::string& msg) const;
    double getRate(const bool isInputFile, const std::string& date) const;
    bool isValidDate(const std::string& date) const;
    bool isValidValue(const std::string& value) const;
    bool printError(const std::string* p_filename, const int lineNumber, const std::string& msg) const;
    std::pair<std::string, double> processLine(const bool isInputFile, const int lineNumber, const std::string& line, const char delim) const;
    BitcoinExchange();
    std::map<std::string, double> _db;
    std::string _filename;
    char _delimiter;
};

#endif // BITCOINEXCHANGE_HPP

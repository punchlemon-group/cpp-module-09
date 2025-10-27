#include <cstdlib>
#include <BitcoinExchange.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./btc <input_file>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        BitcoinExchange database("data.csv", ',');
        std::string inputFile = argv[1];
        database.processFile(&inputFile, '|');
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

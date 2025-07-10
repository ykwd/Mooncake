#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <regex>

inline  std::vector<int> parseInputLengths(const std::string& filename) {
    std::vector<int> input_lengths;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::string line;
    std::regex input_length_regex(R"("input_length"\s*:\s*(\d+))");
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue; // Skip empty lines
        }
        
        std::smatch match;
        if (std::regex_search(line, match, input_length_regex)) {
            try {
                int input_length = std::stoi(match[1].str());
                input_lengths.push_back(input_length);
            } catch (const std::exception& e) {
                // Skip lines where conversion fails
                continue;
            }
        }
    }
    
    return input_lengths;
} 
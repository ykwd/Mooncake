#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <gflags/gflags.h>
#include <unordered_map>

DEFINE_string(file_path, "", "Path to the input file containing JSON lines");

// {"timestamp": 120000, "input_length": 896, "output_length": 354, "hash_ids": [0, 8636]}
struct Seq {
    uint64_t timestamp;
    uint64_t input_length;
    uint64_t output_length;
    std::vector<uint64_t> hash_ids;
};

// Helper function to parse a JSON line into a Seq object
Seq parseSeqFromJson(const std::string& jsonLine) {
    Seq seq;
    
    // Regex patterns to extract values
    std::regex timestampPattern(R"("timestamp":\s*(\d+))");
    std::regex inputLengthPattern(R"("input_length":\s*(\d+))");
    std::regex outputLengthPattern(R"("output_length":\s*(\d+))");
    std::regex hashIdsPattern(R"("hash_ids":\s*\[([^\]]*)\])");
    
    std::smatch matches;
    
    // Parse timestamp
    if (std::regex_search(jsonLine, matches, timestampPattern)) {
        seq.timestamp = std::stoull(matches[1].str());
    }
    
    // Parse input_length
    if (std::regex_search(jsonLine, matches, inputLengthPattern)) {
        seq.input_length = std::stoull(matches[1].str());
    }
    
    // Parse output_length
    if (std::regex_search(jsonLine, matches, outputLengthPattern)) {
        seq.output_length = std::stoull(matches[1].str());
    }
    
    // Parse hash_ids array
    if (std::regex_search(jsonLine, matches, hashIdsPattern)) {
        std::string hashIdsStr = matches[1].str();
        std::istringstream iss(hashIdsStr);
        std::string token;
        
        // Split by comma and parse each number
        while (std::getline(iss, token, ',')) {
            // Remove whitespace
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            
            if (!token.empty()) {
                seq.hash_ids.push_back(std::stoull(token));
            }
        }
    }
    
    return seq;
}

// Main file reader function
std::vector<Seq> readSeqFile(const std::string& filePath) {
    std::vector<Seq> sequences;
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return sequences;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        try {
            Seq seq = parseSeqFromJson(line);
            sequences.push_back(seq);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << std::endl;
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    
    file.close();
    return sequences;
}

// Helper function to print a Seq object
void printSeq(const Seq& seq) {
    std::cout << "Seq {" << std::endl;
    std::cout << "  timestamp: " << seq.timestamp << std::endl;
    std::cout << "  input_length: " << seq.input_length << std::endl;
    std::cout << "  output_length: " << seq.output_length << std::endl;
    std::cout << "  hash_ids: [";
    for (size_t i = 0; i < seq.hash_ids.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << seq.hash_ids[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "}" << std::endl;
}

void count_token_freq(const std::vector<Seq>& sequences) {
    std::unordered_map<uint64_t, uint64_t> token_freq;
    std::unordered_map<uint64_t, int> token_layer;
    int max_layer = 0;
    for (const auto& seq : sequences) {
        for (int l = 0; l < static_cast<int>(seq.hash_ids.size()); l++) {
            uint64_t token = seq.hash_ids[l];
            token_freq[token]++;
            if (token_layer.find(token) == token_layer.end()) {
                token_layer[token] = l;
            } else if (token_layer[token] != l) {
                std::cout << "token " << token << " is at layer " << l << " but was already at layer " << token_layer[token] << std::endl;
                throw std::runtime_error("token is at multiple layers");
            }
            max_layer = std::max(max_layer, l);
        }
    }
    std::vector<std::map<uint64_t, uint64_t>> token_freq_by_layer(max_layer + 1);
    for (const auto& [token, freq] : token_freq) {
        int layer = token_layer[token];
        token_freq_by_layer[layer][freq]++;
    }
    for (int l = 0; l < max_layer + 1; l++) {
        uint64_t token_count = 0;
        uint64_t freq_sum = 0;
        for (const auto& [freq, count] : token_freq_by_layer[l]) {
            token_count += count;
            freq_sum += freq * count;
        }
        std::cout << "layer " << l << ":";
        std::cout << " (token=" << token_count << ", total=" << freq_sum << ")" << ":";
        std::cout << " (freq, token)=";
        for (const auto& [freq, count] : token_freq_by_layer[l]) {
            std::cout << " (" << freq << ", " << count << ")";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Parse command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    // Check if file path is provided
    if (FLAGS_file_path.empty()) {
        std::cerr << "Error: Please provide a file path using --file_path" << std::endl;
        std::cerr << "Usage: " << argv[0] << " --file_path=<path_to_file>" << std::endl;
        return 1;
    }
    
    // Read and parse the file
    std::vector<Seq> sequences = readSeqFile(FLAGS_file_path);
    count_token_freq(sequences);
    
    return 0;
}
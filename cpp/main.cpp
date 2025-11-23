#include "vector_store.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

// Simple JSON array parser for floats (e.g., "[0.1, 0.2, 0.3]")
std::vector<float> parse_json_array(const std::string& json_str) {
    std::vector<float> result;
    std::string cleaned = json_str;

    // Remove brackets and whitespace
    size_t start = cleaned.find('[');
    size_t end = cleaned.find(']');

    if (start == std::string::npos || end == std::string::npos) {
        throw std::runtime_error("Invalid JSON array format");
    }

    cleaned = cleaned.substr(start + 1, end - start - 1);

    // Parse comma-separated floats
    std::stringstream ss(cleaned);
    std::string token;

    while (std::getline(ss, token, ',')) {
        // Trim whitespace
        size_t first = token.find_first_not_of(" \t\n\r");
        size_t last = token.find_last_not_of(" \t\n\r");

        if (first != std::string::npos && last != std::string::npos) {
            token = token.substr(first, last - first + 1);
            result.push_back(std::stof(token));
        }
    }

    return result;
}

void print_usage() {
    std::cout << "VectorForge - Local Vector Database Engine\n";
    std::cout << "Usage:\n";
    std::cout << "  vectorforge add <content> <embedding_json>\n";
    std::cout << "  vectorforge search <embedding_json>\n";
    std::cout << "\nExamples:\n";
    std::cout << "  vectorforge add \"Hello world\" \"[0.1, 0.2, ...]\"\n";
    std::cout << "  vectorforge search \"[0.1, 0.2, ...]\"\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 3) {
            print_usage();
            return 1;
        }

        std::string command = argv[1];
        std::string db_path = "data/database.bin";

        VectorStore store(db_path);

        if (command == "add") {
            if (argc != 4) {
                std::cerr << "Error: 'add' command requires content and embedding\n";
                print_usage();
                return 1;
            }

            std::string content = argv[2];
            std::string embedding_json = argv[3];

            // Parse embedding
            std::vector<float> embedding = parse_json_array(embedding_json);

            if (embedding.size() != EMBEDDING_DIM) {
                std::cerr << "Error: Embedding must have " << EMBEDDING_DIM
                          << " dimensions, got " << embedding.size() << "\n";
                return 1;
            }

            // Save vector
            int id = store.save_vector(content, embedding.data());

            // Output JSON response
            std::cout << "{\"success\":true,\"id\":" << id
                      << ",\"message\":\"Vector stored successfully\"}\n";

        } else if (command == "search") {
            if (argc != 3) {
                std::cerr << "Error: 'search' command requires embedding\n";
                print_usage();
                return 1;
            }

            std::string embedding_json = argv[2];

            // Parse embedding
            std::vector<float> embedding = parse_json_array(embedding_json);

            if (embedding.size() != EMBEDDING_DIM) {
                std::cerr << "Error: Embedding must have " << EMBEDDING_DIM
                          << " dimensions, got " << embedding.size() << "\n";
                return 1;
            }

            // Search for similar vectors
            std::vector<SearchResult> results = store.search_vector(embedding.data(), 3);

            // Output JSON response
            std::cout << "{\"success\":true,\"results\":[";
            for (size_t i = 0; i < results.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << "{\"id\":" << results[i].id
                          << ",\"content\":\"" << results[i].content << "\""
                          << ",\"score\":" << results[i].score << "}";
            }
            std::cout << "]}\n";

        } else {
            std::cerr << "Error: Unknown command '" << command << "'\n";
            print_usage();
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "{\"success\":false,\"error\":\"" << e.what() << "\"}\n";
        return 1;
    }
}

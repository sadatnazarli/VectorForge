#include "vector_store.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>

VectorStore::VectorStore(const std::string& db_path) : db_path_(db_path) {
    // Create database file if it doesn't exist
    std::ifstream test(db_path_);
    if (!test.good()) {
        std::ofstream create(db_path_, std::ios::binary);
        create.close();
    }
    test.close();
}

int VectorStore::get_next_id() {
    std::ifstream file(db_path_, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 1;
    }

    std::streamsize size = file.tellg();
    file.close();

    if (size == 0) {
        return 1;
    }

    // Calculate number of records
    int num_records = size / sizeof(VectorRecord);
    return num_records + 1;
}

int VectorStore::save_vector(const std::string& content, const float* embedding) {
    int id = get_next_id();

    VectorRecord record;
    record.id = id;

    // Copy embedding
    std::memcpy(record.embedding, embedding, EMBEDDING_DIM * sizeof(float));

    // Copy content (ensure null-termination)
    std::memset(record.content, 0, CONTENT_SIZE);
    std::strncpy(record.content, content.c_str(), CONTENT_SIZE - 1);

    // Append to file
    std::ofstream file(db_path_, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open database file for writing");
    }

    file.write(reinterpret_cast<const char*>(&record), sizeof(VectorRecord));
    file.close();

    return id;
}

std::vector<SearchResult> VectorStore::search_vector(const float* query_embedding, int top_k) {
    std::vector<SearchResult> results;

    std::ifstream file(db_path_, std::ios::binary);
    if (!file.is_open()) {
        return results; // Return empty results if file can't be opened
    }

    VectorRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(VectorRecord))) {
        float similarity = cosine_similarity(query_embedding, record.embedding);

        SearchResult result;
        result.id = record.id;
        result.content = std::string(record.content);
        result.score = similarity;

        results.push_back(result);
    }

    file.close();

    // Sort by score (descending)
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    // Return top k results
    if (results.size() > static_cast<size_t>(top_k)) {
        results.resize(top_k);
    }

    return results;
}

float VectorStore::dot_product(const float* vec1, const float* vec2) {
    float sum = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; ++i) {
        sum += vec1[i] * vec2[i];
    }
    return sum;
}

float VectorStore::magnitude(const float* vec) {
    float sum = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; ++i) {
        sum += vec[i] * vec[i];
    }
    return std::sqrt(sum);
}

float VectorStore::cosine_similarity(const float* vec1, const float* vec2) {
    float dot = dot_product(vec1, vec2);
    float mag1 = magnitude(vec1);
    float mag2 = magnitude(vec2);

    // Avoid division by zero
    if (mag1 == 0.0f || mag2 == 0.0f) {
        return 0.0f;
    }

    return dot / (mag1 * mag2);
}

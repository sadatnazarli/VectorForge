#ifndef VECTOR_STORE_H
#define VECTOR_STORE_H

#include <string>
#include <vector>

const int EMBEDDING_DIM = 1536;
const int CONTENT_SIZE = 1024;

struct VectorRecord {
    int id;
    float embedding[EMBEDDING_DIM];
    char content[CONTENT_SIZE];
};

struct SearchResult {
    int id;
    std::string content;
    float score;
};

class VectorStore {
public:
    VectorStore(const std::string& db_path);

    // Save a new vector record to the database
    int save_vector(const std::string& content, const float* embedding);

    // Search for similar vectors, returns top k results
    std::vector<SearchResult> search_vector(const float* query_embedding, int top_k = 3);

    // Get the next available ID
    int get_next_id();

private:
    std::string db_path_;

    // Calculate cosine similarity between two vectors
    float cosine_similarity(const float* vec1, const float* vec2);

    // Calculate magnitude of a vector
    float magnitude(const float* vec);

    // Calculate dot product of two vectors
    float dot_product(const float* vec1, const float* vec2);
};

#endif // VECTOR_STORE_H

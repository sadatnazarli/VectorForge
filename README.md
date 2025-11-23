# VectorForge

A high-performance, local vector database engine written in C++ with a TypeScript Model Context Protocol (MCP) server interface. VectorForge enables AI assistants like Claude to store and retrieve memories locally on your machine.

## Overview

VectorForge is a custom-built RAG (Retrieval-Augmented Generation) system consisting of:

- **C++ Core Engine**: High-performance vector storage and retrieval with cosine similarity search
- **TypeScript MCP Server**: Clean interface for AI assistants via the Model Context Protocol
- **Binary Storage**: Efficient flat-file database storing 1536-dimensional vectors (OpenAI embedding compatible)

## Features

- Zero external dependencies for the C++ core (Standard Library only)
- Fast binary file I/O operations
- Cosine similarity search with configurable top-k results
- Simple JSON-based communication protocol
- MCP-compliant server for easy integration with Claude and other AI assistants
- Deterministic mock embeddings for testing (can be replaced with real embedding models)

## Architecture

```
┌─────────────────┐
│  Claude / AI    │
└────────┬────────┘
         │ MCP Protocol
┌────────▼────────┐
│  TypeScript     │
│  MCP Server     │
│  (Node.js)      │
└────────┬────────┘
         │ Process Execution
┌────────▼────────┐
│  C++ Engine     │
│  vectorforge    │
└────────┬────────┘
         │ Binary I/O
┌────────▼────────┐
│  database.bin   │
│  (Local File)   │
└─────────────────┘
```

## Project Structure

```
VectorForge/
├── cpp/                    # C++ core engine
│   ├── vector_store.h     # Vector storage interface
│   ├── vector_store.cpp   # Implementation
│   └── main.cpp           # CLI wrapper
├── server/                # TypeScript MCP server
│   ├── src/
│   │   └── index.ts       # MCP server implementation
│   ├── dist/              # Compiled JavaScript
│   ├── package.json
│   └── tsconfig.json
├── build/                 # Compiled binaries
│   └── vectorforge        # C++ executable
├── data/                  # Database storage
│   └── database.bin       # Vector database file
├── scripts/               # Helper scripts
│   └── generate_test_vector.py
├── Makefile              # Build automation
└── README.md
```

## Prerequisites

- **C++ Compiler**: g++ with C++17 support
- **Node.js**: Version 18 or higher
- **npm**: Node package manager
- **Python 3**: (optional) For test scripts

## Installation

### 1. Clone the Repository

```bash
git clone <repository-url>
cd VectorForge
```

### 2. Build Everything

```bash
make all
```

This will:
- Compile the C++ engine
- Install NPM dependencies
- Build the TypeScript server

### Alternative: Build Components Separately

```bash
# Build C++ engine only
make cpp

# Install and build TypeScript server only
make server
```

## Usage

### Direct C++ CLI Usage

The C++ binary can be used directly from the command line:

```bash
# Add a vector to the database
./build/vectorforge add "Your text content" "[0.1, 0.2, ..., 1536 values]"

# Search for similar vectors
./build/vectorforge search "[0.1, 0.2, ..., 1536 values]"
```

### MCP Server Usage

The primary way to use VectorForge is through the MCP server:

#### 1. Configure Claude Desktop

Add the following to your Claude Desktop MCP configuration file:

**macOS/Linux**: `~/Library/Application Support/Claude/claude_desktop_config.json`

**Windows**: `%APPDATA%/Claude/claude_desktop_config.json`

```json
{
  "mcpServers": {
    "vectorforge": {
      "command": "node",
      "args": ["/absolute/path/to/VectorForge/server/dist/index.js"]
    }
  }
}
```

Replace `/absolute/path/to/VectorForge` with your actual project path.

#### 2. Restart Claude Desktop

After updating the configuration, restart Claude Desktop to load the MCP server.

#### 3. Use the Tools in Claude

You can now use these tools in your conversations with Claude:

**store_memory**: Store a text memory in the local database
```
Claude, please store this memory: "I prefer TypeScript for backend development"
```

**recall_memory**: Search for similar memories
```
Claude, recall memories about my programming preferences
```

## MCP Tools

### store_memory

Stores a text memory in the local vector database.

**Parameters:**
- `text` (string): The text content to store

**Example:**
```json
{
  "text": "I had a great meeting with the design team about the new UI"
}
```

### recall_memory

Searches for similar memories based on a query.

**Parameters:**
- `query` (string): The search query

**Returns:** Top 3 most similar memories with similarity scores

**Example:**
```json
{
  "query": "design meetings"
}
```

## Testing

### Run C++ Tests

```bash
make test
```

This will:
1. Compile the C++ engine
2. Add test vectors to the database
3. Perform similarity searches
4. Verify the output

### Clean Build

```bash
make clean
```

This removes all build artifacts and the database file.

## Development

### Rebuilding After Changes

```bash
# Rebuild C++ engine
make cpp

# Rebuild TypeScript server
cd server && npm run build
```

### Development Mode (TypeScript)

```bash
cd server
npm run dev  # Watch mode - rebuilds on file changes
```

## How It Works

### Vector Storage

VectorForge uses a simple binary file format:

```c
struct VectorRecord {
    int id;                    // Unique identifier
    float embedding[1536];     // Vector embedding
    char content[1024];        // Original text content
};
```

Records are appended to `data/database.bin` sequentially.

### Similarity Search

The search algorithm:
1. Reads all records from the binary file
2. Computes cosine similarity between query vector and each stored vector
3. Sorts results by similarity score (descending)
4. Returns top k matches

**Cosine Similarity Formula:**
```
similarity = (A · B) / (||A|| × ||B||)
```

Where:
- `A · B` is the dot product
- `||A||` and `||B||` are vector magnitudes

### Mock Embeddings

The current implementation uses deterministic pseudo-random embeddings for testing. For production use, replace `generateMockEmbedding()` in `server/src/index.ts` with a real embedding model like:

- OpenAI's text-embedding-3-small
- Sentence Transformers (via `@xenova/transformers`)
- Cohere embeddings
- Any other 1536-dimensional embedding model

## Performance Characteristics

- **Write Speed**: O(1) - Simple append operation
- **Search Speed**: O(n) - Linear scan through all vectors
- **Storage**: ~6.2 KB per record (4 + 6144 + 1024 bytes)
- **Dimensions**: Fixed at 1536 (configurable in source)

### Optimization Ideas for Large Datasets

For production use with large datasets, consider:
- Implement indexing (e.g., HNSW, IVF)
- Add batch operations
- Use memory mapping for large files
- Implement approximate nearest neighbor search
- Add compression for embeddings

## Troubleshooting

### C++ Compilation Errors

Ensure you have g++ with C++17 support:
```bash
g++ --version
```

### MCP Server Not Appearing in Claude

1. Check the config file path is correct
2. Ensure the absolute path to `index.js` is correct
3. Restart Claude Desktop
4. Check Claude's developer console for errors

### Permission Errors

Ensure the data directory is writable:
```bash
chmod -R 755 data/
```

## Security Considerations

- VectorForge stores data in plaintext binary format
- No encryption is implemented by default
- The database file is stored locally on your machine
- Be cautious about what sensitive information you store

## License

MIT

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Future Enhancements

- [ ] Real embedding model integration
- [ ] HNSW indexing for faster search
- [ ] Metadata filtering
- [ ] Database versioning and migrations
- [ ] Backup and restore functionality
- [ ] Multi-collection support
- [ ] REST API interface
- [ ] Vector dimension flexibility
- [ ] Compression and optimization

## Acknowledgments

Built with:
- C++17 Standard Library
- Model Context Protocol SDK
- Node.js and TypeScript
- Zod for validation

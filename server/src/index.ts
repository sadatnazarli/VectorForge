#!/usr/bin/env node

import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  Tool,
} from "@modelcontextprotocol/sdk/types.js";
import { execFile } from "child_process";
import { promisify } from "util";
import { fileURLToPath } from "url";
import { dirname, join } from "path";
import { z } from "zod";

const execFileAsync = promisify(execFile);

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Path to the C++ executable (relative to the server directory)
const VECTORFORGE_BIN = join(__dirname, "../../build/vectorforge");

// Generate a mock embedding vector (1536 dimensions)
// In a production system, you would use a real embedding model
function generateMockEmbedding(text: string): number[] {
  const embedding: number[] = [];

  // Use text as seed for deterministic "embeddings"
  let seed = 0;
  for (let i = 0; i < text.length; i++) {
    seed += text.charCodeAt(i);
  }

  // Generate 1536 pseudo-random numbers based on the seed
  for (let i = 0; i < 1536; i++) {
    // Simple pseudo-random number generator
    seed = (seed * 9301 + 49297) % 233280;
    embedding.push(seed / 233280);
  }

  return embedding;
}

// Execute the VectorForge C++ binary
async function executeVectorForge(
  command: string,
  args: string[]
): Promise<any> {
  try {
    // Set working directory to VectorForge root
    const { stdout, stderr } = await execFileAsync(
      VECTORFORGE_BIN,
      [command, ...args],
      {
        cwd: join(__dirname, "../../"),
      }
    );

    if (stderr) {
      console.error("VectorForge stderr:", stderr);
    }

    // Parse JSON response from C++ binary
    return JSON.parse(stdout);
  } catch (error: any) {
    console.error("VectorForge execution error:", error);
    throw new Error(
      `Failed to execute VectorForge: ${error.message || error}`
    );
  }
}

// Define the MCP server
const server = new Server(
  {
    name: "vectorforge",
    version: "1.0.0",
  },
  {
    capabilities: {
      tools: {},
    },
  }
);

// Define available tools
const tools: Tool[] = [
  {
    name: "store_memory",
    description:
      "Store a text memory in the local vector database. The text will be embedded and stored for later retrieval.",
    inputSchema: {
      type: "object",
      properties: {
        text: {
          type: "string",
          description: "The text content to store as a memory",
        },
      },
      required: ["text"],
    },
  },
  {
    name: "recall_memory",
    description:
      "Search for similar memories in the vector database based on a query text. Returns the top 3 most similar memories.",
    inputSchema: {
      type: "object",
      properties: {
        query: {
          type: "string",
          description: "The query text to search for similar memories",
        },
      },
      required: ["query"],
    },
  },
];

// Handle list_tools request
server.setRequestHandler(ListToolsRequestSchema, async () => {
  return { tools };
});

// Handle call_tool request
server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    if (name === "store_memory") {
      const { text } = z.object({ text: z.string() }).parse(args);

      // Generate embedding for the text
      const embedding = generateMockEmbedding(text);
      const embeddingJson = JSON.stringify(embedding);

      // Call C++ binary to store the vector
      const result = await executeVectorForge("add", [text, embeddingJson]);

      return {
        content: [
          {
            type: "text",
            text: `✓ Memory stored successfully!\n\nID: ${result.id}\nContent: "${text}"\n\nThe memory has been embedded and stored in the local vector database.`,
          },
        ],
      };
    } else if (name === "recall_memory") {
      const { query } = z.object({ query: z.string() }).parse(args);

      // Generate embedding for the query
      const embedding = generateMockEmbedding(query);
      const embeddingJson = JSON.stringify(embedding);

      // Call C++ binary to search for similar vectors
      const result = await executeVectorForge("search", [embeddingJson]);

      if (!result.results || result.results.length === 0) {
        return {
          content: [
            {
              type: "text",
              text: "No memories found in the database.",
            },
          ],
        };
      }

      // Format the results
      let responseText = `Found ${result.results.length} similar memories:\n\n`;

      result.results.forEach((memory: any, index: number) => {
        responseText += `${index + 1}. [ID: ${memory.id}] (Similarity: ${(
          memory.score * 100
        ).toFixed(1)}%)\n`;
        responseText += `   "${memory.content}"\n\n`;
      });

      return {
        content: [
          {
            type: "text",
            text: responseText,
          },
        ],
      };
    } else {
      throw new Error(`Unknown tool: ${name}`);
    }
  } catch (error: any) {
    return {
      content: [
        {
          type: "text",
          text: `Error: ${error.message || error}`,
        },
      ],
      isError: true,
    };
  }
});

// Start the server
async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error("VectorForge MCP Server running on stdio");
}

main().catch((error) => {
  console.error("Fatal error in main():", error);
  process.exit(1);
});

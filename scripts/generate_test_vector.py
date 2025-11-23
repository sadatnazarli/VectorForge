#!/usr/bin/env python3
import json
import random

# Generate a 1536-dimensional test vector
vector = [random.random() for _ in range(1536)]
print(json.dumps(vector))

#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Building KV-Store Database${NC}"
echo -e "${BLUE}========================================${NC}"

# Clean previous build
make clean > /dev/null 2>&1

# Compile main
if make main; then
    echo -e "${GREEN}✓ Build successful${NC}\n"
else
    echo -e "${YELLOW}✗ Build failed${NC}"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Starting Interactive Mode${NC}"
echo -e "${BLUE}========================================${NC}\n"

# Run main in interactive mode
./main

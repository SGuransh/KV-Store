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

# Clean up any previous test database
rm -rf test_lsm_db

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Running LSM Database Demo${NC}"
echo -e "${BLUE}========================================${NC}\n"

# Run the demo with test inputs
cat << 'EOF' | ./main
open test_lsm_db
insert 1 100
insert 2 200
insert 3 300
insert 4 400
insert 5 500
insert 6 600
insert 7 700
insert 8 800
insert 9 900
insert 10 1000
lsm
insert 11 1100
insert 12 1200
insert 13 1300
insert 14 1400
insert 15 1500
insert 16 1600
insert 17 1700
insert 18 1800
insert 19 1900
insert 20 2000
lsm
insert 21 2100
insert 22 2200
insert 23 2300
insert 24 2400
insert 25 2500
insert 26 2600
insert 27 2700
insert 28 2800
insert 29 2900
insert 30 3000
lsm
scan 1 30
search 5
search 15
search 25
status
close
exit
EOF

echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}  Checking Generated Files${NC}"
echo -e "${BLUE}========================================${NC}"

if [ -d "test_lsm_db" ]; then
    echo -e "${GREEN}Database directory created:${NC}"
    ls -lh test_lsm_db/
    
    if [ -f "test_lsm_db/manifest.txt" ]; then
        echo -e "\n${GREEN}Manifest file contents:${NC}"
        cat test_lsm_db/manifest.txt
    fi
fi

echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}  Demo Complete${NC}"
echo -e "${BLUE}========================================${NC}"

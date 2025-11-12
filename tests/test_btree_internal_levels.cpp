#include "../BTree/BTreeSST.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

void configure_context(BuildContext& ctx, size_t leafCount) {
    ctx.leafNodeCount = leafCount;

    std::vector<size_t> levelSizes;
    size_t currentLevelSize = leafCount;
    size_t totalInternalNodes = 0;

    while (currentLevelSize > 1) {
        currentLevelSize = (currentLevelSize + MAX_INTERNAL_CHILDREN - 1) / MAX_INTERNAL_CHILDREN;
        levelSizes.push_back(currentLevelSize);
        totalInternalNodes += currentLevelSize;
    }

    ctx.internalLevelCount = levelSizes.size();
    ctx.totalInternalNodes = totalInternalNodes;

    if (ctx.internalLevelCount > 0) {
        ctx.internalLevelSizes = new size_t[ctx.internalLevelCount];
        for (size_t i = 0; i < ctx.internalLevelCount; ++i) {
            ctx.internalLevelSizes[i] = levelSizes[i];
        }
    }
}

std::vector<int32_t> compute_expected_structure(const BuildContext& ctx, const std::vector<int32_t>& leafMaxima) {
    std::vector<int32_t> expected(ctx.totalInternalNodes * MAX_INTERNAL_KEYS, 0);
    std::vector<int32_t> current = leafMaxima;
    
    // Build all levels bottom-up and store them
    std::vector<std::vector<int32_t>> allLevels;
    
    for (size_t levelIdx = 0; levelIdx < ctx.internalLevelCount; ++levelIdx) {
        size_t nodesInLevel = ctx.internalLevelSizes[levelIdx];
        std::vector<int32_t> levelData(nodesInLevel * MAX_INTERNAL_KEYS, 0);
        std::vector<int32_t> nextLevel;
        size_t srcIdx = 0;

        for (size_t nodeIdx = 0; nodeIdx < nodesInLevel && srcIdx < current.size(); ++nodeIdx) {
            size_t nodeOffset = nodeIdx * MAX_INTERNAL_KEYS;
            size_t remaining = current.size() - srcIdx;
            size_t keysForNode = 0;

            if (remaining >= MAX_INTERNAL_CHILDREN) {
                keysForNode = MAX_INTERNAL_CHILDREN - 1;
                for (size_t k = 0; k < keysForNode; ++k) {
                    levelData[nodeOffset + k] = current[srcIdx + k];
                }
                nextLevel.push_back(current[srcIdx + keysForNode]);
                srcIdx += MAX_INTERNAL_CHILDREN;
            } else {
                keysForNode = (remaining > 0) ? remaining - 1 : 0;
                for (size_t k = 0; k < keysForNode; ++k) {
                    levelData[nodeOffset + k] = current[srcIdx + k];
                }
                if (remaining > 0) {
                    nextLevel.push_back(current[srcIdx + keysForNode]);
                }
                srcIdx = current.size();
            }
        }

        allLevels.push_back(levelData);
        current = nextLevel;
    }
    
    // Now write levels in reverse order (root first) to match implementation
    size_t writeOffset = 0;
    for (int levelIdx = ctx.internalLevelCount - 1; levelIdx >= 0; --levelIdx) {
        const auto& levelData = allLevels[levelIdx];
        for (size_t i = 0; i < levelData.size(); ++i) {
            expected[writeOffset + i] = levelData[i];
        }
        writeOffset += levelData.size();
    }

    return expected;
}

std::vector<int32_t> read_internal_nodes(int fd, size_t totalNodes) {
    std::vector<int32_t> buffer(totalNodes * MAX_INTERNAL_KEYS, 0);
    if (buffer.empty()) {
        return buffer;
    }

    const size_t bytesToRead = buffer.size() * sizeof(int32_t);
    ssize_t bytesRead = pread(fd, buffer.data(), bytesToRead, Page::PAGE_SIZE);

    if (bytesRead != static_cast<ssize_t>(bytesToRead)) {
        buffer.clear();
    }

    return buffer;
}

static void print_btree(const BuildContext& ctx, const std::vector<int32_t>& internal, const std::vector<int32_t>& leafMaxima) {
    std::cout << "Found B-tree structure:\n";

    if (ctx.internalLevelCount == 0) {
        std::cout << "  (No internal levels) Leaves: ";
        for (size_t i = 0; i < leafMaxima.size(); ++i) {
            std::cout << leafMaxima[i] << (i + 1 < leafMaxima.size() ? " " : "");
        }
        std::cout << "\n";
        return;
    }

    // Internal nodes are stored root-first in memory
    // So we need to read them in reverse level order
    size_t nodeIndex = 0;
    for (int level = ctx.internalLevelCount - 1; level >= 0; --level) {
        size_t nodesInLevel = ctx.internalLevelSizes[level];
        std::cout << "  Level " << level << " (nodes=" << nodesInLevel << "):\n    ";
        for (size_t n = 0; n < nodesInLevel; ++n) {
            std::cout << "[";
            size_t base = (nodeIndex + n) * MAX_INTERNAL_KEYS;
            bool first = true;
            for (size_t k = 0; k < MAX_INTERNAL_KEYS; ++k) {
                int32_t v = 0;
                if (base + k < internal.size()) v = internal[base + k];
                if (!first) std::cout << ",";
                // Print empty slots as '_' for clarity
                if (v == 0) {
                    std::cout << "_";
                } else {
                    std::cout << v;
                }
                first = false;
            }
            std::cout << "]";
            if (n + 1 < nodesInLevel) std::cout << " ";
        }
        std::cout << "\n";
        nodeIndex += nodesInLevel;
    }

    // Print leaf maxima as final level
    std::cout << "  Leaves (" << leafMaxima.size() << "):\n    ";
    for (size_t i = 0; i < leafMaxima.size(); ++i) {
        std::cout << leafMaxima[i];
        if (i + 1 < leafMaxima.size()) std::cout << " ";
    }
    std::cout << "\n";
}

bool run_test_case(const std::string& name, const std::vector<int32_t>& leafMaxima) {
    char tmpTemplate[] = "/tmp/btree_internal_levelsXXXXXX";
    int fd = mkstemp(tmpTemplate);
    if (fd < 0) {
        std::perror("mkstemp failed");
        return false;
    }

    BuildContext ctx(tmpTemplate);
    ctx.fd = fd;

    configure_context(ctx, leafMaxima.size());

    if (ctx.internalLevelCount == 0) {
        std::cerr << "[" << name << "] Skipping: no internal levels for leaf count " << leafMaxima.size() << std::endl;
        ctx.fd = -1;
        close(fd);
        unlink(tmpTemplate);
        return false;
    }

    BTreeSST sst;
    sst.buildInternalLevels(ctx, leafMaxima.data());

    std::vector<int32_t> actual = read_internal_nodes(fd, ctx.totalInternalNodes);
    std::vector<int32_t> expected = compute_expected_structure(ctx, leafMaxima);

    std::cout << "\n[" << name << "]" << std::endl;
    print_btree(ctx, actual, leafMaxima);

    bool passed = (actual == expected);

    if (!passed) {
        std::cerr << "[" << name << "] FAILED" << std::endl;
        if (actual.empty()) {
            std::cerr << "  No internal node data was written to disk." << std::endl;
        } else {
            std::cerr << "  Expected structure:\n";
            print_btree(ctx, expected, leafMaxima);
            std::cerr << "  Expected: ";
            for (int32_t value : expected) {
                std::cerr << value << ' ';
            }
            std::cerr << "\n  Actual:   ";
            for (int32_t value : actual) {
                std::cerr << value << ' ';
            }
            std::cerr << std::endl;
        }
    } else {
        std::cout << "[" << name << "] PASSED" << std::endl;
    }

    ctx.fd = -1; // Prevent double-close in BuildContext cleanup
    close(fd);
    unlink(tmpTemplate);

    return passed;
}

} // namespace

int main() {
    bool allPassed = true;

    allPassed &= run_test_case(
        "Docstring Dry Run",
        {3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60}
    );

    allPassed &= run_test_case(
        "Exact Branching",
        {5, 10, 15, 20}
    );

    allPassed &= run_test_case(
        "Partial Final Node",
        {2, 4, 6, 8, 10}
    );

    if (allPassed) {
        std::cout << "All buildInternalLevels tests passed." << std::endl;
        return 0;
    }

    std::cerr << "buildInternalLevels tests failed." << std::endl;
    return 1;
}

max_per_node = [3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60]

MAX_INTERNAL_KEYS = 3

class Context:
    def __init__(self):
        self.leafNodeCount = 20
        self.internalLevelCount = 3
        self.totalInternalNodes = 8
        self.internalLevelSizes = [1, 2, 5]

ctx = Context()

print("Building internal levels bottom-up...")

# Step 1: Maintain a dynamic array of max-values to be processed
currentLevel = list(max_per_node[:ctx.leafNodeCount])

# Step 3: Allocate contiguous array for all internal nodes
allInternalNodes = [0] * (ctx.totalInternalNodes * MAX_INTERNAL_KEYS)

# Step 4: Build each internal level bottom-up (reverse iteration)
writeOffset = sum(ctx.internalLevelSizes[:-1])  # Offset in allInternalNodes array

for levelIdx in range(ctx.internalLevelCount - 1, -1, -1):
    # Reverse iteration
    numNodesInLevel = ctx.internalLevelSizes[levelIdx]  # 5
    nextLevel = []
    
    print(f"  Building internal level {levelIdx} ({numNodesInLevel} nodes)")
    
    # Step 2: Extract every Bth value for next level, keep B-1 values for current level
    currentLevelSize = len(currentLevel)  # 20
    srcIdx = 0
    
    for nodeIdx in range(numNodesInLevel):
        keysInThisNode = MAX_INTERNAL_KEYS
        
        # Check if this is the last node in this level
        if nodeIdx == numNodesInLevel - 1:
            # Last node - might have fewer keys
            # Already have this value in ctx.lastNodeKeys
            remainingChildren = currentLevelSize - srcIdx
            if remainingChildren > 0:
                keysInThisNode = remainingChildren - 1  # X - 1 keys for X children
            else:
                keysInThisNode = 0
        
        # Copy keys for this internal node
        for keyIdx in range(keysInThisNode):
            if srcIdx < currentLevelSize:
                # Change this offset
                allInternalNodes[writeOffset * MAX_INTERNAL_KEYS + keyIdx] = currentLevel[srcIdx]
                srcIdx += 1
        
        # Extract the Bth value (largest key) for next level
        if srcIdx < currentLevelSize:
            nextLevel.append(currentLevel[srcIdx])
            srcIdx += 1
        
        writeOffset += 1
    
    # Move to next level
    currentLevel = nextLevel
    writeOffset -= ctx.internalLevelSizes[levelIdx - 1]
    writeOffset -= ctx.internalLevelSizes[levelIdx]


print("Finished building internal levels.")
print(allInternalNodes)
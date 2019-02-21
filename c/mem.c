#include <xeroskernel.h>
#include <i386.h>

/**
 * A struct that contains the metadata for an allocated region of memory (or free space).
 */
typedef struct MemoryHeader {
    unsigned long dataSize;
    struct MemoryHeader *prev;
    struct MemoryHeader *next;
    const char *sanityCheck;
    unsigned char dataStart[0];
} MemoryHeader;

extern long freemem;        /* start of free memory */
extern char *maxaddr;        /* end of memory address range */

static const char* sanityValue = "allocated!#@";
static size_t paragraphSize = 16;
static MemoryHeader *freeMemoryList = NULL;

/**
 * Checks if the memory address given by {@param address} is aligned to 16 bytes.
 * @return 1 if true, otherwise 0
 */
static int isaligned(long address) {
    return address % paragraphSize == 0 ? 1 : 0;
}

/**
 * Aligns an address to a 16 byte paragraph boundary.
 * @param address
 * @return the next paragraph boundary (can be the same address if the address is already aligned).
 */
static long align(long address) {
    return isaligned(address) == 0 ? (address & ~0xF) : address;
}

/**
 * Removes a chunk from the free list.
 * @param chunk The chunk to remove
 */
static void removeChunk(MemoryHeader* chunk) {
    if (!chunk) return;
    if (chunk == freeMemoryList) freeMemoryList = chunk->next;
    if (chunk->prev) chunk->prev->next = chunk->next;
    if (chunk->next) chunk->next->prev = chunk->prev;
}

/**
 * Shaves off the given chunk, either replacing it with a smaller chunk if there is room left, or removing it from the free list.
 * @param chunk The chunk to split
 * @param amount The amount of bytes to remove from the chunk
 */
static void splitChunk(MemoryHeader* chunk, size_t amount) {
    if (chunk->dataSize == amount) {
        removeChunk(chunk);
    } else {
        MemoryHeader* remainingBlock = ((MemoryHeader*) chunk->dataStart) + (amount / paragraphSize);
        *remainingBlock = (MemoryHeader) {
            chunk->dataSize - sizeof(MemoryHeader) - amount,
            chunk->prev,
            chunk->next,
            NULL
        };

        if (chunk == freeMemoryList) freeMemoryList = remainingBlock;
        if (chunk->prev) chunk->prev->next = remainingBlock;
        if (chunk->next) chunk->next->prev = remainingBlock;
    }
}

void kmeminit(void) {
    long freememAligned = align(freemem);
    freeMemoryList = (MemoryHeader*) freememAligned;
    MemoryHeader* afterHole = (MemoryHeader*) align(HOLEEND);
    *afterHole = (MemoryHeader) {
        (unsigned long) maxaddr - HOLEEND - paragraphSize,
        freeMemoryList,
        NULL,
        0
    };
    *freeMemoryList = (MemoryHeader) {
        (unsigned long) HOLESTART - freemem - paragraphSize,
        NULL,
        afterHole,
        0
    };
}

/**
 * Returns the data size required for the requested number of bytes.
 * @param requestedSize The number of bytes that was requested by the user
 */
static size_t getRequiredSize(size_t requestedSize) {
    size_t amount = (requestedSize) / 16 + ((requestedSize % 16) ? 1 : 0);
    return amount * 16;
}

/**
 * Searches the free memory list for a chunk with enough data.
 *
 */
static MemoryHeader* findFreeChunk(size_t dataSize) {
    MemoryHeader *curr = freeMemoryList;

    while (curr) {
        if (curr->dataSize >= dataSize) return curr;
        curr = curr->next;
    }

    return NULL; // No chunk with enough size found.
}

void *kmalloc(size_t size) {
    size_t amount = getRequiredSize(size);
    MemoryHeader* chunk = findFreeChunk(amount);

    if (!chunk) {
        kprintf("No chunk large enough was found.\n");
        return NULL;
    }

    splitChunk(chunk, amount);
    chunk->dataSize = amount;
    chunk->sanityCheck = sanityValue;
    return chunk->dataStart;
}

static MemoryHeader* coalesceChunk(MemoryHeader* newBlock) {
    MemoryHeader* curr = freeMemoryList;
    void* newBlockEnd = ((void*) newBlock) + newBlock->dataSize + paragraphSize;

    // Search for adjacent chunks.
    MemoryHeader* before = NULL;
    MemoryHeader* after = NULL;
    while (curr) {
        void* currEnd = ((void*) curr) + curr->dataSize + paragraphSize;
        if (curr == newBlockEnd) after = curr;
        if (currEnd == newBlock) before = curr;
        curr = curr->next;
    }

    removeChunk(before);
    removeChunk(after);

    if (before) {
        before->dataSize += newBlock->dataSize + paragraphSize;
        newBlock = before;
    }

    if (after) newBlock->dataSize += after->dataSize + paragraphSize;

    return newBlock;
}

int kfree(void *ptr) {
    MemoryHeader* newBlock = ptr;
    newBlock--;

    if (newBlock->sanityCheck != sanityValue) {
        kprintf("Sanity check failed!\nShould be '%s' but was '%s'.\n", sanityValue, newBlock->sanityCheck);
        return 0;
    }

    newBlock->sanityCheck = NULL;
    newBlock = coalesceChunk(newBlock);

    newBlock->prev = NULL;
    newBlock->next = freeMemoryList;
    if (freeMemoryList) freeMemoryList->prev = newBlock;
    freeMemoryList = newBlock;
    return 1;
}

static void printMemoryHeader(MemoryHeader* h) {
    kprintf("0x%08x: { Size: 0x%x, Prev: 0x%08x, Next: 0x%08x, Sanity: %d }\n", h, h->dataSize, h->prev, h->next, h->sanityCheck);
}

void printCurrentFreeList(void) {
    kprintf("Free Memory List:\n");

    MemoryHeader* curr = freeMemoryList;
    size_t freeChunkCount = 0;
    while (curr) {
        printMemoryHeader(curr);
        freeChunkCount++;
        curr = curr->next;
    }

    kprintf("%d free chunks.\n", freeChunkCount);
}

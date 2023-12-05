#pragma once

// Note: Unused

#include <windows.h>

#define BLOCK_SIZE 1024

typedef struct Block
{
    Block *next;
    size_t size;
    size_t cap;
    char mem[];
} Block;

typedef struct Arena
{
    Block *start, *end;
    size_t numBlocks;
} Arena;

#ifndef ARENA
#define ARENA

Arena *arenaCreate(size_t size)
{

}

void arenaExtend(Arena *arena, size_t numBlocks)
{

}

void arenaRemove(Arena *arena, Block *block)
{

}

void arenaFree(Arena *arena)
{
    Block *block = arena->start;
    while (block != NULL)
    {
        Block *prev = block;
        block = block->next;
        free(prev);
    }

    free(arena);
}

Block *blockAlloc(Arena *arena)
{
    size_t size = sizeof(Block) + BLOCK_SIZE;
    HANDLE heap = GetProcessHeap();
    Block* block = (Block*)HeapAlloc(heap, HEAP_ZERO_MEMORY, size);

    block->cap = BLOCK_SIZE;
    block->next = NULL;
    block->size = 0;

    return block;
}

void blockFree(Arena *arena, Block *block)
{
    arena->numBlocks--;
    free(block);
}

#endif
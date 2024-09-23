#pragma once

#include <stdio.h>

template <int SIZE>
class LList
{
        int next[SIZE];
        int prev[SIZE];
        int head;
        int tail;
        int firstFree;

        void Reset();
        int count;

    public:

        LList() { Reset(); }

        int AddPos();
        bool RemoveAt(int pos);
        inline int Head() { return head; }
        inline int Tail() { return tail; }
        inline int Next(int pos) { return next[pos]; }
        inline int Prev(int pos) { return prev[pos]; }
        bool ValidItemAtPos(int pos);
};

template <int SIZE>
void LList<SIZE>::Reset()
{
    for (int i = 1; i < SIZE; i++)
    {
        next[i - 1] = i;
        prev[i] = -1;
    }

    prev[0] = -1;
    firstFree = 0;
    head = tail = -1;
    count = 0;
}

template <int SIZE>
int LList<SIZE>::AddPos()
{
    if (count == SIZE)
        return -1;

    int pos = firstFree;
    firstFree = next[firstFree];

    prev[pos] = tail;
    if (tail != -1) next[tail] = pos;
    tail = pos;
    if (head == -1) head = pos;
    next[pos] = -1;

    count++;

    return pos;
}

template <int SIZE>
bool LList<SIZE>::ValidItemAtPos(int pos)
{
    if (pos < 0 || pos >= SIZE)
        return false;
    if (pos != head && prev[pos] == -1)
        return false;

    return true;
}

template <int SIZE>
bool LList<SIZE>::RemoveAt(int pos)
{
    if (count == 0)
        return false;

    if (!ValidItemAtPos(pos))
        return false;

    if (prev[pos] == -1) head = next[pos];
    else next[prev[pos]] = next[pos];

    if (next[pos] == -1) tail = prev[pos];
    else prev[next[pos]] = prev[pos];

    prev[pos] = -1;
    next[pos] = firstFree;
    firstFree = pos;

    count--;

    return true;
}

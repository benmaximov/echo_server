#include <gtest/gtest.h>
#include "llist_safe.h"


TEST(LList, Support1) {
    LList<100> list;
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(list.AddPos(), i);
    EXPECT_EQ(list.AddPos(), -1);
    EXPECT_EQ(list.Count(), 100);
    EXPECT_EQ(list.Head(), 0);
    EXPECT_EQ(list.Tail(), 99);
    for (int i = 1; i < 100; i++)
    {
        EXPECT_EQ(list.Next(i - 1), i);
        EXPECT_EQ(list.Prev(i), i - 1);
    }
}

TEST(LList, Support2) {
    LList<100> list;
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(list.AddPos(), i);
    EXPECT_EQ(list.RemoveAt(0), true);
    EXPECT_EQ(list.RemoveAt(99), true);
    EXPECT_EQ(list.RemoveAt(50), true);
    EXPECT_EQ(list.Count(), 97);
    EXPECT_EQ(list.Head(), 1);
    EXPECT_EQ(list.Tail(), 98);
    EXPECT_EQ(list.Next(49), 51);
    EXPECT_EQ(list.Prev(51), 49);
    EXPECT_EQ(list.AddPos(), 50);
    EXPECT_EQ(list.Head(), 1);
    EXPECT_EQ(list.Tail(), 50);
    EXPECT_EQ(list.Next(50), -1);
    EXPECT_EQ(list.Prev(50), 98);
    EXPECT_EQ(list.Count(), 98);
    EXPECT_EQ(list.RemoveAt(0), false);
    EXPECT_EQ(list.RemoveAt(99), false);
}

void* runner(void* arg) {
    LList<10000>* llist = (LList<10000>*)arg;
    for (int i = 0; i < 1000; i++)
    {
        usleep(100);
        llist->AddPos();
    }
    return NULL;
}

TEST(LList, ThreadSafetyTest) {
    LList<10000> llist;

    pthread_t thread[10];
    for (int i = 0; i < 10; i++)
        pthread_create(&thread[i], NULL, runner, &llist);

    void *retVal;
    for (int i = 0; i < 10; i++)
        pthread_join(thread[i], &retVal);

    EXPECT_EQ(llist.Count(), 1000);
}
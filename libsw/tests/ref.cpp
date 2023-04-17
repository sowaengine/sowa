#include "sw/ref.hpp"
#include <gtest/gtest.h>

TEST(RefTest, works) {
    sw::ref<int> v;
    EXPECT_EQ(v.count(), 0);
    EXPECT_EQ(v.get(), nullptr);
}

TEST(RefTest, make) {
    sw::ref<int> v = sw::ref<int>::make(12);
    EXPECT_EQ(v.count(), 1);
    EXPECT_NE(v.get(), nullptr);
    EXPECT_EQ(*v.get(), 12);

    v = sw::ref<int>::make();
    EXPECT_EQ(v.count(), 1);
    EXPECT_NE(v.get(), nullptr);
}

TEST(RefTest, share) {
    sw::ref<int> v1 = sw::ref<int>::make();
    sw::ref<int> v2 = v1;

    EXPECT_EQ(v1.count(), 2);
    EXPECT_EQ(v2.count(), 2);
    EXPECT_EQ(v1.get(), v2.get());
}

TEST(RefTest, nullAssignment) {
    sw::ref<int> v1;
    sw::ref<int> v2;
    sw::ref<int> v3;
    sw::ref<int> v4;
    EXPECT_EQ(v1.count(), 0);
    EXPECT_EQ(v2.count(), 0);
    EXPECT_EQ(v3.count(), 0);
    EXPECT_EQ(v4.count(), 0);

    v1 = sw::ref<int>::make(12);
    EXPECT_EQ(v1.count() == 1, true);
    v2 = v1;
    EXPECT_EQ(v1.count(), 2);
    EXPECT_EQ(v2.count(), 2);

    v2 = nullptr;
    EXPECT_EQ(v1.count(), 1);
    EXPECT_NE(v1.get(), nullptr);
    EXPECT_EQ(v2.count(), 0);
    EXPECT_EQ(v2.get(), nullptr);
}

TEST(RefTest, asteriskOperator) {
    sw::ref<int> v = sw::ref<int>::make(12);
    EXPECT_EQ(*v, 12);
}

TEST(RefTest, arrowOperator) {
    struct my_struct {
        int func() {
            return 12;
        }
    };

    sw::ref<my_struct> v = sw::ref<my_struct>::make();
    EXPECT_EQ(v->func(), 12);
}
#include "sw/track.hpp"
#include <gtest/gtest.h>

TEST(TrackTest, works) {
	sw::track<int> v;
	EXPECT_EQ(v.changed(), true);
}
TEST(TrackTest, reset) {
	sw::track<int> v;
	EXPECT_EQ(v.reset(), true);
	EXPECT_EQ(v.reset(), false);
}
TEST(TrackTest, assignment) {
	sw::track<int> v;
	EXPECT_EQ(v.reset(), true);
	
    v = 12;
    EXPECT_EQ(v.changed(), true);
    EXPECT_EQ(v.reset(), true);
    EXPECT_EQ(v.changed(), false);
    EXPECT_EQ(v.reset(), false);
    EXPECT_EQ((int)v, 12);
    EXPECT_EQ(*v, 12);
}

TEST(TrackTest, copy) {
    sw::track<int> v1;
    sw::track<int> v2;

    v1 = 12;

    EXPECT_EQ(v1.changed(), true);
    EXPECT_EQ(v2.changed(), true);

    v1.reset();
    v2.reset();

    EXPECT_EQ(v1.changed(), false);
    EXPECT_EQ(v2.changed(), false);

    v1 = v2;

    EXPECT_EQ(v1.changed(), true);
    EXPECT_EQ(v2.changed(), false);

    EXPECT_EQ(*v1, *v2);
}

TEST(TrackTest, method) {
    struct some_struct {
        int member;
        int func() {
            return 12;
        }
    };

    sw::track<some_struct> v;
    EXPECT_EQ(v.reset(), true);

    v->func();

    EXPECT_EQ(v.reset(), true);

    EXPECT_EQ(v->member, 0);
    EXPECT_EQ(v.reset(), true);
}

TEST(TrackTest, constructor_initialize) {
    struct some_struct {
        int called_constructor;

        some_struct() {
            called_constructor = 1;
        }

        some_struct(int v1) {
            called_constructor = 2;
        }

        some_struct(int v1, int v2) {
            called_constructor = 3;
        }
    };

    sw::track<some_struct> v;
    EXPECT_EQ(v.reset(), true);
    EXPECT_EQ((*v).called_constructor, 1);
    EXPECT_EQ(v.reset(), true);

    sw::track<some_struct> v1(12);
    EXPECT_EQ(v1.reset(), true);
    EXPECT_EQ((*v1).called_constructor, 2);
    EXPECT_EQ(v1.reset(), true);

    sw::track<some_struct> v2(12, 14);
    EXPECT_EQ(v2.reset(), true);
    EXPECT_EQ((*v2).called_constructor, 3);
    EXPECT_EQ(v2.reset(), true);
}

TEST(TrackTest, operatortest) {
    sw::track<int> v;
    EXPECT_EQ(v.operator int(), 0);
}
#include <gtest/gtest.h>

#include <traceCollector.hpp>

TEST( BasicTest, testSingletonTraceLog ) {
	traceCollector *ptr	 = nullptr;
	traceCollector *ptr1 = nullptr;

	ptr = traceCollector::getInstance();
	EXPECT_NE( ptr, nullptr );

	ptr1 = traceCollector::getInstance();
	EXPECT_EQ( ptr, ptr1 );
}

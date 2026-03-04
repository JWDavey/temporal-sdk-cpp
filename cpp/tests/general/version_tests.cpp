#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "temporalio/version.h"

// ===========================================================================
// Version macros
// ===========================================================================

TEST(VersionTest, MajorVersionDefined) {
    EXPECT_GE(TEMPORALIO_VERSION_MAJOR, 0);
}

TEST(VersionTest, MinorVersionDefined) {
    EXPECT_GE(TEMPORALIO_VERSION_MINOR, 0);
}

TEST(VersionTest, PatchVersionDefined) {
    EXPECT_GE(TEMPORALIO_VERSION_PATCH, 0);
}

TEST(VersionTest, VersionStringDefined) {
    EXPECT_NE(std::strlen(TEMPORALIO_VERSION_STRING), 0u);
}

TEST(VersionTest, VersionStringFormat) {
    std::string ver = TEMPORALIO_VERSION_STRING;
    // Should contain at least two dots (major.minor.patch)
    auto first_dot = ver.find('.');
    ASSERT_NE(first_dot, std::string::npos);
    auto second_dot = ver.find('.', first_dot + 1);
    ASSERT_NE(second_dot, std::string::npos);

    // Parse and verify components match macros
    int major = std::stoi(ver.substr(0, first_dot));
    int minor = std::stoi(ver.substr(first_dot + 1, second_dot - first_dot - 1));
    int patch = std::stoi(ver.substr(second_dot + 1));

    EXPECT_EQ(major, TEMPORALIO_VERSION_MAJOR);
    EXPECT_EQ(minor, TEMPORALIO_VERSION_MINOR);
    EXPECT_EQ(patch, TEMPORALIO_VERSION_PATCH);
}

// ===========================================================================
// version() function
// ===========================================================================

TEST(VersionTest, VersionFunctionNotNull) {
    const char* ver = temporalio::version();
    EXPECT_NE(ver, nullptr);
}

TEST(VersionTest, VersionFunctionNonEmpty) {
    const char* ver = temporalio::version();
    EXPECT_GT(std::strlen(ver), 0u);
}

TEST(VersionTest, VersionFunctionMatchesMacro) {
    EXPECT_STREQ(temporalio::version(), TEMPORALIO_VERSION_STRING);
}

TEST(VersionTest, VersionFunctionConsistent) {
    // Calling multiple times should return the same pointer
    const char* v1 = temporalio::version();
    const char* v2 = temporalio::version();
    EXPECT_EQ(v1, v2);
}

TEST(VersionTest, CurrentVersionIs010) {
    EXPECT_EQ(TEMPORALIO_VERSION_MAJOR, 0);
    EXPECT_EQ(TEMPORALIO_VERSION_MINOR, 1);
    EXPECT_EQ(TEMPORALIO_VERSION_PATCH, 0);
    EXPECT_STREQ(TEMPORALIO_VERSION_STRING, "0.1.0");
}

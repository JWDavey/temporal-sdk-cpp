#include <temporalio/common/search_attributes.h>

#include <gtest/gtest.h>

namespace temporalio::common {
namespace {

TEST(SearchAttributeKeyTest, ConstructAndAccess) {
    auto key = search_attribute::create_keyword("my-attr");
    EXPECT_EQ(key.name(), "my-attr");
    EXPECT_EQ(key.value_type(), IndexedValueType::kKeyword);
}

TEST(SearchAttributeKeyTest, DifferentTypes) {
    auto text_key = search_attribute::create_text("text-field");
    EXPECT_EQ(text_key.value_type(), IndexedValueType::kText);

    auto int_key = search_attribute::create_int("int-field");
    EXPECT_EQ(int_key.value_type(), IndexedValueType::kInt);

    auto double_key = search_attribute::create_double("double-field");
    EXPECT_EQ(double_key.value_type(), IndexedValueType::kDouble);

    auto bool_key = search_attribute::create_bool("bool-field");
    EXPECT_EQ(bool_key.value_type(), IndexedValueType::kBool);

    auto datetime_key = search_attribute::create_datetime("datetime-field");
    EXPECT_EQ(datetime_key.value_type(), IndexedValueType::kDatetime);

    auto keyword_list_key =
        search_attribute::create_keyword_list("keyword-list-field");
    EXPECT_EQ(keyword_list_key.value_type(), IndexedValueType::kKeywordList);
}

TEST(SearchAttributeCollectionTest, EmptyCollection) {
    SearchAttributeCollection collection;
    EXPECT_TRUE(collection.entries().empty());
}

TEST(IndexedValueTypeTest, EnumValues) {
    EXPECT_EQ(static_cast<int>(IndexedValueType::kText), 0);
    EXPECT_EQ(static_cast<int>(IndexedValueType::kKeyword), 1);
    EXPECT_EQ(static_cast<int>(IndexedValueType::kInt), 2);
    EXPECT_EQ(static_cast<int>(IndexedValueType::kDouble), 3);
    EXPECT_EQ(static_cast<int>(IndexedValueType::kBool), 4);
    EXPECT_EQ(static_cast<int>(IndexedValueType::kDatetime), 5);
    EXPECT_EQ(static_cast<int>(IndexedValueType::kKeywordList), 6);
}

} // namespace
} // namespace temporalio::common

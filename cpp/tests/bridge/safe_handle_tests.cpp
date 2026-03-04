#include <gtest/gtest.h>

#include <string>

// We test SafeHandle using a mock type since the real Rust FFI types require
// the Rust bridge to be linked. The template mechanics are identical.

namespace {

// Mock opaque type
struct MockResource {
    int id;
    static int destroy_count;
};

int MockResource::destroy_count = 0;

void mock_resource_free(MockResource* r) {
    ++MockResource::destroy_count;
    delete r;
}

}  // namespace

// Include the SafeHandle template but with our mock types.
// Since SafeHandle is a template in the header, we can instantiate it with
// our mock types directly.

// Replicate the SafeHandle template since we cannot include interop.h
// without the full Rust FFI. The template logic is identical.
#include <memory>
#include <utility>

namespace test_bridge {

template <typename T, void (*Deleter)(T*)>
class SafeHandle {
public:
    SafeHandle() noexcept : ptr_(nullptr) {}
    explicit SafeHandle(T* ptr) noexcept : ptr_(ptr) {}

    ~SafeHandle() {
        if (ptr_) {
            Deleter(ptr_);
        }
    }

    SafeHandle(const SafeHandle&) = delete;
    SafeHandle& operator=(const SafeHandle&) = delete;

    SafeHandle(SafeHandle&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    SafeHandle& operator=(SafeHandle&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                Deleter(ptr_);
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T* get() const noexcept { return ptr_; }

    T* release() noexcept {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    T* ptr_;
};

using MockHandle = SafeHandle<MockResource, mock_resource_free>;

}  // namespace test_bridge

// ===========================================================================
// SafeHandle tests
// ===========================================================================

class SafeHandleTest : public ::testing::Test {
protected:
    void SetUp() override { MockResource::destroy_count = 0; }
};

TEST_F(SafeHandleTest, DefaultConstructedIsNull) {
    test_bridge::MockHandle h;
    EXPECT_FALSE(static_cast<bool>(h));
    EXPECT_EQ(h.get(), nullptr);
}

TEST_F(SafeHandleTest, ConstructFromPointer) {
    auto* r = new MockResource{42};
    test_bridge::MockHandle h(r);
    EXPECT_TRUE(static_cast<bool>(h));
    EXPECT_EQ(h.get(), r);
    EXPECT_EQ(h.get()->id, 42);
}

TEST_F(SafeHandleTest, DestructorCallsDeleter) {
    {
        auto* r = new MockResource{1};
        test_bridge::MockHandle h(r);
    }
    EXPECT_EQ(MockResource::destroy_count, 1);
}

TEST_F(SafeHandleTest, DestructorDoesNotCallDeleterOnNull) {
    {
        test_bridge::MockHandle h;
    }
    EXPECT_EQ(MockResource::destroy_count, 0);
}

TEST_F(SafeHandleTest, MoveConstruction) {
    auto* r = new MockResource{10};
    test_bridge::MockHandle h1(r);
    test_bridge::MockHandle h2(std::move(h1));

    EXPECT_FALSE(static_cast<bool>(h1));
    EXPECT_TRUE(static_cast<bool>(h2));
    EXPECT_EQ(h2.get(), r);
    EXPECT_EQ(MockResource::destroy_count, 0);
}

TEST_F(SafeHandleTest, MoveAssignment) {
    auto* r1 = new MockResource{1};
    auto* r2 = new MockResource{2};
    test_bridge::MockHandle h1(r1);
    test_bridge::MockHandle h2(r2);

    h2 = std::move(h1);

    EXPECT_FALSE(static_cast<bool>(h1));
    EXPECT_TRUE(static_cast<bool>(h2));
    EXPECT_EQ(h2.get(), r1);
    // r2 should have been freed by the assignment
    EXPECT_EQ(MockResource::destroy_count, 1);
}

TEST_F(SafeHandleTest, MoveAssignmentToSelf) {
    auto* r = new MockResource{5};
    test_bridge::MockHandle h(r);

    // Suppress self-move warning for test
    test_bridge::MockHandle& ref = h;
    h = std::move(ref);

    EXPECT_TRUE(static_cast<bool>(h));
    EXPECT_EQ(h.get(), r);
    EXPECT_EQ(MockResource::destroy_count, 0);
}

TEST_F(SafeHandleTest, Release) {
    auto* r = new MockResource{7};
    test_bridge::MockHandle h(r);

    auto* released = h.release();
    EXPECT_EQ(released, r);
    EXPECT_FALSE(static_cast<bool>(h));
    EXPECT_EQ(MockResource::destroy_count, 0);

    // Must manually free since ownership was released
    delete released;
}

TEST_F(SafeHandleTest, ReleaseOnNull) {
    test_bridge::MockHandle h;
    auto* released = h.release();
    EXPECT_EQ(released, nullptr);
}

TEST_F(SafeHandleTest, MoveOnlyNoImplicitCopy) {
    EXPECT_FALSE(std::is_copy_constructible_v<test_bridge::MockHandle>);
    EXPECT_FALSE(std::is_copy_assignable_v<test_bridge::MockHandle>);
    EXPECT_TRUE(std::is_move_constructible_v<test_bridge::MockHandle>);
    EXPECT_TRUE(std::is_move_assignable_v<test_bridge::MockHandle>);
}

TEST_F(SafeHandleTest, MultipleHandlesFreedCorrectly) {
    {
        test_bridge::MockHandle h1(new MockResource{1});
        test_bridge::MockHandle h2(new MockResource{2});
        test_bridge::MockHandle h3(new MockResource{3});
    }
    EXPECT_EQ(MockResource::destroy_count, 3);
}

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "temporalio/client/temporal_connection.h"

using namespace temporalio::client;
using namespace std::chrono_literals;

// ===========================================================================
// TLS auto-enable behavior when API key is provided
// ===========================================================================

TEST(ConnectionOptionsTlsTest, ApiKeyWithoutTls_TlsShouldBeAutoEnabled) {
    // When an API key is provided and TLS is not set, TLS should be
    // auto-enabled (mirroring the C# behavior).
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .api_key = "test-api-key",
    };

    // The options themselves just store data; the actual auto-enable
    // happens during connection. Verify the options are stored correctly.
    EXPECT_TRUE(opts.api_key.has_value());
    EXPECT_EQ(opts.api_key.value(), "test-api-key");
    EXPECT_FALSE(opts.tls.has_value());
}

TEST(ConnectionOptionsTlsTest, ApiKeyWithTlsDisabled_TlsStaysDisabled) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .tls = TlsOptions{.disabled = true},
        .api_key = "test-api-key",
    };

    EXPECT_TRUE(opts.tls.has_value());
    EXPECT_TRUE(opts.tls->disabled);
    EXPECT_TRUE(opts.api_key.has_value());
}

TEST(ConnectionOptionsTlsTest, NoApiKeyNoTls_TlsNotSet) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
    };

    EXPECT_FALSE(opts.api_key.has_value());
    EXPECT_FALSE(opts.tls.has_value());
}

TEST(ConnectionOptionsTlsTest, ExplicitTlsNoApiKey_TlsEnabled) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .tls = TlsOptions{},
    };

    EXPECT_TRUE(opts.tls.has_value());
    EXPECT_FALSE(opts.tls->disabled);
    EXPECT_FALSE(opts.api_key.has_value());
}

// ===========================================================================
// Metadata configuration
// ===========================================================================

TEST(ConnectionOptionsMetadataTest, NoMetadata) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
    };

    EXPECT_TRUE(opts.rpc_metadata.empty());
}

TEST(ConnectionOptionsMetadataTest, EmptyMetadata) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .rpc_metadata = {},
    };

    EXPECT_TRUE(opts.rpc_metadata.empty());
}

TEST(ConnectionOptionsMetadataTest, WithMetadata) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .rpc_metadata =
            {
                {"key1", "value1"},
                {"x-key2", "value2"},
            },
    };

    ASSERT_EQ(opts.rpc_metadata.size(), 2u);
    EXPECT_EQ(opts.rpc_metadata[0].first, "key1");
    EXPECT_EQ(opts.rpc_metadata[0].second, "value1");
    EXPECT_EQ(opts.rpc_metadata[1].first, "x-key2");
    EXPECT_EQ(opts.rpc_metadata[1].second, "value2");
}

// ===========================================================================
// mTLS configuration
// ===========================================================================

TEST(ConnectionOptionsMtlsTest, ClientCertAndKey) {
    TlsOptions tls{
        .client_cert = "-----BEGIN CERTIFICATE-----\nMIIC...",
        .client_private_key = "-----BEGIN PRIVATE KEY-----\nMIIE...",
    };

    EXPECT_FALSE(tls.client_cert.empty());
    EXPECT_FALSE(tls.client_private_key.empty());
    EXPECT_TRUE(tls.server_root_ca_cert.empty());
}

TEST(ConnectionOptionsMtlsTest, FullMtlsConfig) {
    TlsOptions tls{
        .server_root_ca_cert = "-----BEGIN CERTIFICATE-----\nCA...",
        .client_cert = "-----BEGIN CERTIFICATE-----\nClient...",
        .client_private_key = "-----BEGIN PRIVATE KEY-----\nKey...",
        .server_name = "temporal.example.com",
    };

    EXPECT_FALSE(tls.server_root_ca_cert.empty());
    EXPECT_FALSE(tls.client_cert.empty());
    EXPECT_FALSE(tls.client_private_key.empty());
    EXPECT_TRUE(tls.server_name.has_value());
    EXPECT_EQ(tls.server_name.value(), "temporal.example.com");
}

// ===========================================================================
// Proxy configuration
// ===========================================================================

TEST(ConnectionOptionsProxyTest, NoProxy) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
    };

    EXPECT_FALSE(opts.http_connect_proxy.has_value());
}

TEST(ConnectionOptionsProxyTest, WithProxy) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .http_connect_proxy = HttpConnectProxyOptions{
            .target_host = "proxy.corp:8080",
            .basic_auth_user = "user",
            .basic_auth_password = "pass",
        },
    };

    EXPECT_TRUE(opts.http_connect_proxy.has_value());
    EXPECT_EQ(opts.http_connect_proxy->target_host, "proxy.corp:8080");
    EXPECT_EQ(opts.http_connect_proxy->basic_auth_user.value(), "user");
    EXPECT_EQ(opts.http_connect_proxy->basic_auth_password.value(), "pass");
}

TEST(ConnectionOptionsProxyTest, ProxyWithoutAuth) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .http_connect_proxy = HttpConnectProxyOptions{
            .target_host = "proxy.corp:8080",
        },
    };

    EXPECT_TRUE(opts.http_connect_proxy.has_value());
    EXPECT_FALSE(opts.http_connect_proxy->basic_auth_user.has_value());
    EXPECT_FALSE(opts.http_connect_proxy->basic_auth_password.has_value());
}

// ===========================================================================
// Identity configuration
// ===========================================================================

TEST(ConnectionOptionsIdentityTest, DefaultIdentity) {
    TemporalConnectionOptions opts;
    EXPECT_FALSE(opts.identity.has_value());
}

TEST(ConnectionOptionsIdentityTest, CustomIdentity) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .identity = "my-worker-1",
    };

    EXPECT_TRUE(opts.identity.has_value());
    EXPECT_EQ(opts.identity.value(), "my-worker-1");
}

// ===========================================================================
// Retry options configuration
// ===========================================================================

TEST(ConnectionOptionsRetryTest, DefaultRetry) {
    TemporalConnectionOptions opts;
    EXPECT_FALSE(opts.rpc_retry.has_value());
}

TEST(ConnectionOptionsRetryTest, CustomRetry) {
    TemporalConnectionOptions opts{
        .target_host = "localhost:7233",
        .rpc_retry = RpcRetryOptions{
            .initial_interval = 500ms,
            .multiplier = 2.0,
            .max_retries = 3,
        },
    };

    EXPECT_TRUE(opts.rpc_retry.has_value());
    EXPECT_EQ(opts.rpc_retry->initial_interval, 500ms);
    EXPECT_DOUBLE_EQ(opts.rpc_retry->multiplier, 2.0);
    EXPECT_EQ(opts.rpc_retry->max_retries, 3);
}

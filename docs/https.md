# HTTPS

HTTPS uses the same HTTP parser, router, middleware, and response API over a TLS stream. Enable TLS at build time, then select HTTPS in runtime configuration. A TLS-enabled build can still run plain HTTP.

## Build with TLS

From the repository root, with `VCPKG_ROOT` set:

```sh
cmake --preset vcpkg-debug-tls -Dmoji_BUILD_EXAMPLES=ON
cmake --build --preset vcpkg-debug-tls --target 02_https --parallel
```

The preset enables `moji_ENABLE_TLS` and selects the manifest's optional `tls` feature. OpenSSL is acquired by vcpkg. Regular HTTP presets do not request OpenSSL.

Without vcpkg:

```sh
cmake -S . -B build/tls \
  -DCMAKE_BUILD_TYPE=Debug \
  -Dmoji_ENABLE_TLS=ON \
  -Dmoji_BUILD_EXAMPLES=ON
cmake --build build/tls --target 02_https --parallel
```

An OpenSSL development installation must be available for this path; `find_package(OpenSSL)` does not download one. See [build settings](build-system.md).

## Configure the server

```cpp
auto config = moji::ConfigBuilder()
    .port(8443)
    .threads(4)
    .protocol(moji::NetworkProtocol::HTTPS)
    .certificate_chain_file("certs/cert.pem")
    .private_key_file("certs/key.pem")
    .build();

moji::App app{config};
```

Certificate and key paths are owned strings. They can point to separate PEM files or a combined file. File paths are resolved from the working directory at startup. Missing paths or certificate/key-loading errors prevent the listener from starting.

The current server uses a TLS server context and disables SSLv2, SSLv3, TLS 1.0, and TLS 1.1. It supports TLS 1.2 and, where available in OpenSSL, TLS 1.3. It does not configure client-certificate authentication or certificate hot reload. The example uses an unencrypted private key; there is no config-level password callback.

Requesting HTTPS from a build without TLS support throws at startup. It never silently starts a plain HTTP listener instead.

## Generate local test files

From the repository root, using OpenSSL 3:

```sh
mkdir -p examples/02_https/certs

openssl req -x509 -newkey rsa:2048 -sha256 -days 30 -noenc \
  -keyout examples/02_https/certs/key.pem \
  -out examples/02_https/certs/cert.pem \
  -subj "/CN=localhost" \
  -addext "subjectAltName=DNS:localhost,IP:127.0.0.1" \
  -addext "basicConstraints=critical,CA:FALSE" \
  -addext "extendedKeyUsage=serverAuth"

chmod 600 examples/02_https/certs/key.pem
```

This generates a self-signed certificate for local testing, valid for 30 days, and its matching unencrypted key. The certificate covers both `localhost` and `127.0.0.1`. This is not the publicly trusted certificate setup for a deployed domain.

Run the example from the repository root, passing the two paths explicitly:

```sh
./build/vcpkg-debug-tls/examples/02_https/02_https \
  examples/02_https/certs/cert.pem \
  examples/02_https/certs/key.pem
```

In another terminal, also from the repository root:

```sh
curl --cacert examples/02_https/certs/cert.pem https://localhost:8443/
curl --cacert examples/02_https/certs/cert.pem https://127.0.0.1:8443/json
```

`--cacert` explicitly trusts the local test certificate while retaining hostname verification. No browser-wide trust-store modification is needed for these commands.

## Connection lifecycle

The server configures one shared SSL context before accepting connections. Each HTTPS connection gets a separate `TlsStream` and `HttpSession<TlsStream>`.

```text
TCP accept → TLS handshake → HTTP request/response loop → TLS shutdown → TCP cleanup
```

The handshake occurs once per connection, before HTTP parsing. Keep-alive reuses the established connection; it does not repeat the handshake or TLS shutdown after every request.

The handshake uses `timeout_seconds`. A connection reaching its normal close path uses `shutdown_timeout` for the TLS closing exchange. Application-wide `stop()` is still immediate and does not guarantee that exchange for every active connection.

TLS handshake failures are connection failures. A peer that disconnects during the handshake may produce a session error such as `stream truncated`. A truncated HTTP read is never dispatched as a successful request. Read/shutdown handling recognizes a missing TLS closing notification without trying to turn it into a successful partial message.

## Common local setup errors

| Message/symptom | Check |
|---|---|
| `use_certificate_chain_file: No such file or directory` | Server's path relative to its working directory |
| curl error 77 loading `--cacert` | Client's certificate path relative to that terminal's directory |
| `Acceptor bind failed: Permission denied` on 443 | Use 8443 locally unless privileged-port access is configured |
| TLS build succeeds but server speaks HTTP | Set `NetworkProtocol::HTTPS` as well as the build option |
| TLS compiler/link errors in a standalone example | Enable TLS in that example's own CMake configuration |
| Old `02_hello_world` executable still exists | Build/run the current `02_https` target; old artifacts are not renamed automatically |

The listener currently binds all IPv4 interfaces. Deployments may terminate TLS in a reverse proxy and run moji in HTTP mode behind it. In that case moji's OpenSSL dependency can remain disabled.

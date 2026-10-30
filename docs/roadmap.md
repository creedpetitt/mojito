# Roadmap

These are planned features, not APIs available in the current source. Priorities:

1. **Async HTTP client.** A coroutine-based client that fits the existing Asio executor and TLS model.
2. **WebSockets.** Connection upgrades and messaging with clear ownership and cancellation.
3. **GoogleTest coverage.** A maintained automated suite for routing, request/response behavior, middleware, lifecycle, execution models, and HTTP/HTTPS.
4. **Small built-in middleware collection.** Start with CORS; keep the existing custom middleware API as the foundation.
5. **Static-directory serving.** URL-prefix-to-directory mapping with defined path handling, MIME behavior, and error responses.

Individual file responses already exist through `Response::file()`. Static-directory serving is a separate feature.
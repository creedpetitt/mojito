#include <moji/app.h>

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <certificate-chain.pem> <private-key.pem>\n";
        return 1;
    }

    try {
        auto config = moji::ConfigBuilder()
            .port(8443)
            .threads(4)
            .protocol(moji::NetworkProtocol::HTTPS)
            .certificate_chain_file(argv[1])
            .private_key_file(argv[2])
            .build();

        moji::App app{config};
        app.get("/", []() {
            return "Hello over HTTPS!\n";
        });
        app.get("/json", []() {
            return moji::Json{{"message", "Hello over HTTPS!"}};
        });

        std::cout << "HTTPS example: https://localhost:8443\n";
        app.init();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

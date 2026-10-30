#include <moji/app.h>

#include <exception>
#include <iostream>

int main() {
    try {
        moji::App app;
        app.get("/", []() {
            return "Hello from moji!\n";
        });
        app.get("/json", []() {
            return moji::Json{{"message", "Hello from moji!"}};
        });

        std::cout << "HTTP example: http://localhost:8080\n";
        app.init();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

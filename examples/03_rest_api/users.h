#pragma once

#include <moji/app.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace example {

struct User {
    int id;
    std::string name;
};

// All routes share one store. A worker's serial event loop does not protect
// application state accessed by routes on other workers.
class UserStore {
public:
    User create(std::string name);
    std::optional<User> find(int id) const;
    std::optional<User> update(int id, std::string name);
    bool erase(int id);

private:
    mutable std::mutex mutex_;
    std::unordered_map<int, User> users_;
    int next_id_{1};
};

void register_user_routes(moji::App& app, std::shared_ptr<UserStore> store);

} // namespace example

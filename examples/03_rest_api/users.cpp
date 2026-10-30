#include "users.h"

#include <moji/exceptions.h>

#include <utility>

namespace example {

User UserStore::create(std::string name) {
    std::lock_guard lock(mutex_);
    User user{next_id_++, std::move(name)};
    users_.emplace(user.id, user);
    return user;
}

std::optional<User> UserStore::find(int id) const {
    std::lock_guard lock(mutex_);
    const auto it = users_.find(id);
    if (it == users_.end()) return std::nullopt;
    return it->second;
}

std::optional<User> UserStore::update(int id, std::string name) {
    std::lock_guard lock(mutex_);
    const auto it = users_.find(id);
    if (it == users_.end()) return std::nullopt;
    it->second.name = std::move(name);
    return it->second;
}

bool UserStore::erase(int id) {
    std::lock_guard lock(mutex_);
    return users_.erase(id) != 0;
}

namespace {

moji::Json user_json(const User& user) {
    return moji::Json{{"id", user.id}, {"name", user.name}};
}

int user_id(const moji::Request& req) {
    const int id = req.param<int>("id");
    if (id < 1) throw moji::BadRequest("id must be positive");
    return id;
}

std::string read_name(const moji::Request& req) {
    auto body = req.json();
    auto name = body["name"].as<std::string>();
    if (name.empty() || name.size() > 80) {
        throw moji::BadRequest("name must contain between 1 and 80 bytes");
    }
    return name;
}

} // namespace

void register_user_routes(moji::App& app, std::shared_ptr<UserStore> store) {
    app.get("/", []() {
        return moji::Json{{"service", "users example"}, {"storage", "in memory"}};
    });

    app.get("/users/:id", [store](const moji::Request& req) {
        const auto user = store->find(user_id(req));
        if (!user) throw moji::NotFound("User not found");
        return user_json(*user);
    });

    app.post("/users", [store](const moji::Request& req, moji::Response& res) {
        const auto user = store->create(read_name(req));
        res.created("/users/" + std::to_string(user.id)).json(user_json(user));
    });

    app.put("/users/:id", [store](const moji::Request& req) {
        const auto id = user_id(req);
        auto name = read_name(req);
        const auto user = store->update(id, std::move(name));
        if (!user) throw moji::NotFound("User not found");
        return user_json(*user);
    });

    app.del("/users/:id", [store](const moji::Request& req, moji::Response& res) {
        if (!store->erase(user_id(req))) throw moji::NotFound("User not found");
        res.no_content();
    });
}

} // namespace example

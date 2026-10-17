#pragma once

#include <string>
#include <unordered_map>

namespace sowa {

template <typename Key, typename Value> class Store {
  public:
	void Clear() {
		_store.clear();
	}

	void Set(const Key &key, const Value &value) {
		_store[key] = value;
	}

	Value Get(const Key &key) const {
		auto it = _store.find(key);
		if (it == _store.end()) {
			return Value();
		}
		return it->second;
	}

	bool Has(const Key &key) const {
		return _store.find(key) != _store.end();
	}

	void Remove(const Key &key) {
		_store.erase(key);
	}

	const std::unordered_map<Key, Value> &GetStore() const {
		return _store;
	}

  private:
	std::unordered_map<Key, Value> _store;
};

using StringStore = Store<std::string, std::string>;

} // namespace sowa

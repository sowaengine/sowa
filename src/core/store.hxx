#ifndef SW_STORE_HXX
#define SW_STORE_HXX
#pragma once

#include <string>
#include <unordered_map>

template <typename ValueType = std::string, typename KeyType = std::string>
class Store {
  public:
	using store_type = Store<ValueType, KeyType>;

	static store_type &get() {
		static store_type *store = new store_type;
		return *store;
	}

	inline ValueType get_value(KeyType key) {
		if (has_key(key)) {
			return m_values[key];
		}
		return ValueType();
	}

	inline void set_value(KeyType key, ValueType value) {
		m_values[key] = value;
	}

	inline void remove_key(KeyType key) {
		m_values.erase(key);
	}

	inline bool has_key(KeyType key) {
		return m_values.find(key) != m_values.end();
	}

	inline void clear() {
		m_values.clear();
	}

	Store() = default;
	~Store() = default;

  private:
	std::unordered_map<KeyType, ValueType> m_values;
};

#endif // SW_STORE_HXX
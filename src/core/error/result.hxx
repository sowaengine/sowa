#ifndef SW_RESULT_HXX
#define SW_RESULT_HXX
#pragma once

#include <optional>
#include <tuple>

#include "err_code.hxx"

template <typename T = std::nullopt_t>
class Result {
  public:
	Result() = default;
	Result(std::optional<T> value) : m_value(value) {
		if (value.has_value())
			m_err_code = OK;
	}
	Result(const T &value) : m_value(value), m_err_code(OK) {}
	Result(ErrorCode err_code) : m_err_code(err_code) {}
	~Result() = default;

	inline operator std::tuple<std::optional<T>, ErrorCode>() { return std::make_tuple(m_value, m_err_code); }
	inline bool ok() { return m_err_code == OK && m_value.has_value(); }
	inline ErrorCode error() { return m_err_code; }
	inline std::optional<T> &value() { return m_value; }
	inline T value_or(const T &fallback) { return m_value.value_or(fallback); }
	inline T &get() {
		if (!m_value.has_value()) {
			m_value = T{};
		}
		return m_value.value();
	}

	template <size_t I>
	auto &get() & {
		if constexpr (I == 0)
			return m_value;
		else if constexpr (I == 1)
			return m_err_code;
	}

	template <size_t I>
	auto const &get() const & {
		if constexpr (I == 0)
			return m_value;
		else if constexpr (I == 1)
			return m_err_code;
	}

	template <size_t I>
	auto &&get() && {
		if constexpr (I == 0)
			return std::move(m_value);
		else if constexpr (I == 1)
			return std::move(m_err_code);
	}

	inline operator T &() { return this->get(); }

  private:
	std::optional<T> m_value = std::nullopt;
	ErrorCode m_err_code = ERR_FAILED;
};

namespace std {
template <typename T>
struct tuple_size<Result<T>> : std::integral_constant<size_t, 2> {};

template <typename T>
struct tuple_element<0, Result<T>> {
	using type = std::optional<T>;
};

template <typename T>
struct tuple_element<1, Result<T>> {
	using type = ErrorCode;
};

} // namespace std

#endif // SW_RESULT_HXX
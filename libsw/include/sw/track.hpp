#ifndef SW_TRACK_HPP_
#define SW_TRACK_HPP_

namespace sw {

template <typename T>
class track {
	typedef track<T> track_t;

  public:
	bool reset() {
		bool old_changed = m_changed;
		m_changed = false;
		return old_changed;
	}

	bool changed() {
		return m_changed;
	}

  public:
	track() {
		m_changed = true;
		m_val = T();
	}

	template <typename... Args>
	track(Args... a) {
		m_changed = true;
		m_val = T(a...);
	}

	~track() {
	}

	// copy
	track(track &t) {
		m_changed = true;
		m_val = t.m_val;
	}
	track &operator=(const track &t) {
		m_changed = true;
		m_val = t.m_val;

		return *this;
	}
	track &operator=(track &t) {
		m_changed = true;
		m_val = t.m_val;
		
		return *this;
	}

	track(const track &t) {
		m_changed = true;
		m_val = t.m_val;
	}

	operator T() {
		m_changed = true;
		return m_val;
	}

	T *operator->() {
		m_changed = true;
		return &m_val;
	}
	T &operator*() {
		m_changed = true;
		return m_val;
	}

  private:
	T m_val;
	bool m_changed;
};
} // namespace sw

#endif // SW_TRACK_HPP_
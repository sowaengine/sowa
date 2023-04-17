#ifndef SW_REF_HPP_
#define SW_REF_HPP_

namespace sw {
template <typename T>
class ref {
	typedef ref<T> ref_t;

  public:
	template <typename... Args>
	static ref make(Args... args) {
		ref r;
		r.p_val = new T(args...);
		r.p_refcount = new unsigned(1);
		return r;
	}

	ref() {
		p_val = nullptr;
		p_refcount = nullptr;
	}
	~ref() {
		clean();
	}

	ref(const ref& v) {
		p_refcount = v.p_refcount;
		p_val = v.p_val;

		if (p_refcount != nullptr && p_val != nullptr) {
			(*p_refcount)++;
		}
	}
	
	ref operator=(const ref &v) {
		clean();

		p_refcount = v.p_refcount;
		p_val = v.p_val;

		if (p_refcount != nullptr && p_val != nullptr) {
			(*p_refcount)++;
		}

		return *this;
	}

	ref operator=(ref* v) {
		if(v == nullptr) {
			clean();
		} else {
			*this = *v;
		}
		return *this;
	}

	unsigned count() {
		unsigned v = (p_refcount == nullptr || p_refcount == zeroloc_()) ? 0 : *p_refcount;
		return v;
	}

	T *get() {
		return p_val;
	}

	T& operator*() {
		return *p_val;
	}

	T* operator->() {
		return p_val;
	}

	// stores number 0, deleted references set their p_refcount points to zeroloc
	// weak_ref uses to test if data exist
	static unsigned* zeroloc_() {
		static unsigned* loc = new unsigned(0);
		*loc = 0;
		return loc;
	}
  private:
	T *p_val;
	unsigned *p_refcount;

	void clean() {
		if (p_refcount != nullptr) {
			(*p_refcount)--;
			if (*p_refcount <= 0) {
				if (p_val != nullptr) {
					delete p_val;
				}
				delete p_refcount;
			}
		}
		p_refcount = zeroloc_();
		p_val = nullptr;
	}
};
} // namespace sw

#endif // SW_REF_HPP_
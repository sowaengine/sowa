#ifndef SW_RESOURCE_LOCKER_HXX
#define SW_RESOURCE_LOCKER_HXX

#include <unordered_map>

#include "resource/resource.hxx"
#include "resource/resource_manager.hxx"

class GlobalResourceLocker {
  public:
	static GlobalResourceLocker &get();

	void increment(RID rid);
	void decrement(RID rid);
	int get_ref_count(RID rid);
	void modify_ref(RID rid, int count);

	inline const std::unordered_map<RID, int> &get_references() const { return m_references; }

  private:
	friend class App;

	GlobalResourceLocker() = default;
	~GlobalResourceLocker() = default;

	void collect();

	std::unordered_map<RID, int> m_references;
};

class ResourceLocker {
  public:
	void increment(RID rid);
	void decrement(RID rid);

	// resets reference counter, unregisters registered RIDs
	void reset();

	inline const std::unordered_map<RID, int> &get_references() const { return m_references; }

	ResourceLocker() = default;
	~ResourceLocker();

  private:
	std::unordered_map<RID, int> m_references;
};

#endif // SW_RESOURCE_LOCKER_HXX
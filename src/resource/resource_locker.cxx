#include "resource_locker.hxx"

#include <algorithm>

#include "utils/utils.hxx"

// * Global Resource Locker * //

GlobalResourceLocker &GlobalResourceLocker::get() {
	static GlobalResourceLocker *locker = new GlobalResourceLocker;
	return *locker;
}

void GlobalResourceLocker::increment(RID rid) {
	modify_ref(rid, 1);
}

void GlobalResourceLocker::decrement(RID rid) {
	modify_ref(rid, -1);
}

int GlobalResourceLocker::get_ref_count(RID rid) {
	return m_references[rid];
}

void GlobalResourceLocker::modify_ref(RID rid, int count) {
	m_references[rid] += count;
}

void GlobalResourceLocker::collect() {
	for (auto &[id, count] : m_references) {
		if (count <= 0) {
			ResourceManager::get().Unload(id);
		}
	}

	for (auto it = m_references.begin(); it != m_references.end();) {
		if (it->second == 0)
			it = m_references.erase(it);
		else
			++it;
	}
}

// * Resource Locker * //

void ResourceLocker::increment(RID rid) {
	m_references[rid]++;
	GlobalResourceLocker::get().increment(rid);
}

void ResourceLocker::decrement(RID rid) {
	if (m_references[rid]-- <= 0) {
		return;
	}
	m_references[rid]--;
	GlobalResourceLocker::get().decrement(rid);
}

void ResourceLocker::reset() {
	for (auto &[id, count] : m_references) {
		GlobalResourceLocker::get().modify_ref(id, -count);
	}
	m_references.clear();
}

ResourceLocker::~ResourceLocker() {
	reset();
}
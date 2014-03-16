// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef SECTORCACHE_H
#define SECTORCACHE_H

#include <functional>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include "libs.h"
#include "galaxy/SystemPath.h"
#include "graphics/Drawables.h"
#include "JobQueue.h"
#include "RefCounted.h"

template <typename T, typename CompareT>
class GalaxyObjectCache {
	friend T;
public:
	~GalaxyObjectCache();
	RefCountedPtr<T> GetCached(const SystemPath& path);
	RefCountedPtr<T> GetIfCached(const SystemPath& path);
	void ClearCache(); 	// Completely clear slave caches
	bool IsEmpty() { return m_attic.empty(); }

	typedef std::vector<SystemPath> PathVector;
	typedef std::map<SystemPath,RefCountedPtr<T>,CompareT> CacheMap;
	typedef std::map<SystemPath,T*,CompareT> AtticMap;
	typedef std::function<void()> CacheFilledCallback;

	class Slave : public RefCounted {
		friend class GalaxyObjectCache<T,CompareT>;
	public:
		RefCountedPtr<T> GetCached(const SystemPath& path);
		RefCountedPtr<T> GetIfCached(const SystemPath& path);
		typename CacheMap::const_iterator Begin() const { return m_cache.begin(); }
		typename CacheMap::const_iterator End() const { return m_cache.end(); }

		void FillCache(const PathVector& paths, CacheFilledCallback callback = CacheFilledCallback());
		void Erase(const SystemPath& path);
		void Erase(const typename CacheMap::const_iterator& it);
		void ClearCache();
		bool IsEmpty() { return m_cache.empty(); }
		~Slave();

	private:
		GalaxyObjectCache* m_master;
		CacheMap m_cache;
		JobSet m_jobs;

		Slave(GalaxyObjectCache* master, JobQueue* jobQueue);
		void MasterDeleted();
		void AddToCache(std::vector<RefCountedPtr<T> >& objects);
	};

	RefCountedPtr<Slave> NewSlaveCache();

protected:
	static const unsigned CACHE_JOB_SIZE = 100;

	void AddToCache(std::vector<RefCountedPtr<T> >& objects);
	bool HasCached(const SystemPath& path) const;
	void RemoveFromAttic(const SystemPath& path);

	// ********************************************************************************
	// Overloaded Job class to handle generating a collection of sectors
	// ********************************************************************************
	class CacheJob : public Job
	{
	public:
		CacheJob(std::unique_ptr<std::vector<SystemPath> > path, Slave* slaveCache, CacheFilledCallback callback = CacheFilledCallback());

		virtual void OnRun();    // RUNS IN ANOTHER THREAD!! MUST BE THREAD SAFE!
		virtual void OnFinish();  // runs in primary thread of the context
		virtual void OnCancel() {}  // runs in primary thread of the context

	protected:
		std::unique_ptr<std::vector<SystemPath> > m_paths;
		std::vector<RefCountedPtr<T> > m_objects;
		Slave* m_slaveCache;
		CacheFilledCallback m_callback;
	};

	std::set<Slave*> m_slaves;
	AtticMap m_attic;	// Those contains non-refcounted pointers which are kept alive by RefCountedPtrs in slave caches
						// or elsewhere. The Sector destructor ensures that it is removed from here.
						// This ensures, that there is only ever one object for each Sector.
};

class Sector;
typedef GalaxyObjectCache<Sector, SystemPath::LessSectorOnly> SectorCache;


class StarSystem;
typedef GalaxyObjectCache<StarSystem, SystemPath::LessSystemOnly> StarSystemCache;

#endif

//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_catalog_set.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/transaction/transaction.hpp"
#include "duckdb/common/case_insensitive_map.hpp"
#include "duckdb/common/mutex.hpp"
#include "duckdb/common/shared_ptr.hpp"

namespace duckdb {
struct DropInfo;
class NanodbcResult;
class NanodbcSchemaEntry;
class NanodbcTransaction;

class NanodbcCatalogSet {
public:
	NanodbcCatalogSet(Catalog &catalog, bool is_loaded);

	optional_ptr<CatalogEntry> GetEntry(ClientContext &context, const string &name);
	void DropEntry(ClientContext &context, DropInfo &info);
	void Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback);
	virtual optional_ptr<CatalogEntry> CreateEntry(unique_ptr<CatalogEntry> entry);
	void ClearEntries();
	virtual bool SupportReload() const {
		return false;
	}
	virtual optional_ptr<CatalogEntry> ReloadEntry(ClientContext &context, const string &name);

protected:
	virtual void LoadEntries(ClientContext &context) = 0;
	//! Whether or not the catalog set contains dependencies to itself that have
	//! to be resolved WHILE loading
	virtual bool HasInternalDependencies() const {
		return false;
	}
	void TryLoadEntries(ClientContext &context);

protected:
	Catalog &catalog;

private:
	mutex entry_lock;
	mutex load_lock;
	unordered_map<string, unique_ptr<CatalogEntry>> entries;
	case_insensitive_map_t<string> entry_map;
	atomic<bool> is_loaded;
};

class NanodbcInSchemaSet : public NanodbcCatalogSet {
public:
	NanodbcInSchemaSet(NanodbcSchemaEntry &schema, bool is_loaded);

	optional_ptr<CatalogEntry> CreateEntry(unique_ptr<CatalogEntry> entry) override;

protected:
	NanodbcSchemaEntry &schema;
};

} // namespace duckdb
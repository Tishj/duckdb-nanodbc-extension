
#pragma once

#include "duckdb/common/case_insensitive_map.hpp"
#include "duckdb/catalog/catalog_entry.hpp"
#include "duckdb/catalog/catalog.hpp"

namespace duckdb {
struct CreateTableInfo;
class OdbcSchemaEntry;

class OdbcTableSet {
public:
	explicit OdbcTableSet(OdbcSchemaEntry &schema);
public:
	optional_ptr<CatalogEntry> GetEntry(ClientContext &context, const EntryLookupInfo &lookup);
	void Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback);
protected:
	void LoadEntries(ClientContext &context);

protected:
	OdbcSchemaEntry &schema;
	Catalog &catalog;
private:
	mutex entry_lock;
	case_insensitive_map_t<unique_ptr<CatalogEntry>> tables;
};

} // namespace duckdb

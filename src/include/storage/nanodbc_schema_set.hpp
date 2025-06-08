//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_schema_set.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/nanodbc_catalog_set.hpp"
#include "storage/nanodbc_schema_entry.hpp"

namespace duckdb {
struct CreateSchemaInfo;

class NanodbcSchemaSet : public NanodbcCatalogSet {
public:
	explicit NanodbcSchemaSet(Catalog &catalog, string schema_to_load);

public:
	optional_ptr<CatalogEntry> CreateSchema(ClientContext &context, CreateSchemaInfo &info);

	static string GetInitializeQuery(const string &schema = string());

protected:
	void LoadEntries(ClientContext &context) override;

protected:
	//! Schema to load - if empty loads all schemas (default behavior)
	string schema_to_load;
};

} // namespace duckdb

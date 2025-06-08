//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_table_set.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/nanodbc_catalog_set.hpp"
#include "storage/nanodbc_table_entry.hpp"

namespace duckdb {
struct CreateTableInfo;
class NanodbcConnection;
class NanodbcResult;
class NanodbcSchemaEntry;

class NanodbcTableSet : public NanodbcInSchemaSet {
public:
	explicit NanodbcTableSet(NanodbcSchemaEntry &schema);

public:
	optional_ptr<CatalogEntry> ReloadEntry(ClientContext &context, const string &table_name) override;

protected:
	void LoadEntries(ClientContext &context) override;
	bool SupportReload() const override {
		return true;
	}
	void CreateEntries(NanodbcTransaction &transaction, NanodbcResult &result, idx_t start, idx_t end);
};

} // namespace duckdb

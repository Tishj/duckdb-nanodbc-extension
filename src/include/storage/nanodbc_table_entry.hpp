//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_table_entry.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"

namespace duckdb {

struct NanodbcTableInfo {
	NanodbcTableInfo() {
		create_info = make_uniq<CreateTableInfo>();
		create_info->columns.SetAllowDuplicates(true);
	}
	NanodbcTableInfo(const string &schema, const string &table) {
		create_info = make_uniq<CreateTableInfo>(string(), schema, table);
		create_info->columns.SetAllowDuplicates(true);
	}
	NanodbcTableInfo(const SchemaCatalogEntry &schema, const string &table) {
		create_info = make_uniq<CreateTableInfo>((SchemaCatalogEntry &)schema, table);
		create_info->columns.SetAllowDuplicates(true);
	}

	const string &GetTableName() const {
		return create_info->table;
	}

	unique_ptr<CreateTableInfo> create_info;
	vector<NanodbcType> Nanodbc_types;
	vector<string> Nanodbc_names;
	idx_t approx_num_pages = 0;
};

class NanodbcTableEntry : public TableCatalogEntry {
public:
	NanodbcTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, CreateTableInfo &info);
	NanodbcTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, NanodbcTableInfo &info);

public:
	unique_ptr<BaseStatistics> GetStatistics(ClientContext &context, column_t column_id) override;
	TableFunction GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) override;
	TableStorageInfo GetStorageInfo(ClientContext &context) override;
	void BindUpdateConstraints(Binder &binder, LogicalGet &get, LogicalProjection &proj, LogicalUpdate &update, ClientContext &context) override;
};

} // namespace duckdb

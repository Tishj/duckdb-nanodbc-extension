#include "storage/nanodbc_catalog.hpp"
#include "storage/nanodbc_table_entry.hpp"
#include "storage/nanodbc_transaction.hpp"
#include "duckdb/storage/statistics/base_statistics.hpp"
#include "duckdb/storage/table_storage_info.hpp"
#include "nanodbc_scanner.hpp"

namespace duckdb {

NanodbcTableEntry::NanodbcTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, CreateTableInfo &info)
    : TableCatalogEntry(catalog, schema, info) {
	for (idx_t c = 0; c < columns.LogicalColumnCount(); c++) {
		auto &col = columns.GetColumn(LogicalIndex(c));
		Nanodbc_types.push_back(NanodbcUtils::CreateEmptyNanodbcType(col.GetType()));
		Nanodbc_names.push_back(col.GetName());
	}
	approx_num_pages = 0;
}

NanodbcTableEntry::NanodbcTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, NanodbcTableInfo &info)
    : TableCatalogEntry(catalog, schema, *info.create_info), Nanodbc_types(std::move(info.nanodbc_types)),
      Nanodbc_names(std::move(info.nanodbc_names)) {
	D_ASSERT(Nanodbc_types.size() == columns.LogicalColumnCount());
	approx_num_pages = info.approx_num_pages;
}

unique_ptr<BaseStatistics> NanodbcTableEntry::GetStatistics(ClientContext &context, column_t column_id) {
	return nullptr;
}

void NanodbcTableEntry::BindUpdateConstraints(Binder &binder, LogicalGet &, LogicalProjection &, LogicalUpdate &,
                                               ClientContext &) {
}

TableFunction NanodbcTableEntry::GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) {
	auto &pg_catalog = catalog.Cast<NanodbcCatalog>();
	auto &transaction = Transaction::Get(context, catalog).Cast<NanodbcTransaction>();

	auto result = make_uniq<NanodbcBindData>();

	result->schema_name = schema.name;
	result->table_name = name;
	result->dsn = transaction.GetDSN();
	result->SetCatalog(pg_catalog);
	result->SetTable(*this);
	for (auto &col : columns.Logical()) {
		result->types.push_back(col.GetType());
	}
	result->names = Nanodbc_names;
	result->Nanodbc_types = Nanodbc_types;
	result->read_only = transaction.IsReadOnly();
	NanodbcScanFunction::PrepareBind(pg_catalog.GetNanodbcVersion(), context, *result, approx_num_pages);

	bind_data = std::move(result);
	auto function = NanodbcScanFunction();
	Value filter_pushdown;
	if (context.TryGetCurrentSetting("pg_experimental_filter_pushdown", filter_pushdown)) {
		function.filter_pushdown = BooleanValue::Get(filter_pushdown);
	}
	return function;
}

TableStorageInfo NanodbcTableEntry::GetStorageInfo(ClientContext &context) {
	auto &transaction = Transaction::Get(context, catalog).Cast<NanodbcTransaction>();
	auto &db = transaction.GetConnection();
	TableStorageInfo result;
	result.cardinality = 0;
	result.index_info = db.GetIndexInfo(name);
	return result;
}

} // namespace duckdb

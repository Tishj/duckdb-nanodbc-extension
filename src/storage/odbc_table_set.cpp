#include "duckdb/parser/constraints/not_null_constraint.hpp"
#include "duckdb/parser/constraints/unique_constraint.hpp"
#include "duckdb/parser/expression/constant_expression.hpp"
#include "duckdb/planner/parsed_data/bound_create_table_info.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/catalog/dependency_list.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "duckdb/parser/constraints/list.hpp"
#include "duckdb/parser/parser.hpp"

#include "storage/odbc_catalog.hpp"
#include "storage/odbc_table_set.hpp"
//#include "storage/odbc_table_entry.hpp"
#include "storage/odbc_transaction.hpp"
#include "storage/odbc_schema_entry.hpp"

namespace duckdb {

OdbcTableSet::OdbcTableSet(OdbcSchemaEntry &schema) : schema(schema), catalog(schema.ParentCatalog()) {
}

void OdbcTableSet::Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback) {
	lock_guard<mutex> l(entry_lock);
	LoadEntries(context);
	for (auto &entry : tables) {
		callback(*entry.second);
	}
}

void OdbcTableSet::LoadEntries(ClientContext &context) {
	if (!tables.empty()) {
		return;
	}

	auto &odbc_catalog = catalog.Cast<OdbcCatalog>();
	throw NotImplementedException("Nanodbc TableSet LoadEntries");
}

optional_ptr<CatalogEntry> OdbcTableSet::GetEntry(ClientContext &context, const EntryLookupInfo &lookup) {
	LoadEntries(context);
	lock_guard<mutex> l(entry_lock);
	auto entry = tables.find(lookup.GetEntryName());
	if (entry == tables.end()) {
		return nullptr;
	}
	return entry->second.get();
}

} // namespace duckdb

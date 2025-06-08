#include "storage/nanodbc_schema_set.hpp"
#include "storage/nanodbc_index_set.hpp"
#include "storage/nanodbc_table_set.hpp"
#include "storage/nanodbc_type_set.hpp"
#include "storage/nanodbc_transaction.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "storage/nanodbc_table_set.hpp"
#include "storage/nanodbc_catalog.hpp"
#include "duckdb/common/shared_ptr.hpp"

namespace duckdb {

NanodbcSchemaSet::NanodbcSchemaSet(Catalog &catalog, string schema_to_load_p)
    : NanodbcCatalogSet(catalog, false), schema_to_load(std::move(schema_to_load_p)) {
}

void NanodbcSchemaSet::LoadEntries(ClientContext &context) {
	auto &nanodbc_catalog = catalog.Cast<NanodbcCatalog>();

	throw NotImplementedException("LoadEntries not implemented");
}

optional_ptr<CatalogEntry> NanodbcSchemaSet::CreateSchema(ClientContext &context, CreateSchemaInfo &info) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);

	throw NotImplementedException("CreateSchema not implemented");

	return CreateEntry(std::move(schema_entry));
}

} // namespace duckdb

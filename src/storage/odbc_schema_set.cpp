#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/catalog/catalog.hpp"

#include "storage/odbc_catalog.hpp"
#include "storage/odbc_schema_set.hpp"
#include "storage/odbc_transaction.hpp"
//#include "storage/odbc_schema_entry.hpp"

namespace duckdb {

OdbcSchemaSet::OdbcSchemaSet(Catalog &catalog) : catalog(catalog) {
}

optional_ptr<CatalogEntry> OdbcSchemaSet::GetEntry(ClientContext &context, const string &name) {
	LoadEntries(context);
	lock_guard<mutex> l(entry_lock);
	auto entry = entries.find(name);
	if (entry == entries.end()) {
		return nullptr;
	}
	return entry->second.get();
}

void OdbcSchemaSet::Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback) {
	lock_guard<mutex> l(entry_lock);
	LoadEntries(context);
	for (auto &entry : entries) {
		callback(*entry.second);
	}
}

void OdbcSchemaSet::LoadEntries(ClientContext &context) {
	if (!entries.empty()) {
		return;
	}

	auto &ic_catalog = catalog.Cast<OdbcCatalog>();
	throw NotImplementedException("Nanodbc SchemaSet LoadEntries");
}

optional_ptr<CatalogEntry> OdbcSchemaSet::CreateEntryInternal(ClientContext &context, unique_ptr<CatalogEntry> entry) {
	auto result = entry.get();
	if (result->name.empty()) {
		throw InternalException("OdbcSchemaSet::CreateEntry called with empty name");
	}
	entries.insert(make_pair(result->name, std::move(entry)));
	return result;
}

} // namespace duckdb

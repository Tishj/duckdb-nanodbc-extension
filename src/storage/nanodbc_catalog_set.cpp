#include "storage/nanodbc_catalog_set.hpp"
#include "storage/nanodbc_transaction.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "storage/nanodbc_schema_entry.hpp"

namespace duckdb {

NanodbcCatalogSet::NanodbcCatalogSet(Catalog &catalog, bool is_loaded_p) : catalog(catalog), is_loaded(is_loaded_p) {
}

optional_ptr<CatalogEntry> NanodbcCatalogSet::GetEntry(ClientContext &context, const string &name) {
	TryLoadEntries(context);
	{
		lock_guard<mutex> l(entry_lock);
		auto entry = entries.find(name);
		if (entry != entries.end()) {
			// entry found
			return entry->second.get();
		}
	}
	// entry not found
	if (SupportReload()) {
		// try loading entries again - maybe there has been a change remotely
		auto entry = ReloadEntry(context, name);
		if (entry) {
			return entry;
		}
	}
	// check the case insensitive map if there are any entries
	auto name_entry = entry_map.find(name);
	if (name_entry == entry_map.end()) {
		// no entry found
		return nullptr;
	}
	// try again with the entry we found in the case insensitive map
	auto entry = entries.find(name_entry->second);
	if (entry == entries.end()) {
		// still not found
		return nullptr;
	}
	return entry->second.get();
}

void NanodbcCatalogSet::TryLoadEntries(ClientContext &context) {
	if (HasInternalDependencies()) {
		if (is_loaded) {
			return;
		}
	}
	lock_guard<mutex> lock(load_lock);
	if (is_loaded) {
		return;
	}
	is_loaded = true;
	LoadEntries(context);
}

optional_ptr<CatalogEntry> NanodbcCatalogSet::ReloadEntry(ClientContext &context, const string &name) {
	throw InternalException("NanodbcCatalogSet does not support ReloadEntry");
}

void NanodbcCatalogSet::DropEntry(ClientContext &context, DropInfo &info) {
	string drop_query = "DROP ";
	drop_query += CatalogTypeToString(info.type) + " ";
	if (info.if_not_found == OnEntryNotFound::RETURN_NULL) {
		drop_query += " IF EXISTS ";
	}
	if (!info.schema.empty()) {
		drop_query += KeywordHelper::WriteQuoted(info.schema, '"') + ".";
	}
	drop_query += KeywordHelper::WriteQuoted(info.name, '"');
	if (info.cascade) {
		drop_query += "CASCADE";
	}
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	transaction.Query(drop_query);

	// erase the entry from the catalog set
	lock_guard<mutex> l(entry_lock);
	entries.erase(info.name);
}

void NanodbcCatalogSet::Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback) {
	TryLoadEntries(context);
	lock_guard<mutex> l(entry_lock);
	for (auto &entry : entries) {
		callback(*entry.second);
	}
}

optional_ptr<CatalogEntry> NanodbcCatalogSet::CreateEntry(unique_ptr<CatalogEntry> entry) {
	lock_guard<mutex> l(entry_lock);
	auto result = entry.get();
	if (result->name.empty()) {
		throw InternalException("NanodbcCatalogSet::CreateEntry called with empty name");
	}
	entry_map.insert(make_pair(result->name, result->name));
	entries.insert(make_pair(result->name, std::move(entry)));
	return result;
}

void NanodbcCatalogSet::ClearEntries() {
	entry_map.clear();
	entries.clear();
	is_loaded = false;
}

NanodbcInSchemaSet::NanodbcInSchemaSet(NanodbcSchemaEntry &schema, bool is_loaded)
    : NanodbcCatalogSet(schema.ParentCatalog(), is_loaded), schema(schema) {
}

optional_ptr<CatalogEntry> NanodbcInSchemaSet::CreateEntry(unique_ptr<CatalogEntry> entry) {
	entry->internal = schema.internal;
	return NanodbcCatalogSet::CreateEntry(std::move(entry));
}

} // namespace duckdb

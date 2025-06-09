#include "storage/odbc_schema_entry.hpp"
#include "storage/odbc_transaction.hpp"

#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/parsed_data/create_index_info.hpp"
#include "duckdb/planner/parsed_data/bound_create_table_info.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/parser/constraints/list.hpp"
#include "duckdb/common/unordered_set.hpp"
#include "duckdb/parser/parsed_data/alter_info.hpp"
#include "duckdb/parser/parsed_data/alter_table_info.hpp"
#include "duckdb/parser/parsed_expression_iterator.hpp"

namespace duckdb {

OdbcSchemaEntry::OdbcSchemaEntry(Catalog &catalog, CreateSchemaInfo &info)
    : SchemaCatalogEntry(catalog, info), tables(*this) {
}

OdbcSchemaEntry::~OdbcSchemaEntry() {
}

OdbcTransaction &GetTransaction(CatalogTransaction transaction) {
	if (!transaction.transaction) {
		throw InternalException("No transaction!?");
	}
	return transaction.transaction->Cast<OdbcTransaction>();
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateTable(CatalogTransaction transaction, BoundCreateTableInfo &info) {
	throw NotImplementedException("Create Table");
}

void OdbcSchemaEntry::DropEntry(ClientContext &context, DropInfo &info) {
	throw NotImplementedException("Drop Entry");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateFunction(CatalogTransaction transaction, CreateFunctionInfo &info) {
	throw BinderException("Iceberg databases do not support creating functions");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateIndex(CatalogTransaction transaction, CreateIndexInfo &info,
                                                       TableCatalogEntry &table) {
	throw NotImplementedException("Create Index");
}

string GetUCCreateView(CreateViewInfo &info) {
	throw NotImplementedException("Get Create View");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateView(CatalogTransaction transaction, CreateViewInfo &info) {
	throw NotImplementedException("Create View");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateType(CatalogTransaction transaction, CreateTypeInfo &info) {
	throw BinderException("Iceberg databases do not support creating types");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateSequence(CatalogTransaction transaction, CreateSequenceInfo &info) {
	throw BinderException("Iceberg databases do not support creating sequences");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateTableFunction(CatalogTransaction transaction,
                                                               CreateTableFunctionInfo &info) {
	throw BinderException("Iceberg databases do not support creating table functions");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateCopyFunction(CatalogTransaction transaction,
                                                              CreateCopyFunctionInfo &info) {
	throw BinderException("Iceberg databases do not support creating copy functions");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreatePragmaFunction(CatalogTransaction transaction,
                                                                CreatePragmaFunctionInfo &info) {
	throw BinderException("Iceberg databases do not support creating pragma functions");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::CreateCollation(CatalogTransaction transaction, CreateCollationInfo &info) {
	throw BinderException("Iceberg databases do not support creating collations");
}

void OdbcSchemaEntry::Alter(CatalogTransaction transaction, AlterInfo &info) {
	throw NotImplementedException("Alter Schema Entry");
}

static bool CatalogTypeIsSupported(CatalogType type) {
	switch (type) {
	case CatalogType::TABLE_ENTRY:
	case CatalogType::VIEW_ENTRY:
		return true;
	default:
		return false;
	}
}

void OdbcSchemaEntry::Scan(ClientContext &context, CatalogType type, const std::function<void(CatalogEntry &)> &callback) {
	if (!CatalogTypeIsSupported(type)) {
		return;
	}
	GetCatalogSet(type).Scan(context, callback);
}
void OdbcSchemaEntry::Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) {
	throw NotImplementedException("Scan without context not supported");
}

optional_ptr<CatalogEntry> OdbcSchemaEntry::LookupEntry(CatalogTransaction transaction, const EntryLookupInfo &lookup_info) {
	auto type = lookup_info.GetCatalogType();
	if (!CatalogTypeIsSupported(type)) {
		return nullptr;
	}
	return GetCatalogSet(type).GetEntry(transaction.GetContext(), lookup_info);
}

OdbcTableSet &OdbcSchemaEntry::GetCatalogSet(CatalogType type) {
	switch (type) {
	case CatalogType::TABLE_ENTRY:
	case CatalogType::VIEW_ENTRY:
		return tables;
	default:
		throw InternalException("Type not supported for GetCatalogSet");
	}
}

} // namespace duckdb

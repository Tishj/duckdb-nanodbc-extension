#include "storage/nanodbc_schema_entry.hpp"
#include "storage/nanodbc_table_entry.hpp"
#include "storage/nanodbc_transaction.hpp"
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

NanodbcSchemaEntry::NanodbcSchemaEntry(Catalog &catalog, CreateSchemaInfo &info)
    : SchemaCatalogEntry(catalog, info), tables(*this), indexes(*this), types(*this) {
}

NanodbcSchemaEntry::NanodbcSchemaEntry(Catalog &catalog, CreateSchemaInfo &info)
    : SchemaCatalogEntry(catalog, info) {
}

bool NanodbcSchemaEntry::SchemaIsInternal(const string &name) {
	if (name == "information_schema" || StringUtil::StartsWith(name, "pg_")) {
		return true;
	}
	return false;
}

NanodbcTransaction &GetNanodbcTransaction(CatalogTransaction transaction) {
	if (!transaction.transaction) {
		throw InternalException("No transaction!?");
	}
	return transaction.transaction->Cast<NanodbcTransaction>();
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreateView(CatalogTransaction transaction, CreateViewInfo &info) {
	throw BinderException("Nanodbc databases do not support creating views");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreateType(CatalogTransaction transaction, CreateTypeInfo &info) {
	throw BinderException("Nanodbc databases do not support creating types");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreateSequence(CatalogTransaction transaction,
                                                               CreateSequenceInfo &info) {
	throw BinderException("Nanodbc databases do not support creating sequences");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreateTableFunction(CatalogTransaction transaction,
                                                                    CreateTableFunctionInfo &info) {
	throw BinderException("Nanodbc databases do not support creating table functions");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreateCopyFunction(CatalogTransaction transaction,
                                                                   CreateCopyFunctionInfo &info) {
	throw BinderException("Nanodbc databases do not support creating copy functions");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreatePragmaFunction(CatalogTransaction transaction,
                                                                     CreatePragmaFunctionInfo &info) {
	throw BinderException("Nanodbc databases do not support creating pragma functions");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::CreateCollation(CatalogTransaction transaction,
                                                                CreateCollationInfo &info) {
	throw BinderException("Nanodbc databases do not support creating collations");
}

void NanodbcSchemaEntry::Alter(CatalogTransaction transaction, AlterInfo &info) {
	throw BinderException("Nanodbc databases do not support altering entries");
}

bool CatalogTypeIsSupported(CatalogType type) {
	switch (type) {
	case CatalogType::VIEW_ENTRY:
	case CatalogType::TABLE_ENTRY:
		return true;
	default:
		return false;
	}
}

void NanodbcSchemaEntry::Scan(ClientContext &context, CatalogType type,
                               const std::function<void(CatalogEntry &)> &callback) {
	if (!CatalogTypeIsSupported(type)) {
		return;
	}
	GetCatalogSet(type).Scan(context, callback);
}
void NanodbcSchemaEntry::Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) {
	throw NotImplementedException("Scan without context not supported");
}

void NanodbcSchemaEntry::DropEntry(ClientContext &context, DropInfo &info) {
	throw BinderException("Nanodbc databases do not support dropping entries");
}

optional_ptr<CatalogEntry> NanodbcSchemaEntry::GetEntry(CatalogTransaction transaction, CatalogType type, const string &name) {
	if (!CatalogTypeIsSupported(type)) {
		return nullptr;
	}
	return GetCatalogSet(type).GetEntry(transaction.GetContext(), name);
}

NanodbcCatalogSet &NanodbcSchemaEntry::GetCatalogSet(CatalogType type) {
	switch (type) {
	case CatalogType::TABLE_ENTRY:
	case CatalogType::VIEW_ENTRY:
		return tables;
	default:
		throw InternalException("Type not supported for GetCatalogSet");
	}
}

} // namespace duckdb

//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_schema_entry.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "storage/nanodbc_table_set.hpp"

namespace duckdb {
class NanodbcTransaction;

class NanodbcSchemaEntry : public SchemaCatalogEntry {
public:
	NanodbcSchemaEntry(Catalog &catalog, CreateSchemaInfo &info);

public:
	optional_ptr<CatalogEntry> CreateTable(CatalogTransaction transaction, BoundCreateTableInfo &info) override;
	optional_ptr<CatalogEntry> CreateFunction(CatalogTransaction transaction, CreateFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreateIndex(CatalogTransaction transaction, CreateIndexInfo &info, TableCatalogEntry &table) override;
	optional_ptr<CatalogEntry> CreateView(CatalogTransaction transaction, CreateViewInfo &info) override;
	optional_ptr<CatalogEntry> CreateSequence(CatalogTransaction transaction, CreateSequenceInfo &info) override;
	optional_ptr<CatalogEntry> CreateTableFunction(CatalogTransaction transaction, CreateTableFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreateCopyFunction(CatalogTransaction transaction, CreateCopyFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreatePragmaFunction(CatalogTransaction transaction, CreatePragmaFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreateCollation(CatalogTransaction transaction, CreateCollationInfo &info) override;
	optional_ptr<CatalogEntry> CreateType(CatalogTransaction transaction, CreateTypeInfo &info) override;
	void Alter(CatalogTransaction transaction, AlterInfo &info) override;
	void Scan(ClientContext &context, CatalogType type, const std::function<void(CatalogEntry &)> &callback) override;
	void Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) override;
	void DropEntry(ClientContext &context, DropInfo &info) override;
	optional_ptr<CatalogEntry> GetEntry(CatalogTransaction transaction, CatalogType type, const string &name) override;

private:
	//void AlterTable(NanodbcTransaction &transaction, RenameTableInfo &info);
	//void AlterTable(NanodbcTransaction &transaction, RenameColumnInfo &info);
	//void AlterTable(NanodbcTransaction &transaction, AddColumnInfo &info);
	//void AlterTable(NanodbcTransaction &transaction, RemoveColumnInfo &info);
	NanodbcCatalogSet &GetCatalogSet(CatalogType type);

private:
	NanodbcTableSet tables;
	NanodbcIndexSet indexes;
	NanodbcTypeSet types;
};

} // namespace duckdb

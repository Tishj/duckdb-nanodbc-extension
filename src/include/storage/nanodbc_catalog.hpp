//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_catalog.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/common/enums/access_mode.hpp"
#include "odbc_connection.hpp"
#include "storage/nanodbc_schema_set.hpp"

namespace duckdb {
class NanodbcCatalog;
class NanodbcSchemaEntry;

class NanodbcCatalog : public Catalog {
public:
	explicit NanodbcCatalog(AttachedDatabase &db_p, string connection_string, string attach_path,
	                         AccessMode access_mode, string schema_to_load);
	~NanodbcCatalog();

	string connection_string;
	string attach_path;
	AccessMode access_mode;

public:
	void Initialize(bool load_builtin) override;
	string GetCatalogType() override {
		return "Nanodbc";
	}

	optional_ptr<CatalogEntry> CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) override;

	void ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) override;

	optional_ptr<SchemaCatalogEntry> GetSchema(CatalogTransaction transaction, const string &schema_name,
	                                           OnEntryNotFound if_not_found,
	                                           QueryErrorContext error_context = QueryErrorContext()) override;

	unique_ptr<PhysicalOperator> PlanInsert(ClientContext &context, LogicalInsert &op, unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<PhysicalOperator> PlanCreateTableAs(ClientContext &context, LogicalCreateTable &op, unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<PhysicalOperator> PlanDelete(ClientContext &context, LogicalDelete &op, unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<PhysicalOperator> PlanUpdate(ClientContext &context, LogicalUpdate &op, unique_ptr<PhysicalOperator> plan) override;
	unique_ptr<LogicalOperator> BindCreateIndex(Binder &binder, CreateStatement &stmt, TableCatalogEntry &table, unique_ptr<LogicalOperator> plan) override;

	//! Whether or not this is an in-memory Nanodbc database
	bool InMemory() override;
	string GetDBPath() override;

private:
	void DropSchema(ClientContext &context, DropInfo &info) override;

private:
	NanodbcSchemaSet schemas;
	string default_schema;
};

} // namespace duckdb

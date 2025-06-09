#pragma once

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/common/enums/access_mode.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include "duckdb/parser/parsed_data/attach_info.hpp"
#include "duckdb/storage/storage_extension.hpp"

namespace duckdb {

class OdbcCatalog : public Catalog {
public:
	explicit OdbcCatalog(AttachedDatabase &db_p, AccessMode access_mode);
	~OdbcCatalog() override;
public:
	static unique_ptr<Catalog> Attach(StorageExtensionInfo *storage_info, ClientContext &context, AttachedDatabase &db, const string &name, AttachInfo &info, AccessMode access_mode);

public:
	void Initialize(bool load_builtin) override;
	string GetCatalogType() override {
		return "odbc";
	}
	void DropSchema(ClientContext &context, DropInfo &info) override;
	optional_ptr<CatalogEntry> CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) override;
	void ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) override;
	optional_ptr<SchemaCatalogEntry> LookupSchema(CatalogTransaction transaction, const EntryLookupInfo &schema_lookup, OnEntryNotFound if_not_found) override;
	PhysicalOperator &PlanInsert(ClientContext &context, PhysicalPlanGenerator &planner, LogicalInsert &op, optional_ptr<PhysicalOperator> plan) override;
	PhysicalOperator &PlanCreateTableAs(ClientContext &context, PhysicalPlanGenerator &planner, LogicalCreateTable &op, PhysicalOperator &plan) override;
	PhysicalOperator &PlanDelete(ClientContext &context, PhysicalPlanGenerator &planner, LogicalDelete &op, PhysicalOperator &plan) override;
	PhysicalOperator &PlanUpdate(ClientContext &context, PhysicalPlanGenerator &planner, LogicalUpdate &op, PhysicalOperator &plan) override;
	unique_ptr<LogicalOperator> BindCreateIndex(Binder &binder, CreateStatement &stmt, TableCatalogEntry &table, unique_ptr<LogicalOperator> plan) override;
	DatabaseSize GetDatabaseSize(ClientContext &context) override;
	//! Whether or not this is an in-memory Iceberg database
	bool InMemory() override;
	string GetDBPath() override;

public:
	AccessMode access_mode;
	string connection_string;
};

} // namespace duckdb

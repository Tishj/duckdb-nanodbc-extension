#include "storage/odbc_catalog.hpp"
#include "storage/odbc_transaction.hpp"

namespace duckdb {

OdbcCatalog::OdbcCatalog(AttachedDatabase &db_p, AccessMode access_mode)
    : Catalog(db_p), access_mode(access_mode) {
}

OdbcCatalog::~OdbcCatalog() = default;

//===--------------------------------------------------------------------===//
// Catalog API
//===--------------------------------------------------------------------===//

void OdbcCatalog::Initialize(bool load_builtin) {
}

void OdbcCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	auto &transaction = OdbcTransaction::Get(context, *this);

	throw NotImplementedException("Nanodbc ScanSchemas");
	//auto &schemas = transaction.GetSchemas();
	//schemas.Scan(context, [&](CatalogEntry &schema) {
	//	callback(schema.Cast<OdbcSchemaEntry>());
	//});
}

optional_ptr<SchemaCatalogEntry> OdbcCatalog::LookupSchema(CatalogTransaction transaction,
                                                         const EntryLookupInfo &schema_lookup,
                                                         OnEntryNotFound if_not_found) {
	auto &odbc_transaction = OdbcTransaction::Get(transaction.GetContext(), *this);

	throw NotImplementedException("Nanodbc LookupSchema");
	//auto &schemas = odbc_transaction.GetSchemas();

	//auto &schema_name = schema_lookup.GetEntryName();
	//auto entry = schemas.GetEntry(transaction.GetContext(), schema_name);
	//if (!entry && if_not_found != OnEntryNotFound::RETURN_NULL) {
	//	throw CatalogException(schema_lookup.GetErrorContext(), "Schema with name \"%s\" not found", schema_name);
	//}

	//return reinterpret_cast<SchemaCatalogEntry *>(entry.get());
}

optional_ptr<CatalogEntry> OdbcCatalog::CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) {
	throw NotImplementedException("OdbcCatalog::CreateSchema not implemented");
}

void OdbcCatalog::DropSchema(ClientContext &context, DropInfo &info) {
	throw NotImplementedException("OdbcCatalog::DropSchema not implemented");
}

PhysicalOperator &OdbcCatalog::PlanInsert(ClientContext &context, PhysicalPlanGenerator &planner, LogicalInsert &op,
                                        optional_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("OdbcCatalog PlanInsert");
}
PhysicalOperator &OdbcCatalog::PlanCreateTableAs(ClientContext &context, PhysicalPlanGenerator &planner,
                                               LogicalCreateTable &op, PhysicalOperator &plan) {
	throw NotImplementedException("OdbcCatalog PlanCreateTableAs");
}
PhysicalOperator &OdbcCatalog::PlanDelete(ClientContext &context, PhysicalPlanGenerator &planner, LogicalDelete &op,
                                        PhysicalOperator &plan) {
	throw NotImplementedException("OdbcCatalog PlanDelete");
}
PhysicalOperator &OdbcCatalog::PlanUpdate(ClientContext &context, PhysicalPlanGenerator &planner, LogicalUpdate &op,
                                        PhysicalOperator &plan) {
	throw NotImplementedException("OdbcCatalog PlanUpdate");
}
unique_ptr<LogicalOperator> OdbcCatalog::BindCreateIndex(Binder &binder, CreateStatement &stmt, TableCatalogEntry &table,
                                                       unique_ptr<LogicalOperator> plan) {
	throw NotImplementedException("OdbcCatalog BindCreateIndex");
}

bool OdbcCatalog::InMemory() {
	return false;
}

string OdbcCatalog::GetDBPath() {
	return connection_string;
}

DatabaseSize OdbcCatalog::GetDatabaseSize(ClientContext &context) {
	DatabaseSize size;
	return size;
}

//===--------------------------------------------------------------------===//
// Attach
//===--------------------------------------------------------------------===//

unique_ptr<Catalog> OdbcCatalog::Attach(StorageExtensionInfo *storage_info, ClientContext &context, AttachedDatabase &db, const string &name, AttachInfo &info, AccessMode access_mode) {
	//! First handle generic attach options
	for (auto &entry : info.options) {
		auto lower_name = StringUtil::Lower(entry.first);
		if (lower_name == "type" || lower_name == "read_only") {
			continue;
		}

		// TODO: deal with any attach options here
	}

	auto catalog = make_uniq<OdbcCatalog>(db, access_mode);
	return std::move(catalog);
}

} // namespace duckdb

#include "duckdb.hpp"

#include "nanodbc_storage.hpp"
#include "storage/nanodbc_catalog.hpp"
#include "duckdb/parser/parsed_data/attach_info.hpp"
#include "storage/nanodbc_transaction_manager.hpp"

namespace duckdb {

static unique_ptr<Catalog> NanodbcAttach(StorageExtensionInfo *storage_info, ClientContext &context,
                                          AttachedDatabase &db, const string &name, AttachInfo &info,
                                          AccessMode access_mode) {
	auto &config = DBConfig::GetConfig(context);
	if (!config.options.enable_external_access) {
		throw PermissionException("Attaching Nanodbc databases is disabled through configuration");
	}
	string attach_path = info.path;

	string secret_name;
	string schema_to_load;
	for (auto &entry : info.options) {
		auto lower_name = StringUtil::Lower(entry.first);
		if (lower_name == "type" || lower_name == "read_only") {
			// already handled
		} else if (lower_name == "secret") {
			secret_name = entry.second.ToString();
		} else if (lower_name == "schema") {
			schema_to_load = entry.second.ToString();
		} else {
			throw BinderException("Unrecognized option for Nanodbc attach: %s", entry.first);
		}
	}
	auto connection_string = NanodbcCatalog::GetConnectionString(context, attach_path, secret_name);
	return make_uniq<NanodbcCatalog>(db, std::move(connection_string), std::move(attach_path), access_mode, std::move(schema_to_load));
}

static unique_ptr<TransactionManager> NanodbcCreateTransactionManager(StorageExtensionInfo *storage_info,
                                                                       AttachedDatabase &db, Catalog &catalog) {
	auto &nanodbc_catalog = catalog.Cast<NanodbcCatalog>();
	return make_uniq<NanodbcTransactionManager>(db, nanodbc_catalog);
}

NanodbcStorageExtension::NanodbcStorageExtension() {
	attach = NanodbcAttach;
	create_transaction_manager = NanodbcCreateTransactionManager;
}

} // namespace duckdb

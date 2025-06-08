#include "storage/nanodbc_catalog.hpp"
#include "storage/nanodbc_schema_entry.hpp"
#include "storage/nanodbc_transaction.hpp"
#include "nanodbc_connection.hpp"
#include "duckdb/storage/database_size.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "duckdb/main/attached_database.hpp"
#include "duckdb/main/secret/secret_manager.hpp"

namespace duckdb {

NanodbcCatalog::NanodbcCatalog(AttachedDatabase &db_p, string connection_string_p, string attach_path_p, AccessMode access_mode, string schema_to_load)
    : Catalog(db_p), connection_string(std::move(connection_string_p)), attach_path(std::move(attach_path_p)), access_mode(access_mode), schemas(*this, schema_to_load),
      default_schema(schema_to_load) {
	if (default_schema.empty()) {
		default_schema = "public";
	}
	Value connection_limit;
	auto &db_instance = db_p.GetDatabase();

	auto connection = connection_pool.GetConnection();
}

string EscapeConnectionString(const string &input) {
	string result = "'";
	for (auto c : input) {
		if (c == '\\') {
			result += "\\\\";
		} else if (c == '\'') {
			result += "\\'";
		} else {
			result += c;
		}
	}
	result += "'";
	return result;
}

string AddConnectionOption(const KeyValueSecret &kv_secret, const string &name) {
	Value input_val = kv_secret.TryGetValue(name);
	if (input_val.IsNull()) {
		// not provided
		return string();
	}
	string result;
	result += name;
	result += "=";
	result += EscapeConnectionString(input_val.ToString());
	result += " ";
	return result;
}

unique_ptr<SecretEntry> GetSecret(ClientContext &context, const string &secret_name) {
	auto &secret_manager = SecretManager::Get(context);
	auto transaction = CatalogTransaction::GetSystemCatalogTransaction(context);
	// FIXME: this should be adjusted once the `GetSecretByName` API supports this use case
	auto secret_entry = secret_manager.GetSecretByName(transaction, secret_name, "memory");
	if (secret_entry) {
		return secret_entry;
	}
	secret_entry = secret_manager.GetSecretByName(transaction, secret_name, "local_file");
	if (secret_entry) {
		return secret_entry;
	}
	return nullptr;
}

string NanodbcCatalog::GetConnectionString(ClientContext &context, const string &attach_path, string secret_name) {
	// if no secret is specified we default to the unnamed Nanodbc secret, if it exists
	string connection_string = attach_path;
	bool explicit_secret = !secret_name.empty();
	if (!explicit_secret) {
		// look up settings from the default unnamed Nanodbc secret if none is provided
		secret_name = "__default_Nanodbc";
	}

	auto secret_entry = GetSecret(context, secret_name);
	if (secret_entry) {
		// secret found - read data
		const auto &kv_secret = dynamic_cast<const KeyValueSecret &>(*secret_entry->secret);
		string new_connection_info;

		new_connection_info += AddConnectionOption(kv_secret, "user");
		new_connection_info += AddConnectionOption(kv_secret, "password");
		new_connection_info += AddConnectionOption(kv_secret, "host");
		new_connection_info += AddConnectionOption(kv_secret, "port");
		new_connection_info += AddConnectionOption(kv_secret, "dbname");

		connection_string = new_connection_info + connection_string;
	} else if (explicit_secret) {
		// secret not found and one was explicitly provided - throw an error
		throw BinderException("Secret with name \"%s\" not found", secret_name);
	}
	return connection_string;
}


NanodbcCatalog::~NanodbcCatalog() = default;

void NanodbcCatalog::Initialize(bool load_builtin) {
}

optional_ptr<CatalogEntry> NanodbcCatalog::CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) {
	auto &Nanodbc_transaction = NanodbcTransaction::Get(transaction.GetContext(), *this);
	auto entry = schemas.GetEntry(transaction.GetContext(), info.schema);
	if (entry) {
		switch (info.on_conflict) {
		case OnCreateConflict::REPLACE_ON_CONFLICT: {
			DropInfo try_drop;
			try_drop.type = CatalogType::SCHEMA_ENTRY;
			try_drop.name = info.schema;
			try_drop.if_not_found = OnEntryNotFound::RETURN_NULL;
			try_drop.cascade = false;
			schemas.DropEntry(transaction.GetContext(), try_drop);
			break;
		}
		case OnCreateConflict::IGNORE_ON_CONFLICT:
			return entry;
		case OnCreateConflict::ERROR_ON_CONFLICT:
		default:
			throw BinderException("Failed to create schema \"%s\": schema already exists", info.schema);
		}
	}
	return schemas.CreateSchema(transaction.GetContext(), info);
}

void NanodbcCatalog::DropSchema(ClientContext &context, DropInfo &info) {
	return schemas.DropEntry(context, info);
}

void NanodbcCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	schemas.Scan(context, [&](CatalogEntry &schema) { callback(schema.Cast<NanodbcSchemaEntry>()); });
}

optional_ptr<SchemaCatalogEntry> NanodbcCatalog::GetSchema(CatalogTransaction transaction, const string &schema_name,
                                                            OnEntryNotFound if_not_found,
                                                            QueryErrorContext error_context) {
	if (schema_name == DEFAULT_SCHEMA) {
		return GetSchema(transaction, default_schema, if_not_found, error_context);
	}
	auto &Nanodbc_transaction = NanodbcTransaction::Get(transaction.GetContext(), *this);
	if (schema_name == "pg_temp") {
		return GetSchema(transaction, Nanodbc_transaction.GetTemporarySchema(), if_not_found, error_context);
	}
	auto entry = schemas.GetEntry(transaction.GetContext(), schema_name);
	if (!entry && if_not_found != OnEntryNotFound::RETURN_NULL) {
		throw BinderException("Schema with name \"%s\" not found", schema_name);
	}
	return reinterpret_cast<SchemaCatalogEntry *>(entry.get());
}

bool NanodbcCatalog::InMemory() {
	return false;
}

string NanodbcCatalog::GetDBPath() {
	return attach_path;
}

} // namespace duckdb
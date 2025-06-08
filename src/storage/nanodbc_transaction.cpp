#include "storage/nanodbc_transaction.hpp"
#include "storage/nanodbc_catalog.hpp"
#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/catalog/catalog_entry/index_catalog_entry.hpp"
#include "duckdb/catalog/catalog_entry/view_catalog_entry.hpp"
#include "nanodbc_result.hpp"

namespace duckdb {

NanodbcTransaction::NanodbcTransaction(NanodbcCatalog &nanodbc_catalog, TransactionManager &manager,
                                         ClientContext &context)
    : Transaction(manager, context), access_mode(nanodbc_catalog.access_mode) {
	connection = nanodbc_catalog.GetConnectionPool().GetConnection();
}

NanodbcTransaction::~NanodbcTransaction() = default;

void NanodbcTransaction::Start() {
	transaction_state = NanodbcTransactionState::TRANSACTION_NOT_YET_STARTED;
}
void NanodbcTransaction::Commit() {
	if (transaction_state == NanodbcTransactionState::TRANSACTION_STARTED) {
		transaction_state = NanodbcTransactionState::TRANSACTION_FINISHED;
		GetConnectionRaw().Execute("COMMIT");
	}
}
void NanodbcTransaction::Rollback() {
	if (transaction_state == NanodbcTransactionState::TRANSACTION_STARTED) {
		transaction_state = NanodbcTransactionState::TRANSACTION_FINISHED;
		GetConnectionRaw().Execute("ROLLBACK");
	}
}

static string GetBeginTransactionQuery(AccessMode access_mode) {
	string result = "BEGIN TRANSACTION ISOLATION LEVEL REPEATABLE READ";
	if (access_mode == AccessMode::READ_ONLY) {
		result += " READ ONLY";
	}
	return result;
}

NanodbcConnection &NanodbcTransaction::GetConnection() {
	auto &con = GetConnectionRaw();
	if (transaction_state == NanodbcTransactionState::TRANSACTION_NOT_YET_STARTED) {
		transaction_state = NanodbcTransactionState::TRANSACTION_STARTED;
		string query = GetBeginTransactionQuery(access_mode);
		con.Execute(query);
	}
	return con;
}

NanodbcConnection &NanodbcTransaction::GetConnectionRaw() {
	return connection.GetConnection();
}

string NanodbcTransaction::GetDSN() {
	return GetConnectionRaw().GetDSN();
}

unique_ptr<NanodbcResult> NanodbcTransaction::Query(const string &query) {
	auto &con = GetConnectionRaw();
	if (transaction_state == NanodbcTransactionState::TRANSACTION_NOT_YET_STARTED) {
		transaction_state = NanodbcTransactionState::TRANSACTION_STARTED;
		string transaction_start = GetBeginTransactionQuery(access_mode);
		transaction_start += ";\n";
		return con.Query(transaction_start + query);
	}
	return con.Query(query);
}

vector<unique_ptr<NanodbcResult>> NanodbcTransaction::ExecuteQueries(const string &queries) {
	auto &con = GetConnectionRaw();
	if (transaction_state == NanodbcTransactionState::TRANSACTION_NOT_YET_STARTED) {
		transaction_state = NanodbcTransactionState::TRANSACTION_STARTED;
		string transaction_start = GetBeginTransactionQuery(access_mode);
		transaction_start += ";\n";
		return con.ExecuteQueries(transaction_start + queries);
	}
	return con.ExecuteQueries(queries);
}

string NanodbcTransaction::GetTemporarySchema() {
	if (temporary_schema.empty()) {
		auto result = Query("SELECT nspname FROM pg_namespace WHERE oid = pg_my_temp_schema();");
		if (result->Count() < 1) {
			// no temporary tables exist yet in this connection
			// create a random temporary table and return
			Query("CREATE TEMPORARY TABLE __internal_temporary_table(i INTEGER)");
			result = Query("SELECT nspname FROM pg_namespace WHERE oid = pg_my_temp_schema();");
			if (result->Count() < 1) {
				throw BinderException("Could not find temporary schema pg_temp_NNN for this connection");
			}
		}
		temporary_schema = result->GetString(0, 0);
	}
	return temporary_schema;
}

NanodbcTransaction &NanodbcTransaction::Get(ClientContext &context, Catalog &catalog) {
	return Transaction::Get(context, catalog).Cast<NanodbcTransaction>();
}

} // namespace duckdb

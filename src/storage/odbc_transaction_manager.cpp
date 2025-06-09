#include "storage/odbc_transaction_manager.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

OdbcTransactionManager::OdbcTransactionManager(AttachedDatabase &db_p, OdbcCatalog &catalog)
    : TransactionManager(db_p), catalog(catalog) {
}

Transaction &OdbcTransactionManager::StartTransaction(ClientContext &context) {
	auto transaction = make_uniq<OdbcTransaction>(catalog, *this, context);
	transaction->Start();
	auto &result = *transaction;
	lock_guard<mutex> l(transaction_lock);
	transactions[result] = std::move(transaction);
	return result;
}

ErrorData OdbcTransactionManager::CommitTransaction(ClientContext &context, Transaction &transaction) {
	auto &ic_transaction = transaction.Cast<OdbcTransaction>();
	ic_transaction.Commit();
	lock_guard<mutex> l(transaction_lock);
	transactions.erase(transaction);
	return ErrorData();
}

void OdbcTransactionManager::RollbackTransaction(Transaction &transaction) {
	auto &ic_transaction = transaction.Cast<OdbcTransaction>();
	ic_transaction.Rollback();
	lock_guard<mutex> l(transaction_lock);
	transactions.erase(transaction);
}

void OdbcTransactionManager::Checkpoint(ClientContext &context, bool force) {
	auto &transaction = OdbcTransaction::Get(context, db.GetCatalog());
	// auto &db = transaction.GetConnection();
	// db.Execute("CHECKPOINT");
}

} // namespace duckdb

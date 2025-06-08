#include "storage/nanodbc_transaction_manager.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

NanodbcTransactionManager::NanodbcTransactionManager(AttachedDatabase &db_p, NanodbcCatalog &nanodbc_catalog)
    : TransactionManager(db_p), nanodbc_catalog(nanodbc_catalog) {
}

Transaction &NanodbcTransactionManager::StartTransaction(ClientContext &context) {
	auto transaction = make_uniq<NanodbcTransaction>(nanodbc_catalog, *this, context);
	transaction->Start();
	auto &result = *transaction;
	lock_guard<mutex> l(transaction_lock);
	transactions[result] = std::move(transaction);
	return result;
}

ErrorData NanodbcTransactionManager::CommitTransaction(ClientContext &context, Transaction &transaction) {
	auto &nanodbc_transaction = transaction.Cast<NanodbcTransaction>();
	nanodbc_transaction.Commit();
	lock_guard<mutex> l(transaction_lock);
	transactions.erase(transaction);
	return ErrorData();
}

void NanodbcTransactionManager::RollbackTransaction(Transaction &transaction) {
	auto &nanodbc_transaction = transaction.Cast<NanodbcTransaction>();
	nanodbc_transaction.Rollback();
	lock_guard<mutex> l(transaction_lock);
	transactions.erase(transaction);
}

void NanodbcTransactionManager::Checkpoint(ClientContext &context, bool force) {
	auto &transaction = NanodbcTransaction::Get(context, db.GetCatalog());
	auto &db = transaction.GetConnection();
	db.Execute("CHECKPOINT");
}

} // namespace duckdb

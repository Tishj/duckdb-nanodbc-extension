
#pragma once

#include "duckdb/transaction/transaction_manager.hpp"
#include "storage/odbc_catalog.hpp"
#include "storage/odbc_transaction.hpp"

namespace duckdb {

class OdbcTransactionManager : public TransactionManager {
public:
	OdbcTransactionManager(AttachedDatabase &db_p, OdbcCatalog &catalog);

	Transaction &StartTransaction(ClientContext &context) override;
	ErrorData CommitTransaction(ClientContext &context, Transaction &transaction) override;
	void RollbackTransaction(Transaction &transaction) override;

	void Checkpoint(ClientContext &context, bool force = false) override;

private:
	OdbcCatalog &catalog;
	mutex transaction_lock;
	reference_map_t<Transaction, unique_ptr<OdbcTransaction>> transactions;
};

} // namespace duckdb

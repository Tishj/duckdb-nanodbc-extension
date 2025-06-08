//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_transaction_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/transaction/transaction_manager.hpp"
#include "storage/nanodbc_catalog.hpp"
#include "storage/nanodbc_transaction.hpp"
#include "duckdb/common/reference_map.hpp"

namespace duckdb {

class NanodbcTransactionManager : public TransactionManager {
public:
	NanodbcTransactionManager(AttachedDatabase &db_p, NanodbcCatalog &Nanodbc_catalog);

	Transaction &StartTransaction(ClientContext &context) override;
	ErrorData CommitTransaction(ClientContext &context, Transaction &transaction) override;
	void RollbackTransaction(Transaction &transaction) override;

	void Checkpoint(ClientContext &context, bool force = false) override;

private:
	NanodbcCatalog &Nanodbc_catalog;
	mutex transaction_lock;
	reference_map_t<Transaction, unique_ptr<NanodbcTransaction>> transactions;
};

} // namespace duckdb

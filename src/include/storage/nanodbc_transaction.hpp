//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/nanodbc_transaction.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/transaction/transaction.hpp"
#include "nanodbc_connection.hpp"
#include "storage/nanodbc_connection_pool.hpp"

namespace duckdb {
class NanodbcCatalog;
class NanodbcSchemaEntry;
class NanodbcTableEntry;

enum class NanodbcTransactionState { TRANSACTION_NOT_YET_STARTED, TRANSACTION_STARTED, TRANSACTION_FINISHED };

class NanodbcTransaction : public Transaction {
public:
	NanodbcTransaction(NanodbcCatalog &Nanodbc_catalog, TransactionManager &manager, ClientContext &context);
	~NanodbcTransaction() override;

	void Start();
	void Commit();
	void Rollback();

	NanodbcConnection &GetConnection();
	string GetDSN();
	unique_ptr<NanodbcResult> Query(const string &query);
	vector<unique_ptr<NanodbcResult>> ExecuteQueries(const string &queries);
	static NanodbcTransaction &Get(ClientContext &context, Catalog &catalog);

	string GetTemporarySchema();

private:
	NanodbcTransactionState transaction_state;
	AccessMode access_mode;
	string temporary_schema;

private:
	//! Retrieves the connection **without** starting a transaction if none is active
	NanodbcConnection &GetConnectionRaw();
};

} // namespace duckdb

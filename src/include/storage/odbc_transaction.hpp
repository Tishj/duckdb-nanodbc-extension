
#pragma once

#include "duckdb/transaction/transaction.hpp"
#include "storage/odbc_schema_set.hpp"

namespace duckdb {
class OdbcCatalog;

enum class OdbcTransactionState { TRANSACTION_NOT_YET_STARTED, TRANSACTION_STARTED, TRANSACTION_FINISHED };

class OdbcTransaction : public Transaction {
public:
	OdbcTransaction(OdbcCatalog &catalog, TransactionManager &manager, ClientContext &context);
	~OdbcTransaction() override;

public:
	void Start();
	void Commit();
	void Rollback();
	static OdbcTransaction &Get(ClientContext &context, Catalog &catalog);
	AccessMode GetAccessMode() const {
		return access_mode;
	}
	OdbcSchemaSet &GetSchemas() {
		return schemas;
	}
public:
	OdbcSchemaSet schemas;
private:
	OdbcTransactionState transaction_state;
	AccessMode access_mode;
};

} // namespace duckdb

#include "storage/odbc_transaction.hpp"
#include "storage/odbc_catalog.hpp"

namespace duckdb {

OdbcTransaction::OdbcTransaction(OdbcCatalog &catalog, TransactionManager &manager, ClientContext &context)
    : Transaction(manager, context), schemas(catalog), access_mode(catalog.access_mode) {
	//	connection = ICConnection::Open(catalog.path);
}

OdbcTransaction::~OdbcTransaction() = default;

void OdbcTransaction::Start() {
	transaction_state = OdbcTransactionState::TRANSACTION_NOT_YET_STARTED;
}
void OdbcTransaction::Commit() {
	if (transaction_state == OdbcTransactionState::TRANSACTION_STARTED) {
		transaction_state = OdbcTransactionState::TRANSACTION_FINISHED;
		//		connection.Execute("COMMIT");
	}
}
void OdbcTransaction::Rollback() {
	if (transaction_state == OdbcTransactionState::TRANSACTION_STARTED) {
		transaction_state = OdbcTransactionState::TRANSACTION_FINISHED;
		//		connection.Execute("ROLLBACK");
	}
}

OdbcTransaction &OdbcTransaction::Get(ClientContext &context, Catalog &catalog) {
	return Transaction::Get(context, catalog).Cast<OdbcTransaction>();
}

} // namespace duckdb

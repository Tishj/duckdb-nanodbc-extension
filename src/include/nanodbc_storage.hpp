//===----------------------------------------------------------------------===//
//                         DuckDB
//
// nanodbc_storage.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/storage/storage_extension.hpp"

namespace duckdb {

class NanodbcStorageExtension : public StorageExtension {
public:
	NanodbcStorageExtension();
};

} // namespace duckdb

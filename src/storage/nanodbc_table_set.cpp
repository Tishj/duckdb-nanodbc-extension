#include "storage/nanodbc_table_set.hpp"
#include "storage/nanodbc_transaction.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "duckdb/parser/constraints/not_null_constraint.hpp"
#include "duckdb/parser/constraints/unique_constraint.hpp"
#include "duckdb/parser/expression/constant_expression.hpp"
#include "duckdb/planner/parsed_data/bound_create_table_info.hpp"
#include "duckdb/catalog/dependency_list.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "duckdb/parser/constraints/list.hpp"
#include "storage/nanodbc_schema_entry.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/common/string_util.hpp"
#include "nanodbc_conversion.hpp"

namespace duckdb {

NanodbcTableSet::NanodbcTableSet(NanodbcSchemaEntry &schema, unique_ptr<NanodbcResultSlice> table_result_p)
    : NanodbcInSchemaSet(schema, !table_result_p), table_result(std::move(table_result_p)) {
}

void NanodbcTableSet::AddColumn(optional_ptr<NanodbcTransaction> transaction, optional_ptr<NanodbcSchemaEntry> schema, NanodbcResult &result, idx_t row, NanodbcTableInfo &table_info) {
	NanodbcTypeData type_info;
	idx_t column_index = 3;
	auto column_name = result.GetString(row, column_index);
	type_info.type_name = result.GetString(row, column_index + 1);
	type_info.type_modifier = result.GetInt64(row, column_index + 2);
	type_info.array_dimensions = result.GetInt64(row, column_index + 3);
	bool is_not_null = result.GetBool(row, column_index + 5);
	string default_value;

	NanodbcType nanodbc_type;
	auto column_type = NanodbcUtils::TypeToLogicalType(transaction, schema, type_info, nanodbc_type);
	table_info.nanodbc_types.push_back(std::move(nanodbc_type));
	table_info.nanodbc_names.push_back(column_name);
	ColumnDefinition column(std::move(column_name), std::move(column_type));
	if (!default_value.empty()) {
		auto expressions = Parser::ParseExpressionList(default_value);
		if (expressions.empty()) {
			throw InternalException("Expression list is empty");
		}
		column.SetDefaultValue(std::move(expressions[0]));
	}
	auto &create_info = *table_info.create_info;
	if (is_not_null) {
		create_info.constraints.push_back(
		    make_uniq<NotNullConstraint>(LogicalIndex(create_info.columns.PhysicalColumnCount())));
	}
	create_info.columns.AddColumn(std::move(column));
}

void NanodbcTableSet::AddConstraint(NanodbcResult &result, idx_t row, NanodbcTableInfo &table_info) {
	idx_t column_index = 9;
	auto constraint_type = result.GetString(row, column_index + 1);
	auto constraint_key = result.GetString(row, column_index + 2);
	if (constraint_key.empty() || constraint_key.front() != '{' || constraint_key.back() != '}') {
		// invalid constraint key
		D_ASSERT(0);
		return;
	}

	auto &create_info = *table_info.create_info;
	auto splits = StringUtil::Split(constraint_key.substr(1, constraint_key.size() - 2), ",");
	vector<string> columns;
	for (auto &split : splits) {
		auto index = std::stoull(split);
		if (index <= 0 || index > create_info.columns.LogicalColumnCount()) {
			return;
		}
		columns.push_back(create_info.columns.GetColumn(LogicalIndex(index - 1)).Name());
	}

	create_info.constraints.push_back(make_uniq<UniqueConstraint>(std::move(columns), constraint_type == "p"));
}

void NanodbcTableSet::AddColumnOrConstraint(optional_ptr<NanodbcTransaction> transaction,
                                             optional_ptr<NanodbcSchemaEntry> schema, NanodbcResult &result,
                                             idx_t row, NanodbcTableInfo &table_info) {
	if (result.IsNull(row, 3)) {
		// constraint
		AddConstraint(result, row, table_info);
	} else {
		AddColumn(transaction, schema, result, row, table_info);
	}
}

void NanodbcTableSet::CreateEntries(NanodbcTransaction &transaction, NanodbcResult &result, idx_t start, idx_t end) {
	vector<unique_ptr<NanodbcTableInfo>> tables;
	unique_ptr<NanodbcTableInfo> info;

	for (idx_t row = start; row < end; row++) {
		auto table_name = result.GetString(row, 1);
		if (!info || info->GetTableName() != table_name) {
			if (info) {
				tables.push_back(std::move(info));
			}
			auto approx_num_pages = result.IsNull(row, 2) ? 0 : result.GetInt64(row, 2);
			info = make_uniq<NanodbcTableInfo>(schema, table_name);
			info->approx_num_pages = approx_num_pages;
		}
		AddColumnOrConstraint(&transaction, &schema, result, row, *info);
	}
	if (info) {
		tables.push_back(std::move(info));
	}
	for (auto &tbl_info : tables) {
		auto table_entry = make_uniq<NanodbcTableEntry>(catalog, schema, *tbl_info);
		CreateEntry(std::move(table_entry));
	}
}

void NanodbcTableSet::LoadEntries(ClientContext &context) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	if (table_result) {
		CreateEntries(transaction, table_result->GetResult(), table_result->start, table_result->end);
		table_result.reset();
	} else {
		auto query = GetInitializeQuery(schema.name);

		auto result = transaction.Query(query);
		auto rows = result->Count();

		CreateEntries(transaction, *result, 0, rows);
	}
}

unique_ptr<NanodbcTableInfo> NanodbcTableSet::GetTableInfo(NanodbcTransaction &transaction,
                                                             NanodbcSchemaEntry &schema, const string &table_name) {
	auto query = NanodbcTableSet::GetInitializeQuery(schema.name, table_name);
	auto result = transaction.Query(query);
	auto rows = result->Count();
	if (rows == 0) {
		return nullptr;
	}
	auto table_info = make_uniq<NanodbcTableInfo>(schema, table_name);
	for (idx_t row = 0; row < rows; row++) {
		AddColumnOrConstraint(&transaction, &schema, *result, row, *table_info);
	}
	table_info->approx_num_pages = result->GetInt64(0, 2);
	return table_info;
}

unique_ptr<NanodbcTableInfo> NanodbcTableSet::GetTableInfo(NanodbcConnection &connection, const string &schema_name,
                                                             const string &table_name) {
	auto query = NanodbcTableSet::GetInitializeQuery(schema_name, table_name);
	auto result = connection.Query(query);
	auto rows = result->Count();
	if (rows == 0) {
		throw InvalidInputException("Table %s does not contain any columns.", table_name);
	}
	auto table_info = make_uniq<NanodbcTableInfo>(schema_name, table_name);
	for (idx_t row = 0; row < rows; row++) {
		AddColumnOrConstraint(nullptr, nullptr, *result, row, *table_info);
	}
	table_info->approx_num_pages = result->GetInt64(0, 2);
	return table_info;
}

optional_ptr<CatalogEntry> NanodbcTableSet::ReloadEntry(ClientContext &context, const string &table_name) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	auto table_info = GetTableInfo(transaction, schema, table_name);
	if (!table_info) {
		return nullptr;
	}
	auto table_entry = make_uniq<NanodbcTableEntry>(catalog, schema, *table_info);
	auto table_ptr = table_entry.get();
	CreateEntry(std::move(table_entry));
	return table_ptr;
}

// FIXME - this is almost entirely copied from TableCatalogEntry::ColumnsToSQL - should be unified
string NanodbcColumnsToSQL(const ColumnList &columns, const vector<unique_ptr<Constraint>> &constraints) {
	std::stringstream ss;

	ss << "(";

	// find all columns that have NOT NULL specified, but are NOT primary key
	// columns
	logical_index_set_t not_null_columns;
	logical_index_set_t unique_columns;
	logical_index_set_t pk_columns;
	unordered_set<string> multi_key_pks;
	vector<string> extra_constraints;
	for (auto &constraint : constraints) {
		if (constraint->type == ConstraintType::NOT_NULL) {
			auto &not_null = constraint->Cast<NotNullConstraint>();
			not_null_columns.insert(not_null.index);
		} else if (constraint->type == ConstraintType::UNIQUE) {
			auto &pk = constraint->Cast<UniqueConstraint>();
			vector<string> constraint_columns = pk.columns;
			if (pk.index.index != DConstants::INVALID_INDEX) {
				// no columns specified: single column constraint
				if (pk.is_primary_key) {
					pk_columns.insert(pk.index);
				} else {
					unique_columns.insert(pk.index);
				}
			} else {
				// multi-column constraint, this constraint needs to go at the end after
				// all columns
				if (pk.is_primary_key) {
					// multi key pk column: insert set of columns into multi_key_pks
					for (auto &col : pk.columns) {
						multi_key_pks.insert(col);
					}
				}
				extra_constraints.push_back(constraint->ToString());
			}
		} else if (constraint->type == ConstraintType::FOREIGN_KEY) {
			auto &fk = constraint->Cast<ForeignKeyConstraint>();
			if (fk.info.type == ForeignKeyType::FK_TYPE_FOREIGN_KEY_TABLE ||
			    fk.info.type == ForeignKeyType::FK_TYPE_SELF_REFERENCE_TABLE) {
				extra_constraints.push_back(constraint->ToString());
			}
		} else {
			extra_constraints.push_back(constraint->ToString());
		}
	}

	for (auto &column : columns.Logical()) {
		if (column.Oid() > 0) {
			ss << ", ";
		}
		ss << KeywordHelper::WriteQuoted(column.Name(), '"') << " ";
		ss << NanodbcUtils::TypeToString(column.Type());
		bool not_null = not_null_columns.find(column.Logical()) != not_null_columns.end();
		bool is_single_key_pk = pk_columns.find(column.Logical()) != pk_columns.end();
		bool is_multi_key_pk = multi_key_pks.find(column.Name()) != multi_key_pks.end();
		bool is_unique = unique_columns.find(column.Logical()) != unique_columns.end();
		if (not_null && !is_single_key_pk && !is_multi_key_pk) {
			// NOT NULL but not a primary key column
			ss << " NOT NULL";
		}
		if (is_single_key_pk) {
			// single column pk: insert constraint here
			ss << " PRIMARY KEY";
		}
		if (is_unique) {
			// single column unique: insert constraint here
			ss << " UNIQUE";
		}
		if (column.Generated()) {
			ss << " GENERATED ALWAYS AS(" << column.GeneratedExpression().ToString() << ")";
		} else if (column.HasDefaultValue()) {
			ss << " DEFAULT(" << column.DefaultValue().ToString() << ")";
		}
	}
	// print any extra constraints that still need to be printed
	for (auto &extra_constraint : extra_constraints) {
		ss << ", ";
		ss << extra_constraint;
	}

	ss << ")";
	return ss.str();
}

string GetNanodbcCreateTable(CreateTableInfo &info) {
	for (idx_t i = 0; i < info.columns.LogicalColumnCount(); i++) {
		auto &col = info.columns.GetColumnMutable(LogicalIndex(i));
		col.SetType(NanodbcUtils::ToNanodbcType(col.GetType()));
	}

	std::stringstream ss;
	ss << "CREATE TABLE ";
	if (info.on_conflict == OnCreateConflict::IGNORE_ON_CONFLICT) {
		ss << "IF NOT EXISTS ";
	}
	if (!info.schema.empty()) {
		ss << KeywordHelper::WriteQuoted(info.schema, '"');
		ss << ".";
	}
	ss << KeywordHelper::WriteQuoted(info.table, '"');
	ss << NanodbcColumnsToSQL(info.columns, info.constraints);
	ss << ";";
	return ss.str();
}

optional_ptr<CatalogEntry> NanodbcTableSet::CreateTable(ClientContext &context, BoundCreateTableInfo &info) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	auto create_sql = GetNanodbcCreateTable(info.Base());
	transaction.Query(create_sql);
	auto tbl_entry = make_uniq<NanodbcTableEntry>(catalog, schema, info.Base());
	return CreateEntry(std::move(tbl_entry));
}

string NanodbcTableSet::GetAlterTablePrefix(const string &name, optional_ptr<CatalogEntry> entry) {
	string sql = "ALTER TABLE ";
	sql += KeywordHelper::WriteQuoted(schema.name, '"') + ".";
	sql += KeywordHelper::WriteQuoted(entry ? entry->name : name, '"');
	return sql;
}

string NanodbcTableSet::GetAlterTableColumnName(const string &name, optional_ptr<CatalogEntry> entry) {
	if (!entry || entry->type != CatalogType::TABLE_ENTRY) {
		return name;
	}
	auto &table = entry->Cast<NanodbcTableEntry>();
	string column_name = name;
	auto column_index = table.GetColumnIndex(column_name, true);
	if (!column_index.IsValid()) {
		return name;
	}
	return table.nanodbc_names[column_index.index];
}

string NanodbcTableSet::GetAlterTablePrefix(ClientContext &context, const string &name) {
	auto entry = GetEntry(context, name);
	return GetAlterTablePrefix(name, entry);
}

void NanodbcTableSet::AlterTable(ClientContext &context, RenameTableInfo &info) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	string sql = GetAlterTablePrefix(context, info.name);
	sql += " RENAME TO ";
	sql += KeywordHelper::WriteQuoted(info.new_table_name, '"');
	transaction.Query(sql);
}

void NanodbcTableSet::AlterTable(ClientContext &context, RenameColumnInfo &info) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	auto entry = GetEntry(context, info.name);
	string sql = GetAlterTablePrefix(info.name, entry);
	sql += " RENAME COLUMN  ";
	string column_name = GetAlterTableColumnName(info.old_name, entry);
	sql += KeywordHelper::WriteQuoted(column_name, '"');
	sql += " TO ";
	sql += KeywordHelper::WriteQuoted(info.new_name, '"');

	transaction.Query(sql);
}

void NanodbcTableSet::AlterTable(ClientContext &context, AddColumnInfo &info) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	string sql = GetAlterTablePrefix(context, info.name);
	sql += " ADD COLUMN  ";
	if (info.if_column_not_exists) {
		sql += "IF NOT EXISTS ";
	}
	sql += KeywordHelper::WriteQuoted(info.new_column.Name(), '"');
	sql += " ";
	sql += info.new_column.Type().ToString();
	transaction.Query(sql);
}

void NanodbcTableSet::AlterTable(ClientContext &context, RemoveColumnInfo &info) {
	auto &transaction = NanodbcTransaction::Get(context, catalog);
	auto entry = GetEntry(context, info.name);
	string sql = GetAlterTablePrefix(info.name, entry);
	sql += " DROP COLUMN  ";
	if (info.if_column_exists) {
		sql += "IF EXISTS ";
	}
	string column_name = GetAlterTableColumnName(info.removed_column, entry);
	sql += KeywordHelper::WriteQuoted(column_name, '"');
	transaction.Query(sql);
}

void NanodbcTableSet::AlterTable(ClientContext &context, AlterTableInfo &alter) {
	switch (alter.alter_table_type) {
	case AlterTableType::RENAME_TABLE:
		AlterTable(context, alter.Cast<RenameTableInfo>());
		break;
	case AlterTableType::RENAME_COLUMN:
		AlterTable(context, alter.Cast<RenameColumnInfo>());
		break;
	case AlterTableType::ADD_COLUMN:
		AlterTable(context, alter.Cast<AddColumnInfo>());
		break;
	case AlterTableType::REMOVE_COLUMN:
		AlterTable(context, alter.Cast<RemoveColumnInfo>());
		break;
	default:
		throw BinderException("Unsupported ALTER TABLE type - Nanodbc tables only "
		                      "support RENAME TABLE, RENAME COLUMN, "
		                      "ADD COLUMN and DROP COLUMN");
	}
	ClearEntries();
}

} // namespace duckdb

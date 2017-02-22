//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// convert_query_to_op.cpp
//
// Identification: src/optimizer/query_property_extractor.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "optimizer/query_property_extractor.h"

#include "catalog/catalog.h"
#include "expression/expression_util.h"
#include "optimizer/properties.h"
#include "parser/select_statement.h"
#include "parser/sql_statement.h"
#include "storage/data_table.h"

namespace peloton {
namespace optimizer {

PropertySet QueryPropertyExtractor::GetProperties(parser::SQLStatement *stmt) {
  stmt->Accept(this);
  return property_set_;
}

void QueryPropertyExtractor::Visit(const parser::SelectStatement *select_stmt) {

  // Get table pointer, id, and schema.
  storage::DataTable *target_table =
      catalog::Catalog::GetInstance()->GetTableWithName(
          select_stmt->from_table->GetDatabaseName(),
          select_stmt->from_table->GetTableName());
  auto &schema = *target_table->GetSchema();

  // Add predicate property
  auto predicate = select_stmt->where_clause;
  if (predicate != nullptr) {
    expression::ExpressionUtil::TransformExpression(&schema, predicate);
    property_set_.AddProperty(std::shared_ptr<PropertyPredicate>(
        new PropertyPredicate(predicate->Copy())));
  }

  std::vector<oid_t> column_ids;
  bool needs_projection = false;

  std::vector<expression::TupleValueExpression*> column_exprs;

  // Transform output expressions
  for (auto col : *select_stmt->select_list) {
    expression::ExpressionUtil::TransformExpression(column_ids, col, schema, needs_projection);
    if (col->GetExpressionType() == ExpressionType::VALUE_TUPLE)
      column_exprs.emplace_back(reinterpret_cast<expression::TupleValueExpression*>(col));
  }

  property_set_.AddProperty(
      std::shared_ptr<PropertyColumns>(new PropertyColumns(column_exprs)));

  if (needs_projection) {
    auto output_expressions =
        std::vector<std::unique_ptr<expression::AbstractExpression> >();
    // Add output expressions property
    for (auto col : *select_stmt->select_list) {
      output_expressions.push_back(
          std::unique_ptr<expression::AbstractExpression>(col->Copy()));
    }
    property_set_.AddProperty(std::shared_ptr<PropertyProjection>(
        new PropertyProjection(std::move(output_expressions))));
  }
};
void QueryPropertyExtractor::Visit(const parser::TableRef *) {}
void QueryPropertyExtractor::Visit(const parser::JoinDefinition *) {}
void QueryPropertyExtractor::Visit(const parser::GroupByDescription *) {}
void QueryPropertyExtractor::Visit(const parser::OrderDescription *) {}
void QueryPropertyExtractor::Visit(const parser::LimitDescription *) {}

void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::CreateStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::InsertStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::DeleteStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::DropStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::PrepareStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::ExecuteStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::TransactionStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::UpdateStatement *op) {}
void QueryPropertyExtractor::Visit(
    UNUSED_ATTRIBUTE const parser::CopyStatement *op) {}

} /* namespace optimizer */
} /* namespace peloton */

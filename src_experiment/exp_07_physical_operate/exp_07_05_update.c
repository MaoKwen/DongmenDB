//
// Created by sam on 2018/9/18.
// Edited bu mao on 2018/9/30.
//
#include "physicalplan/physicalplan.h"

/*执行 update 语句*/
/*TODO: plan_execute_update， update语句执行*/
int plan_execute_update(dongmendb *db, sql_stmt_update *sqlStmtUpdate , transaction *tx){
    /*删除语句以select的物理操作为基础实现。
     * 1. 使用 sql_stmt_update 的条件参数，调用 physical_scan_select_create 创建select的物理计划并初始化;
     * 2. 执行 select 的物理计划，完成update操作
     * */

    char *tableName = sqlStmtUpdate->tableName;
    arraylist *fields = sqlStmtUpdate->fields;
    arraylist *fieldsExpr = sqlStmtUpdate->fieldsExpr;

    physical_scan *scan = physical_scan_generate(db, sqlStmtUpdate->where, tx);
    scan->beforeFirst(scan);
    size_t updated_lines = 0;

    while (scan->next(scan)) {
        for (size_t i = 0; i < fields->size; ++i) {
            char *currentFieldName = arraylist_get(fields, i);
            variant *val = (variant *) calloc(sizeof(variant), 1);
            enum data_type field_type = scan->getField(scan, tableName, currentFieldName)->type;
            physical_scan_evaluate_expression(arraylist_get(fieldsExpr, i), scan, val);

            if (val->type != field_type) {
                fprintf(stdout, "invalid sql: field type mismatched.");
                return -1;
            }

            if (val->type == DATA_TYPE_INT) {
                scan->setInt(scan, tableName, currentFieldName, val->intValue);
            } else if (val->type == DATA_TYPE_CHAR) {
                /*字符串超出定义时的长度，则截断字符串.*/
                int max_length_str = scan->getField(scan, tableName, currentFieldName)->length;
                if (max_length_str < strlen(val->strValue)) {
                    val->strValue[max_length_str] = '\0';
                }
                scan->setString(scan, tableName, currentFieldName, val->strValue);
            } else {
                // other data type...
            }
        }
        updated_lines += 1;
    }
    scan->close(scan);
    fprintf(stdout, " updated %d lines.\n", updated_lines);

    return updated_lines;
};

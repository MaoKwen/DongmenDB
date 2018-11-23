//
// Created by Sam on 2018/2/13.
//

#include <dongmensql/optimizer.h>

/**
 * 使用关于选择的等价变换规则将条件下推。
 *
 */


Expression *skipSubexpression(Expression *expr);
Expression *findBefore(Expression *begin, Expression *end);
void splitCNF(Expression *expr, SRA_t **sra);
/**
 * 等价变换：将满足条件的 SRA_SELECT 类型的节点进行条件串接
 */
void selectCNFOptimize(SRA_t **sra);


bool field_exists_in_table(table_manager *tm, char *tableName, char *fieldName, transaction *tx);

bool containFields(SRA_t *sra, Expression *cond, table_manager *tm, transaction *tx);

/**
 * 等价变换：SELECT 下移
 */
void switchDownOptimize(SRA_t **sra, table_manager *tm, transaction *tx);


/*输入一个关系代数表达式，输出优化后的关系代数表达式
 * 要求：在查询条件符合合取范式的前提下，根据等价变换规则将查询条件移动至合适的位置。
 * */
SRA_t *dongmengdb_algebra_optimize_condition_pushdown(SRA_t *sra, table_manager *tableManager, transaction *tx){

    /*初始关系代数语法树sra由三个操作构成：SRA_PROJECT -> SRA_SELECT -> SRA_JOIN，即对应语法树中三个节点。*/

    /*第一步：.等价变换：将SRA_SELECT类型的节点进行条件串接*/

        /*1.1 在sra中找到每个SRA_Select节点 */
        /*1.2 检查每个SRA_Select节点中的条件是不是满足串接条件：多条件且用and连接*/
        /*1.3 若满足串接条件则：创建一组新的串接的SRA_Select节点，等价替换当前的SRA_Select节点*/

    /*第二步：等价变换：条件交换*/
        /*2.1 在sra中找到每个SRA_Select节点*/
        /*2.2 对每个SRA_Select节点做以下处理：
         * 在sra中查找 SRA_Select 节点应该在的最优位置：
         *     若子操作也是SRA_Select，则可交换；
         *     若子操作是笛卡尔积，则可交换，需要判断SRA_Select所包含的属性属于笛卡尔积的哪个子操作
         * 最后将SRA_Select类型的节点移动到语法树的最优位置。
         * */

//    printf("\norigin:\n");
//    SRA_print(sra);
    selectCNFOptimize(&sra);
//    printf("\nstep1:\n");
//    SRA_print(sra);
    switchDownOptimize(&sra, tableManager, tx);
//    printf("\nstep2:\n");
//    SRA_print(sra);
//    printf("\n");
    return sra;
}

Expression *skipSubexpression(Expression *expr) {
    if (!expr) return NULL;
    if (expr->term)
        return expr->nextexpr;

    if (expr->opType <= TOKEN_COMMA) {
        int op_num = operators[expr->opType].numbers;
        expr = expr->nextexpr;
        while (op_num--)
            expr = skipSubexpression(expr);

        return expr;
    }

    return NULL;
}

Expression *findBefore(Expression *begin, Expression *end) {
    Expression *iter = begin;
    while (iter->nextexpr != end && iter->nextexpr != NULL)
        iter = iter->nextexpr;
    return iter;
}

bool isCnf(Expression *expr) {
    if (expr->opType == TOKEN_AND) return true;
    return false;
}

void splitCNF(Expression *expr, SRA_t **sra) {
    Expression *left = expr->nextexpr;
    Expression *right = skipSubexpression(left);

    Expression *left_tail = findBefore(left, right);
    left_tail->nextexpr = NULL;

    (*sra)->select.sra = SRASelect((*sra)->select.sra, left);
    (*sra)->select.cond = right;
}

void selectCNFOptimize(SRA_t **sra) {
    SRA_t *s = *sra;
    if (!s) return;
    switch (s->t) {
        case SRA_SELECT:
            if (isCnf(s->select.cond))
                splitCNF(s->select.cond, sra);
            selectCNFOptimize(&(s->select.sra));
            break;
        case SRA_PROJECT:
            selectCNFOptimize(&(s->project.sra));
            break;
        case SRA_JOIN:
            selectCNFOptimize(&(s->join.sra1));
            selectCNFOptimize(&(s->join.sra2));
            break;
        default:
            break;
    }
}

bool field_exists_in_table(table_manager *tm, char *tableName, char *fieldName, transaction *tx) {
    table_info *tableInfo = table_manager_get_tableinfo(tm, tableName, tx);

    arraylist *fieldsName = tableInfo->fieldsName;
    for (size_t i = 0; i < fieldsName->size; ++i)
        if (strcmp(arraylist_get(fieldsName, i), fieldName) != 0)
            return false;

    return true;
}

bool containFields(SRA_t *sra, Expression *cond, table_manager *tm, transaction *tx) {
    switch (sra->t) {
        case SRA_SELECT:
            return containFields(sra->select.sra, cond, tm, tx);
        case SRA_JOIN:
            return containFields(sra->join.sra1, cond, tm, tx) &&
                   containFields(sra->join.sra2, cond, tm, tx);
        case SRA_TABLE:
            for (Expression *iter = cond; iter != NULL; iter = iter->nextexpr) {
                if (iter->term && iter->term->t == TERM_COLREF) {
                    if (iter->term->ref->tableName) {
                        if (strcmp(iter->term->ref->tableName, sra->table.ref->table_name) != 0)
                            return false;
                    } else {
                        // todo: fix
//                        if (field_exists_in_table(tm, sra->table.ref->table_name, iter->term->ref->columnName))
//                            return false;
                    }
                }
            }
            return true;
        default:
            return false;
    }
}

void switchDownOptimize(SRA_t **sra, table_manager *tm, transaction *tx) {
    SRA_t *s1 = *sra;
    if (!s1) return;
    switch (s1->t) {
        case SRA_SELECT: {
            switchDownOptimize(&(s1->select.sra), tm, tx);
            SRA_t *s2 = s1->select.sra;
            if (s2->t == SRA_JOIN) {
                bool switch_left = containFields(s2->join.sra1, s1->select.cond, tm, tx);
                bool switch_right = containFields(s2->join.sra2, s1->select.cond, tm, tx);
                if (switch_left && !switch_right) {
                    s1->select.sra = s2->join.sra1;
                    s2->join.sra1 = s1;
                    *sra = s2;
                    switchDownOptimize(&(s1->join.sra1), tm, tx);
                } else if (switch_right && !switch_left) {
                    s1->select.sra = s2->join.sra2;
                    s2->join.sra2 = s1;
                    *sra = s2;
                    switchDownOptimize(&(s1->join.sra2), tm, tx);
                }
            } else if (s2->t == SRA_SELECT) {
                s1->select.sra = s2->select.sra;
                s2->select.sra = s1;
                *sra = s2;
                switchDownOptimize(&(s2->select.sra), tm, tx);
            }
        }
            break;
        case SRA_PROJECT:
            switchDownOptimize(&(s1->project.sra), tm, tx);
            break;
        case SRA_JOIN:
            switchDownOptimize(&(s1->join.sra1), tm, tx);
            switchDownOptimize(&(s1->join.sra2), tm, tx);
            break;
        default:
            break;
    }
}
//
// Created by Sam on 2018/2/13.
//

#include <dongmensql/optimizer.h>

/**
 * 使用关于选择的等价变换规则将条件下推。
 *
 */

Expression *findTail(Expression *begin, Expression *end) {
    Expression *iter = begin;
    while (iter->nextexpr != end && iter->nextexpr != NULL)
        iter = iter->nextexpr;
    return iter;
}

/**
 * partitionCNF 递归地遍历合取范式的左右分支，拆分条件并把 *sra 包裹成 SRA_SELECT
 * @param expr 表达式
 * @param sra 输出参数，sra 的地址
 * @return expression 的下一个节点
 *
 * Select((((a = 1) AND (b = 2)) AND (c = 3)),
 *      Table(F))
 * ==>
 * Select((c = 3),
 *    Select((a = 1),
 *       Select((b = 2),
 *          Table(F))))
 */
Expression *partitionCNF(Expression *expr, SRA_t **sra);


/*输入一个关系代数表达式，输出优化后的关系代数表达式
 * 要求：在查询条件符合合取范式的前提下，根据等价变换规则将查询条件移动至合适的位置。
 * */
SRA_t *dongmengdb_algebra_optimize_condition_pushdown(SRA_t *sra) {

    SRA_t *ori_select = sra->project.sra;
    SRA_t *select = ori_select->select.sra;
    Expression *conds = ori_select->select.cond; //  [((... ... and) ... and) ... and]

    partitionCNF(conds, &select);
    return SRAProject(select, sra->project.expr_list);
}

Expression *partitionCNF(Expression *expr, SRA_t **sra) {
    if (!expr) return NULL;
    if (expr->term) {
        return expr->nextexpr;
    }
    if (expr->opType <= TOKEN_COMMA && expr->opType != TOKEN_AND) {
        int op_num = operators[expr->opType].numbers;
        expr = expr->nextexpr;
        while (op_num--) {
            expr = partitionCNF(expr, sra);
        }
        return expr;
    }

    if (expr->opType == TOKEN_AND) {
        Expression *left = expr->nextexpr;
        Expression *right = partitionCNF(left, sra);
        Expression *right_end = partitionCNF(right, sra);

        Expression *right_tail = findTail(right, right_end);
        right_tail->nextexpr = NULL;
        *sra = SRASelect(*sra, right);

        if (left->opType == TOKEN_AND) // skip left if it already been finished
            return right_end;

        Expression *left_tail = findTail(left, right);
        left_tail->nextexpr = NULL;
        *sra = SRASelect(*sra, left);

        return right_end;
    }
}

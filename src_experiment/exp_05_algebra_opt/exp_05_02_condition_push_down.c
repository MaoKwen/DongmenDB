//
// Created by Sam on 2018/2/13.
//

#include <dongmensql/optimizer.h>

/**
 * 使用关于选择的等价变换规则将条件下推。
 *
 */


/*输入一个关系代数表达式，输出优化后的关系代数表达式
 * 要求：在查询条件符合合取范式的前提下，根据等价变换规则将查询条件移动至合适的位置。
 * */

/*
dongmendb>.optimizer select a from F where a = 1 and b = 2 and c = 3;
.optimizer select a from F where a = 1 and b = 2 and c = 3;

Project([a],
        Select((((a = 1) AND (b = 2)) AND (c = 3)),
                Table(F)
        )
)

optimized relational algebra:
Project([a],
        Select((c = 3),
                Select((b = 2),
                        Select((a = 1),
                                Table(F)
                        )
                )
        )
)
*/

Expression *optimize_condition_pushdown(SRA_t **sra, Expression *expr) {
    if (!expr) return NULL;
    if (expr->term) {
        return expr->nextexpr;
    }
    if (expr->opType <= TOKEN_COMMA && expr->opType != TOKEN_AND) {
        int op_num = operators[expr->opType].numbers;
        expr = expr->nextexpr;
        while (op_num--) {
            expr = optimize_condition_pushdown(sra, expr);
        }
        return expr;
    }

    if (expr->opType == TOKEN_AND) {
        Expression *left = expr->nextexpr;
        Expression *right = optimize_condition_pushdown(sra, left);
        Expression *right_back = optimize_condition_pushdown(sra, right);

        Expression *right_tail = right;
        while (right_tail->nextexpr != right_back && right_tail->nextexpr != NULL)
            right_tail = right_tail->nextexpr;
        right_tail->nextexpr = NULL;

        if (left->opType == TOKEN_AND) {
            *sra = SRASelect(*sra, right);
            return right_back;
        }

        Expression *left_tail = left;
        while (left_tail->nextexpr != right)
            left_tail = left_tail->nextexpr;
        left_tail->nextexpr = NULL;

        *sra = SRASelect(SRASelect(*sra, left), right);
        return right_back;
    }
}

SRA_t *dongmengdb_algebra_optimize_condition_pushdown(SRA_t *sra){

    SRA_t *ori_select = sra->project.sra;
    SRA_t *select = ori_select->select.sra;
    Expression *conds = ori_select->select.cond; //  [((... ... and) ... and) ... and]

    optimize_condition_pushdown(&select, conds);
    return SRAProject(select, sra->project.expr_list);

    return sra;
}
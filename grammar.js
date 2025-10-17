let Prec = {
    SUFFIX_EXPR: 9,
    BINARY_EXPR3: 8,
    BINARY_EXPR2: 7,
    BINARY_EXPR1: 6,
    BINARY_EXPR0: 5,
    UNARY_EXPR: 4,
    FN_DEF: 3,
    STRING_CHAR: 2
};

function statement_group($) {
    return seq(
        repeat(seq($._statement, "\n")),
        optional($._statement),
    );
}

module.exports = grammar({
    name: "stp",
    conflicts: $ => [
        [$.function_call, $.identifier_or_member_access],
        [$.fn_pos_arg_list],
        [$.pos_args_decl]
    ],

    extras: $ => [/\s/, $.comment],

    rules: {
        source_file: $ => statement_group($),

        _statement: $ => choice(
            $.assignment,
            $.symbol_decl_statement,
            $.if_else_stmt,
            $.while_stmt,
            $.for_in_stmt,
            $.function_definition,
            $.expression_statement,
            $.import_statement,
            $.comment,
            // $.lambda,
            "break",
            "cont",
            "exit",
        ),

        symbol_decl_statement: $ => seq(
            "sym",
            field("sym_name", $.identifier)
        ),

        return_stmt: $ => seq(
            "ret",
            field("ret_expr", $._expression)
        ),

        elseif_clause: $ => seq(
            "elseif",
            $._expression,
            "{",
            alias(statement_group($), $.elseif_clause_stmt),
            "}"
        ),

        else_clause: $ => seq(
            "else",
            "{",
            alias(statement_group($), $.else_clause_stmt),
            "}"
        ),

        if_else_stmt: $ => seq(
            "if", $._expression, "{",
            alias(
                statement_group($), $.if_clause_stmt
            ),
            "}",
            repeat($.elseif_clause),
            optional($.else_clause)
        ),

        while_stmt: $ => seq(
            "while",
            field("loop_expr", $._expression),
            "{",
            alias(statement_group($), $.loop_statements),
            "}"
        ),

        for_in_stmt: $ => seq(
            "for", field("loop_var", $.identifier),
            "in",
            field("loop_expr", $._expression),
            "{",
            statement_group($),
            "}"
        ),

        assignment: $ => seq(
            $.identifier, "=", $._expression, optional(";")
        ),

        member_access: $ => prec.left(seq(
            $._expression,
            ".",
            $.identifier
        )),

        function_definition: $ => prec(Prec.FN_DEF, seq(
            "fn",
            field(
                "fn_name",
                alias($.identifier, $.function_name)
            ),
            "(",
            optional($.pos_args_decl),
            optional(
                seq(",", $.keyword_args_decl)
            ),
            ")",
            "{",
            alias(field(
                "fn_body",
                repeat(choice(
                    $._statement,
                    $.return_stmt,
                ))
            ), $.fn_body),
            "}"
        )),

        lambda: $ => seq(
            $.identifier,
            "(",
            optional($.pos_args_decl),
            ")",
            ":",
            $._statement
        ),

        pos_args_decl: $ => seq(
            alias($.identifier, $.param_name),
            repeat(
                seq(",", alias($.identifier, $.param_name))
            )
        ),

        keyword_args_decl: $ => seq(
            $.fn_keyword_arg,
            repeat(
                seq(",", $.fn_keyword_arg)
            )
        ),

        import_statement: $ => seq("import", $.identifier, optional(";")),

        expression_statement: $ => seq($._expression, optional(";")),

        _expression: $ => choice(
            $.matrix,
            $.range_expr,
            $.binary_expression,
            $.unary_expression,
            $.suffix_expression,
            $.function_call,
            $.identifier_or_member_access,
            $.number,
            $.percentage,
            $.string,
            $.bracketed_expr,
        ),

        range_expr: $ => seq(
            field("start", $.number),
            "...",
            optional(seq(
                field("step", $.number),
                "...",
            )),
            field("end", $.number)
        ),

        bracketed_expr: $ => seq("(", $._expression, ")"),

        matrix_row: $ => prec.left(seq(
            repeat1($._expression),
            ";"
        )),

        matrix_row_last: $ => prec.left(seq(
            repeat1($._expression),
            optional(";")
        )),

        matrix: $ => seq(
            "[",
            repeat($.matrix_row),
            $.matrix_row_last,
            "]"
        ),

        binary_expression: $ => choice(
            $.binary_expression_left0,
            $.binary_expression_left1,
            $.binary_expression_left2,
            $.binary_expression_right
        ),

        binary_expression_right: $ => prec.right(Prec.BINARY_EXPR3, seq(
            $._expression,
            $.binary_operator_right,
            $._expression
        )),

        binary_expression_left0: $ => prec.left(Prec.BINARY_EXPR0, seq(
            $._expression,
            $.binary_operator_left0,
            $._expression
        )),

        binary_expression_left1: $ => prec.left(Prec.BINARY_EXPR1, seq(
            $._expression,
            $.binary_operator_left1,
            $._expression
        )),

        binary_expression_left2: $ => prec.left(Prec.BINARY_EXPR2, seq(
            $._expression,
            $.binary_operator_left2,
            $._expression
        )),

        binary_operator_left0: $ => choice(
            "==", "!=", ">", "<", ">=", "<=", "in", "and", "not", "or"
        ),

        binary_operator_left1: $ => choice(
            "-", "+",
        ),

        binary_operator_left2: $ => choice(
            "*", "/",
            ".*", "./", " mod ", "@", "&"
        ),

        binary_operator_right: $ => choice(
            "^", ".^"
        ),

        unary_expression: $ => prec.right(Prec.UNARY_EXPR, seq(
            choice("~", "+", "-"),
            $._expression
        )),

        suffix_expression: $ => prec.left(Prec.SUFFIX_EXPR, seq(
            $._expression,
            field(
                "operator",
                choice("'", "!")
            ),
        )),

        fn_keyword_arg: $ => seq(
            field(
                "argument_name",
                alias($.identifier, $.param_name)
            ),
            "=",
            $._expression
        ),

        fn_pos_arg_list: $ => seq(
            $._expression,
            repeat(seq(",", $._expression))
        ),

        fn_keyword_arg_list: $ => seq(
            $.fn_keyword_arg,
            repeat(seq(",", $.fn_keyword_arg))
        ),

        function_call: $ => seq(
            field("fn_name", $.identifier),
            "(",
            optional($.fn_pos_arg_list),
            optional(seq(",", $.fn_keyword_arg_list)),
            ")"
        ),

        percentage: $ => seq($.number, "%"),

        comment: _ => token(seq("#", /.*/)),

        identifier: _ => token(/[a-zA-Z_][a-zA-Z0-9_]*/),
        identifier_or_member_access: $ => choice($.identifier, $.member_access),

        number: _ => token(/\d+(\.\d+)?/),
        integer: _ => token(/\d+?/),

        escape_sequence: _ => /\\[rntbf"\\]/,
        unicode_escape: $ => seq(
            "\\x",
            alias(token.immediate(/[0-9A-Fa-f]{2}|[0-9A-Fa-f]{4}|[0-9A-Fa-f]{8}/), $.hex_digits)
        ),
        octal_escape: $ => /\\[0-7]{3}/,
        formatting_snippet: $ => seq(
            "\\{",
            field("formatting_expr", $._expression),
            "\\}"
        ),
        string_char: $ => token(prec(Prec.STRING_CHAR, /[^"\\]+/)),
        string: $ => seq(
            "\"",
            field("string_chars",
                repeat(choice(
                    $.formatting_snippet,
                    $.string_char,
                    $.escape_sequence,
                    $.unicode_escape,
                    $.octal_escape,
                ))
            ),
            "\""
        )
    }
});

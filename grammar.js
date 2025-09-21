Prec = {
    BINARY_EXPR3: 8,
    BINARY_EXPR2: 7,
    BINARY_EXPR1: 6,
    BINARY_EXPR0: 5,
    UNARY_EXPR: 4,
    FN_DEF: 3,
    STRING_CHAR: 2
}

module.exports = grammar({
    name: "stp",
    conflicts: $ => [
        [$._expression, $.function_call],
    ],

    extras: $ => [/\s/, $.comment],

    rules: {
        source_file: $ => repeat($._statement),

        _statement: $ => choice(
            $.assignment,
            $.symbol_decl_statement,
            $.if_else_stmt,
            $.while_stmt,
            $.foreach_in_stmt,
            $.function_definition,
            $.expression_statement,
            $.import_statement,
            $.comment
        ),

        symbol_decl_statement: $ => seq(
            "sym",
            field("sym_name", $.identifier)
        ),

        loop_statements: $ => choice(
            $._statement,
            "break",
            "cont"
        ),

        return_stmt: $ => seq(
            "ret",
            field("ret_expr", $._expression)
        ),

        elseif_clause: $ => seq(
            "elseif",
            $._expression,
            "{",
            alias(repeat($._statement), $.elseif_clause_stmt),
            "}"
        ),

        else_clause: $ => seq(
            "else",
            "{",
            alias(repeat($._statement), $.else_clause_stmt),
            "}"
        ),

        if_else_stmt: $ => seq(
            "if", $._expression, "{", alias(repeat($._statement), $.if_clause_stmt), "}",
            repeat($.elseif_clause),
            optional($.else_clause)
        ),

        while_stmt: $ => seq(
            "while", $._expression, "{", repeat($.loop_statements), "}"
        ),

        foreach_in_stmt: $ => seq(
            "foreach", alias($.identifier, $.loop_var), "in", $.identifier_or_member_access, "{",
            repeat($.loop_statements),
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
                alias($.identifier, $.function_name
                )
            ),
            "(",
            field(
                "parameter_list",
                $.parameter_list,
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

        parameter_list: $ => seq(
            alias($.identifier, $.param_name),
            repeat(
                seq(",", alias($.identifier, $.param_name))
            )
        ),

        import_statement: $ => seq("import", $.identifier, optional(";")),

        expression_statement: $ => seq($._expression, optional(";")),

        _expression: $ => choice(
            $.matrix,
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
            "==", "!=", ">", "<", ">=", "<=",
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
            choice("!", "+", "-"),
            $._expression
        )),

        suffix_expression: $ => prec.left(Prec.UNARY_EXPR, seq(
            $._expression,
            token.immediate(choice("'"))
        )),

        function_call: $ => seq(
            field("fn_name", $.identifier_or_member_access),
            "(",
            optional(seq($._expression, repeat(seq(",", $._expression)))),
            ")"
        ),

        percentage: $ => seq($.number, "%"),

        comment: _ => token(seq("#", /.*/)),

        identifier: _ => token(/[a-zA-Z_][a-zA-Z0-9_]*/),
        identifier_or_member_access: $ => choice($.identifier, $.member_access),
        number: _ => token(/\d+(\.\d+)?/),

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

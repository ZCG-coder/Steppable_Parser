module.exports = grammar({
    name: 'stp',
    conflicts: $ => [
        [$.function_definition, $._expression],
        [$.matrix],
        [$.identifier_or_member_access, $._expression]
    ],

    extras: $ => [
        /\s/,
        $.comment,
        /\\\r?\n/
    ],

    rules: {
        source_file: $ => optional(seq(
            $._statement,
            repeat(seq('\n', $._statement)),
            optional('\n'))),

        _statement: $ => choice(
            $.assignment,
            $.if_else_stmt,
            $.while_stmt,
            $.foreach_in_stmt,
            $.object_definition,
            $.function_definition,
            $.expression_statement,
            $.import_statement,
            $.comment
        ),

        loop_statements: $ => choice(
            $._statement,
            'break',
            'cont'
        ),

        if_else_stmt: $ => seq(
            'if', $._expression, '{', repeat($._statement), '}',
            optional(
                repeat(seq('elseif', $._expression, '{', repeat($._statement), '}')),
            ),
            optional(
                seq('else', '{', repeat($._statement), '}'),
            )
        ),

        while_stmt: $ => seq(
            'while', $._expression, '{', repeat($.loop_statements), '}'
        ),

        foreach_in_stmt: $ => seq(
            'foreach', alias($.identifier, $.loop_var), 'in', $.identifier_or_member_access, '{',
            repeat($.loop_statements),
            '}'
        ),

        assignment: $ => seq(
            $.identifier_or_member_access, '=', $._expression, optional(';')
        ),

        object_definition: $ => seq(
            alias($.identifier, $.object_name),
            '{', repeat($.assignment), '}'
        ),

        member_access: $ => seq(
            $._expression,
            '.',
            $.identifier
        ),

        function_definition: $ => seq(
            alias($.identifier, $.function_name),
            $.parameter_list, '->', alias($.identifier, $.type), '{', $._expression, '}'
        ),

        parameter_list: $ => seq(
            alias($.identifier, $.type), alias($.identifier, $.param_name),
            repeat(seq(',', alias($.identifier, $.type), alias($.identifier, $.param_name)))
        ),

        import_statement: $ => seq('import', $.identifier, optional(';')),

        expression_statement: $ => seq($._expression, optional(';')),

        _expression: $ => choice(
            $.matrix,
            $.binary_expression,
            $.modulus_binary_expr,
            $.unary_expression,
            $.function_call,
            $.identifier,
            $.member_access,
            $.number,
            $.percentage,
            $.string,
            seq('(', $._expression, ')')
        ),

        matrix: $ => seq(
            '|',
            repeat1($._expression),
            '|'
        ),

        binary_expression: $ => prec.left(1, seq(
            $._expression,
            $.binary_operator,
            $._expression
        )),

        binary_operator: $ => choice(
            '^', '&', '*', '/', '-', '+',
            '==', '!=', '>', '<', '>=', '<=',
            '.*', './', '.^'
        ),

        modulus_binary_expr: $ => prec.left(2, seq(
            $._expression,
            ' mod ',
            $._expression
        )),

        unary_expression: $ => prec.left(3, seq(
            choice('!', '+', '-'),
            $._expression
        )),

        function_call: $ => seq(
            $.identifier_or_member_access,
            '(',
            optional(seq($._expression, repeat(seq(',', $._expression)))),
            ')'
        ),

        percentage: $ => seq($.number, '%'),

        comment: $ => token(seq('#', /.*/)),

        identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
        identifier_or_member_access: $ => choice($.identifier, $.member_access),
        number: $ => /\d+(\.\d+)?/,

        escape_sequence: $ => token.immediate(/\\[rnt"\\]/),
        formatting_snippet: $ => seq(
            token.immediate('\\{'),
            $._expression,
            token.immediate('\\}')
        ),
        string_content: $ => token(prec(1, /[^"\\]+/)),
        string: $ => seq(
            '"',
            repeat(
                choice(
                    $.formatting_snippet,
                    $.string_content,
                    $.escape_sequence
                )
            ),
            '"'
        )
    }
});

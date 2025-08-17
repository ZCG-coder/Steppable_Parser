module.exports = grammar({
  name: 'stp',
  conflicts: $ => [
    [$.function_call, $._expression],
    [$.function_definition, $._expression],
  ],

  extras: $ => [
    /\s/,
    $.comment,
    /\\\r?\n/
  ],

  rules: {
    source_file: $ => repeat($._statement),

    _statement: $ => choice(
      $.assignment,
      $.object_definition,
      $.function_definition,
      $.expression_statement,
      $.import_statement,
      $.comment
    ),

    assignment: $ => seq(
      $.identifier, '=', $._expression, optional(';')
    ),

    object_definition: $ => seq(
      $.identifier, '{', repeat($.assignment), '}'
    ),

    member_access: $ => seq(
      $._expression,
      '.',
      $.identifier
    ),

    function_definition: $ => seq(
      $.identifier, $.parameter_list, '->', $.type, '{', $._expression, '}'
    ),

    parameter_list: $ => seq(
      $.type, $.identifier, repeat(seq(',', $.type, $.identifier))
    ),

    type: $ => $.identifier,

    import_statement: $ => seq('import', $.identifier, optional(';')),

    expression_statement: $ => seq($._expression, optional(';')),

    _expression: $ => choice(
      $.matrix,
      $.binary_expression,
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
      repeat1($.number),
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

    unary_expression: $ => prec.left(2, seq(
      choice('!', '+', '-'),
      $._expression
    )),

    function_call: $ => seq(
      $.identifier,
      '(',
      optional(seq($._expression, repeat(seq(',', $._expression)))),
      ')'
    ),

    percentage: $ => seq($.number, '%'),

    comment: $ => token(seq('#', /.*/)),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
    number: $ => /\d+(\.\d+)?/,
    string: $ => /"([^"\\]|\\.)*"/
  }
});

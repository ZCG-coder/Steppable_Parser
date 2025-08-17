/**
 * @file Stp grammar for tree-sitter
 * @author Andy Zhang <z-c-ge@outlook.com>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "stp",

  rules: {
    // TODO: add the actual grammar rules
    source_file: $ => "hello"
  }
});

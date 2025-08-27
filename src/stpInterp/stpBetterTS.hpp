#pragma once
#include "tree_sitter/api.h"

#include <string>

inline TSNode ts_node_child_by_field_name(const TSNode& self, const std::string& fieldName)
{
    return ts_node_child_by_field_name(self, fieldName.c_str(), fieldName.length());
}
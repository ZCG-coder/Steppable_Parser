#pragma once

extern char* const STP_typeNames[];

enum STP_TypeID
{
    STP_TypeID_NULL = 0,
    STP_TypeID_NUMBER = 1,
    STP_TypeID_MATRIX_2D = 2,
    STP_TypeID_STRING = 3,
    STP_TypeID_FUNC = 4,
    STP_TypeID_OBJECT = 5,
    STP_TypeID_SYMBOL = 6,
};

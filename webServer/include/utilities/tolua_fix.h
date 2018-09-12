
#ifndef __TOLUA_FIX_H_
#define __TOLUA_FIX_H_

#include "tolua++.h"
    
/**
* Verify the value at the given acceptable index is a function or not.
*
* @param L the current lua_State.
* @param lo the given acceptable index lo of stack.
* @param type useless.
* @param def useless.
* @param err if trigger the error, record the error message to err.
* @return 1 if the value at the given acceptable index is a function, otherwise return 0.
* @lua NA
* @js NA
*/
TOLUA_API int toluafix_isfunction(lua_State* L, int lo, const char* type, int def, tolua_Error* err);

#endif // __TOLUA_FIX_H_

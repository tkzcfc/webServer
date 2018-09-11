#include "tolua_fix.h"
#include <stdio.h>
#include <stdlib.h>


TOLUA_API int toluafix_isfunction(lua_State* L, int lo, const char* type, int def, tolua_Error* err)
{
	if (lua_gettop(L) >= abs(lo) && lua_isfunction(L, lo))
	{
		return 1;
	}
	err->index = lo;
	err->array = 0;
	err->type = "[not function]";
	return 0;
}
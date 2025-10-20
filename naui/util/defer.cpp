#include "defer.h"
#include <vector>

struct NauiDeferCallback
{
	NauiDeferFunction func;
	void* args;
};

static std::vector<NauiDeferCallback> g_deffered_calls;

void naui_defer_internal(NauiDeferFunction func, void* args) 
{
	if(!func)
		return;

	g_deffered_calls.push_back({ func, args });
}

void naui_process_deferred()
{
	for(NauiDeferCallback& callback : g_deffered_calls)
	{
		if(callback.func == nullptr)
			continue;

		callback.func(callback.args);
	}

}

void naui_flush_defer()
{
	g_deffered_calls.clear();
}
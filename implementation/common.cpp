#include "paintbox.h"

namespace Paintbox {
	
	static const char* next_resource_name;
	static const char* next_resource_file;
	static int32_t next_resource_line;
	static bool next_resource_info_set;
	
	void register_resource(Resource* resource) {
		// #thread_safety: This operation should be atomic, but we don't care about that right now. We don't support multithreading anyway.
		
		static uint64_t last_uid;
		
		auto this_uid = last_uid + 1;
		resource->uid = this_uid;
		last_uid = this_uid;
		
		resource->name = next_resource_name;
		resource->file = next_resource_file;
		resource->line = next_resource_line;
		
		next_resource_name = nullptr;
		next_resource_file = nullptr;
		next_resource_line = 0;
		next_resource_info_set = false;
	}
	
	void mark_next_resource(const char* name, const char* file, int32_t line) {
		// #thread_safety: If we have multiple threads, these variables should be per thread.
		
		paintbox_assert_log(!next_resource_info_set, "mark_next_resource() must be called just before creating a new resource.");
		
		next_resource_name = name;
		next_resource_file = file;
		next_resource_line = line;
		next_resource_info_set = true;
	}
	
}
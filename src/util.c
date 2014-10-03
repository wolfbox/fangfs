#include <string.h>
#include "util.h"

int path_join(const char* p1, const char* p2, char* out, size_t out_len) {
	if(out == NULL) return 1;

	size_t i = 0;
	while(p1[i] != 0) {
		if(i >= out_len) {
			goto error;
		}

		out[i] = p1[i];
		i += 1;
	}

	const size_t p1_len = i + 1;

	if((p1_len + 1) >= out_len) {
		goto error;
	}

	size_t p2i = 0;
	// Add a connecting path sep only if we need to
	if(p1[i-1] != '/' && p2[0] != '/') {
		out[i] = '/';
		i += 1;
	} else if(p1[i-1] == '/' && p2[0] == '/') {
		p2i += 1;
	}

	while(p2[p2i] != 0) {
		out[i] = p2[p2i];
		i += 1;
		p2i += 1;

		if(i >= out_len) {
			goto error;
		}
	}

	out[i] = 0;
	return 0;

	error: {
		// Keep people from accidentally using this buffer if they forget to
		// check the return code.
		out[0] = 0;
		return 1;
	}
}

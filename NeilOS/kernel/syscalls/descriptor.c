//
//  descriptor.c
//  product
//
//  Created by Neil Singh on 2/15/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "descriptor.h"
#include <memory/allocation/heap.h>

bool file_descriptor_release(file_descriptor_t* f) {
	if (--f->ref_count == 0) {
		if (f->close) {
			f->close(f);
			kfree(f);
			return true;
		}
	}
	return false;
}

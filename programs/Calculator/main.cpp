//
//  main.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include <NeilOS/NeilOS.h>

#include "Controller.h"

int main(int argc, const char * argv[]) {
	return NSApplication::Run(argc, argv, new Controller);
}

//
//  aistat.mm
//  libaistat
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifdef OBJC_OLD_DISPATCH_PROTOTYPES
#undef OBJC_OLD_DISPATCH_PROTOTYPES
#define OBJC_OLD_DISPATCH_PROTOTYPES 1
#endif

#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void dump(const char *clName, Class cl) {
	SEL supportedSensorsClsSel = sel_registerName("supportedSensors");
	SEL identifierClsSel = sel_registerName("identifier");
	SEL hideIntergratedGraphicsInstSel = sel_registerName("hideIntergratedGraphics");
	SEL maximumMemoryClsSel = sel_registerName("maximumMemory");
	SEL memorySlotsClsSel = sel_registerName("memorySlots");
	SEL screenSizeClsSel = sel_registerName("screenSize");
	SEL nameClsSel = sel_registerName("name");
	SEL variationNameInstSel = sel_registerName("variationName");
	SEL defaultSensorKeysInstSel = sel_registerName("defaultSensorKeys");
	SEL cpuTemperatureSensorsInstSel = sel_registerName("cpuTemperatureSensors");

	NSObject *model = [[cl alloc] init];

	// Check some basics, the rest do not seem to work well.
	if (!class_respondsToSelector(cl, supportedSensorsClsSel))
		return;

	NSString *identifier = objc_msgSend(cl, identifierClsSel);
	NSString *name = objc_msgSend(cl, nameClsSel);
	NSString *variationName = objc_msgSend(model, variationNameInstSel);

	// Screen size is encoded differently in random models @_@
	float screenSize = 0;
	char *type = method_copyReturnType(class_getClassMethod(cl, screenSizeClsSel));
	if (type) {
		if (!strcmp(type, "q"))
			screenSize = (unsigned int)(uint64_t)objc_msgSend(cl, screenSizeClsSel);
		else if (!strcmp(type, "f"))
			screenSize = ((float (*)(id, SEL))objc_msgSend)(cl, screenSizeClsSel);
		free(type);
	}

	printf("Dumping %s from %s\n", [identifier UTF8String], clName);

	NSMutableString *descr = [[NSMutableString alloc] initWithFormat:@"Name: %@", name];
	if (screenSize > 0 && variationName)
		[descr appendFormat:@" (%g inch, %@)", screenSize, variationName];
	else if (screenSize > 0)
		[descr appendFormat:@" (%g inch)", screenSize];
	else if (variationName)
		[descr appendFormat:@" (%@)", variationName];

	unsigned int numSlots = (unsigned int)(uint64_t)objc_msgSend(cl, memorySlotsClsSel);
	unsigned int maxMemory = (unsigned int)((double (*)(id, SEL))objc_msgSend)(cl, maximumMemoryClsSel);

	puts([descr UTF8String]);

	if (numSlots == 0)
		printf("Memory: soldered slots with %u MB total\n", maxMemory);
	else
		printf("Memory: %u slots with %u MB total\n", numSlots, maxMemory);

	NSArray *defaultSensors = objc_msgSend(model, defaultSensorKeysInstSel);
	NSArray *temperatureSensors = objc_msgSend(model, cpuTemperatureSensorsInstSel);
	bool hideIGPU = (bool)(uintptr_t)objc_msgSend(model, hideIntergratedGraphicsInstSel);

	printf("Defaults:");
	for (NSString *sensor in defaultSensors) {
		printf(" %s", [sensor UTF8String]);
	}

	printf(", temperature:");

	for (NSString *sensor in temperatureSensors) {
		printf(" %s", [sensor UTF8String]);
	}

	if (hideIGPU)
		puts(", hidden IGPU");
	else
		puts("");

	NSArray *sensors = objc_msgSend(cl, supportedSensorsClsSel);
	for (NSDictionary *sensor in sensors) {
		NSString *low  = [[NSString stringWithFormat:@"%@", [sensor objectForKey:@"low"]] stringByPaddingToLength:7 withString:@" " startingAtIndex:0];
		NSString *high = [[NSString stringWithFormat:@"%@", [sensor objectForKey:@"high"]] stringByPaddingToLength:7 withString:@" " startingAtIndex:0];

		puts([[NSString stringWithFormat:@"Key %@ low %@ high %@ %@", [sensor objectForKey:@"key"],
			   low, high, [sensor objectForKey:@"name"]] UTF8String]);
	}

	puts("");
}

__attribute__((constructor))
void startup() {
	int num = objc_getClassList(nullptr, 0);
	// printf("Found %d classes\n", num);
	if (num <= 0) return exit(-1);

	Class *classes = (Class *)calloc(num, sizeof(Class));
	// printf("Allocated %d classes at %p\n", num, classes);
	if (!classes) return exit(-1);

	objc_getClassList(classes, num);

	for (int i = 0; i < num; i++) {
		if (!classes[i]) continue;
		const char *name = object_getClassName(classes[i]);
		if (!name) continue;
		if (strncmp(name, "MacBook", strlen("MacBook")) &&
			strncmp(name, "Xserve", strlen("Xserve")) &&
			strncmp(name, "Macmini", strlen("Macmini")) &&
			strncmp(name, "iMac", strlen("iMac")) &&
			strncmp(name, "MacPro", strlen("MacPro")) &&
			strncmp(name, "FauxMac", strlen("FauxMac")))
			continue;

		@try {
			dump(name, classes[i]);
		}
		@catch (NSException *) {
			// This should not happen, but at least we will to continue.
			printf("Failed to obtain info from %s class\n", name);
		}
	}

	free(classes);

	exit(0);
}

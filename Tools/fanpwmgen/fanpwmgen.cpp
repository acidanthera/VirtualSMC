//
//  fanpwmgen.cpp
//  fanpwmgen
//
//  Copyright © 2023 xCuri0. All rights reserved.
//

#include "fanpwmgen.hpp"

int g_fan;

float fltToFpe2(uint16_t *str, int size) {
  if (size != 2)
	return 0;

  return htons(*(uint16_t*)str) * 4.0f;
}

float fpe2ToFlt(char *str, int size) {
  if (size != 2)
	return 0;

  return ntohs(*(uint16_t*)str) / 4.0f;
}

void usage(char *prog) {
  printf("VirtualSMC fan#-pwm generation tool\n");
  printf("Usage:\n");
  printf("%s [options]\n", prog);
  printf("    -f <num>   : the fan number to use\n");
  printf("    -s <steps> : number of steps to use when generating curve, default 16 and max 256\n");
  printf("    -t <time>  : time to wait before going to next step, default 2 seconds\n");
  printf("    -h         : help\n");
  printf("\n");
}

void setManual(int fan, bool manual) {
	SMCVal_t val;
	bool result;

	snprintf(val.key, 5, "F%xMd", fan);
	memcpy(&val.bytes, &manual, 1);
	val.dataSize = 1;

	result = SMCWriteKey(val);
	if (!result) {
	  fprintf(stderr, "Error: SMCWriteKey() = %08x\n", result);
	}
}

void setRpm(int fan, uint16_t rpm) {
	SMCVal_t val;
	bool result;

	snprintf(val.key, 5, "F%xTg", fan);
	
	rpm = fltToFpe2(&rpm, 2);

	memcpy(&val.bytes, &rpm, 2);
	val.dataSize = 2;

	result = SMCWriteKey(val);
	if (!result) {
	  fprintf(stderr, "Error: SMCWriteKey() = %08x\n", result);
	}
}

uint16_t getRpm(uint8_t fan) {
	SMCVal_t val;
	char keyS[5];

	snprintf(keyS, 5, "F%xAc", fan);

	if (!SMCReadKey(keyS, &val))
		return 0;

	return fpe2ToFlt(val.bytes, val.dataSize);
	
}

void restoreAuto(int sig) {
	printf("\nCTRL-C detected, restoring automatic fan control before exiting\n");
	setManual(g_fan, false);
	SMCClose();

	exit(1);
}

int main(int argc, char *argv[]) {
	int c;
	extern char *optarg;
	int fan = -1, steps = 15, time = 2;
	uint16_t rpm = 0, rpm2 = 0;
	char *fanpwm;

	while ((c = getopt(argc, argv, "f:s:t:h")) != -1) {
		switch (c) {
			case 'f':
				fan = atoi(optarg);
				break;
			case 's':
				steps = atoi(optarg);
				break;
			case 't':
				time = atoi(optarg);
				break;
			case 'h':
				usage(argv[0]);
				return 0;
				break;
		}
	}
	if (fan == -1) {
		printf("please specify -f <num> to select fan to use\n\n");
		usage(argv[0]);
		return 1;
	}
	if (255 % steps) {
		printf("steps <%d> doesn't multiply to 255\n", steps);

		return 1;
	}
	if (geteuid() != 0) {
		printf("run as root so that smc keys can be accessed\n");

		return 1;
	}
	fanpwm = (char*)malloc((steps * 10) + 1); // max 10 chars for each section

	printf("\e[1mAvoid high CPU/GPU load activity while this is running as your fans will slow down\e[m\n\n");
	printf("\e[1mMake sure all fan control software is closed\e[m\n\n");
	printf("\e[1mMake sure fan%d-pwm is not already set or output will be invalid\e[m\n\n", fan);

	printf("setting fan #%d to manual control\n", fan);
	printf("generating fan%d-pwm using %d steps waiting %d seconds on each\n", fan, steps, time);

	g_fan = fan;

	if (!SMCOpen())
	  return 1;

	setbuf(stdout, NULL);
	signal(SIGINT, restoreAuto);
	setManual(fan, true);

	for (int i = 0; i <= 255; i += (255 / steps)) {
		printf("pwm set to %d ", i);
		setRpm(fan, i * (3315 / 255));

		// give extra time for fan to spin down
		if (i == 0)
			sleep(time * 3);
		else
			sleep(time);

		rpm2 = getRpm(fan);

		if (rpm2 > rpm)
			rpm = rpm2;

		printf("rpm %d\n", rpm);
		snprintf(fanpwm + strlen(fanpwm), (steps * 10) + 1 - strlen(fanpwm), "%d,%d|", rpm, i);
	}
	setManual(fan, false);
	SMCClose();

	printf("\ndone generating fan%d-pwm \n\n", fan);
	printf("fan%d-pwm: %s\n", fan, fanpwm);
}

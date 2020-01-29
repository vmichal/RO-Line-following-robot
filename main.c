//SENSOR_1 higher value ~ white
int WHITE, DARK;
#define FOLLOWING_LEFT_EDGE true

#ifdef _MSC_VER
#define SENSOR_1
#define OUT_B

#define OUT_C

#define LCD_LINE1
#define LCD_LINE2
#define LCD_LINE3
#define LCD_LINE4
#define LCD_LINE5
#define LCD_LINE6
int StrLen();
void NumToStr();
void TextOut();

void //PlayTone(int, int);
void OnRev(int);
void OnFwd(int);
void NumOut(int, int, int);

void SetSensorMode(int, int);
void SetSensorType(int, int);
void Wait(int);
int CurrentTick();
#define ENUM enum
#else
#define ENUM ;/*Insert an empty statement here, because NXC is trying to be smart by #defining all empty symbols to zero omega lul*/
#endif

#define RIGHT_MOTOR OUT_C
#define LEFT_MOTOR OUT_B

enum direction {
	dir_left, dir_right
};


int measure_and_update_bounds() {
	const int value = SENSOR_1;
	if (value < DARK)
		DARK = value;
	if (value > WHITE)
		WHITE = value;
	return value;
}

int spectrum_width() { return WHITE - DARK; }

#if 0
void print_data() {
	NumOut(0, LCD_LINE2, WHITE);
	NumOut(0, LCD_LINE3, measure_and_update_bounds());
	NumOut(0, LCD_LINE4, DARK);
	NumOut(0, LCD_LINE5, CurrentTick());
}


void beep(int times) {
	for (int i = 0; i < times; ++i) {
		//PlayTone(440, 100);
		Wait(50);
	}
}

#else
#define beep(x) ;
#define print_data() ;
#endif

int clamp(const int val, const int min, const int max) {
	if (val < min)
		return min;
	return val > max ? max : val;
}

void right_motor(int val) {
	val = clamp(val, -100, 100);
	if (val < 0)
		OnRev(RIGHT_MOTOR, -val);
	else
		OnFwd(RIGHT_MOTOR, val);
}

void left_motor(int val) {
	val = clamp(val, -100, 100);
	if (val < 0)
		OnRev(LEFT_MOTOR, -val);
	else
		OnFwd(LEFT_MOTOR, val);
}

void move(const int left, const int right, const unsigned long duration) {
	left_motor(left);
	right_motor(right);

	for (unsigned long phase_end = CurrentTick() + duration; CurrentTick() < phase_end;) {
		print_data();
		NumOut(0, LCD_LINE6, phase_end - CurrentTick());
		measure_and_update_bounds();
	}
	left_motor(0);
	right_motor(0);
}


ENUM direction calibrate() {
	DARK = 999;
	WHITE = 0;
	measure_and_update_bounds(); //initial measurement which sets the values

	const unsigned long phase_duration = 800;
	const int speed = 30;

	beep(1);
	move(-speed, speed, phase_duration);


	beep(1);
	move(speed, -speed, phase_duration);

	const int spectrum_left = spectrum_width();

	beep(1);
	move(speed, -speed, phase_duration);

	beep(1);
	move(-speed, speed, phase_duration);
	beep(1);

	const int spectrum_right = spectrum_width();

	return spectrum_left > (spectrum_right / 2) ? dir_left : dir_right;
}


void follow_line(const unsigned long duration, const int standard_speed) {

	const int P = 1, I = 0, D = 0;
	//TODO leave I and D equal to zero, because the ID part just does not fucking work
	int last_error = 0, integral = 0;


	for (unsigned long time_limit = CurrentTick() + duration; CurrentTick() < time_limit; ) {
		const int desired = (WHITE + DARK) / 2;
		const int error = measure_and_update_bounds() - desired;

		integral += error;
		const int derivate = last_error - error;

		int drag_to_line = //as of now this var can have values from -50 to 50 
			P * (error * 100 / spectrum_width())
			+ I * integral
			+ D * derivate;

		if (FOLLOWING_LEFT_EDGE) {
			left_motor(standard_speed + drag_to_line);
			right_motor(standard_speed - drag_to_line);
		}
		else {
			left_motor(standard_speed - drag_to_line);
			right_motor(standard_speed + drag_to_line);
		}
		last_error = error;

		print_data();
		NumOut(0, LCD_LINE6, drag_to_line);
	}
}

void find_line(const direction direction) {
	Wait(1000);

	const bool comming_from_right = direction == dir_left;

	if (comming_from_right)
		right_motor(20);
	else
		left_motor(20);

	const int desired = (WHITE + DARK) / 2;
	while (abs(measure_and_update_bounds() - desired) > desired / 16);

	if (FOLLOWING_LEFT_EDGE == comming_from_right) {
		if (comming_from_right)
			left_motor(10);
		else
			right_motor(10);
		Wait(100);
		while (abs(measure_and_update_bounds() - desired) > desired / 16);
	}


	if (comming_from_right) {
		left_motor(20);
		right_motor(0);
	}
	else {
		right_motor(20);
		left_motor(0);
	}
	Wait(100);
	right_motor(0);
	left_motor(0);

	//PlayTone(800, 500);
	follow_line(1000, 20);
}



task main() {

	SetSensorType(S1, SENSOR_TYPE_LIGHT_ACTIVE);

	SetSensorMode(S1, SENSOR_MODE_RAW);

	//PlayTone(800, 100);
	Wait(1000);

	ENUM direction dir_to_line = calibrate();

	if (spectrum_width() < 100) {
		TextOut(0, LCD_LINE1, "Cannot find", true);
		TextOut(0, LCD_LINE2, "the line!");
		beep(5);
		Wait(20 * 1000);
		StopAllTasks();
	}

	find_line(dir_to_line);


	//PlayTone(800, 500);
	for (int i = 5; i <50; i += 5)
		follow_line(50, i);
	follow_line(110 * 1000, 50);

	right_motor(0);
	left_motor(0);
	TextOut(0, LCD_LINE1, "Time limit", true);
	TextOut(0, LCD_LINE2, "exceeded!");
	beep(5);

	Wait(1000 * 1000);
}
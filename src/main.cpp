#include "main.h"
#include "apix.h"
#include <fcntl.h>	// POSIX open()
#include <unistd.h> // POSIX close() and related functions
#include <stdio.h>	// Header para printf()
#include <cstring>	// Header para memset()

/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
void on_center_button()
{
	static bool pressed = false;
	pressed = !pressed;
	if (pressed)
	{
		pros::lcd::set_text(2, "I was pressed!");
	}
	else
	{
		pros::lcd::clear_line(2);
	}
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void readPipe()
{
	pros::c::serctl(SERCTL_DISABLE_COBS, nullptr);

	int32_t fd = open("serial", O_RDWR); // Abre el archivo serial en modo lectura/escritura
	if (fd == -1)
	{
		printf("Error al abrir el pipe serial.\n");
		return;
	}

	// Configurar la tasa de baudios a 9600
	pros::c::fdctl(fd, DEVCTL_SET_BAUDRATE, (void *)9600);

	// Buffer para almacenar los datos leídos
	char buffer[256]; // Tamaño del buffer, ajusta según tus necesidades
	ssize_t bytesRead;

	// Bucle para leer continuamente del pipe
	while (true)
	{
		memset(buffer, 0, sizeof(buffer));				  // Limpia el buffer
		bytesRead = read(fd, buffer, sizeof(buffer) - 1); // Lee los datos

		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';				 // Asegura que el buffer esté terminado en null
			printf("Datos recibidos: %s\n", buffer); // Imprime los datos recibidos
		}
		else if (bytesRead == -1)
		{
			printf("Error al leer desde el pipe serial.\n");
			break; // Sale del bucle en caso de error
		}

		pros::delay(20); // Retardo para no sobrecargar el bucle
	}

	close(fd); // Cierra el archivo descriptor al finalizar
}

void initialize()
{
	pros::lcd::initialize();
	pros::lcd::set_text(1, "Hello PROS User!");
	pros::lcd::register_btn1_cb(on_center_button);

	// Crear un hilo para ejecutar la función readPipe
	pros::Task readPipeTask(readPipe); // Crea un nuevo hilo que ejecuta la función readPipe
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol()
{
	pros::Controller master(pros::E_CONTROLLER_MASTER);
	pros::MotorGroup left_mg({1, -2, 3});	// Creates a motor group with forwards ports 1 & 3 and reversed port 2
	pros::MotorGroup right_mg({-4, 5, -6}); // Creates a motor group with forwards port 5 and reversed ports 4 & 6

	while (true)
	{
		pros::lcd::print(0, "%d %d %d", (pros::lcd::read_buttons() & LCD_BTN_LEFT) >> 2,
						 (pros::lcd::read_buttons() & LCD_BTN_CENTER) >> 1,
						 (pros::lcd::read_buttons() & LCD_BTN_RIGHT) >> 0); // Prints status of the emulated screen LCDs

		// Arcade control scheme
		int dir = master.get_analog(ANALOG_LEFT_Y);	  // Gets amount forward/backward from left joystick
		int turn = master.get_analog(ANALOG_RIGHT_X); // Gets the turn left/right from right joystick
		left_mg.move(dir - turn);					  // Sets left motor voltage
		right_mg.move(dir + turn);					  // Sets right motor voltage
		pros::delay(20);							  // Run for 20 ms then update
	}
}
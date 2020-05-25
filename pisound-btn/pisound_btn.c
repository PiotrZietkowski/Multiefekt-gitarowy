#include <stdio.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

/// <summary>
/// Dlugosc sciezki wczytywanego skryptu
/// </summary>
int MAX_PATH_LENGTH = 100;

/// <summary>
/// Domyslna konfiguracja przycisku
/// </summary>
static const char *const Skrypt1  = BASE_SCRIPTS_DIR "/Skrypt1.sh";

/// <summary>
/// Sciezka do pliku konfiguracyjnego
/// </summary>
static char g_config_path[MAX_PATH_LENGTH+1]  = "/etc/pisound.conf";

/// <summary>
/// Stany przycisku
/// </summary>
enum action_e
{
	A_DOWN = 0, // Executed every time the button is pushed down.
	A_UP,       // Executed every time the button is released up.
	A_CLICK,    // Executed when the button is short-clicked one or multiple times in quick succession.
	A_HOLD,     // Executed if the button was held for given time.

	// Must be the last one!
	A_COUNT
};

/// <summary>
/// Funkcja wczytujaca plik konfiguracyjny z podanej sciezki
/// </summary>
/// <param name="path_name">Sciezka pliku konfiguracyjnego</param>
static void read_config(const char path_name)
{
	//...
}

/// <summary>
/// Funkcja wczytujaca domyslna konfiguracje
/// </summary>
/// <param name="default_path_name">Sciezka domyslnego pliku konfiguracyjnego</param>
static void get_default_action_and_script(const char default_path_name)
{
	//...
}

/// <summary>
/// Funkcja zwracajaca dlugosc sciezki pliku z konfiguracja
/// </summary>
/// <param name="action">Stan przycisku</param>
static int get_action_script_path(enum action_e action)
{
	//...
}

/// <summary>
/// Wykrycie stanu przycisku i rozpoczecie zwiazanej z nim akcji
/// </summary>
/// <param name="action">Stan przycisku</param>
static void execute_action(enum action_e action)
{
	//...
}

/// <summary>
/// Wlaczenie podanego pinu systemowego interfejsu GPIO
/// </summary>
/// <param name="pin">Numer pinu</param>
static int gpio_export(int pin)
{
	if (!gpio_is_pin_valid(pin))
	{
		fprintf(stderr, "Invalid pin number %d!\n", pin);
		return -1;
	}

	char gpio[64];

	snprintf(gpio, sizeof(gpio), "/sys/class/gpio/gpio%d", pin);

	struct stat s;
	if (stat(gpio, &s) != 0)
	{
		int fd = open("/sys/class/gpio/export", O_WRONLY);
		if (fd == -1)
		{
			fprintf(stderr, "Failed top open /sys/class/gpio/export!\n");
			return -1;
		}
		char str_pin[4];
		snprintf(str_pin, 3, "%d", pin);
		str_pin[3] = '\0';
		const int n = strlen(str_pin)+1;
		int result = write(fd, str_pin, n);
		if (result != n)
		{
			fprintf(stderr, "Failed writing to /sys/class/gpio/export! Error %d.\n",  errno);
			close(fd);
			return -1;
		}
		result = close(fd);
		if (result != 0)
		{
			fprintf(stderr, "Failed closing /sys/class/gpio/export! Error %d.\n", errno);
			return -1;
		}
		// Give some time for the pin to appear.
		usleep(100000);
		return 1;
	}

	// Already exported.
	return 0;
}

/// <summary>
/// Wylaczenie podanego pinu systemowego interfejsu GPIO
/// </summary>
/// <param name="pin">Numer pinu</param>
static int gpio_unexport(int pin)
{
	if (!gpio_is_pin_valid(pin))
	{
		fprintf(stderr, "Invalid pin number %d!\n", pin);
		return -1;
	}

	char gpio[64];

	snprintf(gpio, sizeof(gpio), "/sys/class/gpio/gpio%d", pin);

	struct stat s;
	if (stat(gpio, &s) == 0)
	{
		int fd = open("/sys/class/gpio/unexport", O_WRONLY);
		if (fd == -1)
		{
			fprintf(stderr, "Failed top open /sys/class/gpio/unexport!\n");
			return -1;
		}
		char str_pin[4];
		snprintf(str_pin, 3, "%d", pin);
		str_pin[3] = '\0';
		const int n = strlen(str_pin)+1;
		int result = write(fd, str_pin, n);
		if (result != n)
		{
			fprintf(stderr, "Failed writing to /sys/class/gpio/unexport! Error %d.\n",  errno);
			close(fd);
			return -1;
		}
		result = close(fd);
		if (result != 0)
		{
			fprintf(stderr, "Failed closing /sys/class/gpio/unexport! Error %d.\n", errno);
			return -1;
		}
		return 0;
	}

	// Already unexported.
	return 0;
}

/// <summary>
/// Pobranie indeksu pinu i jego walidacja
/// </summary>
/// <param name="pin">Numer pinu</param>
static int gpio_open(int pin)
{
	if (!gpio_is_pin_valid(pin))
	{
		fprintf(stderr, "Invalid pin number %d!\n", pin);
		return -1;
	}

	char gpio[64];

	snprintf(gpio, sizeof(gpio), "/sys/class/gpio/gpio%d/value", pin);

	int fd = open(gpio, O_RDONLY);

	if (fd == -1)
	{
		fprintf(stderr, "Failed opening %s! Error %d.\n", gpio, errno);
	}

	return fd;
}

/// <summary>
/// Zakonczenie strumienia z dostepem do pinu
/// </summary>
/// <param name="fd">Strumien pinu</param>
static int gpio_close(int fd)
{
	int err = close(fd);
	if (err != 0)
	{
		fprintf(stderr, "Failed closing descriptor %d! Error %d.\n", fd, err);
		return -1;
	}
	return 0;
}

/// <summary>
/// Glowna funkcja programu
/// </summary>
int main()
{
	//...
}
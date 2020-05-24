#include <stdio.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

static const char *const DEFAULT_HOLD_1S  = BASE_SCRIPTS_DIR "/do_nothing.sh";
static char g_config_path[MAX_PATH_LENGTH+1]  = "/etc/pisound.conf";

static void read_config_value(const char *conf, const char *value_name, char *dst, char *args, size_t n, const char *default_value)
{
	const size_t BUFFER_SIZE = 2 * MAX_PATH_LENGTH + 1;
	char line[BUFFER_SIZE];
	char name[BUFFER_SIZE];
	char argument[BUFFER_SIZE];
	char value[BUFFER_SIZE];

	size_t currentLine = 0;

	bool found = false;

	FILE *f = fopen(conf, "rt");

	while (f && !feof(f))
	{
		if (read_line(f, line, sizeof(line)))
		{
			++currentLine;

			argument[0] = '\0';

			char *commentMarker = strchr(line, '#');
			if (commentMarker)
				*commentMarker = '\0'; // Ignore comments.

			char *endline = strchr(line, '\n');
			if (endline)
				*endline = '\0'; // Ignore endline.

			static const char *WHITESPACE_CHARS = " \t";

			char *p = line + strspn(line, WHITESPACE_CHARS);

			if (strlen(p) > 1)
			{
				char *t = strpbrk(p, WHITESPACE_CHARS);

				if (t)
				{
					strncpy(name, p, t - p);
					name[t - p] = '\0';
				}

				t = t + strspn(t, WHITESPACE_CHARS);

				if (args)
				{
					char *a1 = strpbrk(t, WHITESPACE_CHARS);
					if (a1)
					{
						char *a2 = a1 + strspn(a1, WHITESPACE_CHARS);
						if (a2)
						{
							strcpy(argument, a2);
						}
						*a1 = '\0';
					}
				}

				strcpy(value, t);

				if (strlen(name) != 0 && strlen(value) != 0)
				{
					if (strcmp(name, value_name) == 0)
					{
						size_t lenValue = strlen(value);
						size_t lenArgs = strlen(argument);
						if (lenValue < n && lenArgs < n)
						{
							found = true;

							strncpy(dst, value, lenValue);
							dst[lenValue] = '\0';

							if (args)
							{
								strncpy(args, argument, lenArgs);
								args[lenArgs] = '\0';
							}
							break;
						}
						else
						{
							fprintf(stderr, "Too long value set in %s on line %u!\n", conf, currentLine);
						}
					}
				}
				else
				{
					fprintf(stderr, "Unexpected syntax in %s on line %u!\n", conf, currentLine);
				}
			}
		}
	}

	if (f)
		fclose(f);

	if (!found)
	{
		strcpy(dst, default_value);
		if (args)
			*args = '\0';
	}
}

static void get_default_action_and_script(enum action_e action, unsigned arg0, unsigned arg1, const char **name, const char **script)
{
	switch (action)
	{
	case A_DOWN:
		*name = DOWN_VALUE_NAME;
		*script = DEFAULT_DOWN;
		break;
	case A_UP:
		*name = UP_VALUE_NAME;
		*script = DEFAULT_UP;
		break;
	case A_CLICK:
		switch (arg0)
		{
		case 1:
			*name = CLICK_1_VALUE_NAME;
			*script = DEFAULT_CLICK_1;
			break;
		case 2:
			*name = CLICK_2_VALUE_NAME;
			*script = DEFAULT_CLICK_2;
			break;
		case 3:
			*name = CLICK_3_VALUE_NAME;
			*script = DEFAULT_CLICK_3;
			break;
		default:
			*name = CLICK_OTHER_VALUE_NAME;
			*script = DEFAULT_CLICK_OTHER;
			break;
		}
		break;
	case A_HOLD:
		if (arg1 < 3000)
		{
			*name = HOLD_1S_VALUE_NAME;
			*script = DEFAULT_HOLD_1S;
		}
		else if (arg1 < 5000)
		{
			*name = HOLD_3S_VALUE_NAME;
			*script = DEFAULT_HOLD_3S;
		}
		else if (arg1 < 7000)
		{
			*name = HOLD_5S_VALUE_NAME;
			*script = DEFAULT_HOLD_5S;
		}
		else
		{
			*name = HOLD_OTHER_VALUE_NAME;
			*script = DEFAULT_HOLD_OTHER;
		}
		break;
	default:
		*name = NULL;
		*script = NULL;
		break;
	}
}

static int get_action_script_path(enum action_e action, unsigned arg0, unsigned arg1, char *dst, char *args, size_t n)
{
	if (action < 0 || action >= A_COUNT)
		return -EINVAL;

	const char *action_name = NULL;
	const char *default_script = NULL;

	get_default_action_and_script(action, arg0, arg1, &action_name, &default_script);

	if (action_name == NULL || default_script == NULL)
		return -EINVAL;

	char script[MAX_PATH_LENGTH + 1];
	char arguments[MAX_PATH_LENGTH + 1];

	read_config_value(g_config_path, action_name, script, arguments, MAX_PATH_LENGTH, default_script);

	fprintf(stderr, "script = '%s', args = '%s'\n", script, arguments);

	// The path is absolute.
	if (script[0] == '/')
	{
		strncpy(dst, script, n-2);
		dst[n-1] = '\0';
	}
	else // The path is relative to the config file location.
	{
		char tmp[MAX_PATH_LENGTH + 1];

		size_t pathLength = strlen(g_config_path);
		strncpy(tmp, g_config_path, sizeof(tmp)-1);
		dirname(tmp);
		strncat(tmp, "/", sizeof(tmp)-1 - strlen(tmp));
		strncat(tmp, script, sizeof(tmp)-1 - strlen(tmp));
		tmp[sizeof(tmp)-1] = '\0';

		strncpy(dst, tmp, n-1);
		dst[n-1] = '\0';
	}

	strncpy(args, arguments, n-1);
	args[n-1] = '\0';

	return strlen(dst);
}

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


int main(int argc, char **argv, char **envp)
{
	
}
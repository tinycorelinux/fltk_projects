#include "mirrorpicker.h"
#include "other.h"

#define PATH "/usr/local/share/mirrors"
//#define PATH "/tmp/mirrors"

#define TMPSIZE 1024

const char *bestmirror;

static void *testmirror(void *in) {

	const char * const url = (const char * const) in;

	char tmp[PATH_MAX];
	char target[] = "/tmp/mp_XXXXXX";

	mktemp(target);

	snprintf(tmp, PATH_MAX, "wget -q -O %s %s/6.x/x86/tcz/info.lst.gz", target, url);

	// Temp name made. Start clocking
	struct timeval start, end;
	gettimeofday(&start, NULL);

	int ret = system(tmp);
	ret = WEXITSTATUS(ret);

	gettimeofday(&end, NULL);
	unlink(target);

	unsigned int usec = end.tv_sec - start.tv_sec;
	usec *= 1e6;
	usec += end.tv_usec - start.tv_usec;

	// If the fetch failed, add 100 sec
	if (ret)
		usec += 100000000;

	unsigned int *res = (unsigned int *) malloc(sizeof(unsigned int));
	*res = usec;

	return (void *) res;
}

void init(int argc, char **argv) {

	win->show(argc, argv);

	int ret = access(PATH, R_OK);

	// file not found, try to install
	if (ret) {
		system("tce-load -il mirrors.tcz");
	}

	ret = access(PATH, R_OK);

	// file not found, try to download+install
	if (ret) {
		system("tce-load -wil mirrors.tcz");
	}

	ret = access(PATH, R_OK);

	// No net access?
	if (ret) {
		fl_message(gettext("Couldn't load mirror list. Maybe network issue?"));
		exit(1);
	}

	Fl::check();

	// Read the mirrors
	FILE *f = fopen(PATH, "r");
	if (!f) exit(1);

	char buf[TMPSIZE];
	unsigned int count = 0;

	while (fgets(buf, TMPSIZE, f))
		count++;

	rewind(f);

	const char **mirrors;

	mirrors = (const char **) calloc(count + 1, sizeof(char *));
	unsigned int *results = (unsigned int *) calloc(count, sizeof(unsigned int));
	if (!mirrors || !results) exit(1);

	prog->minimum(0);
	prog->maximum(count - 1);

	count = 0;
	while (fgets(buf, TMPSIZE, f)) {
		char *tmp = buf;
		for (; *tmp; tmp++) if (*tmp == '\n') *tmp = 0;

		mirrors[count] = strdup(buf);

		count++;
	}

	fclose(f);

	// Start the threads
	pthread_t tids[count];

	unsigned int i;
	for (i = 0; i < count; i++) {
		pthread_create(&tids[i], NULL, testmirror, (void *) mirrors[i]);
	}

	sprintf(buf, gettext("Checking %u mirrors, please wait..."), count);
	lbl->label(buf);
	Fl::check();

	// Wait for them
	for (i = 0; i < count; i++) {
		void *ptr;
		pthread_join(tids[i], &ptr);
		results[i] = * (unsigned *) ptr;
		prog->value(i);
		Fl::check();
	}

	unsigned int best = 0;
	for (i = 1; i < count; i++) {
		if (results[i] < results[best]) best = i;
	}

	const char *ptr = strchr(mirrors[best], '.');
	ptr = strchr(ptr, '/');

	const unsigned int len = ptr - mirrors[best];

	sprintf(buf, gettext("The fastest mirror was %.*s. Press ok to set it as your mirror."),
		len, mirrors[best]);
	lbl->copy_label(buf);

	bestmirror = mirrors[best];

	okbtn->activate();
}

void accept() {

	FILE *f = fopen("/opt/tcemirror", "w");
	if (!f) exit(1);

	fprintf(f, "%s\n", bestmirror);

	fclose(f);
//	fl_message(gettext("Success."));
	exit(0);
}

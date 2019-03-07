#ifndef PIDFILE_H
#define PIDFILE_H

char* read_file(FILE* file)
{    	
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(length);
    fread(buffer, length, 1, file);
    return buffer;
}

char* get_pidfilename(const char* prog)
{
    char* pidfilename = new char[256];
    // put the pidfile into the home dir, because we should always be allowed to create files there (but may not be allowed to write into /tmp)
    char* env_home_p = getenv("HOME");
    sprintf(pidfilename, "%s/.%s.pid", env_home_p, prog);
    return pidfilename;
}

bool is_already_running(const char* prog)
{
    const char* pidfilename = get_pidfilename(prog);

    FILE* pidfile = fopen(pidfilename, "r");
    if(pidfile)
    {
	// get pid from pidfile
	char *buffer = read_file(pidfile);
	fclose(pidfile);
	int pid = atoi(buffer);
	free(buffer);

	// check proc filesystem for a process with the given pid
	char proc_comm_filename[256];	
	sprintf(proc_comm_filename, "/proc/%d/comm", pid);
	FILE* proc_comm_file = fopen(proc_comm_filename, "r");

	if(proc_comm_file)
	{
	    // a process with the given pid exists, read it's name and check if it's prog
	    // due to pid reusing it might be another process (unlikely but possible)
	    char buffer[256];
	    // read using fgets because the kernel generates the information 'on-the-fly' and thus the virtual files appeare to have a size of zero
	    fgets(buffer, 256, proc_comm_file);
	    fclose(proc_comm_file);
	    
	    // remove trailing \n read by fgets
	    char* nl = strrchr(buffer, '\n');
            if (nl) *nl = '\0';

	    if(strcmp(prog, buffer) == 0)
	    {
		return true;
	    }
	}
    }
	
    // either the pidfile is not existent or the pidfile exists, but there is no process with the given pid or it is not prog
    // => go on
    return false;
}

void create_pidfile(const char* prog)
{
    FILE* pidfile = fopen(get_pidfilename(prog), "w");
    fprintf(pidfile, "%d", getpid());
    fclose(pidfile);
}

void delete_pidfile(const char* prog)
{
    remove(get_pidfilename(prog));
}

#endif

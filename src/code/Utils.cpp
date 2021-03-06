/*
 * Utils.cpp
 *
 *  Created on: Jul 14, 2015
 *      Author: brandon
 */

#include "Utils.hh"

using namespace boost::filesystem;
using namespace std;

string convertToAbsPath(string relPath) {
	path fullPath;

	try{
		fullPath = canonical(relPath);
	}
	catch(...){}

	if (!exists(fullPath)){
		string errMsg = "Error: Can't find the file: " + relPath;
		outputError(errMsg);
	}

	return fullPath.string();
}

int checkIfFileExists(string path) {
	if (exists(path))
		return 1;
	else
		return 0;
}

void split(const string& str, vector<string>& tokens, const string& delimiters) {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

string getFilename(string path) {
	return boost::filesystem::path(path).stem().string();
}

void outputHelp() {
	cout << "GenFit2 Usage:" << endl;
	cout << "GenFit2 [config_file]" << endl;
}

bool createParticlePipe(const char * path) {

	if (checkIfFileExists(path)) {
		unlink(path);
	}

	int fifo_status = mkfifo(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (fifo_status) {
		cout << "Warning: Couldn't create pipe with path: " << path << endl;
		return false;
	}
	else {
		return true;
	}
}

bool isFloat(string number) {
	istringstream iss(number);
	float f;
	iss >> noskipws >> f; // noskipws considers leading whitespace invalid
	// Check the entire string was consumed and if either failbit or badbit is set
	return iss.eof() && !iss.fail();
}

int runCommand(string cmd, string &result) {

	std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);

	if (!pipe) {
		return 1;
	}

	char buffer[1024];
	result.clear();

	while (!feof(pipe.get())) {
		if (fgets(buffer, 1024, pipe.get()) != NULL)
			result += buffer;
	}

	return 0;
}

int runCommand(string cmd) {

	//cout << "Running command: " << cmd << endl;


	int ret = system(cmd.c_str());
	return ret;



	/*
	FILE *in;

	//cout << "Running command: " << cmd << endl;

	if(!(in = popen(cmd.c_str(), "r"))){
		return 1;
	}
	int status = pclose(in);
	//printf("Exit code: %d\n", WEXITSTATUS(status));

	return status;
	*/

	/*
	char *c_cmd = new char[cmd.length()+1];
	std::strcpy(c_cmd, cmd.c_str());

	pid_t pid;
	char *argv[] = {"bash", "-c", c_cmd, NULL};
	int status;
	printf("Run command: %s\n", c_cmd);
	status = posix_spawn(&pid, "/bin/bash", NULL, NULL, argv, environ);
	if (status == 0) {
		printf("Child pid: %i\n", pid);
		if (waitpid(pid, &status, 0) != -1) {
			printf("Child exited with status %i\n", status);
		} else {
			perror("waitpid");
		}
	} else {
		printf("posix_spawn: %s\n", strerror(status));
	}

	return status;
	*/
}

void outputError(string errorMessage) {
	cout << errorMessage << endl;

	exit (1);
}

string toString(unsigned int theNumber) {
	return to_string(static_cast<long long int>(theNumber));
}

string toString(int theNumber) {
	return to_string(static_cast<long long int>(theNumber));
}

string toString(double theNumber) {
	return to_string(static_cast<long double>(theNumber));
}

string toString(float theNumber) {
	return to_string(static_cast<long double>(theNumber));
}

string toString(unsigned long theNumber) {
	return to_string(static_cast<long double>(theNumber));
}

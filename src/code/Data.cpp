/*
 * Exp.cpp
 *
 *  Created on: Jul 17, 2015
 *      Author: brandon
 */

//TODO: Test all these methods
#include "Data.hh"

using namespace std;

Data::Data(std::string path, Swarm * swarm, bool isExp) {
	swarm_ = swarm;
	isExp_ = isExp;

	dataPath = convertToAbsPath(path);
	Data::parseData();

	dataCurrent = &dataOrig_;

	if ( (swarm_->options.objFunc == 4 && isExp)) {
		getColumnAverages();
	}

	if (swarm_->options.divideByInit && !isExp) {
		divideByInit();
		dataCurrent = &dataDividedByInit_;
	}

	if (swarm_->options.logTransformSimData > 0 && !isExp) {
		logTransformData();
		dataCurrent = &dataLogTransformed_;
	}

	if (swarm_->options.standardizeSimData && !isExp) {
		cout << "stand sim" << endl;
		if (swarm_->options.objFunc != 4) {
			getColumnAverages();
		}
		standardizeData();
		dataCurrent = &dataStandardized_;
	}

	if (swarm_->options.standardizeExpData && isExp) {
		cout << "stand exp" << endl;
		if (swarm_->options.objFunc != 4) {
			getColumnAverages();
		}
		standardizeData();
		dataCurrent = &dataStandardized_;
	}
}

Data::Data(map<string, map<double, double>> &dataSet) {
	dataOrig_ = dataSet;
	dataCurrent = &dataOrig_;
	isExp_ = false;
	swarm_ = 0;
}

Data::Data() {
	map<string, map<double, double>> dummySet;
	dataOrig_ = dummySet;
	dataCurrent = 0;
	isExp_ = false;
	swarm_ = 0;
}

std::string Data::getPath() {
	if (!dataPath.empty())
		return dataPath;
	else
		return "empty";
}

void Data::parseData(){
	vector<string> allLines;

	if (swarm_->options.usePipes){
		char buf[5120];
		string line;

		int fd = open(dataPath.c_str(), O_RDONLY);

		if (fd < 0) {
			cout << "Warning: couldn't open " << dataPath << " to gather simulation output." << endl;
		}

		int len;
		while( (len = read(fd, buf, 5120) )){
			buf[len] = 0;
			line = string(buf);
			//cout << line << "!";
			split(line, allLines, "\n");
			//printf("%s", buf);
		}

		if (len < 0) {
			perror("read error");
		}

		close(fd);
	}
	else {

		ifstream dataFile(dataPath);

		std::string line;
		std::string errMsg;

		if (dataFile.is_open()) {
			while (getline(dataFile, line)) {
				allLines.push_back(line);
				//cout << "line: " << line << endl;
			}
		}
		else {
			swarm_->outputError("Error: Couldn't open data file " + dataPath + " for parsing.");
		}
		dataFile.close();
	}

	string replacement = "";
	string basename = boost::regex_replace(getFilename(dataPath),boost::regex("_\\d+_\\d+$"),replacement);

	if(allLines[0].at(0) != '#') {
		swarm_->outputError("Error: Your data file (" + dataPath + ") doesn't contain (#) as the first value.");
	}

	// Do we need to store columns that aren't in .exp?  Maybe not.
	vector<string> columns;
	split(allLines[0], columns, " \t");

	for (vector<string>::iterator l = allLines.begin()+1; l != allLines.end(); ++l) {
		vector<string> values;
		split(*l, values," \t");

		int i = 1;
		for(vector<string>::iterator col = columns.begin(); col != columns.end(); ++col) {

			//cout << "col: " << *col << endl;

			// TODO: Haven't done prefix/exp consistency checks yet so it's possible we're missing
			// a scan parameter.  This needs fixed..
			// Skip column if we encounter a hash, a 'time' column, or a column corresponding to a scan parameter name
			if (*col == "#" || *col == "time" || (swarm_->options.model->actions.find(basename) != swarm_->options.model->actions.end() && *col == swarm_->options.model->actions.at(basename).scanParam)){
				continue;
			}

			double value;
			// Replace any NaN's with our own NaN constant
			if (values[i] == "NaN") {
				value = nan("1");
			}
			else {
				value = stod(values[i]);
			}

			// If our column name ends in "_SD" we need to store the value in our stdev map
			if (col->size() > 3 && col->find("_SD") == col->size()-3) {
				string colWithoutSD = boost::regex_replace(*col,boost::regex("_SD$"),string(""));
				//cout << "inserting sd at col " << colWithoutSD << ": " << stof(values[0]) << " " << stof(values[i]) << endl;
				//standardDeviations[col].insert(make_pair(stof(values[0]),stof(values[i])));
				standardDeviations[colWithoutSD][stod(values[0])] = stod(values[i]);
				//cout << "test " << colWithoutSD << ": " << standardDeviations[colWithoutSD][stof(values[0])] << endl;
				// TODO: Make sure all SD's have values if we're using objfunc 2, lest we div_by_0
				// TODO: Make sure all normal columns also have SD columns if we're using objfunc 2
			}
			// Insert value into data map
			else {
				//cout << "inserting val to col " << *col << ": " << values[0] << " " << value << endl;
				dataOrig_[*col][stod(values[0])] = value;
			}
			++i;
		}
	}
}

void Data::standardizeData() {
	// This function standardizes our data around a mean of 0

	double mean;
	double sqtotal;
	int counter;
	double stdev;

	// Loop through data map
	for(map<string,map<double,double> >::iterator c = dataCurrent->begin(); c != dataCurrent->end(); ++c) {

		// Grab our mean, which has already been calculated
		mean = colAverages[c->first];
		sqtotal = 0;
		counter = 0;

		// Skip the column if mean is already 0
		if (mean == 0) {
			continue;
		}

		// Here we get the riemann sum squared for use in the stdev calculation
		for(map<double,double>::iterator v = c->second.begin(); v != c->second.end(); ++v) {
			//cout << "sd loop" << endl;
			if (std::isnan(v->second)) {
				continue;
			}
			sqtotal += pow(mean - v->second, 2);
			counter++;
		}

		// Cacualate the standard deviation
		stdev = pow((sqtotal / (counter - 1)), 0.5);

		// Loop through and set values according to formula: val_new = (val_old - column_mean) / (column_stdev)
		for(map<double,double>::iterator v = c->second.begin(); v != c->second.end(); ++v) {
			//cout << "stand loop" << endl;
			if (std::isnan(v->second)) {
				dataStandardized_[c->first][v->first] = nan("1");
				continue;
			}
			//cout << "inserting: " << (v->second - mean) / stdev << endl;
			dataStandardized_[c->first][v->first] = (v->second - mean) / stdev;
		}
	}
}

void Data::divideByInit() {
	for(map<string,map<double,double> >::iterator c = dataCurrent->begin(); c != dataCurrent->end(); ++c) {
		for(map<double,double>::iterator v = c->second.begin(); v != c->second.end(); ++v) {
			if (v->second == 0) {
				swarm_->outputError("You chose to divide_by_init, but the first value in the column '" + c->first + "' is 0. We cannot divide by 0.");
			}
			if (std::isnan(v->second)) {
				dataDividedByInit_[c->first][v->first] = nan("1");
				continue;
			}
			dataDividedByInit_[c->first][v->first] = v->second / c->second.begin()->first;
			//cout << "dividing t=" << v->first << " of " << v->second << "by " << c->first << " init of " << c->second.begin()->first << endl;
		}
	}
}

void Data::getColumnAverages() {
	double sum;
	int counter;
	for(map<string,map<double,double> >::iterator c = dataCurrent->begin(); c != dataCurrent->end(); ++c) {
		//cout << "gca col loop" << endl;

		sum = 0;
		counter = 0;

		for(map<double,double>::iterator v = c->second.begin(); v != c->second.end(); ++v) {
			//cout << "gca tp loop" << endl;
			if (std::isnan(v->second)) {
				continue;
			}
			sum += v->second;
			counter++;
		}
		colAverages.insert(pair<string,double>(c->first,sum/counter));
	}
}

void Data::logTransformData() {
	for(map<string,map<double,double> >::iterator c = dataCurrent->begin(); c != dataCurrent->end(); ++c) {
		for(map<double,double>::iterator v = c->second.begin(); v != c->second.end(); ++v) {
			if (v->second == 0) {
				swarm_->outputError("You chose to log transform simulation output, but the first value in the column '" + c->first + "' is 0. We cannot take the log of 0.");
			}
			else if (std::isnan(v->second)) {
				dataLogTransformed_[c->first][v->first] = nan("1");
				continue;
			}
			dataLogTransformed_[c->first][v->first] = log10(v->second)/log10(swarm_->options.logTransformSimData);
		}
	}
}

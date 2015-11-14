/*
 * Model.cpp
 *
 *  Created on: Jul 13, 2015
 *      Author: brandon
 */

#include "Model.hh"

using namespace std;

Model::Model(string path) {
	modelPath_ = convertToAbsPath(path);
	parseModel();
}

void Model::parseNet(string path) {
	string line;
	ifstream modelFile(path);

	if (modelFile.is_open()) {
		while (getline(modelFile, line)) {
			if (line.at(0) == '#') {
				continue;
			}
			string line_with_newline = line + "\n";
			netContents_.push_back(line_with_newline);
			//cout << line_with_newline;
		}
	}
	else {
		string errMsg = "Error: Couldn't open model file " + path + " for parsing.";
		outputError(errMsg);
	}
	modelFile.close();
}

void Model::parseModel() {
	string line;
	ifstream modelFile(modelPath_);
	vector<string> model_vector;

	if (modelFile.is_open()) {
		while (getline(modelFile, line)) {
			string line_with_newline = line + "\n";
			model_vector.push_back(line_with_newline);
		}
	}
	else {
		string errMsg = "Error: Couldn't open model file " + modelPath_ + " for parsing.";
		outputError(errMsg);
	}
	modelFile.close();

	// Find any line-continuation backslashes and merge the lines together
	for (unsigned i = 0 ; i < model_vector.size(); i++) {
		int j = 0;
		while(1) {
			if (regex_search(model_vector[i-j],regex("\\\\"))) {
				model_vector[i-j] = regex_replace(model_vector[i-j],regex("\\\\\\W*"),"");
				model_vector[i-j] = model_vector[i-j] + model_vector[i+1];
				model_vector[i-j] = regex_replace(model_vector[i-j],regex("\\s*"),"");
				if (!regex_search(model_vector[i-j],regex("\\\\"))) {
					fullContents_.push_back(model_vector[i-j]);
				}
				i++;
				j++;
				if (!regex_search(model_vector[i-j],regex("\\\\"))) {
					break;
				}
			}
			else {
				break;
			}
		}
		if (j == 0){
			fullContents_.push_back(model_vector[i]);
		}
	}

	// Go through the model and store important things
	smatch smatches;
	for (string i : fullContents_) {
		if (regex_search(i, regex("^simulate|^simulate_nf|^simulate_ode|^simulate_ssa|^simulate_pla|^parameter_scan"))) {
			action newAction;
			string prefix;
			bool isParScan = 0;

			// If the action is a normal "simulate", find the method in the arguments and store it
			if (regex_search(i, smatches, regex("^simulate\\(\\{.*method=>(\"|')(\\w{2,3})(\"|')"))) {
				newAction.type = smatches[2];
			}
			// If we the action command is simulate_xx, store the sim type
			else if (regex_search(i, smatches, regex("^simulate_(\\w{2})\\(\\{"))) {
				newAction.type = smatches[1];
			}
			// If we found a parameter_scan command, store the type, scan parameter, and t_end
			else if (regex_search(i, smatches, regex("^parameter_scan\\(\\{.*method=>(\"|')(\\w{2,3})(\"|')"))) {
				isParScan = 1;
				newAction.type = smatches[2];

				if (regex_search(i, smatches, regex("parameter=>('|\")(\\w+)('|\")"))) {
					newAction.scanParam = smatches[2];
				}
				if (regex_search(i, smatches, regex("par_max=>(\\w+)"))) {
					newAction.t_end = atof(smatches[1].str().c_str());
				}
				if (regex_search(i, smatches, regex("par_scan_vals=>\\[(.+)\\]"))) {
					vector<string> values;
					split(smatches[1].str(), values, ",");
					newAction.t_end = atof(values.back().c_str());
				}
			}

			// Remove any suffixes from the command
			i = regex_replace(i,regex("suffix=>('|\")\\w+('|\")"), "");

			// Find any prefixes and store them
			if (regex_search(i, smatches, regex("prefix=>('|\")(\\w+)('|\")"))) {
				prefix = smatches[2];
			}

			// If we find a non parameter_scan action, store the t_end value
			if (regex_search(i, smatches, regex("t_end=>(\\w+)")) && !isParScan) {
				newAction.t_end = atof(smatches[1].str().c_str());
			}

			newAction.full = i;
			if (!prefix.empty()) {
				actions.insert(pair<string,action>(prefix,newAction));
			}
		}
		else if (regex_search(i, smatches, regex("(\\s+|=\\s*)(\\w+)__FREE__"))) {
			//freeParams_.insert(pair<string,string>(smatches[2],""));
			FreeParam * fp = new FreeParam(smatches[2]);
			freeParams_.insert(pair<string,FreeParam*>(smatches[2],fp));
		}
		else if (regex_search(i, smatches, regex("^generate_network"))) {
			hasGenerateNetwork_ = true;
		}
	}

	if (actions.size() == 0) {
		outputError("Error: We didn't find any action commands containing prefixes in your model file. Your model must contain an action command that uses a prefix which corresponds to the name of the .exp file to be fit to the data generated by that action command.");
	}
}

void Model::outputModelWithParams(map<string,double> params, string path, string filename, string suffix, bool stopAtNetGen=false, bool onlyActions=false, bool netAndBngl=false, bool usePipe=false, bool isNetFile=false) {

	if (netAndBngl) {
		// First output the .bngl file (containing only action commands)
		outputModelWithParams(params, path, filename, suffix, false, true, false, false, false);
		filename = regex_replace(filename, regex("bngl$"), "net");
		// Then output the .net file
		outputModelWithParams(params, path, filename, "", false, false, false, false, true);

		return;
	}

	if (!usePipe) {
		string fullPath = path + "/" + filename;
		unlink(fullPath.c_str());

		ofstream outFile;
		outFile.open(fullPath);
		smatch matches;

		if (outFile.is_open()) {
			if (isNetFile) {
				bool inParameterBlock = true;
				int numParamsToReplace = params.size();
				int numReplacedParams = 0;
				for (string line : netContents_){
					if (line == "end parameters" || numReplacedParams == numParamsToReplace) {
						inParameterBlock = false;
					}
					else if (inParameterBlock) {
						// Replace free param with generated param
						//double tt = 0;
						for (auto p : params) { // TODO: Is there a faster way to do this than loop through params over and over?
							//cout << "p is " << p.first << endl;
							//Timer tmr;
							if(regex_match(line, matches, regex("\\s+\\d+\\s+(\\w+)\\s+(.+)\\s+"))) {
								//double t = tmr.elapsed();
								//cout << "Match took " << t << " seconds" << endl;
								//tt += t;
								if (matches[1] == p.first) {
									string match = p.first + "\\s+.+";
									string replacement = p.first + " " + to_string(p.second);
									line = regex_replace(line, regex(match), replacement);
									numReplacedParams++;
								}
							}
						}
						//cout << "matches took " << tt << " seconds" << endl;
					}
					//cout << line;
					outFile << line;
				}
			}
			else {
				if (onlyActions) {
					filename = regex_replace(filename, regex("bngl$"), "net");
					string line = "readFile({file=>\"" + path + "/" + filename + "\"})\n";
					outFile << line;
				}
				for (string line : fullContents_){
					if (onlyActions) {
						if (regex_search(line, regex("^simulate|^simulate_nf|^simulate_ode|^simulate_ssa|^simulate_pla|^parameter_scan|^setConcentration|^addConcentration|^saveConcentration|^resetConcentrations|^setParameter|^saveParameters|^resetParameters|^quit|^substanceUnits|^version|^setOption"))) {
							if (!suffix.empty()) {
								string suffixLine = ",suffix=>\"" + suffix + "\"})";
								line = regex_replace(line, regex("\\}\\)"), suffixLine);
							}
						}
						else {
							continue;
						}
						outFile << line;
					}
					else {
						// Skip line if it's empty or if it is a comment
						if (regex_search(line,regex("^\\s*$")) || regex_search(line,regex("^#"))) {
							continue;
						}
						// Replace free param with generated param
						if (regex_search(line,matches,regex("(\\s+|=\\s*)(\\w+)__FREE__"))) {

							line = regex_replace(line, regex("\\w+__FREE__"), to_string(params[matches[2]]));
						}
						// Add in the unique suffix
						if (!suffix.empty()) {
							if (regex_search(line, regex("^simulate|^simulate_nf|^simulate_ode|^simulate_ssa|^simulate_pla|^parameter_scan"))) {
								string suffixLine = ",suffix=>\"" + suffix + "\"})";
								line = regex_replace(line, regex("\\}\\)"), suffixLine);
							}
						}
						outFile << line;
						if (stopAtNetGen) {
							if (regex_search(line,matches,regex("^generate_network"))) {
								break;
							}
						}
					}
				}
			}
			outFile.close();
		}
		else {
			string errMsg = "Couldn't open file '" + fullPath + "' for output";
			outputError(errMsg);
		}
	}
	else {
		const char * fifo = path.c_str();

		int fifo_status = mkfifo(fifo, 0666);
		if (fifo_status) {
			string errMsg = "Couldn't create pipe '" + path + "' for output";
			outputError(errMsg);
		}

		int fd = open(fifo, O_RDONLY);
		if (fd < 0) {
			cout << "Couldn't open " << fifo << endl;
		}

		for (auto line : fullContents_) {

			write(fd,line.c_str(),line.size());
		}
		close(fd);
	}
}

/*
 * Swarm.hh
 *
 *  Created on: Jul 17, 2015
 *      Author: brandon
 */

#ifndef SWARM_HH_
#define SWARM_HH_


#include <vector>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <fstream>
#include <tr1/random>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <set>
#include <string>
#include <map>

#include <boost/random/random_device.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/cauchy_distribution.hpp>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/utility.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Utils.hh"
#include "Timer.hh"
#include "Particle.hh"
#include "Pheromones.hh"

class Model;
class FreeParam;
class Data;
class Pheromones;
class Particle;

#define MAX_LONG 9223372036854775807

// Forward declaration of class boost::serialization::access
namespace boost {
namespace serialization {
class access;
}
}

class Swarm {
public:
	Swarm();

	void addExp(std::string path);
	void setModel(std::string path);
	void setExePath(std::string path) { exePath_ = path; }
	void setConfigPath(std::string path) { configPath_ = path; }
	void setfitType(std::string type);
	void addMutate(std::string mutateString);
	void setsConf(std::string sConf) { sConf_ = sConf; }
	std::string getsConf() { return sConf_; }
	void setJobOutputDir(std::string dir);
	void generateBootstrapMaps(std::vector<std::map<std::string, std::map<std::string, std::map<double,unsigned int>>>> &bootStrapMaps);

	void outputError(std::string errorMessage);

	void initComm();
	void initRNGS(int seed=0);
	void doSwarm();

	void runSGA();
	void runSPSO();
	void runSDE();
	void runAGA();
	void runAPSO();
	void runADE();
	void runSSA();
	void runASA();

	std::vector<double> normalizeParams(std::vector<double> params);
	std::vector<double> deNormalizeParams(std::vector<double> params);

	Particle *createParticle(unsigned int pID);

	void getClusterInformation();
	std::string getClusterCommand(std::string cmd);
	std::string generateTorqueBatchScript(std::string cmd);
	std::string generateSlurmCommand(std::string cmd, bool multiProg = true, unsigned int nCPU = 0);
	std::string generateSlurmMultiProgCmd(std::string runCmd);
	std::string generateSlurmBatchFile(std::string runCmd);
	std::string generateMPICommand(std::string runCmd);

	Pheromones *swarmComm;

	std::multimap<double, std::string> allGenFits;
	std::vector<std::map<std::string, std::map<std::string, std::map<double,unsigned int>>>> bootstrapMaps;

	bool isMaster;
	boost::random::mt19937 generalRand;
	boost::random::mt19937 parameterGenRand;
	unsigned int currentGeneration;
	unsigned int bootstrapCounter;
	bool resumingSavedSwarm;
	bool hasMutate;
	float fitCompareTolerance;
	bool commInit;

	struct SwarmOpts {
		std::string jobName;	// name of the job
		std::string fitType;	// genetic or swarm
		std::string outputDir;	// root directory to use for output
		std::string jobOutputDir;// outputDir + jobName
		std::string bngCommand;	// Path to simulators
		std::map<std::string,Data*> expFiles; // experimental data file
		Model * model; 			// the model file

		// General options
		unsigned int verbosity;		// terminal output verbosity
		bool synchronicity;		// 1 for synchronous
		unsigned int maxGenerations;// maximum number of generations
		unsigned int swarmSize;		// how many particles in the swarm
		float minFit;		// we stop fitting if we reach this value
		unsigned int parallelCount;	// how many particles to run in parallel
		unsigned int objFunc;		// which objective function to use
		bool usePipes;	// whether or not to use pipes to gather simulation output
		bool useCluster;// whether or not we are running on a cluster
		int seed; // seed for the random number engines
		unsigned int bootstrap;

		bool divideByInit;// whether or not to divide simulation outputs by the value at t=0
		int logTransformSimData;// whether or not to log transform simulation data. this value acts as the base.
		bool standardizeSimData;// whether or not to standardize simulation data
		bool standardizeExpData;// whether or not to standardize experimental data

		bool deleteOldFiles; // whether or not to delete unneeded files during the fitting run

		// Genetic algorithm options
		unsigned int extraWeight;	// how much extra weight to add while breeding in genetic algorithm
		float swapRate;	// the rate at which to swap parent parameters during breeding
		bool forceDifferentParents;// whether or not to force difference parents when breeding
		unsigned int maxRetryDifferentParents;// how many times to attempt selection of different parents if forceDifferentParents is true
		unsigned int smoothing;
		unsigned int keepParents;

		unsigned long maxFitTime;	// Maximum amount of time to let the fit run
		unsigned long maxNumSimulations; // Maximum number of simulations to run

		// PSO options
		float inertia; // 0.72
		float cognitive; // 1.49
		float social; // 1.49
		unsigned int nmax; // 20
		unsigned int nmin; // 80
		float inertiaInit; // 1
		float inertiaFinal; // 0.1
		float absTolerance; // 10E-4
		float relTolerance; // 10E-4
		bool mutateQPSO;
		float betaMax;
		float betaMin;

		std::string topology; // fullyconnected
		std::string psoType; // bbpso

		bool enhancedStop; // true
		bool enhancedInertia; // true

		// DE Options
		unsigned int numIslands;
		unsigned int mutateType;
		float cr;
		unsigned int migrationFrequency;
		unsigned int numToMigrate;

		// SA options
		double minTemp;
		double minRadius;
		float localSearchProbability;
		float randParamsProbability;

		unsigned int outputEvery; // In an asynchronous fit, output a fit summary every n simulations

		// Cluster options
		std::string clusterSoftware;// which cluster software to use
		std::string clusterAccount;	// user account to specify in cluster submission commands
		bool saveClusterOutput;		// whether or not to save output during a cluster fit
		std::string clusterQueue;	// The cluster queue to submit to
		bool emailWhenFinished;
		std::string emailAddress;
		std::string hostfile;

		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			//std::cout << " serializing options" << std::endl;

			ar & jobName;
			ar & fitType;
			ar & outputDir;
			ar & jobOutputDir;
			ar & bngCommand;
			ar & expFiles;
			ar & model;

			ar & verbosity;
			ar & synchronicity;
			ar & maxGenerations;
			ar & swarmSize;
			ar & minFit;
			ar & bootstrap;
			ar & parallelCount;
			ar & objFunc;
			ar & usePipes;
			ar & useCluster;
			ar & seed;

			ar & divideByInit;
			ar & logTransformSimData;
			ar & standardizeSimData;
			ar & standardizeExpData;

			ar & deleteOldFiles;

			ar & extraWeight;
			ar & swapRate;
			ar & forceDifferentParents;
			ar & maxRetryDifferentParents;
			ar & smoothing;
			ar & keepParents;

			ar & maxFitTime;
			ar & maxNumSimulations;

			ar & inertia;
			ar & cognitive;
			ar & social;
			ar & nmax;
			ar & nmin;
			ar & inertiaInit;
			ar & inertiaFinal;
			ar & absTolerance;
			ar & relTolerance;
			ar & mutateQPSO;
			ar & betaMin;
			ar & betaMax;

			ar & topology;
			ar & psoType;

			ar & enhancedStop;
			ar & enhancedInertia;

			ar & numIslands;
			ar & mutateType;
			ar & cr;
			ar & migrationFrequency;
			ar & numToMigrate;

			ar & minTemp;
			ar & minRadius;
			ar & localSearchProbability;
			ar & randParamsProbability;

			ar & outputEvery;

			ar & clusterSoftware;
			ar & clusterAccount;
			ar & saveClusterOutput;
			ar & clusterQueue;
			ar & hostfile;
		}
	};
	SwarmOpts options;

private:
	friend class boost::serialization::access;

	void initFit();
	std::vector<std::vector<unsigned int> > generateTopology(unsigned int populationSize);
	void launchParticle(unsigned int pID, bool nextGen = false);
	void runGeneration();
	void breedGenerationGA(std::vector<unsigned int> children = std::vector<unsigned int>());
	void runNelderMead(unsigned int receiver, unsigned int cpu);
	std::map<double, unsigned int> getNearestNeighbors(unsigned int it, unsigned int N);

	void cleanupFiles(const char * path);
	void finishFit();
	void getAllParticleParams();
	void outputRunSummary(std::string outputDir);
	void outputRunSummary();
	void outputBootstrapSummary();
	void killAllParticles(int tag);
	std::vector<unsigned int> checkMasterMessages();
	std::unordered_map<unsigned int, std::vector<double>> checkMasterMessagesDE();
	void checkExternalMessages();
	void resetVariables();
	void generateBestFitModel(std::string outputDirectory);

	void initPSOswarm(bool resumeFit = false);
	void processParticlesPSO(std::vector<unsigned int> particles, bool nextFlight = false);
	void updateEnhancedStop();
	double getEuclidianNorm(double y, unsigned int n);
	void updateParticleWeights();
	double calcParticleWeight(unsigned int particle);
	double calcWeightedAveragePosition();
	void processParamsPSO(std::vector<double> &params, unsigned int pID, double fit);
	bool checkStopCriteria();
	void updateInertia();
	void updateContractionExpansionCoefficient();
	double calcMeanFit();

	void saveSwarmState();

	std::vector<double> calcParticlePosPSO(unsigned int particle);
	std::vector<double> calcParticlePosBBPSO(unsigned int particle, bool exp = false);
	std::vector<double> calcParticlePosQPSO(unsigned int particle, std::vector<double> mBests);
	std::vector<double> getNeighborhoodBestPositions(unsigned int particle);
	std::vector<double> calcQPSOmBests();

	unsigned int pickWeighted(double weightSum, std::multimap<double, unsigned int> &weights, unsigned int extraWeight);
	std::string mutateParamGA(FreeParam* fp, double paramValue);

	std::vector<double> mutateParticleDE(unsigned int particle, float mutateFactor = 0);
	std::vector<double> mutateParticleSA(unsigned int particle, float mutateFactor = 0);
	std::vector<double> crossoverParticleDE(unsigned int particle, std::vector<double> mutationSet, float cr = 0, bool normalize = false);
	void sendMigrationSetDE(unsigned int island, std::vector<std::vector<unsigned int>> islandTopology, std::map<unsigned int, std::vector<std::vector<double>>> &migrationSets);
	void recvMigrationSetDE(unsigned int island, std::map<unsigned int, std::vector<std::vector<double>>> &migrationSets);

	std::vector<double> generateParticleTemps();
	std::vector<double> generateParticleRadii();
	std::vector<float> generateParticleFs();
	std::vector<float> generateParticleCRs();
	unsigned int pickWeightedSA();
	bool metropolisSelection(unsigned int particle, double fit, float particleTemp);
	void swapTR(std::vector<double> particleRadii, std::vector<double> particleTemps);
	std::vector<double> generateTrialPointSA(unsigned int controller, unsigned int receiver, std::vector<double> particleRadii, std::vector<float>particleCRs, std::vector<float>particleFs, std::vector<std::vector<float>> &trialParams);
	double normalizeParam(double oldParam, double min, double max, bool log);
	double deNormalizeParam(double oldParam, double min, double max, bool log);

	void insertKeyByValue(std::multimap<double, unsigned int> &theMap, double key, unsigned int value);

	std::set<unsigned int> runningParticles_;
	std::set<unsigned int>::iterator runningParticlesIterator_;

	std::set<unsigned int> failedParticles_;
	std::set<unsigned int>::iterator failedParticlesIterator_;

	std::string exePath_;
	std::string configPath_;
	std::string sConf_;

	std::vector<std::vector<unsigned int> > populationTopology_;

	// Maybe we can change them to vectors, too
	std::map<unsigned int, double> particleBestFits_;
	std::multimap<double, unsigned int> particleBestFitsByFit_;
	std::multimap<double, unsigned int> swarmBestFits_;
	std::map<unsigned int, std::vector<double>> particleParamVelocities_;

	// Holds the running best parameter set for each particle
	std::map<unsigned int, std::vector<double>> particleBestParamSets_;

	// Holds the current parameter set being used by each particle
	std::map<unsigned int, std::vector<double>> particleCurrParamSets_;

	// Holds particle weights for use in enhancedStop stop criteria
	std::map<unsigned int, double> particleWeights_;

	// Counts how many iterations each particle has performed
	std::map<unsigned int, unsigned int> particleIterationCounter_;

	// In DE, maps islands and particles together
	std::vector<unsigned int> particleToIsland_; // particle -> island
	std::vector<std::vector<unsigned int>> islandToParticle_; // island -> particle

	// Holds the current parameter set being used by each particle
	std::map<unsigned int, std::vector<double>> particleTrialParamSets_;

	unsigned int permanenceCounter_; // 0
	unsigned int flightCounter_; // 0
	double weightedAvgPos_; // 0
	double optimum_; // 0
	unsigned int inertiaUpdateCounter_; // 0;
	double beta_;
	double cauchyMutator_;

	template<typename Archive>
	void serialize(Archive& ar, const unsigned version) {
		//std::cout << " serializing swarm" << std::endl;

		ar & options;

		ar & allGenFits;
		ar & bootstrapMaps;

		ar & isMaster;
		ar & currentGeneration;
		ar & resumingSavedSwarm;
		ar & hasMutate;

		ar & runningParticles_;
		ar & failedParticles_;

		ar & exePath_;
		ar & configPath_;
		ar & sConf_;

		ar & populationTopology_;
		ar & particleBestFits_;
		ar & particleBestFitsByFit_;
		ar & swarmBestFits_;
		ar & particleParamVelocities_;
		ar & particleBestParamSets_;
		ar & particleCurrParamSets_;
		ar & particleWeights_;
		ar & particleIterationCounter_;
		ar & particleToIsland_;
		ar & islandToParticle_;

		ar & permanenceCounter_;
		ar & flightCounter_;
		ar & weightedAvgPos_;
		ar & optimum_;
		ar & inertiaUpdateCounter_;
		ar & beta_;
		ar & cauchyMutator_;
	}

	Timer tmr_;
};

#endif /* SWARM_HH_ */

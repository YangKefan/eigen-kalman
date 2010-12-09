/**
	@file kfExampleTune.cpp
	@brief

	@date 2010

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

// Internal Includes
// - none

// Library/third-party includes
#include <eigenkf/KalmanFilter.h>
#include <Eigen/Eigen>

// Standard includes
#include <iostream>
#include <iomanip>
#include <cmath>
#include <ctime>

using namespace eigenkf;

#define COL 10
#define NOISE_AMPLITUDE 3.0

const double dt = 0.5;


double noise() {
	return ((std::rand() % 100) / 50.0 - 1.0) * NOISE_AMPLITUDE;
}

typedef std::pair<Eigen::Vector2d, Eigen::Vector2d> StatePair;
std::vector<StatePair> generateData() {
	/// Reset the random seed
	std::srand(200);
	std::vector<StatePair> ret;
	for (double t = 0; t < 50.0; t+= dt) {
		Eigen::Vector2d err;
		err[0] = noise();
		err[1] = noise();
		Eigen::Vector2d pos(Eigen::Vector2d::Constant(t));
		Eigen::Vector2d measurement(pos + err);
		ret.push_back(StatePair(pos, measurement));
	}
	return ret;
}

double runSimulation(std::vector<StatePair> const& data, const double measurementVariance, const double processModelVariance) {

	/// We want a simple 2d state
	typedef SimpleState<2> state_t;

	/// Our process model is the simplest possible: doesn't change the mean
	typedef ConstantProcess<2, state_t> process_t;

	/// Create a kalman filter instance with our chosen state and process types
	KalmanFilter<state_t, process_t> kf;

	/// Set our process model's variance
	kf.processModel.sigma = state_t::VecState::Constant(processModelVariance);

	double sumSquaredError = 0;

	for (unsigned int i = 0; i < data.size(); ++i) {
		/// Predict step: Update Kalman filter by predicting ahead by dt
		kf.predict(dt);

		/// "take a measurement" - in this case, noisify the actual measurement
		AbsoluteMeasurement<state_t> meas;
		meas.measurement = data[i].second;
		meas.covariance = Eigen::Vector2d::Constant(measurementVariance).asDiagonal();

		/// Correct step: incorporate information from measurement into KF's state
		kf.correct(meas);
		
		Eigen::Vector2d pos(data[i].first);
		double squaredError = (pos[0] - kf.state.x[0]) * (pos[0] - kf.state.x[0]);
		sumSquaredError += squaredError;
	}
	return sumSquaredError;
}

int main(int argc, char * argv[]) {

	std::vector<StatePair> data = generateData();

	const double INTERVALS = 20;

	double lowMVar = 0;
	double highMVar = NOISE_AMPLITUDE * 5.0;
	double lowPVar = 0;
	double highPVar = 15;

	double minErr = 10000;
	double bestMVar = lowMVar;
	double bestPVar = lowPVar;
	
	// Output column headers
	std::cout << ",";
	for (double pVar = lowPVar; pVar < highPVar; pVar += (highPVar - lowPVar) / INTERVALS) {
		std::cout << pVar << ",";
	}
	std::cout << "process variance" << std::endl;
	
	for (double mVar = lowMVar; mVar < highMVar; mVar += (highMVar - lowMVar) / INTERVALS) {
		/// row headers
		std::cout << mVar;
		for (double pVar = lowPVar; pVar < highPVar; pVar += (highPVar - lowPVar) / INTERVALS) {
			double err = runSimulation(data, mVar, pVar);
			std::cout << "," << err;
			if (err < minErr) {
				bestMVar = mVar;
				bestPVar = pVar;
				minErr = err;
			}
		}
		std::cout << std::endl;
	}
	std::cout << "measurement variance" << std::endl;

	std::cerr << std::endl;
	std::cerr << "Best found in the grid of parameters: " << std::endl;
	std::cerr << "Measurement Variance: " << bestMVar << std::endl;
	std::cerr << "Process variance: " << bestPVar << std::endl;
	std::cerr << "Sum squared error: " << minErr << std::endl;
	std::cerr << std::endl;

	return 0;
}

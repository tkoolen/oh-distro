#ifndef KF_MODELS_HPP_
#define KF_MODELS_HPP_

#include <iostream>
#include <Eigen/Dense>

#include <leg-odometry/kf_conversion_definitions.hpp>
#include <leg-odometry/KalmanFilter_Types.hpp>

#define JOINT_MODEL 0
#define DATAFUSION_MODEL 1

namespace KalmanFilter_Models {

// models should implement this base abstract class
class BaseModel {
public:
	virtual KalmanFilter_Types::Priori propagation_model(const unsigned long &utime, const KalmanFilter_Types::Posterior &post) = 0;
	virtual KalmanFilter_Types::Priori propagation_model(KalmanFilter_Types::Priori prev_priori) = 0;
	virtual VAR_VECTORd measurement_model(VAR_VECTORd Param) = 0;
	
	virtual void identify() = 0;
	
};



class Joint_Model : public BaseModel {

public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	
	KalmanFilter_Types::Priori propagation_model(const unsigned long &utime, const KalmanFilter_Types::Posterior &post);
	KalmanFilter_Types::Priori propagation_model(KalmanFilter_Types::Priori prev_priori);
		
	VAR_VECTORd measurement_model(VAR_VECTORd Param);
	
	void identify();
	
};


class DataFusion_Model : public BaseModel {

public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	
	KalmanFilter_Types::Priori propagation_model(const unsigned long &utime, const KalmanFilter_Types::Posterior &post);
	KalmanFilter_Types::Priori propagation_model(KalmanFilter_Types::Priori prev_priori);
	VAR_VECTORd measurement_model(VAR_VECTORd Param);
	
	void identify();
	
};


}

#endif /*KF_MODELS_HPP_*/

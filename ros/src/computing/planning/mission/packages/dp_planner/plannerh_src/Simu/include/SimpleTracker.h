/*
 * SimpleTracker.h
 *
 *  Created on: Aug 11, 2016
 *      Author: hatem
 */

#ifndef SimpleTracker_H_
#define SimpleTracker_H_

#include "RoadNetwork.h"
#include "opencv2/video/tracking.hpp"
#include <vector>
#include "UtilityH.h"
#include "math.h"
#include <iostream>

namespace SimulationNS
{

#define DEBUG_TRACKER 1
#define NEVER_GORGET_TIME -1000

class KFTrackV
{
private:
	cv::KalmanFilter m_filter;
	double prev_x, prev_y, prev_v, prev_a;
	long m_id;
	double dt;
	int nStates;
	int nMeasure;
	int m_iLife;

public:
	int region_id;
	double forget_time;
	PlannerHNS::DetectedObject obj;

	long GetTrackID()
	{
		return m_id;
	}

	KFTrackV(double x, double y, double a, long id, double _dt)
	{
		region_id = -1;
		forget_time = NEVER_GORGET_TIME; // this is very bad , dangerous
		m_iLife = 0;
		dt = _dt;
		prev_x = x;
		prev_y = y;
		prev_v = 0;
		prev_a = a;
		nStates = 4;
		nMeasure = 2;

		m_id = id;

		m_filter = cv::KalmanFilter(nStates,nMeasure);
		m_filter.transitionMatrix = *(cv::Mat_<float>(nStates, nStates) << 1	,0	,dt	,0  ,
																	   	   0	,1	,0	,dt	,
																	   	   0	,0	,1	,0	,
																	   	   0	,0	,0	,1	);
		m_filter.statePre.at<float>(0) = x;
		m_filter.statePre.at<float>(1) = y;
		m_filter.statePre.at<float>(2) = 0;
		m_filter.statePre.at<float>(3) = 0;

		m_filter.statePost = m_filter.statePre;

		setIdentity(m_filter.measurementMatrix);

		cv::setIdentity(m_filter.measurementNoiseCov, cv::Scalar::all(0.0001));
		cv::setIdentity(m_filter.processNoiseCov, cv::Scalar::all(0.0001));
		cv::setIdentity(m_filter.errorCovPost, cv::Scalar::all(0.075));

		m_filter.predict();
	}

	void UpdateTracking(const double& x, const double& y, const double& a, double& x_new, double & y_new , double& a_new, double& v)
	{
		cv::Mat_<float> measurement(nMeasure,1);
		cv::Mat_<float> prediction(nStates,1);

		measurement(0) = x;
		measurement(1) = y;

		prediction = m_filter.correct(measurement);

		x_new = prediction.at<float>(0);
		y_new = prediction.at<float>(1);
		double vx  = prediction.at<float>(2);
		double vy  = prediction.at<float>(3);

		if(m_iLife > 2)
		{
			v = sqrt(vx*vx+vy*vy);
			a_new = atan2(y_new - y, x_new - x);
		}
		else
		{
			v = 0;
			a_new = a;
		}

		if(v < 0.1)
			v = 0;

		m_filter.predict();
		m_filter.statePre.copyTo(m_filter.statePost);
		m_filter.errorCovPre.copyTo(m_filter.errorCovPost);
		m_iLife++;
	}
	virtual ~KFTrackV(){}
};

class InterestCircle
{
public:
	int id;
	double radius;
	double forget_time;
	std::vector<KFTrackV*> pTrackers;
	InterestCircle* pPrevCircle;
	InterestCircle* pNextCircle;

	InterestCircle(int _id)
	{
		id = _id;
		radius = 0;
		forget_time = NEVER_GORGET_TIME; // never forget
		pPrevCircle = 0;
		pNextCircle = 0;
	}
};

class CostRecordSet
{
public:
	int currobj;
	int prevObj;
	double cost;
	CostRecordSet(int curr_id, int prev_id, double _cost)
	{
		currobj = curr_id;
		prevObj = prev_id;
		cost = _cost;
	}
};

class SimpleTracker
{
public:
	std::vector<InterestCircle*> m_InterestRegions;
	std::vector<KFTrackV*> m_Tracks;
	timespec m_TrackTimer;
	long iTracksNumber;
	PlannerHNS::WayPoint m_PrevState;
	std::vector<PlannerHNS::DetectedObject> m_PrevDetectedObjects;
	std::vector<PlannerHNS::DetectedObject> m_DetectedObjects;

	void CreateTrack(PlannerHNS::DetectedObject& o);
	void CreateTrackV2(PlannerHNS::DetectedObject& o);
	KFTrackV* FindTrack(long index);
	void Track(std::vector<PlannerHNS::DetectedObject>& objects_list);
	void TrackV2();
	void CoordinateTransform(const PlannerHNS::WayPoint& refCoordinate, PlannerHNS::DetectedObject& obj);
	void CoordinateTransformPoint(const PlannerHNS::WayPoint& refCoordinate, PlannerHNS::GPSPoint& obj);
	void AssociateObjects();
	void InitializeInterestRegions(double horizon, double init_raduis, double init_time, std::vector<InterestCircle*>& regions);
	void AssociateAndTrack();
	void AssociateToRegions(KFTrackV& detectedObject);
	void CleanOldTracks();

	void DoOneStep(const PlannerHNS::WayPoint& currPose, const std::vector<PlannerHNS::DetectedObject>& obj_list);

	SimpleTracker(double horizon = 100);
	virtual ~SimpleTracker();

public:
	double m_DT;
	double m_MAX_ASSOCIATION_DISTANCE;
	int m_MAX_TRACKS_AFTER_LOSING;
	bool m_bUseCenterOnly;
	double m_MaxKeepTime;
};

} /* namespace BehaviorsNS */

#endif /* SimpleTracker_H_ */

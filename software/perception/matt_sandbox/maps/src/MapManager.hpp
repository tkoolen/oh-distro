#ifndef _MapManager_hpp_
#define _MapManager_hpp_

#include <unordered_map>
#include <boost/shared_ptr.hpp>
#include <pcl/point_types.h>
#include <Eigen/Geometry>

class PointDataBuffer;
class LocalMap;

class MapManager {
public:
  typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;
  typedef boost::shared_ptr<LocalMap> MapPtr;

  struct MapDelta {
    PointCloud::Ptr mAdded;
    PointCloud::Ptr mRemoved;
  };

protected:
  typedef std::unordered_map<int64_t, MapPtr> MapCollection;


public:
  // constructor/destructor
  MapManager();
  ~MapManager();

  // completely clear the state
  void clear();

  // setters for map properties
  void setMapResolution(const double iResolution);
  void setMapDimensions(const Eigen::Vector3d iDims);
  void setDataBufferLength(const int iLength);

  // create new map and make it the active one
  bool createMap(const Eigen::Isometry3d& iToLocal =
                 Eigen::Isometry3d::Identity(),
                 const int iId=-1);

  // switch to use map with given id
  bool useMap(const int64_t iId);

  // get current active map, or null if there is no current map
  MapPtr getActiveMap() const;

  // add data to internal buffer but not to map
  bool addToBuffer(const int64_t iTime, const PointCloud::Ptr& iPoints,
                   const Eigen::Isometry3d& iToLocal);

  // correct pose of buffered points
  bool updatePose(const int64_t iTime, const Eigen::Isometry3d& iToLocal);

  // fuse all current buffered points to rebuild current map
  bool fuseAll();

  // compute delta between current and previous version of current map
  bool computeDelta(MapDelta& oDelta);

  // make previous version of map same as current for deltas
  bool resetDeltaBase();

protected:
  MapPtr mActiveMap;
  MapCollection mMaps;
  boost::shared_ptr<PointDataBuffer> mPointDataBuffer;

  double mMapResolution;
  Eigen::Vector3d mMapDimensions;
  int mDataBufferLength;

  int mNextMapId;
};

#endif

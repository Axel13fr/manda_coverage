#ifndef SWATHRECORD_H
#define SWATHRECORD_H

#include <Eigen/Core>

using EPoint = Eigen::Vector2d;
/**
 * @enum BoatSide
 * @brief Indicates the side of a boat for the swath
 */
enum class BoatSide { Stbd, Port, Unknown };

/**
   * @struct SwathRecord
   * @brief Stores the location and width of one measured sonar swath.
   */
class SwathRecord
{
public:
    double loc_x;
    double loc_y;
    double heading;
    // Could make this a map w/ BoatSide index
    double swath_stbd;
    double swath_port;
    // This will either be the nadir depth (if this is all we have) or outer
    // swath depth
    double depth;

    /**
    * Gets the x,y position of the edge of a swath from a record
    * @param  record The swath record to use for location and width
    * @param  side   The side of the boat on which to project the swath
    * @return        Location of the swath outer points
    */
    EPoint outerPoint(BoatSide side) const;
};

#endif // SWATHRECORD_H

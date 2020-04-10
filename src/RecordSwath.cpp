/************************************************************/
/*    NAME: Damian Manda                                    */
/*    ORGN: UNH                                             */
/*    FILE: RecordSwath.cpp                                 */
/*    DATE: 23 Feb 2016                                     */
/************************************************************/

#include "RecordSwath.h"
#include <cmath>
#include <algorithm>

#define DEBUG false
#define TURN_THRESHOLD 20

// Procedure: angle180
//   Purpose: Convert angle to be strictly in the rang (-180, 180].

static double angle180(double degval)
{
    while(degval > 180)
        degval -= 360.0;
    while(degval <= -180)
        degval += 360.0;
    return(degval);
}

//---------------------------------------------------------------
// Procedure: radAngleWrap

static double radAngleWrap(double radval)
{
    if((radval <= M_PI) && (radval >= -M_PI))
        return(radval);

    if(radval > M_PI)
    return(radval - (2*M_PI));
    else
        return(radval + (2*M_PI));
}

//---------------------------------------------------------------
// Procedure: headingToRadians
// Converts true heading (clockwize from N) to
// radians in a counterclockwize x(E) - y(N) coordinate system
// .

static double headingToRadians(double degval)
{
    return(radAngleWrap( (90.0-degval)*M_PI/180.0));
}

//---------------------------------------------------------------
// Procedure: angle360
//   Purpose: Convert angle to be strictly in the rang [0, 360).

static double angle360(double degval)
{
    while(degval >= 360.0)
        degval -= 360.0;
    while(degval < 0.0)
        degval += 360.0;
    return(degval);
}


//---------------------------------------------------------
// Constructor

RecordSwath::RecordSwath(double interval) : m_min_allowable_swath(0),m_interval(interval),
                         m_has_records(false), m_acc_dist(0),
                         m_previous_record{0, 0, 0, 0, 0, 0},
                         m_output_side(BoatSide::Unknown)
{
    // Initialize the point records
    m_interval_swath[BoatSide::Port] = std::vector<double>();
    m_interval_swath[BoatSide::Stbd] = std::vector<double>();
}

bool RecordSwath::AddRecord(double swath_stbd, double swath_port, double loc_x,
                            double loc_y, double heading, double depth)
{
    // Dont add records at duplicate location
    if (loc_x == m_previous_record.loc_x && loc_y == m_previous_record.loc_y
            && heading == m_previous_record.heading)
        return false;

    SwathRecord record = {loc_x, loc_y, heading, swath_stbd, swath_port, depth};
    m_interval_record.push_back(record);
    m_interval_swath[BoatSide::Stbd].push_back(swath_stbd);
    m_interval_swath[BoatSide::Port].push_back(swath_port);

    if (m_has_records)
    {
        m_acc_dist += hypot((m_last_x - loc_x), (m_last_y - loc_y));
        #if DEBUG
        std::cout << "Accumulated distance: " + std::to_string(m_acc_dist) + "\n";
        #endif

        if (m_acc_dist >= m_interval)
        {
            #if DEBUG
            std::cout << "Running MinInterval()\n";
            #endif
            m_acc_dist = 0;
            MinInterval();
        } 
        else
        {
            //Override the min interval on turns to the outside
            double turn = angle180(angle180(heading) - angle180(m_min_record.back().heading));
            if ((turn > TURN_THRESHOLD && m_output_side == BoatSide::Port)
                    || (turn < -TURN_THRESHOLD && m_output_side == BoatSide::Stbd))
            {
                #if DEBUG
                std::cout << "Adding Turn Based Point\n";
                #endif
                m_min_record.push_back(record);
                m_interval_record.clear();
                m_interval_swath.clear();
            }
        }
    }

    m_last_x = loc_x;
    m_last_y = loc_y;
    m_has_records = true;
    m_previous_record = record;

    // Add progressively to the coverage model
    return AddToCoverage(record);
}

bool RecordSwath::AddToCoverage(SwathRecord record) 
{
    // Tackle this later
    return true;
}

void RecordSwath::MinInterval()
{
    // Get the record from the side we are offsetting
    if (m_output_side == BoatSide::Unknown)
    {
        throw std::runtime_error("Cannot find swath minimum without output side.");
        return;
    }
    std::vector<double>* side_record = &m_interval_swath[m_output_side];

    std::size_t min_index = 0;
    if (side_record->size() > 0)
    {
        min_index = std::min_element(side_record->begin(), side_record->end()) - side_record->begin();
    }

    if (m_interval_record.size() > min_index)
    {
        // Add the first point if this is the first interval in the record
        if (m_min_record.size() == 0 && min_index != 0)
        {
            #if DEBUG
            std::cout << "Saving First record of line\n";
            #endif
            m_min_record.push_back(m_interval_record[0]);
        }
        m_min_record.push_back(m_interval_record[min_index]);
        // These are always cleared in the python version
        m_interval_record.clear();
        m_interval_swath.clear();
    }
}

bool RecordSwath::SaveLast()
{
    if (m_min_record.size() > 0 && m_interval_record.size() > 0)
    {
        SwathRecord last_min = m_min_record.back();
        SwathRecord last_rec = m_interval_record.back();
        if (last_min.loc_x != last_rec.loc_x || last_min.loc_y != last_rec.loc_y)
        {
            #if DEBUG
            std::cout << "Saving last record of line, (" << last_rec.loc_x << ", "
                << last_rec.loc_y << ")\n";
            #endif
            m_min_record.push_back(last_rec);
        }
        return true;
    }
    return false;
}

void RecordSwath::ResetLine()
{
    m_interval_record.clear();
    m_min_record.clear();
    m_interval_swath[BoatSide::Stbd].clear();
    m_interval_swath[BoatSide::Port].clear();
    m_acc_dist = 0;
    m_has_records = false;
}

EPointVec RecordSwath::SwathOuterPts(BoatSide side)
{
    EPointVec points;
    for (const auto &record : m_min_record)
    {
        // #if DEBUG
        // std::cout << "Getting swath outer point for " << record->loc_x
        //   << ", "  << record->loc_y << "\n";
        // #endif
        auto outer_pt = OuterPoint(record, side);
        points.push_back(outer_pt);
    }
    return points;
}

// list<XYPt> RecordSwath::SwathOuterPts(BoatSide side) {
//   list<XYPt> points;
//   std::list<SwathRecord>::iterator record;
//   for (record = m_min_record.begin(); record != m_min_record.end(); record++) {
//     XYPoint outer_pt = OuterPoint(*record, m_output_side);
//     XYPt outer_pt_simple = {outer_point.x(), outer_point.y()}
//     points.add_vertex(outer_pt);
//   }
//   return points;
// }

EPoint RecordSwath::OuterPoint(const SwathRecord &record, BoatSide side)
{
    // Could have SwathRecord be a class with functions to return representation
    // as a vector or point.
    double swath_width = 0;
    double rotate_degs = 0;
    if (side == BoatSide::Stbd)
    {
        swath_width = record.swath_stbd;
        rotate_degs = 90;
    }
    else if (side == BoatSide::Port)
    {
        swath_width = record.swath_port;
        rotate_degs = -90;
    }

    auto orient_rad = headingToRadians(angle360(record.heading + rotate_degs));
    auto x_dot = cos(orient_rad) * swath_width;
    auto y_dot = sin(orient_rad) * swath_width;

    return EPoint(record.loc_x + x_dot, record.loc_y + y_dot);
}

std::pair<EPoint, EPoint> RecordSwath::LastOuterPoints()
{
    if (m_has_records)
    {
        auto port_point = OuterPoint(m_previous_record, BoatSide::Port);
        auto stbd_point = OuterPoint(m_previous_record, BoatSide::Stbd);
        return std::make_pair(port_point, stbd_point);
    }
    return std::make_pair(EPoint(), EPoint());
}

double RecordSwath::SwathWidth(BoatSide side, unsigned int index)
{
    if (m_min_record.size() > index)
    {
        std::list<SwathRecord>::iterator list_record = std::next(m_min_record.begin(), index);
        if (side == BoatSide::Stbd)
        {
            return list_record->swath_stbd;
        }
        else if (side == BoatSide::Port)
        {
            return list_record->swath_port;
        }
    }
    return 0;
}

std::vector<double> RecordSwath::AllSwathWidths(BoatSide side)
{
    std::vector<double> widths;
    widths.reserve(m_min_record.size());
    std::list<SwathRecord>::iterator list_record;
    for (list_record = m_min_record.begin(); list_record != m_min_record.end(); list_record++)
    {
        if (side == BoatSide::Stbd)
        {
            widths.push_back(list_record->swath_stbd);
        }
        else if (side == BoatSide::Port)
        {
            widths.push_back(list_record->swath_port);
        }
    }
    return widths;
}

EPoint RecordSwath::SwathLocation(unsigned int index)
{
    if (m_min_record.size() > index)
    {
        std::list<SwathRecord>::iterator list_record = std::next(m_min_record.begin(), index);
        return EPoint(list_record->loc_x, list_record->loc_y);
    }
    throw std::out_of_range("Swath index out of range.");
}

bool RecordSwath::ValidRecord()
{
    return (m_min_record.size() > 1);
}

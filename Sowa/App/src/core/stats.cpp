#include "stats.hpp"

namespace sowa {
Stats &Stats::Instance() {
	static Stats *stats = new Stats;
	return *stats;
}

Stats::StatsData Stats::Data() {
    return m_data;
}
void Stats::Reset() {
    m_data = StatsData();
}
void Stats::DrawCall() {
    m_data.drawCall++;
}
void Stats::Batch2DDrawCall() {
    m_data.drawCall++;
    m_data.drawCall_Batch2d++;
}
} // namespace sowa

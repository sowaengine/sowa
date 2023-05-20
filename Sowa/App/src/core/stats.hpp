#ifndef SW_STATS_HPP_
#define SW_STATS_HPP_

#include <cstdint>

namespace sowa {
class Stats {
  public:
    struct StatsData {
        uint32_t drawCall = 0;
        uint32_t drawCall_Batch2d = 0;
    };

	static Stats &Instance();
    
    StatsData Data();
    void Reset();
	void DrawCall();
    void Batch2DDrawCall();

  private:
	StatsData m_data;
};
} // namespace sowa

#endif // SW_STATS_HPP_
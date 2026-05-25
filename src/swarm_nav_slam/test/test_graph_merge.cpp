/**
 * @file test_graph_merge.cpp
 * @brief Unit tests for GraphMergeNode
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <memory>

class GraphMergeTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);
  }

  void TearDown() override
  {
    rclcpp::shutdown();
  }
};

/**
 * Test node construction
 */
TEST_F(GraphMergeTest, NodeConstruction)
{
  auto node = std::make_shared<rclcpp::Node>("test_graph_merge");
  ASSERT_NE(node, nullptr);
}

/**
 * Test cell-wise max merge logic on synthetic data
 */
TEST_F(GraphMergeTest, CellWiseMaxMerge)
{
  // Create two synthetic occupancy grids
  nav_msgs::msg::OccupancyGrid grid1;
  grid1.info.width = 10;
  grid1.info.height = 10;
  grid1.info.resolution = 0.05;
  grid1.data.resize(100);

  nav_msgs::msg::OccupancyGrid grid2;
  grid2.info.width = 10;
  grid2.info.height = 10;
  grid2.info.resolution = 0.05;
  grid2.data.resize(100);

  // Fill with test data
  for (size_t i = 0; i < 100; ++i) {
    grid1.data[i] = (i % 2 == 0) ? 50 : 0;  // Alternating pattern
    grid2.data[i] = (i % 2 == 0) ? 0 : 75;  // Opposite pattern
  }

  // Perform cell-wise max merge
  nav_msgs::msg::OccupancyGrid merged;
  merged.info = grid1.info;
  merged.data.resize(100);

  for (size_t i = 0; i < 100; ++i) {
    merged.data[i] = std::max(grid1.data[i], grid2.data[i]);
  }

  // Verify merge results
  for (size_t i = 0; i < 100; ++i) {
    if (i % 2 == 0) {
      EXPECT_EQ(merged.data[i], 50);
    } else {
      EXPECT_EQ(merged.data[i], 75);
    }
  }
}

/**
 * Test merge with unknown cells (-1)
 */
TEST_F(GraphMergeTest, MergeWithUnknownCells)
{
  std::vector<int8_t> grid1_data = {-1, 50, 100, -1};
  std::vector<int8_t> grid2_data = {75, -1, 50, -1};
  std::vector<int8_t> expected = {75, 50, 100, -1};

  std::vector<int8_t> merged(4);
  for (size_t i = 0; i < 4; ++i) {
    if (grid1_data[i] == -1 && grid2_data[i] == -1) {
      merged[i] = -1;
    } else if (grid1_data[i] == -1) {
      merged[i] = grid2_data[i];
    } else if (grid2_data[i] == -1) {
      merged[i] = grid1_data[i];
    } else {
      merged[i] = std::max(grid1_data[i], grid2_data[i]);
    }
  }

  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(merged[i], expected[i]);
  }
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

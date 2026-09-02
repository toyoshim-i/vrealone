// SPDX-License-Identifier: Apache-2.0
#include <exception>
#include <iostream>

#include "test_support.hpp"

int main() {
  try {
    RunCoordinateTransformTests();
    RunControlServerTests();
    RunDisplayConfigTests();
    RunEdidTests();
    RunQuaternionTests();
    RunPoseSnapshotTests();
    RunRecenterTests();
    RunGazeDwellDetectorTests();
    RunTapDetectorTests();
#if VREALONE_HAS_FUSION
    RunFusionTrackerTests();
#endif
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All vrealone tests passed.\n";
  return 0;
}

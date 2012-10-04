// /////////////////////////////////////////////////////////////////////////
//
// Query slice algorithm
//
// Author: Denis Arnaud
// Date:   October 2012
//
// /////////////////////////////////////////////////////////////////////////
// STL
#include <sstream>
#include <fstream>
#include <string>
#include <list>
// Boost Unit Test Framework (UTF)
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE SliceTestSuite
#include <boost/test/unit_test.hpp>
// OpenTrep
#include <opentrep/bom/QuerySlices.hpp>

namespace boost_utf = boost::unit_test;

// (Boost) Unit Test XML Report
std::ofstream utfReportStream ("SliceTestSuite_utfresults.xml");

/**
 * Configuration for the Boost Unit Test Framework (UTF)
 */
struct UnitTestConfig {
  /** Constructor. */
  UnitTestConfig() {
    boost_utf::unit_test_log.set_stream (utfReportStream);
    boost_utf::unit_test_log.set_format (boost_utf::XML);
    boost_utf::unit_test_log.set_threshold_level (boost_utf::log_test_units);
    //boost_utf::unit_test_log.set_threshold_level (boost_utf::log_successful_tests);
  }

  /** Destructor. */
  ~UnitTestConfig() {
  }
};


// /////////////// Main: Unit Test Suite //////////////

// Set the UTF configuration (re-direct the output to a specific file)
BOOST_GLOBAL_FIXTURE (UnitTestConfig);

// Start the test suite
BOOST_AUTO_TEST_SUITE (master_test_suite)

/**
 * Test the sliceing algorithm with a small string
 */
BOOST_AUTO_TEST_CASE (slice_small_string) {

  // Output log File
  std::string lLogFilename ("SliceTestSuite.log");

  // Set the log parameters
  std::ofstream logOutputFile;
  // Open and clean the log outputfile
  logOutputFile.open (lLogFilename.c_str());
  logOutputFile.clear();

  const std::string lLax1Str = "los angeles";
  const std::string lLax2Str = "lso angeles";
  const std::string lRio1Str = "rio de janeiro";
  const std::string lRio2Str = "rio de janero";
  const std::string lRek1Str = "reikjavik";
  const std::string lRek2Str = "rekyavik";
  const std::string lSfoRio1Str = "san francisco rio de janeiro";
  const std::string lSfoRio2Str = "san francicso rio de janero";
  const std::string lSfoRio3Str = "sna francicso rio de janero";
  const std::string lChelseaStr = "chelsea municipal airport";

  //
  OPENTREP::QuerySlices lQuerySlices (lSfoRio1Str);
  logOutputFile << lQuerySlices << std::endl;

  BOOST_CHECK_MESSAGE (lQuerySlices.size() == 2,
                       "The query ('" << lSfoRio1Str
                       << "') should contain two slices."
                       << " However, its size is " << lQuerySlices.size()
                       << ".");

  //
  OPENTREP::QuerySlices lAnotherQuerySlices (lChelseaStr);
  logOutputFile << lAnotherQuerySlices << std::endl;

  // Close the Log outputFile
  logOutputFile.close();
}

// End the test suite
BOOST_AUTO_TEST_SUITE_END()


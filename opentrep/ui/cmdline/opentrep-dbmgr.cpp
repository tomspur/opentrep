// STL
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
// Boost (Extended STL)
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
// GNU Readline Wrapper
#include <opentrep/ui/cmdline/SReadline.hpp>
// OpenTREP
#include <opentrep/Location.hpp>
#include <opentrep/OPENTREP_Service.hpp>
#include <opentrep/basic/BasConst_OPENTREP_Service.hpp>
#include <opentrep/config/opentrep-paths.hpp>
#include <opentrep/service/Logger.hpp>


// //////// Type definitions ///////
typedef std::vector<std::string> WordList_T;


// //////// Constants //////
/**
 * Default name and location for the log file.
 */
const std::string K_OPENTREP_DEFAULT_LOG_FILENAME ("opentrep-dbmgr.log");


// ///////// Parsing of Options & Configuration /////////
/** Early return status (so that it can be differentiated from an error). */
const int K_OPENTREP_EARLY_RETURN_STATUS = 99;

/**
 * List of strings, representing the tokens as entered by the user on
 * a command-line.
 */
typedef std::vector<std::string> TokenList_T;

/**
 * Enumeration representing the command entered by the user on the command-line.
 */
struct Command_T {
  typedef enum {
    NOP = 0,
    QUIT,
    HELP,
    INFO,
    TUTORIAL,
    CREATE_USER,
    RESET_CONNECTION_STRING,
    CREATE_TABLES,
    CREATE_INDEXES,
    FILL_FROM_POR_FILE,
    LIST_BY_IATA,
    LIST_BY_ICAO,
    LIST_BY_FAA,
    LIST_BY_GEONAMEID,
    LIST_NB,
    LIST_ALL,
    LIST_CONT,
    LAST_VALUE
  } Type_T;
};

// ///////// Parsing of Options & Configuration /////////
// A helper function to simplify the main part.
template<class T> std::ostream& operator<< (std::ostream& os,
                                            const std::vector<T>& v) {
  std::copy (v.begin(), v.end(), std::ostream_iterator<T> (std::cout, " ")); 
  return os;
}

/**
 * Read and parse the command line options.
 */
int readConfiguration (int argc, char* argv[], 
                       std::string& ioPORFilepath, 
                       std::string& ioXapianDBFilepath,
                       std::string& ioSQLDBTypeString,
                       std::string& ioSQLDBConnectionString,
                       std::string& ioLogFilename) {

  // Declare a group of options that will be allowed only on command line
  boost::program_options::options_description generic ("Generic options");
  generic.add_options()
    ("prefix", "print installation prefix")
    ("version,v", "print version string")
    ("help,h", "produce help message");

  // Declare a group of options that will be allowed both on command
  // line and in config file
  boost::program_options::options_description config ("Configuration");
  config.add_options()
    ("porfile,p",
     boost::program_options::value< std::string >(&ioPORFilepath)->default_value(OPENTREP::DEFAULT_OPENTREP_POR_FILEPATH),
     "POR file-path (e.g., ori_por_public.csv)")
    ("xapiandb,d",
     boost::program_options::value< std::string >(&ioXapianDBFilepath)->default_value(OPENTREP::DEFAULT_OPENTREP_XAPIAN_DB_FILEPATH),
     "Xapian database filepath (e.g., /tmp/opentrep/xapian_traveldb)")
    ("sqldbtype,t",
     boost::program_options::value< std::string >(&ioSQLDBTypeString)->default_value(OPENTREP::DEFAULT_OPENTREP_SQL_DB_TYPE),
     "SQL database type (e.g., nodb for no SQL database, sqlite for SQLite, mysql for MariaDB/MySQL)")
    ("sqldbconx,s",
     boost::program_options::value< std::string >(&ioSQLDBConnectionString)->default_value(OPENTREP::DEFAULT_OPENTREP_SQLITE_DB_FILEPATH),
     "SQL database connection string (e.g., ~/tmp/opentrep/sqlite_travel.db for SQLite, \"db=trep_trep user=trep password=trep\" for MariaDB/MySQL)")
    ("log,l",
     boost::program_options::value< std::string >(&ioLogFilename)->default_value(K_OPENTREP_DEFAULT_LOG_FILENAME),
     "Filepath for the logs")
    ;

  // Hidden options, will be allowed both on command line and
  // in config file, but will not be shown to the user.
  boost::program_options::options_description hidden ("Hidden options");
  hidden.add_options()
    ("copyright",
     boost::program_options::value< std::vector<std::string> >(),
     "Show the copyright (license)");
        
  boost::program_options::options_description cmdline_options;
  cmdline_options.add(generic).add(config).add(hidden);

  boost::program_options::options_description config_file_options;
  config_file_options.add(config).add(hidden);

  boost::program_options::options_description visible ("Allowed options");
  visible.add(generic).add(config);
        
  boost::program_options::positional_options_description p;
  p.add ("copyright", -1);
        
  boost::program_options::variables_map vm;
  boost::program_options::
    store (boost::program_options::command_line_parser (argc, argv).
	   options (cmdline_options).positional(p).run(), vm);

  std::ifstream ifs ("opentrep-dbmgr.cfg");
  boost::program_options::store (parse_config_file (ifs, config_file_options),
                                 vm);
  boost::program_options::notify (vm);
    
  if (vm.count ("help")) {
    std::cout << visible << std::endl;
    return K_OPENTREP_EARLY_RETURN_STATUS;
  }

  if (vm.count ("version")) {
    std::cout << PACKAGE_NAME << ", version " << PACKAGE_VERSION << std::endl;
    return K_OPENTREP_EARLY_RETURN_STATUS;
  }

  if (vm.count ("prefix")) {
    std::cout << "Installation prefix: " << PREFIXDIR << std::endl;
    return K_OPENTREP_EARLY_RETURN_STATUS;
  }

  if (vm.count ("porfile")) {
    ioPORFilepath = vm["porfile"].as< std::string >();
  }

  if (vm.count ("xapiandb")) {
    ioXapianDBFilepath = vm["xapiandb"].as< std::string >();
  }

  if (vm.count ("sqldbtype")) {
    ioSQLDBTypeString = vm["sqldbtype"].as< std::string >();
  }

  if (vm.count ("sqldbconx")) {
    ioSQLDBConnectionString = vm["sqldbconx"].as< std::string >();
  }

  if (vm.count ("log")) {
    ioLogFilename = vm["log"].as< std::string >();
  }

  // Information
  std::cout << "Type the 'info' command to get a few details (e.g., file-path)"
            << std::endl;

  return 0;
}

// //////////////////////////////////////////////////////////////////
void initReadline (swift::SReadline& ioInputReader) {

  // Prepare the list of my own completers
  std::vector<std::string> Completers;

  // The following is supported:
  // - "identifiers"
  // - special identifier %file - means to perform a file name completion
  Completers.push_back ("help");
  Completers.push_back ("info");
  Completers.push_back ("tutorial");
  Completers.push_back ("create_user");
  Completers.push_back ("reset_connection_string %connection_string");
  Completers.push_back ("create_tables");
  Completers.push_back ("create_indexes");
  Completers.push_back ("fill_from_por_file");
  Completers.push_back ("list_by_iata %iata_code");
  Completers.push_back ("list_by_icao %icao_code");
  Completers.push_back ("list_by_faa %faa_code");
  Completers.push_back ("list_by_geonameid %geoname_id");
  Completers.push_back ("list_nb");
  Completers.push_back ("list_all");
  Completers.push_back ("list_cont");
  Completers.push_back ("quit");

  // Now register the completers.
  // Actually it is possible to re-register another set at any time
  ioInputReader.RegisterCompletions (Completers);
}

// //////////////////////////////////////////////////////////////////
Command_T::Type_T extractCommand (TokenList_T& ioTokenList) {
  Command_T::Type_T oCommandType = Command_T::LAST_VALUE;

  // Interpret the user input
  if (ioTokenList.empty() == false) {
    TokenList_T::iterator itTok = ioTokenList.begin();
    std::string lCommand (*itTok);
    boost::algorithm::to_lower (lCommand);
    if (lCommand == "help") {
      oCommandType = Command_T::HELP;

    } else if (lCommand == "info") {
      oCommandType = Command_T::INFO;
    
    } else if (lCommand == "tutorial") {
      oCommandType = Command_T::TUTORIAL;
    
    } else if (lCommand == "create_user") {
      oCommandType = Command_T::CREATE_USER;
    
    } else if (lCommand == "reset_connection_string") {
      oCommandType = Command_T::RESET_CONNECTION_STRING;

    } else if (lCommand == "create_tables") {
      oCommandType = Command_T::CREATE_TABLES;

    } else if (lCommand == "create_indexes") {
      oCommandType = Command_T::CREATE_INDEXES;

    } else if (lCommand == "fill_from_por_file") {
      oCommandType = Command_T::FILL_FROM_POR_FILE;

    } else if (lCommand == "list_by_iata") {
      oCommandType = Command_T::LIST_BY_IATA;

    } else if (lCommand == "list_by_icao") {
      oCommandType = Command_T::LIST_BY_ICAO;

    } else if (lCommand == "list_by_faa") {
      oCommandType = Command_T::LIST_BY_FAA;

    } else if (lCommand == "list_by_geonameid") {
      oCommandType = Command_T::LIST_BY_GEONAMEID;

    } else if (lCommand == "list_nb") {
      oCommandType = Command_T::LIST_NB;

    } else if (lCommand == "list_all") {
      oCommandType = Command_T::LIST_ALL;

    } else if (lCommand == "list_cont") {
      oCommandType = Command_T::LIST_CONT;

    } else if (lCommand == "quit") {
      oCommandType = Command_T::QUIT;
    }

    // Remove the first token (the command), as the corresponding information
    // has been extracted in the form of the returned command type enumeration
    ioTokenList.erase (itTok);

  } else {
    oCommandType = Command_T::NOP;
  }

  return oCommandType;
}

// //////////////////////////////////////////////////////////////////
void parseConnectionString (const TokenList_T& iTokenList,
                            std::string& ioConnectionString) {
  // Interpret the user input
  if (iTokenList.empty() == false) {

    // Read the database connection string
    TokenList_T::const_iterator itTok = iTokenList.begin();
    if (itTok->empty() == false) {
      ioConnectionString = *itTok;
    }
  }
}

// //////////////////////////////////////////////////////////////////
void parsePlaceKey (const TokenList_T& iTokenList, std::string& ioPlaceKey) {
  // Interpret the user input
  if (iTokenList.empty() == false) {

    // Read the IATA code
    TokenList_T::const_iterator itTok = iTokenList.begin();
    if (itTok->empty() == false) {
      ioPlaceKey = *itTok;
    }
  }
}

// /////////////////////////////////////////////////////////
std::string toString (const TokenList_T& iTokenList) {
  std::ostringstream oStr;

  // Re-create the string with all the tokens, trimmed by read-line
  unsigned short idx = 0;
  for (TokenList_T::const_iterator itTok = iTokenList.begin();
       itTok != iTokenList.end(); ++itTok, ++idx) {
    if (idx != 0) {
      oStr << " ";
    }
    oStr << *itTok;
  }

  return oStr.str();
}

// /////////////////////////////////////////////////////////
TokenList_T extractTokenList (const TokenList_T& iTokenList,
                              const std::string& iRegularExpression) {
  TokenList_T oTokenList;

  // Re-create the string with all the tokens (which had been trimmed
  // by read-line)
  const std::string lFullLine = toString (iTokenList);

  // See the caller for the regular expression
  boost::regex expression (iRegularExpression);
  
  std::string::const_iterator start = lFullLine.begin();
  std::string::const_iterator end = lFullLine.end();

  boost::match_results<std::string::const_iterator> what;
  boost::match_flag_type flags = boost::match_default | boost::format_sed; 
  regex_search (start, end, what, expression, flags);
  
  // Put the matched strings in the list of tokens to be returned back
  // to the caller
  const unsigned short lMatchSetSize = what.size();
  for (unsigned short matchIdx = 1; matchIdx != lMatchSetSize; ++matchIdx) {
    const std::string lMatchedString (std::string (what[matchIdx].first,
                                                   what[matchIdx].second));
    //if (lMatchedString.empty() == false) {
    oTokenList.push_back (lMatchedString);
    //}
  }

  // DEBUG
  // std::cout << "After (token list): " << oTokenList << std::endl;

  return oTokenList;
}    

// /////////////////////////////////////////////////////////
TokenList_T extractTokenListForIataCode (const TokenList_T& iTokenList) {
  /**
   * Expected format:
   *   line:         iata_code
   *   iata_code:    word (alpha{3})
   */
  const std::string lRegEx ("^([[:alpha:]]{3})$");

  //
  const TokenList_T& oTokenList = extractTokenList (iTokenList, lRegEx);
  return oTokenList;
}    

// /////////////////////////////////////////////////////////
TokenList_T extractTokenListForIcaoCode (const TokenList_T& iTokenList) {
  /**
   * Expected format:
   *   line:         icao_code
   *   icao_code:    word ((alpha|digit){4})
   */
  const std::string lRegEx ("^(([[:alpha:]]|[[:digit:]]){4})$");

  //
  const TokenList_T& oTokenList = extractTokenList (iTokenList, lRegEx);
  return oTokenList;
}    

// /////////////////////////////////////////////////////////
TokenList_T extractTokenListForFaaCode (const TokenList_T& iTokenList) {
  /**
   * Expected format:
   *   line:         faa_code
   *   faa_code:     word (alpha{3,4})
   */
  const std::string lRegEx ("^(([[:alpha:]]|[[:digit:]]){3,4})$");

  //
  const TokenList_T& oTokenList = extractTokenList (iTokenList, lRegEx);
  return oTokenList;
}    

// /////////////////////////////////////////////////////////
TokenList_T extractTokenListForGeonameID (const TokenList_T& iTokenList) {
  /**
   * Expected format:
   *   line:          geoname_id
   *   geoname_id:    number (digit{1,11})
   */
  const std::string lRegEx ("^([[:digit:]]{1,11})$");

  //
  const TokenList_T& oTokenList = extractTokenList (iTokenList, lRegEx);
  return oTokenList;
}    


// /////////////// M A I N /////////////////
int main (int argc, char* argv[]) {

  // Readline history
  const unsigned int lHistorySize (100);
  const std::string lHistoryFilename ("opentrep-dbmgr.hist");
  const std::string lHistoryBackupFilename ("opentrep-dbmgr.hist.bak");

  // Output log File
  std::string lLogFilename;

  // File-path of POR (points of reference)
  std::string lPORFilepathStr;

  // Xapian database name (directory of the index)
  std::string lXapianDBNameStr;

  // SQL database type
  std::string lSQLDBTypeStr;

  // SQL database connection string
  std::string lSQLDBConnectionStr;

  // Call the command-line option parser
  const int lOptionParserStatus =
    readConfiguration (argc, argv, lPORFilepathStr, lXapianDBNameStr,
                       lSQLDBTypeStr, lSQLDBConnectionStr, lLogFilename);

  if (lOptionParserStatus == K_OPENTREP_EARLY_RETURN_STATUS) {
    return 0;
  }

  // Set the log parameters
  std::ofstream logOutputFile;
  // open and clean the log outputfile
  logOutputFile.open (lLogFilename.c_str());
  logOutputFile.clear();

  // Initialise the context
  const OPENTREP::PORFilePath_T lPORFilepath (lPORFilepathStr);
  const OPENTREP::TravelDBFilePath_T lXapianDBName (lXapianDBNameStr);
  const OPENTREP::DBType lDBType (lSQLDBTypeStr);
  const OPENTREP::SQLDBConnectionString_T lSQLDBConnStr (lSQLDBConnectionStr);
  OPENTREP::OPENTREP_Service opentrepService (logOutputFile, lPORFilepath,
                                              lXapianDBName,
                                              lDBType, lSQLDBConnStr);

  // DEBUG
  OPENTREP_LOG_DEBUG ("====================================================");
  OPENTREP_LOG_DEBUG ("=       Beginning of the interactive session       =");
  OPENTREP_LOG_DEBUG ("====================================================");

  // Initialise the GNU readline wrapper
  swift::SReadline lReader (lHistoryFilename, lHistorySize);
  initReadline (lReader);

  // Now we can ask user for a line
  std::string lUserInput;
  bool EndOfInput (false);
  Command_T::Type_T lCommandType (Command_T::NOP);
  
  while (lCommandType != Command_T::QUIT && EndOfInput == false) {
    // Prompt
    std::ostringstream oPromptStr;
    oPromptStr << "opentrep> ";

    // Call read-line, which will fill the list of tokens
    TokenList_T lTokenListByReadline;
    lUserInput = lReader.GetLine (oPromptStr.str(), lTokenListByReadline,
                                  EndOfInput);

    // The history can be saved to an arbitrary file at any time
    lReader.SaveHistory (lHistoryBackupFilename);

    // The end-of-input typically corresponds to a CTRL-D typed by the user
    if (EndOfInput) {
      std::cout << std::endl;
      break;
    }

    // Interpret the user input
    lCommandType = extractCommand (lTokenListByReadline);

    switch (lCommandType) {

      // ////////////////////////////// Help ////////////////////////
    case Command_T::HELP: {
      std::cout << std::endl;
      std::cout << "Commands: " << std::endl;
      std::cout << " CTRL-L (Control and L keys)" << "\t" << "Clean the screen"
                << std::endl;
      std::cout << " help" << "\t\t\t\t" << "Display this help" << std::endl;
      std::cout << " info" << "\t\t\t\t"
                << "Display details for the current session "
                << "(e.g., file-paths for the log file, SQL database)"
                << std::endl;
      std::cout << " tutorial" << "\t\t\t" << "Display examples" << std::endl;
      std::cout << " quit" << "\t\t\t\t" << "Quit the application" << std::endl;
      std::cout << " create_user" << "\t\t\t"
                << "On MySQL, create the 'trep' user and the 'trep_trep' database."
                << " MySQL administrative rights are required." << std::endl;
      std::cout << " reset_connection_string" << "\t"
                << "Reset/update the connection string to a MySQL database."
                << " The connection string must be given"
                << std::endl;
      std::cout << " create_tables" << "\t\t\t"
                << "Create/reset the SQLite3/MySQL tables"
                << std::endl;
      std::cout << " create_indexes" << "\t\t\t"
                << "Create/reset the SQLite3/MySQL indexes"
                << std::endl;
      std::cout << " fill_from_por_file" << "\t\t"
                << "Parse the file of POR and fill-in the SQL database ori_por table."
                << std::endl << "\t\t\t\t"
                << "Note that, as that command takes minutes, the connection to the SQL database may be lost and the program will exit abnormally."
                << std::endl << "\t\t\t\t"
                << "In that latter case, just re-execute the program and check how far the indexation went by executing the following command."
                << std::endl;
      std::cout << " list_nb" << "\t\t\t"
                << "Display the number of the entries of the database."
                << std::endl;
      std::cout << " list_all" << "\t\t\t"
                << "List all the entries of the database, page by page."
                << "Type the 'list_cont' command for a page down" << std::endl;
      std::cout << " list_by_iata" << "\t\t\t"
                << "List all the entries for a given IATA code"
                << std::endl;
      std::cout << " list_by_icao" << "\t\t\t"
                << "List all the entries for a given ICAO code"
                << std::endl;
      std::cout << " list_by_faa" << "\t\t\t"
                << "List all the entries for a given FAA code"
                << std::endl;
      std::cout << " list_by_geonameid" << "\t\t"
                << "List all the entries for a given Geoname ID"
                << std::endl;
      std::cout << std::endl;
      break;
    }
 
      // ////////////////////////////// Information ////////////////////////
    case Command_T::INFO: {
      const OPENTREP::OPENTREP_Service::FilePathSet_T& lFPSet =
        opentrepService.getFilePaths();
      const OPENTREP::OPENTREP_Service::DBFilePathPair_T& lDBFPPair =
        lFPSet.second;
      const OPENTREP::SQLDBConnectionString_T& lSQLConnStr = lDBFPPair.second;
      std::cout << std::endl;
      std::cout << "Log file-path: " << "\t\t\t\t" << lLogFilename
                << std::endl;
      std::cout << "POR file-path: " << "\t\t\t\t" << lPORFilepathStr
                << std::endl;
      std::cout << "SQL database type: " << "\t\t\t" << lDBType.describe()
                << std::endl;
      std::cout << "SQL database connection string: " << "\t" << lSQLConnStr
                << std::endl;
      std::cout << std::endl;
      break;
    }
 
      // /////////////////////////// Help with Examples //////////////////////
    case Command_T::TUTORIAL: {
      std::cout << std::endl;
      std::cout << "Typical succession of Commands: " << std::endl;
      std::cout <<" reset_connection_string db=mysql user=root password=<passwd>"
                << std::endl;
      std::cout << " create_user" << std::endl;
      std::cout <<" reset_connection_string db=trep_trep user=trep password=trep"
                << std::endl;
      std::cout << " create_tables" << std::endl;
      std::cout << " fill_from_por_file" << std::endl;
      std::cout << " create_indexes" << std::endl;
      std::cout << " list_nb" << std::endl;
      std::cout << " list_by_iata nce" << std::endl;
      std::cout << " list_by_icao lfmn" << std::endl;
      std::cout << " list_by_faa afm" << std::endl;
      std::cout << " list_by_geonameid 6299418" << std::endl;
      std::cout << std::endl;
      break;
    }
 
      // ////////////////////////////// Quit ////////////////////////
    case Command_T::QUIT: {
      break;
    }

      // ////////////////////////////// List Number /////////////////////////
    case Command_T::LIST_NB: {
      // Call the underlying OpenTREP service
      const OPENTREP::NbOfMatches_T nbOfMatches =
        opentrepService.getNbOfPORFromDB();

      //
      std::cout << nbOfMatches << " (geographical) location(s) have been found."
                << std::endl;

      break;
    }

      // ////////////////////////////// List All /////////////////////////
    case Command_T::LIST_ALL: {
      // For now, just hard code a single IATA code.
      // TODO: implement the page down process, so that the full list
      // can be retrieved and browsed.
      const std::string lIataCodeStr ("NCE");

      // Call the underlying OpenTREP service
      const OPENTREP::IATACode_T lIataCode (lIataCodeStr);
      OPENTREP::LocationList_T lLocationList;
      const OPENTREP::NbOfMatches_T nbOfMatches =
        opentrepService.listByIataCode (lIataCode, lLocationList);

      //
      std::cout << nbOfMatches << " (geographical) location(s) have been found "
                << "matching the IATA code ('" << lIataCodeStr << "')."
                << std::endl;
      
      if (nbOfMatches != 0) {
        OPENTREP::NbOfMatches_T idx = 1;
        for (OPENTREP::LocationList_T::const_iterator itLocation =
               lLocationList.begin();
             itLocation != lLocationList.end(); ++itLocation, ++idx) {
          const OPENTREP::Location& lLocation = *itLocation;
          std::cout << " [" << idx << "]: " << lLocation.toString() << std::endl;
        }

      } else {
        std::cout << "List of unmatched words:" << std::endl;
        std::cout << " [" << 1 << "]: " << lIataCodeStr << std::endl;
      }

      break;
    }

      // ////////////////////////// List by IATA code ////////////////////////
    case Command_T::LIST_BY_IATA: {
      //
      TokenList_T lTokenList = extractTokenListForIataCode(lTokenListByReadline);

      // Parse the parameters given by the user, giving default values
      // in case the user does not specify some (or all) of them
      std::string lIataCodeStr ("NCE");
      parsePlaceKey (lTokenList, lIataCodeStr);

      // Call the underlying OpenTREP service
      const OPENTREP::IATACode_T lIataCode (lIataCodeStr);
      OPENTREP::LocationList_T lLocationList;
      const OPENTREP::NbOfMatches_T nbOfMatches =
        opentrepService.listByIataCode (lIataCode, lLocationList);

      //
      std::cout << nbOfMatches << " (geographical) location(s) have been found "
                << "matching the IATA code ('" << lIataCodeStr << "')."
                << std::endl;
      
      if (nbOfMatches != 0) {
        OPENTREP::NbOfMatches_T idx = 1;
        for (OPENTREP::LocationList_T::const_iterator itLocation =
               lLocationList.begin();
             itLocation != lLocationList.end(); ++itLocation, ++idx) {
          const OPENTREP::Location& lLocation = *itLocation;
          std::cout << " [" << idx << "]: " << lLocation << std::endl;
        }

      } else {
        std::cout << "List of unmatched words:" << std::endl;
        std::cout << " [" << 1 << "]: " << lIataCodeStr << std::endl;
      }

      break;
    }

      // ////////////////////////// List by ICAO code ////////////////////////
    case Command_T::LIST_BY_ICAO: {
      //
      TokenList_T lTokenList = extractTokenListForIcaoCode(lTokenListByReadline);

      // Parse the parameters given by the user, giving default values
      // in case the user does not specify some (or all) of them
      std::string lIcaoCodeStr ("LFMN");
      parsePlaceKey (lTokenList, lIcaoCodeStr);

      // Call the underlying OpenTREP service
      const OPENTREP::ICAOCode_T lIcaoCode (lIcaoCodeStr);
      OPENTREP::LocationList_T lLocationList;
      const OPENTREP::NbOfMatches_T nbOfMatches =
        opentrepService.listByIcaoCode (lIcaoCode, lLocationList);

      //
      std::cout << nbOfMatches << " (geographical) location(s) have been found "
                << "matching the ICAO code ('" << lIcaoCodeStr << "')."
                << std::endl;
      
      if (nbOfMatches != 0) {
        OPENTREP::NbOfMatches_T idx = 1;
        for (OPENTREP::LocationList_T::const_iterator itLocation =
               lLocationList.begin();
             itLocation != lLocationList.end(); ++itLocation, ++idx) {
          const OPENTREP::Location& lLocation = *itLocation;
          std::cout << " [" << idx << "]: " << lLocation << std::endl;
        }

      } else {
        std::cout << "List of unmatched words:" << std::endl;
        std::cout << " [" << 1 << "]: " << lIcaoCodeStr << std::endl;
      }

      break;
    }

      // ////////////////////////// List by FAA code ////////////////////////
    case Command_T::LIST_BY_FAA: {
      //
      TokenList_T lTokenList = extractTokenListForFaaCode(lTokenListByReadline);

      // Parse the parameters given by the user, giving default values
      // in case the user does not specify some (or all) of them
      std::string lFaaCodeStr ("jfk");
      parsePlaceKey (lTokenList, lFaaCodeStr);

      // Call the underlying OpenTREP service
      const OPENTREP::FAACode_T lFaaCode (lFaaCodeStr);
      OPENTREP::LocationList_T lLocationList;
      const OPENTREP::NbOfMatches_T nbOfMatches =
        opentrepService.listByFaaCode (lFaaCode, lLocationList);

      //
      std::cout << nbOfMatches << " (geographical) location(s) have been found "
                << "matching the FAA code ('" << lFaaCodeStr << "')."
                << std::endl;
      
      if (nbOfMatches != 0) {
        OPENTREP::NbOfMatches_T idx = 1;
        for (OPENTREP::LocationList_T::const_iterator itLocation =
               lLocationList.begin();
             itLocation != lLocationList.end(); ++itLocation, ++idx) {
          const OPENTREP::Location& lLocation = *itLocation;
          std::cout << " [" << idx << "]: " << lLocation << std::endl;
        }

      } else {
        std::cout << "List of unmatched words:" << std::endl;
        std::cout << " [" << 1 << "]: " << lFaaCodeStr << std::endl;
      }

      break;
    }

      // ////////////////////////// List by Geoname ID ////////////////////////
    case Command_T::LIST_BY_GEONAMEID: {
      //
      TokenList_T lTokenList =
        extractTokenListForGeonameID (lTokenListByReadline);

      // Parse the parameters given by the user, giving default values
      // in case the user does not specify some (or all) of them
      std::string lGeonameIDStr ("6299418");
      parsePlaceKey (lTokenList, lGeonameIDStr);

      // Call the underlying OpenTREP service
      OPENTREP::GeonamesID_T lGeonameID;

      try {

        lGeonameID = boost::lexical_cast<OPENTREP::GeonamesID_T> (lGeonameIDStr);

      } catch (boost::bad_lexical_cast& eCast) {
        lGeonameID = 6299418;
        std::cerr << "The Geoname ID ('" << lGeonameIDStr
                  << "') cannot be understood. The default value ("
                  << lGeonameID << ") is kept." << std::endl;
      }

      OPENTREP::LocationList_T lLocationList;
      const OPENTREP::NbOfMatches_T nbOfMatches =
        opentrepService.listByGeonameID (lGeonameID, lLocationList);

      //
      std::cout << nbOfMatches << " (geographical) location(s) have been found "
                << "matching the Geoname ID ('" << lGeonameIDStr << "')."
                << std::endl;
      
      if (nbOfMatches != 0) {
        OPENTREP::NbOfMatches_T idx = 1;
        for (OPENTREP::LocationList_T::const_iterator itLocation =
               lLocationList.begin();
             itLocation != lLocationList.end(); ++itLocation, ++idx) {
          const OPENTREP::Location& lLocation = *itLocation;
          std::cout << " [" << idx << "]: " << lLocation << std::endl;
        }

      } else {
        std::cout << "List of unmatched items:" << std::endl;
        std::cout << " [" << 1 << "]: " << lGeonameIDStr << std::endl;
      }

      break;
    }

      // ///////////////////////// Database Creation /////////////////////////
    case Command_T::CREATE_USER: {
      //
      std::cout << "Creating the 'trep' user and 'trep_trep' database"
                << std::endl;
    
      // On MySQL, create the 'trep' user and 'trep_trep' database.
      // On other database types, do nothing.
      const bool lCreationSuccessful = opentrepService.createSQLDBUser();

      // Reporting
      if (lCreationSuccessful == true) {
        std::cout << "The 'trep' user and 'trep_trep' database have been created"
                  << std::endl;
      }

      break;
    }

      // ///////////////////////// Database Creation /////////////////////////
    case Command_T::RESET_CONNECTION_STRING: {
      // Parse the parameters given by the user, giving default values
      // in case the user does not specify some (or all) of them
      const std::string lConnectionStringStr = toString (lTokenListByReadline);

      //
      std::cout << "Reset the connection string" << std::endl;
    
      // Reset the connection string
      const OPENTREP::SQLDBConnectionString_T
        lConnectionString (lConnectionStringStr);
      opentrepService.setSQLDBConnectString (lConnectionString);

      //
      std::cout << "The connection string has been reset" << std::endl;

      break;
    }

      // ///////////////////////// Tables Creation /////////////////////////
    case Command_T::CREATE_TABLES: {
      //
      std::cout << "Creating/resetting the " << lDBType.describe()
                << " database tables" << std::endl;
    
      // Create/reset the SQLite3/MySQL tables
      opentrepService.createSQLDBTables();

      //
      std::cout << "The " << lDBType.describe()
                << " tables has been created/resetted" << std::endl;

      break;
    }

      // ///////////////////////// Indexes Creation /////////////////////////
    case Command_T::CREATE_INDEXES: {
      //
      std::cout << "Creating/resetting the " << lDBType.describe()
                << " database indexes" << std::endl;
    
      // Create/reset the SQLite3/MySQL indexes
      opentrepService.createSQLDBIndexes();

      //
      std::cout << "The " << lDBType.describe()
                << " indexes has been created/resetted" << std::endl;

      break;
    }

      // ///////////////////////// POR File Indexing /////////////////////////
    case Command_T::FILL_FROM_POR_FILE: {
      //
      std::cout << "Indexing the POR file and filling in the SQL database may "
                << "take a few minutes on some architectures "
                << "(and a few seconds on fastest ones)..."
                << std::endl;
    
      // Launch the indexation
      const OPENTREP::NbOfDBEntries_T lNbOfEntries =
        opentrepService.fillInFromPORFile();

      //
      std::cout << lNbOfEntries << " entries have been processed" << std::endl;

      break;
    }

      // /////////////////////////// Default / No value ///////////////////////
    case Command_T::NOP: {
      break;
    }
 
    case Command_T::LAST_VALUE:
    default: {
      // DEBUG
      std::ostringstream oStr;
      oStr << "That command is not yet understood: '" << lUserInput
           << "' => " << lTokenListByReadline;
      OPENTREP_LOG_DEBUG (oStr.str());
      std::cout << oStr.str() << std::endl;
    }
    }
  }

  // DEBUG
  OPENTREP_LOG_DEBUG ("End of the session. Exiting.");
  std::cout << "End of the session. Exiting." << std::endl;

  // Close the Log outputFile
  logOutputFile.close();
    
  return 0;
}

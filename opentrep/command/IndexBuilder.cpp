// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <cassert>
#include <string>
#include <vector>
#include <exception>
// Boost
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
// OpenTrep
#include <opentrep/bom/Utilities.hpp>
#include <opentrep/bom/StringPartition.hpp>
#include <opentrep/bom/WordCombinationHolder.hpp>
#include <opentrep/bom/World.hpp>
#include <opentrep/bom/Place.hpp>
#include <opentrep/factory/FacPlace.hpp>
#include <opentrep/command/PORParserHelper.hpp>
#include <opentrep/command/IndexBuilder.hpp>
#include <opentrep/service/Logger.hpp>
// Xapian
#include <xapian.h>

namespace OPENTREP {

  // //////////////////////////////////////////////////////////////////////
  void tokeniseAndAddToDocument (const std::string& iPhrase,
                                 Xapian::Document& ioDocument,
                                 Xapian::WritableDatabase& ioDatabase) {

    // Boost Tokeniser
    typedef boost::tokenizer<boost::char_separator<char> > Tokeniser_T;

    // Define the separators
    const boost::char_separator<char> lSepatorList(" .,;:|+-*/_=!@#$%`~^&(){}[]?'<>\"");

    // Initialise the phrase to be tokenised
    Tokeniser_T lTokens (iPhrase, lSepatorList);
    for (Tokeniser_T::const_iterator tok_iter = lTokens.begin();
         tok_iter != lTokens.end(); ++tok_iter) {
      const std::string& lTerm = *tok_iter;
      
      ioDatabase.add_spelling (lTerm);
      ioDocument.add_term (lTerm);

      // OPENTREP_LOG_DEBUG ("Added term: " << lTerm);
    }
  }
  
  // //////////////////////////////////////////////////////////////////////
  void tokeniseAndAddToDocumentNew (const std::string& iPhrase,
                                    Xapian::Document& ioDocument,
                                    Xapian::WritableDatabase& ioDatabase) {

    // Create a list made of all the word combinations of the initial phrase
    WordCombinationHolder lWordCombinationHolder (iPhrase);

    // Browse the list of unique strings (word combinations)
    const WordCombinationHolder::StringList_T& lStringList =
      lWordCombinationHolder._list;
    for (WordCombinationHolder::StringList_T::const_iterator itString =
           lStringList.begin();
         itString != lStringList.end(); ++itString) {
      const std::string& lWordCombination = *itString;

      // Add that combination of words into the Xapian index
      tokeniseAndAddToDocument (lWordCombination, ioDocument, ioDatabase);
      //ioDocument.add_term (lWordCombination);
      //ioDatabase.add_spelling (lWordCombination);
    } 

    // DEBUG
    OPENTREP_LOG_DEBUG ("Added terms for '" << iPhrase << "': "
                        << lWordCombinationHolder << " into "
                        << ioDocument.get_description());
  }
  
  // //////////////////////////////////////////////////////////////////////
  void addToXapian (const Place& iPlace, Xapian::Document& ioDocument,
                    Xapian::WritableDatabase& ioDatabase) {

    // Build a Xapian TermGenerator:
    // http://xapian.org/docs/apidoc/html/classXapian_1_1TermGenerator.html
    // It is an helper to insert terms into the Xapian index for the given
    // document.
    Xapian::TermGenerator lTermGenerator;
    lTermGenerator.set_database (ioDatabase);
    lTermGenerator.set_document (ioDocument);

    const Place::StringSet_T& lTermSet = iPlace.getTermSet();
    for (Place::StringSet_T::const_iterator itString = lTermSet.begin();
         itString != lTermSet.end(); ++itString) {
      const std::string& lString = *itString;
      lTermGenerator.index_text (lString);
    }

    const Place::StringSet_T& lSpellingSet = iPlace.getSpellingSet();
    for (Place::StringSet_T::const_iterator itTerm = lSpellingSet.begin();
         itTerm != lSpellingSet.end(); ++itTerm) {
      const std::string& lTerm = *itTerm;
      ioDatabase.add_spelling (lTerm);
    }

    // DEBUG
    OPENTREP_LOG_DEBUG ("Added terms for '" << iPlace.describeKey()
                        << "': " << iPlace.describeSets()
                        << " into " << ioDocument.get_description());
  }

  // //////////////////////////////////////////////////////////////////////
  void IndexBuilder::addDocumentToIndex (Xapian::WritableDatabase& ioDatabase,
                                         Place& ioPlace) {

    // Build a Xapian document
    Xapian::Document lDocument;
    
    // The document data is indeed meta-data, allowing a human being
    // to read it
    lDocument.set_data (ioPlace.toString());
      
    // Build the (STL) sets of terms to be added to the Xapian index and
    // spelling dictionary
    ioPlace.buildIndexSets();

    // Add the (STL) sets of terms to the Xapian index and spelling dictionary
    addToXapian (ioPlace, lDocument, ioDatabase);

    // Add the document to the database
    const Xapian::docid& lDocID = ioDatabase.add_document (lDocument);
      
    // Assign back the newly generated Xapian document ID to the
    // Place object
    ioPlace.setDocID (lDocID);
  }

  // //////////////////////////////////////////////////////////////////////
  NbOfDBEntries_T IndexBuilder::
  buildSearchIndex (const PORFilePath_T& iPORFilePath,
                    const TravelDatabaseName_T& iTravelDatabaseName) {
    NbOfDBEntries_T oNbOfEntries = 0;

    // Check that the directory for the Xapian database (index) exists and,
    // if not, create it.
    // DEBUG
    OPENTREP_LOG_DEBUG ("The Xapian database ('" << iTravelDatabaseName
                        << "') will be cleared");
    boost::filesystem::remove_all (iTravelDatabaseName);
    boost::filesystem::create_directories (iTravelDatabaseName);

    // Create the Xapian database (index). As the directory has been fully
    // cleaned, deleted and re-created, that Xapian database (index) is empty.
    Xapian::WritableDatabase lDatabase (iTravelDatabaseName, Xapian::DB_CREATE);

    /**
     * Begin a transation on the Xapian database (index).
     *
     * Normally, there are around 11,000 entries (rows/documents) to
     * be indexed. Not specifying the beginning of a transaction would
     * mean that every document addition would end up in a corresponding
     * independant transaction, which would be very much inefficient.
     */
    lDatabase.begin_transaction();

    /**
     * Parse the list of POR (points of reference).
     */

    // DEBUG
    OPENTREP_LOG_DEBUG ("Parsing por input file: " << iPORFilePath);

    // Check whether the file to be parsed exists and is readable.
    if (!(boost::filesystem::exists (iPORFilePath)
          && boost::filesystem::is_regular_file (iPORFilePath))) {
      OPENTREP_LOG_ERROR ("The POR file " << iPORFilePath
                          << " does not exist or cannot be open." << std::endl);

      throw FileNotFoundException ("The POR file " + iPORFilePath
                                   + " does not exist or cannot be read");
    }

    // Open the file to be parsed
    boost::filesystem::ifstream fileToBeParsed (iPORFilePath);
    Place& lPlace = FacPlace::instance().create();
    std::string itReadLine;
    while (std::getline (fileToBeParsed, itReadLine)) {
      // Initialise the parser
      PORStringParser lStringParser (itReadLine);

      // Parse the string
      const Location& lLocation = lStringParser.generateLocation();
      //const LocationKey& lLocationKey = lLocation.getKey();

      // DEBUG
      //OPENTREP_LOG_DEBUG ("[BEF-ADD] " << lLocationKey);

      // When the line/string was relevant, create a BOM instance from
      // the Location structure.
      if (!(lLocation.getCommonName() == "NotAvailable")) {
        // Fill the Place object with the Location structure.
        lPlace.setLocation (lLocation);

        // Add the document, associated to the Place object, to the Xapian index
        IndexBuilder::addDocumentToIndex (lDatabase, lPlace);

        // DEBUG
        /*
        OPENTREP_LOG_DEBUG ("[AFT-ADD] " << lLocationKey
                            << ", Place: " << lPlace);
        */

        // Iteration
        ++oNbOfEntries;

        // DEBUG
        OPENTREP_LOG_DEBUG ("[" << oNbOfEntries << "] " << lPlace);

        // Reset for next turn
        lPlace.resetMatrix();
        lPlace.resetIndexSets();
      }
    }

    // Commit the pending modifications on the Xapian database (index)
    lDatabase.commit_transaction();

    // DEBUG
    OPENTREP_LOG_DEBUG ("Xapian has indexed " << oNbOfEntries << " entries.");

    /**
     * Close the Xapian database (index).
     *
     * \note For a yet unknown reason, when the current method is called within
     *       the Boost Unit Test framework, that latter kills the process.
     *       When called from within GDB, all is fine.
     */
    lDatabase.close();

    return oNbOfEntries;
  }

}

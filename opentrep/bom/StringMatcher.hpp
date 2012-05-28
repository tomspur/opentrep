#ifndef __OPENTREP_BOM_STRINGMATCHER_HPP
#define __OPENTREP_BOM_STRINGMATCHER_HPP

// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <string>
// OpenTREP
#include <opentrep/OPENTREP_Types.hpp>
#include <opentrep/bom/BomAbstract.hpp>
#include <opentrep/bom/MatchingDocuments.hpp>

// Forward declarations
namespace Xapian {
  class MSet;
  class Database;
  class Document;
}

namespace OPENTREP {

  /**
   * @brief Class grouping a few utility methods.
   *
   * The utility methods heavily use the Xapian library.
   *
   * \see <a href="http://www.xapian.org">Xapian's Web site</a>
   * for more information.
   */
  class StringMatcher : public BomAbstract {
  public:
    // ///////////////////////////////////////////////
    /**
     * Search, within the Xapian database, for occurrences of the
     * words of the search string.
     *
     * The algorithm is much simpler than the above one.
     *
     * @param Xapian::MSet& The Xapian matching set. It can be empty.
     * @param const TravelQuery_T& The query string.
     * @param MatchingDocuments& The set of matching documents, if any.
     * @param const Xapian::Database& The Xapian index/database.
     * @return std::string The query string, potentially corrected,
     *         which has yielded matches.
     */
    static std::string searchString (Xapian::MSet&, const TravelQuery_T&,
                                     MatchingDocuments&,
                                     const Xapian::Database&);
    
    /**
     * Extract the best matching Xapian document.
     *
     * If there are several such best matching documents (for
     * instance, several at, say, 100%), one is taken randomly. Well,
     * as we take the first one of the STL multimap, it is not exactly
     * randomly, but the result is the same: it appears to be random.
     *
     * @param Xapian::MSet& The Xapian matching set. It can be empty.
     */
    static void extractBestMatchingDocumentFromMSet (const Xapian::MSet&,
                                                     MatchingDocuments&);

    /**
     * Remove the word furthest at right.
     */
    static void removeFurthestRightWord (TravelQuery_T&);

    /**
     * Remove the word furthest at left.
     */
    static Word_T removeFurthestLeftWord (TravelQuery_T&);

    /**
     * Remove, from a string, the part corresponding to the one given
     * as parameter.
     */
    static void
    subtractParsedToRemaining (const TravelQuery_T& iAlreadyParsedQueryString,
                               TravelQuery_T& ioRemainingQueryString);
  };

}
#endif // __OPENTREP_BOM_STRINGMATCHER_HPP
